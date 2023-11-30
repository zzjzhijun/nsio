#pragma once

#include <functional>
#include <thread>
#include <future>
#include <memory>
#include <deque>

#include <task.h>
#include <io_loop.h>
#include <fd_aio.h>
#include <byte_buffer.h>

struct byte_buffer;

namespace co2
{
struct io_context final : io_loop
{
    std::unique_ptr<std::thread> _loc_thread;
    std::unique_ptr<io_aio> _aio;
    std::unique_ptr<byte_buffer_pool> _buf_cache = byte_buffer_pool::new_inst(32 << 20);

    io_context(bool use_aio = false);
    ~io_context() noexcept;

    void co_spawn(co2::task<void> && entrance) noexcept
    {
        auto handle = entrance.get_handle();
        entrance.detach();
        async_call([handle]() { handle(); });
    }

    void run_worker();
    void run_local(std::function<void()> init);

    void stop();

    void thread_func(std::promise<void> & ready);

    int aio_read(int fd, off_t offset, size_t count, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept
    {
        assert(!buff.get());

        _buf_cache->malloc(count, 12, 0).swap(buff);
        return _aio->aio_read(fd, offset, count, buff, icb);
    }

    int aio_write(int fd, off_t offset, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept
    {
        return _aio->aio_write(fd, offset, buff, icb);
    }
};

extern thread_local io_context * this_context;

struct scope_exit
{
    std::function<void()> _func;
    inline scope_exit(std::function<void()> func) : _func(std::move(func))
    {
    }

    inline ~scope_exit() noexcept
    {
        _func();
    }
};

struct cancel_delay
{
    int _timer_id;
    scope_exit _se;
    inline explicit cancel_delay(int timer_id)
        : _timer_id(timer_id), _se([this] { this_context->cancel_delay(_timer_id); })
    {
    }
};

}
