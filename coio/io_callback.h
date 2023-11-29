#pragma once

#include <linux/aio_abi.h>
namespace co2
{
struct es_file;
struct es_time;
struct io_callback
{
    //文件读写通知
    virtual void on_aio_event(io_event &)
    {
    }

    //文件读写通知
    virtual void on_file_event(es_file &, int)
    {
    }

    //定时器通知
    virtual void on_time_event(es_time &)
    {
    }

    virtual ~io_callback() noexcept = default;
};
};
