#include <unistd.h>
#include <assert.h>
#include <netinet/in.h>

#include <chrono>
#include <csignal>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

#include "zio.h"
#include "log.h"
#include "clock.h"

#include "stop_signal.h"
#include <nio_awaiter.h>

uint64_t sum_p = 0;
uint64_t sum_B = 0;

zclock cc;
co2::task<> tcp_echo1(int fd)
{
    co2::scope_exit exit_func(
        [fd]()
        {
            co2::this_context->cancel_fdall(fd);
            ::close(fd);
        });

    constexpr int N = 1000;
    char * buff = new char[N];
    for (;;)
    {
        co2::recv_lazy task(fd, std::span(buff, N), -1);

        auto len = co_await task;

        if (len != N) [[unlikely]]
        {
            log_errno("fd:%d len=%d is_timeout=%d", fd, len, task._is_timeout);
            break;
        }

        len = co_await co2::send_lazy(fd, std::span(buff, N));
        if (len != N) [[unlikely]]
        {
            log_errno("fd:%d len=%d is_timeout=%d", fd, len, task._is_timeout);
            break;
        }

        sum_B += len;
        if (++sum_p % 1000 != 0) [[likely]]
            continue;

        auto ms = cc.count_ms();
        if (ms > 800)
        {
            log_info("qps:%.2lfk MBps:%.2lf", sum_p * 1e3 / ms / 1000, sum_B * 1e3 / ms / 1024 / 1024);
            sum_p = 0;
            sum_B = 0;
            cc.reset();
        }
    }

    delete[] buff;
}
co2::task<> tcp_echo2(int fd)
{
    co2::scope_exit exit_func(
        [fd]()
        {
            co2::this_context->cancel_fdall(fd);
            ::close(fd);
        });

    char * buff = new char[8192];
    for (;;)
    {
        co2::recv_lazy task(fd, std::span(buff, 4), -1);

        auto len = co_await task;

        if (len != 4) [[unlikely]]
        {
            log_errno("fd:%d len=%d is_timeout=%d", fd, len, task._is_timeout);
            break;
        }

        int len_n = htonl(*(uint32_t *)&buff[0]);

        co2::recv_lazy task_data(fd, std::span(buff + 4, len_n), -1);
        len = co_await task_data;

        if (len != len_n) [[unlikely]]
        {
            log_errno("fd:%d len=%d != %d is_timeout=%d", fd, len, len_n, task_data._is_timeout);
            break;
        }

        int len_s = co_await co2::send_lazy(fd, std::span(buff, len + 4));
        if (len + 4 != len_s) [[unlikely]]
        {
            log_errno("fd:%d len=%d != %d is_timeout=%d", fd, len_s, len + 4, task_data._is_timeout);
            break;
        }

        sum_B += len_s;
        if (++sum_p % 10000 != 0) [[likely]]
            continue;

        auto ms = cc.count_ms();
        if (ms > 800)
        {
            log_info("qps:%.2lfk MBps:%.2lf", sum_p * 1e3 / ms / 1000, sum_B * 1e3 / ms / 1024 / 1024);
            sum_p = 0;
            sum_B = 0;
            cc.reset();
        }
    }

    delete[] buff;
}

co2::task<> tcp_server(short port)
{
    int sock = zio::listen_on(port);

    if (sock < 0)
    {
        co_return;
    }

    zio::setblocking(sock, false);
    for (;;)
    {
        int fd = co_await co2::accept(sock);
        if (fd >= 0)
        {
            co2::this_context->co_spawn(tcp_echo1(fd));
            continue;
        }
        break;
    }

    zio::close(sock);
}

volatile int force_quit = 0;

int main()
{
    stop_signal ss(&force_quit);

    co2::io_context ctx;
    ctx.start();

    ctx.co_spawn(tcp_server(4000));

    while (!force_quit)
    {
        usleep(10000);
    }

    ctx.stop();

    return 0;
}
