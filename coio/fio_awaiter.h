#pragma once

#include <memory>
#include <io_awaiter.h>

namespace co2{

struct aio_read_block : io_awaiter
{
    int _fd;
    aio_read_block(int fd, off_t offset, size_t count, std::unique_ptr<byte_buffer> & buff) noexcept
    {
        _fd = fd;
        int rc = this_context->aio_read(fd, offset, count, buff, this);
    }

    int on_fd_event(int) override final;
};

struct aio_write_block : io_awaiter
{
    int _fd;
    aio_write_block(int fd, off_t offset, std::unique_ptr<byte_buffer> & buff) noexcept
    {
        _fd = fd;
        int rc = this_context->aio_write(fd, offset, buff, this);
    }

    int on_fd_event(int) override final;
};
}
