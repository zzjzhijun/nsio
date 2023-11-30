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

#include "byte_buffer.h"
#include "io_context.h"
#include "zio.h"
#include "log.h"
#include "clock.h"

#include "stop_signal.h"
#include <nio_awaiter.h>

uint64_t sum_p = 0;
uint64_t sum_B = 0;

co2::task<> tcp_echo(int fd)
{
    co2::scope_exit exit_func(
        [fd]()
        {
            co2::this_context->cancel_fdall(fd);
            ::close(fd);
        });

    zclock cc;
    for (;;)
    {
        std::unique_ptr<byte_buffer> buff;
        co2::recv_request task(fd, buff, -1);

        auto len = co_await task;

        if (len <= 0) [[unlikely]]
        {
            log_errno("recv fd:%d len=%d, remote closed.", fd, len);
            break;
        }

        if (len != (int)buff->data_len()) [[unlikely]]
        {
            log_errno("recv fd:%d len=%d is_timeout=%d", fd, len, task._is_timeout);
            break;
        }

        int len_s = co_await co2::send_lazy(fd, std::span(buff->data(), buff->data_len()));
        if (len != len_s) [[unlikely]]
        {
            log_errno("send fd:%d len=%d != %d ", fd, len_s, len + 4);
            break;
        }

        sum_B += len_s;
        if (++sum_p % 100 == 0)
        {
            auto ms = cc.count_ms();
            if (ms > 500)
            {
                log_info("qps:%.2lfk MBps:%.2lf", sum_p * 1e3 / ms / 1000, sum_B * 1e3 / ms / 1024 / 1024);
                sum_p = 0;
                sum_B = 0;
                cc.reset();
            }
        }
    }
}

co2::task<> tcp_echo2(int fd)
{
    co2::scope_exit exit_func(
        [fd]()
        {
            co2::this_context->cancel_fdall(fd);
            ::close(fd);
        });

    zclock cc;
    for (;;)
    {
        std::unique_ptr<byte_buffer> buff;

        int rc = co_await co2::recv_request2(fd, buff);

        if (rc <= 0) [[unlikely]]
        {
            log_errno("recv fd:%d len=%d", fd, rc);
            break;
        }

        int len_s = co_await co2::send_lazy(fd, std::span(buff->data(), buff->data_len()));
        if (rc != len_s) [[unlikely]]
        {
            log_errno("send fd:%d len=%d != %d ", fd, len_s, rc);
            break;
        }

        sum_B += len_s;
        if (++sum_p % 100 == 0 && sum_B > (10ul << 20)) [[likely]]
        {
            auto ms = cc.count_ms();
            if (ms > 800)
            {
                log_info("qps:%.2lfk MBps:%.2lf", sum_p * 1e3 / ms / 1000, sum_B * 1e3 / ms / 1024 / 1024);
                sum_p = 0;
                sum_B = 0;
                cc.reset();
            }
        }
    }
}
volatile int force_quit = 0;

co2::task<> tcp_server(short port)
{
    int sock = zio::listen_on(port);
    co2::scope_exit _(
        [sock]()
        {
            zio::close(sock);
            force_quit = 1;
        });

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
            zio::setiobuf(fd, 4 << 20, 4 << 20);
            co2::this_context->co_spawn(tcp_echo(fd));
            continue;
        }
        break;
    }
}


int main()
{
    stop_signal ss(&force_quit);

    co2::io_context ctx;
    ctx.run_worker();

    ctx.co_spawn(tcp_server(4000));

    while (!force_quit)
    {
        usleep(10000);
    }

    ctx.stop();

    return 0;
}
