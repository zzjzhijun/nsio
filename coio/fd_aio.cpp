#include <deque>
#include <memory>
#include <memory>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <linux/aio_abi.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <functional>
#include <vector>

#include <byte_buffer.h>

#include "io_loop.h"
#include "log.h"
#include "fd_aio.h"

namespace co2
{
struct io_aio_impl : io_aio
{
    static constexpr uint32_t _max_events = 256;

    aio_context_t _ctx = 0;
    int _efd = -1;
    uint64_t _req=0,_res=0;

    io_aio_impl()
    {
        if (io_setup(_max_events, &_ctx))
        {
            log_errno("zaio::io_setup failed");
            return;
        }

        _efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    }

    ~io_aio_impl()
    {
        if (_ctx != 0)
            io_destroy(_ctx);

        if (_efd != -1)
            ::close(_efd);
    }

    int event_fd() noexcept override
    {
        return _efd;
    }

    void on_file_event(es_file & es, int) noexcept override
    {
        eventfd_t v;
        ::eventfd_read(_efd, &v);
        fetch_events();
        es._func_r = this;
    }

    void fetch_events() noexcept
    {
        for(;;)
        {
            struct io_event e[_max_events];
            auto rc = io_getevents(_ctx, 1, _max_events, e, 0);

            if(rc>0)
            {
                for (int i = 0; i < rc; i++)
                {
                    io_callback * icb = (io_callback *)e[i].data;
                    icb->on_aio_event(e[i]);
                }

                _res+=rc;
                break;
            }

            if(rc<0)
            {
                if(errno==EINTR)
                    continue;

                log_errno("io_getevents");
            }
        }
    }

    virtual void submit(iocb*pb)
    {
        for(;;)
        {
            auto rc=io_submit(_ctx, 1, &pb);
            if(rc==1)
            {
                ++_req;
                break;
            }

            if(rc==-1)
            {
                if (errno == EAGAIN)
                {
                    this->fetch_events();
                    continue;
                }

                if (errno == EINTR)
                    continue;

                log_errno("io_submit");
            }
        }
    }

    virtual int aio_read(
        int fd, off_t offset, size_t count, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept override
    {
        iocb b;

        buff->set_data_begin(0);
        buff->set_data_len(count);

        set_iocb(b, fd, false, offset, buff->data(), count, _efd, icb);

        submit(&b);
        return 0;
    }

    virtual int
    aio_write(int fd, off_t offset, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept override
    {
        iocb b;

        set_iocb(b, fd, true, offset, buff->data(), buff->data_len(), _efd, icb);

        submit(&b);

        return 0;
    }

    static void
    set_iocb(iocb & b, int fd, bool is_write, size_t off, const void * buff, size_t len, int efd, void * icb)
    {
        memset(&b, 0, sizeof(iocb));

        b.aio_data = (uint64_t)icb;
        b.aio_lio_opcode = is_write ? IOCB_CMD_PWRITE : IOCB_CMD_PREAD;
        b.aio_fildes = fd;
        b.aio_offset = off;
        b.aio_buf = (uint64_t)buff;
        b.aio_nbytes = len;
        b.aio_flags = IOCB_FLAG_RESFD;
        b.aio_resfd = efd;
    }

    static inline long io_setup(unsigned maxevents, aio_context_t * ctx)
    {
        return syscall(SYS_io_setup, maxevents, ctx);
    }

    static inline long io_submit(aio_context_t ctx, long nr, struct iocb ** iocbpp)
    {
        return syscall(SYS_io_submit, ctx, nr, iocbpp);
    }

    static inline long
    io_getevents(aio_context_t ctx, long min_nr, long nr, struct io_event * events, struct timespec * timeout)
    {
        return syscall(SYS_io_getevents, ctx, min_nr, nr, events, timeout);
    }

    static inline int io_destroy(aio_context_t ctx)
    {
        return syscall(SYS_io_destroy, ctx);
    }
};

std::unique_ptr<io_aio> io_aio::new_inst()
{
    return std::make_unique<io_aio_impl>();
}
}
