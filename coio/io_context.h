#pragma once

#include <functional>
#include <thread>
#include <future>
#include <memory>

#include <task.h>
#include <io_loop.h>

namespace co2
{

struct io_context final : io_loop
{
    std::unique_ptr<std::thread> _loc_thread;

    io_context();

    void co_spawn(co2::task<void> && entrance) noexcept
    {
        auto handle = entrance.get_handle();
        entrance.detach();
        async_call([handle]() { handle(); });
    }

    void start();
    void stop();
    void join();

    void thread_func(std::promise<void> & ready);
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
