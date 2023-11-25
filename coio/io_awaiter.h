#pragma once

#include <cassert>
#include <cerrno>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <span>
#include <type_traits>

#include <log.h>

#include "io_context.h"
#include "io_loop.h"
#include "zio.h"

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

struct connect : io_awaiter
{
    int _sockfd;
    connect(int sockfd, const char * ip, uint16_t port) noexcept;

    virtual void on_time_out() override final;
    virtual int on_fd_event(int) override final;
};

struct accept : io_awaiter
{
    int _serv_fd;
    explicit accept(int serv_fd) noexcept;
    virtual int on_fd_event(int) override final;
};

struct recv_some : io_awaiter
{
    int _sockfd;
    int _flags;
    std::span<char> _buff;

    recv_some(int sockfd, std::span<char> buf, int flags = 0) noexcept;
    virtual int on_fd_event(int) override final;
};

struct recv_lazy : io_awaiter
{
    int _timer_id = -1;
    int _is_timeout = 0;
    int _recved_bytes = 0;
    int _sockfd;
    int _flags;
    std::span<char> _buff;

    recv_lazy(int sockfd, std::span<char> buf, double time_out_sec = -1, int flags = 0) noexcept;
    virtual int on_fd_event(int) override final;
    virtual void on_time_out() override final;
    void cancel_delay()
    {
        if (_timer_id < 0)
            return;
        this_context->cancel_delay(_timer_id);
    }
};

struct send_some : io_awaiter
{
    int _sockfd;
    int _flags;
    std::span<const char> _buff;

    send_some(int sockfd, std::span<const char> buf, int flags = 0) noexcept;
    virtual int on_fd_event(int) override final;
};

struct send_lazy : io_awaiter
{
    int _timer_id = -1;
    int _is_timeout = 0;
    int _sended_bytes = 0;
    int _sockfd;
    int _flags;
    std::span<char> _buff;

    send_lazy(int sockfd, std::span<char> buf, double time_out_sec = -1, int flags = 0) noexcept;
    virtual int on_fd_event(int) override final;
    virtual void on_time_out() override final;
    void cancel_delay()
    {
        if (_timer_id < 0)
            return;
        this_context->cancel_delay(_timer_id);
    }
};

#if 0
struct io_timeout_remove : io_awaiter
{
    inline io_timeout_remove(uint64_t user_data, unsigned flags) noexcept
    {
        sqe->prep_timeout_remove(user_data, flags);
    }
};

struct io_cancel_fd : io_awaiter
{
    inline io_cancel_fd(int fd, unsigned int flags) noexcept
    {
        sqe->prep_cancle_fd(fd, flags);
    }
};

struct io_connect : io_awaiter
{
    inline io_connect(int sockfd, const sockaddr * addr, socklen_t addrlen) noexcept
    {
        sqe->prep_connect(sockfd, addr, addrlen);
    }
};

struct io_read : io_awaiter
{
    inline io_read(int fd, std::span<char> buf, uint64_t offset) noexcept
    {
        sqe->prep_read(fd, buf, offset);
    }
};

struct io_write : io_awaiter
{
    inline io_write(int fd, std::span<const char> buf, uint64_t offset) noexcept
    {
        sqe->prep_write(fd, buf, offset);
    }
};

struct io_yield
{
    static constexpr bool await_ready() noexcept
    {
        return false;
    }

    static void await_suspend(std::coroutine_handle<> current) noexcept
    {
        auto & worker = *detail::this_thread.worker;
        worker.co_spawn_unsafe(current);
    }

    constexpr void await_resume() const noexcept
    {
    }

    constexpr io_yield() noexcept = default;
};

struct io_who_am_i
{
    static constexpr bool await_ready() noexcept
    {
        return false;
    }

    constexpr bool await_suspend(std::coroutine_handle<> current) noexcept
    {
        handle = current;
        return false;
    }

    [[nodiscard]] std::coroutine_handle<> await_resume() const noexcept
    {
        return handle;
    }

    constexpr io_who_am_i() noexcept = default;

    std::coroutine_handle<> handle;
};

using io_forget = std::suspend_always;

struct io_resume_on
{
public:
    static constexpr bool await_ready() noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> current) const noexcept
    {
        if (resume_ctx != detail::this_thread.ctx) [[likely]]
        {
            resume_ctx->worker.co_spawn_auto(current);
            return true;
        }
        return false;
    }

    constexpr void await_resume() const noexcept
    {
    }

    explicit io_resume_on(co2::io_context & resume_context) noexcept : resume_ctx(&resume_context)
    {
    }

private:
    co2::io_context * resume_ctx;
};

/****************************
 *    Helper for link_io    *
 ****************************
 */

inline io_link_io && operator&&(io_awaiter && lhs, struct io_link_timeout && rhs) noexcept
{
    set_link_awaiter(lhs);
    return std::move(rhs);
}

#endif

}
