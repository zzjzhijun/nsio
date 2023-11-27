#include <fcntl.h>
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
#include <random>

#include "byte_buffer.h"
#include "io_context.h"
#include "zio.h"
#include "log.h"
#include "clock.h"
#include "stop_signal.h"

#include <fio_awaiter.h>

volatile int force_quit = 0;

co2::task<> write_disk(std::string path, uint32_t block_size)
{
    int fd = ::open(path.c_str(), O_NONBLOCK | O_RDWR | O_DIRECT);
    if (fd < 0)
    {
        log_errno("open file:%s", path.c_str());
        co_return;
    }

    auto buff = co2::this_context->_buf_cache->malloc(block_size, 12);
    buff->set_data_begin(0);
    buff->set_data_len(block_size);
    memset(buff->data(), 'X', block_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1 << 10, 10 << 10);

    for (; !force_quit;)
    {
        size_t off = distrib(gen);
        off <<= 20;

        auto rc = co_await co2::aio_write_block(fd, off, buff);
        if (rc < 0)
        {
            log_errno("aio-write");
            break;
        }
    }

    ::close(fd);
    force_quit = 1;
}


int main()
{
    stop_signal sw(&force_quit);

    co2::io_context ctx;
    ctx.start();

    char path[128];

    for (int i = 0; i < 10; i++)
    {
        sprintf(path, "/dev/sd%c", 'a' + i);
        ctx.co_spawn(write_disk(path, 4 << 20));
    }

    while (!force_quit)
    {
        usleep(10000);
    }

    ctx.stop();

    return 0;
}
