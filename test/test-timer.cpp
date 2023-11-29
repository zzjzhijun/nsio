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

#include <nio_awaiter.h>

uint64_t sum_p = 0;
uint64_t sum_B = 0;


co2::task<> timer0(double num_sec)
{
    co_await co2::timeout(num_sec);
}

volatile int force_quit = 0;
co2::task<> timer1(double num_sec)
{
    co_await co2::timeout(num_sec);
    force_quit = 1;
}

static void signal_handler(int signum)
{
    log_warn("Signal %d received, preparing to exit...", signum);
    force_quit = 1;
}

int main()
{
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    co2::io_context ctx;
    ctx.run_worker();

    for (int i = 0; i < 1000000; i++)
    {
        ctx.co_spawn(timer0(0));
    }
    ctx.co_spawn(timer1(1e-9));

    while (!force_quit)
    {
        usleep(10000);
    }

    ctx.stop();

    return 0;
}
