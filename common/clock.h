#pragma once

#include <chrono>
#include <ctime>
#include <stdio.h>

struct zclock
{
    struct std::chrono::time_point<std::chrono::steady_clock> m_start;

    zclock()
    {
        reset();
    }

    void reset()
    {
        m_start = std::chrono::steady_clock::now();
    }

    static uint64_t now_sec()
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();

        return std::chrono::duration_cast<std::chrono::seconds>(now).count();
    }

    static uint64_t now_ms()
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();

        return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    }

    static uint32_t now_sec(char * buff)
    {
        time_t sec = now_sec();
        struct tm ptm;
        return strftime(buff, 128, "%F %T", (localtime_r(&sec, &ptm)));
    }

    static uint32_t now_sec(char * buff, time_t sec)
    {
        struct tm ptm;
        return strftime(buff, 128, "%F %T", (localtime_r(&sec, &ptm)));
    }

    static uint32_t now_ms(char * buff)
    {
        auto nms = now_ms();
        time_t sec = nms / 1000;
        struct tm ptm;
        auto n = strftime(buff, 128, "%F %T", (localtime_r(&sec, &ptm)));
        int msec = nms % 1000;
        sprintf(&buff[n], ".%03d", msec);
        return n + 4;
    }

    static uint32_t now_ms(char * buff, uint64_t sec0)
    {
        auto nms = sec0;
        time_t sec = nms / 1000;
        struct tm ptm;
        auto n = strftime(buff, 128, "%F %T", (localtime_r(&sec, &ptm)));
        int msec = nms % 1000;
        sprintf(&buff[n], ".%03d", msec);
        return n + 4;
    }

    uint64_t count_ns() const
    {
        auto tmp = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::nanoseconds>(tmp - m_start).count();
    }

    uint64_t count_us() const
    {
        auto tmp = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::microseconds>(tmp - m_start).count();
    }

    uint64_t count_ms() const
    {
        auto tmp = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(tmp - m_start).count();
    }

    uint64_t count_sec() const
    {
        auto tmp = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::seconds>(tmp - m_start).count();
    }

    template <typename duration>
    uint64_t pin()
    {
        auto tmp = std::chrono::steady_clock::now();
        auto cnt = std::chrono::duration_cast<duration>(tmp - m_start).count();
        m_start = tmp;
        return cnt;
    }

    uint64_t pin_ns()
    {
        return pin<std::chrono::nanoseconds>();
    }

    uint64_t pin_us()
    {
        return pin<std::chrono::microseconds>();
    }

    uint64_t pin_ms()
    {
        return pin<std::chrono::milliseconds>();
    }
};
