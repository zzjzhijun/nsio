#pragma once
#include <signal.h>

struct stop_signal
{
    volatile int * _stop_run_ = 0;
    stop_signal(volatile int * stop_run) : _stop_run_(stop_run)
    {
        assert(inst() == nullptr);

        signal(SIGINT, &signal_handler);
        signal(SIGTERM, &signal_handler);

        inst() = this;
    }

    static stop_signal *& inst()
    {
        static stop_signal * inst = nullptr;
        return inst;
    }

    static void signal_handler(int signum)
    {
        auto n = *inst()->_stop_run_;
        *inst()->_stop_run_ = n + 1;

        log_warn("Signal %d received,num:%d, preparing to exit...", signum, n);
    }
};
