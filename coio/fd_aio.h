#pragma once

#include <io_callback.h>

struct byte_buffer;

namespace co2
{
struct io_aio : io_callback
{
    virtual int
    aio_read(int fd, off_t offset, size_t count, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept
        = 0;
    virtual int aio_write(int fd, off_t offset, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept = 0;

    virtual void on_file_event(es_file &, int) = 0;

    virtual int event_fd() noexcept = 0;

    virtual ~io_aio() noexcept = default;

    static std::unique_ptr<io_aio> new_inst();
};

}
