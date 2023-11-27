#include "io_context.h"
#include <fio_awaiter.h>
#include <sys/eventfd.h>

namespace co2
{
int aio_read_block::on_fd_event(int event)
{
    this_context->aio_complete(_fd);
    if (event <= 0) [[unlikely]]
    {
        log_warn("fd:%d error:%d %s", _fd, -event, strerror(-event));
        _handle.resume();
        return 0;
    }

    eventfd_t v;
    ::eventfd_read(_efd, &v);
    _result = 0;
    _handle.resume();
    return 0;
}

int aio_write_block::on_fd_event(int event)
{
    this_context->aio_complete(_efd);
    if (event <= 0) [[unlikely]]
    {
        log_warn("fd:%d error:%d %s", _fd, -event, strerror(-event));
        _handle.resume();
        return 0;
    }

    eventfd_t v;
    ::eventfd_read(_efd, &v);
    _result = 0;
    _handle.resume();
    return 0;
}
}

