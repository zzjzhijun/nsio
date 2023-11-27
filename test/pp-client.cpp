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

volatile int force_quit = 0;

co2::task<> send_recv(int fd, int send_len, int count)
{
    char * buff = new char[send_len];
    memset(buff, 'X', send_len);

    *(uint32_t *)buff = htonl(send_len - 4);

    int len;
    for (int i = 0; i < count; i++)
    {
        len = co_await co2::send_lazy(fd, std::span(buff, send_len));

        if (len != send_len) [[unlikely]]
        {
            log_errno("fd:%d len=%d", fd, len);
            break;
        }

        len = co_await co2::recv_lazy(fd, std::span(buff, send_len));

        if (len != send_len) [[unlikely]]
        {
            log_errno("fd:%d len=%d", fd, len);
            break;
        }
    }

    delete[] buff;
}

co2::task<> tcp_client(const char * ip, uint16_t port, int slen, int count)
{
    int sock = zio::build_stream();
    auto rc = co_await co2::connect(sock, ip, port);

    if (rc < 0)
        co_return;

    zclock cc;
    co_await send_recv(sock, slen, count);
    auto us = cc.count_us();
    log_info("count:%d, %ld us,  %.3lfus", count, us, us * 1.0 / count);
    co2::this_context->cancel_fdall(sock);
    zio::close(sock);

    force_quit = 1;
}


int main()
{
    stop_signal sw(&force_quit);

    co2::io_context ctx;
    ctx.start();

    ctx.co_spawn(tcp_client("127.0.0.1", 4000, 1000, 1 << 20));

    while (!force_quit)
    {
        usleep(10000);
    }

    ctx.stop();

    return 0;
}
