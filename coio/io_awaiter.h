#include <cassert>
#include <cerrno>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <fcntl.h>
#include <span>
#include <type_traits>

#include <log.h>

#include "io_context.h"
#include "io_loop.h"

namespace co2{

struct io_awaiter : io_callback
{
    static constexpr bool await_ready() noexcept
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> current) noexcept
    {
        _handle = current;
    }

    int32_t await_resume() const noexcept
    {
        return _result;
    }

    io_awaiter(const io_awaiter &) = delete;
    io_awaiter & operator=(const io_awaiter &) = delete;
#if 1
    io_awaiter & operator=(io_awaiter && o) = delete;
    io_awaiter(io_awaiter && o) = delete;
#else
    io_awaiter & operator=(io_awaiter && o) noexcept
    {
        if (_handle != std::noop_coroutine())
        {
            _handle.destroy();
        }

        _result = o._result;
        _handle = o._handle;

        o._handle = std::noop_coroutine();

        return *this;
    }

    io_awaiter(io_awaiter && o) noexcept : _result(o._result), _handle(o._handle)
    {
        o._handle = std::noop_coroutine();
    }
#endif

    int32_t _result = -1;
    std::coroutine_handle<> _handle;

    io_awaiter() noexcept
    {
    }
};

struct timeout : io_awaiter
{
    int _timer_id = -1;

    inline explicit timeout(double num_sec) noexcept
    {
        if (num_sec > 0)
        {
            _timer_id = co2::this_context->delay_call(num_sec, this);
        }
    }

    template <class rep, class period>
    inline explicit timeout(std::chrono::duration<rep, period> duration) noexcept
    {
        if (duration.count() > 0)
        {
            auto when = std::chrono::system_clock::now() + duration;
            _timer_id = co2::this_context->delay_call(when, this);
        }
    }

    virtual void on_time_event(es_time &) override
    {
        _result = 0;
        _timer_id = 0;

        _handle.resume();
    }

    ~timeout()
    {
        if (_timer_id > 0)
        {
            this_context->cancel_delay(_timer_id);
        }
    }
};

#if 0
struct timeout_op : io_awaiter
{
    int _timer_id = -1;
    int timeout_flag = 0;

    io_awaiter * o;
    timeout delay;

    auto timer_task() -> task<>
    {
        co_await this->delay;
        this->o.cancel_request();
        timeout_flag = 1;
        this->_handle.resume();
    }

    auto other_task() -> task<>
    {
        _result = co_await this->o;
        this->delay.cancel_request();
        this->_handle.resume();
    }

    template <typename W>
    inline explicit timeout_op(W && other, double num_sec) noexcept : o(std::move(other)), delay(num_sec)
    {
        this_context->co_spawn(timer_task());
        this_context->co_spawn(other_task());
    }

    ~timeout_op()
    {
    }
};
#endif
}
