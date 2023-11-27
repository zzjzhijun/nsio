#include <cassert>
#include <cerrno>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <span>
#include <type_traits>

#include <log.h>

#include "io_context.h"

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

        if (_result != -1) [[unlikely]]
        {
            _handle.resume();
        }
    }

    int32_t await_resume() const noexcept
    {
        return result();
    }

    [[nodiscard]] int32_t result() const noexcept
    {
        return _result;
    }

    std::suspend_never detach() noexcept
    {
        return {};
    }

    io_awaiter(const io_awaiter &) = delete;
    io_awaiter(io_awaiter &&) = delete;
    io_awaiter & operator=(const io_awaiter &) = delete;
    io_awaiter & operator=(io_awaiter &&) = delete;

protected:
    int32_t _result = -1;
    std::coroutine_handle<> _handle;
    io_awaiter() noexcept
    {
    }
};

struct timeout : io_awaiter
{
    virtual void on_time_out() override final
    {
        _handle.resume();
    }

    inline explicit timeout(double num_sec) noexcept
    {
        co2::this_context->delay_call(num_sec, this);
    }

    template <class rep, class period>
    inline explicit timeout(std::chrono::duration<rep, period> duration) noexcept
    {
        auto when = std::chrono::system_clock::now() + duration;
        co2::this_context->delay_call(when, this);
    }
};

}
