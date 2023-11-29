#pragma once

#include "io_context.h"
#include <io_awaiter.h>

namespace co2{
struct accept : io_awaiter
{
    int _serv_fd;
    explicit accept(int serv_fd) noexcept;

    void on_file_event(es_file &, int) override;
};


struct connect : timeout
{
    int _is_timeout = 0;
    int _sockfd;
    connect(int sockfd, const char * ip, uint16_t port, double time_out_sec) noexcept;

    void on_time_event(es_time &) override;
    void on_file_event(es_file &, int) override;
};

struct recv_lazy : timeout
{
    int _is_timeout = 0;
    int _recved_bytes = 0;
    int _sockfd;
    int _flags;
    std::span<char> _buff;

    recv_lazy(int sockfd, std::span<char> buf, double time_out_sec = -1, int flags = 0) noexcept;
    void on_file_event(es_file &, int) override;
    void on_time_event(es_time &) override;
};

struct send_lazy : timeout
{
    int _is_timeout = 0;
    int _sended_bytes = 0;
    int _sockfd;
    int _flags;
    std::span<char> _buff;

    send_lazy(int sockfd, std::span<char> buf, double time_out_sec = -1, int flags = 0) noexcept;
    void on_file_event(es_file &, int) override;
    void on_time_event(es_time &) override;
};

}
