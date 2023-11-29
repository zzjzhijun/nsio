#pragma once

#include <io_awaiter.h>

namespace co2{

struct aio_block_rw : io_awaiter
{
    void on_aio_event(io_event & e) override
    {
        if (e.res2 < 0) [[unlikely]]
        {
            errno = e.res;
            log_errno("aio error");
        }
        else
        {
            _result = e.res;
        }

        _handle.resume();
    }
};

struct aio_read_block : aio_block_rw
{
    aio_read_block(int fd, off_t offset, size_t count, std::unique_ptr<byte_buffer> & buff) noexcept
    {
        this_context->aio_read(fd, offset, count, buff, this);
    }
};

struct aio_write_block : aio_block_rw
{
    aio_write_block(int fd, off_t offset, std::unique_ptr<byte_buffer> & buff) noexcept
    {
        this_context->aio_write(fd, offset, buff, this);
    }
};
}
