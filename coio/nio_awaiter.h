#pragma once

#include <io_awaiter.h>

namespace co2{

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

}
