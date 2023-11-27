#pragma once

struct byte_buffer;

namespace co2
{
struct io_aio
{
    virtual int aio_read(int fd, off_t offset, size_t count, std::unique_ptr<byte_buffer> & buff) noexcept = 0;
    virtual int aio_write(int fd, off_t offset, std::unique_ptr<byte_buffer> & buff) noexcept = 0;

    virtual void aio_complete(int efd) noexcept = 0;

    virtual ~io_aio() noexcept = default;

    static std::unique_ptr<io_aio> new_inst();
};

}
