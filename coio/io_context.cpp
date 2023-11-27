
#include "io_context.h"

namespace co2
{

thread_local io_context * this_context = nullptr;

io_context::io_context()
{
    this->wait_read(_aio->event_fd(), _aio.get());
}

void io_context::thread_func(std::promise<void> & ready)
{
    m_thread_id = std::this_thread::get_id();
    this_context = this;
    ready.set_value();

    this->run();
};

void io_context::start()
{
    std::promise<void> prom;
    _loc_thread = std::make_unique<std::thread>(std::bind(&io_context::thread_func, this, std::ref(prom)));
    prom.get_future().wait();
}

void io_context::stop()
{
    this->break_run();

    if (!is_local_thread())
    {
        this->join();
    }
}

void io_context::join()
{
    if (!!_loc_thread && _loc_thread->joinable())
    {
        _loc_thread->join();
    }
}

}
