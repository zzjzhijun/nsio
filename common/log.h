
#pragma once

#include <stdio.h>
#include <string.h>
#include <chrono>

inline uint32_t _log_time(char * buff)
{
    auto nms
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count();
    time_t sec = nms / 1000;
    struct tm ptm;
    auto n = strftime(buff, 128, "%F %T", (localtime_r(&sec, &ptm)));
    int msec = nms % 1000;
    sprintf(&buff[n], ".%03d", msec);
    return n + 4;
}

inline void _log_print(const char * s, int len)
{
    fwrite(s, 1, len, stderr);
}

#define log_impl(level, F, L, fmt, ...) \
    do \
    { \
        char _log_buf_[4096]; \
        int _n_ = sprintf(&_log_buf_[0], "%s ", #level); \
        _n_ += _log_time(&_log_buf_[_n_]); \
        _n_ += sprintf(&_log_buf_[_n_], " %s:%d ", strrchr(F, '/') + 1, L); \
        _n_ += snprintf(&_log_buf_[_n_], 4096 - _n_ - 1, fmt "\n", ##__VA_ARGS__); \
        _log_print(_log_buf_, _n_); \
    } while (0)

#define log_debug(fmt, ...) log_impl(D, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) log_impl(I, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) log_impl(W, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_impl(E, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_errno(fmt, ...) log_impl(E, __FILE__, __LINE__, fmt ":%s", ##__VA_ARGS__, strerror(errno))

#define log_nt(fmt, ...) \
    do \
    { \
        char buff[4096]; \
        int n = sprintf(&buff[0], fmt "\n", ##__VA_ARGS__); \
        log_print(buff, n); \
    } while (0)
