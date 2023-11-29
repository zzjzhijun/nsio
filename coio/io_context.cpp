
#include "io_context.h"
#include "fd_aio.h"
#include <functional>
#include <memory>

namespace co2
{

thread_local io_context * this_context = nullptr;

io_context::io_context(bool use_aio)
{
    if (use_aio)
    {
        this->_aio = io_aio::new_inst();
    }
}

io_context::~io_context()
{
}

static void _run(io_context * ic, std::function<void()> && init)
{
    if (ic->_aio)
    {
        ic->wait_read(ic->_aio->event_fd(), ic->_aio.get());
    }
    init();
    ic->run();
    if (ic->_aio)
    {
        ic->cancel_fdall(ic->_aio->event_fd());
    }
}

void io_context::thread_func(std::promise<void> & ready)
{
    this_context = this;
    ready.set_value();
    _run(this, [] {});
};

void io_context::run_worker()
{
    std::promise<void> prom;
    _loc_thread = std::make_unique<std::thread>(std::bind(&io_context::thread_func, this, std::ref(prom)));
    prom.get_future().wait();
}

void io_context::run_local(std::function<void()> init)
{
    this_context = this;
    _run(this, std::move(init));
}

void io_context::stop()
{
    this->break_run();

    if (!is_local_thread())
    {
        if (!!_loc_thread && _loc_thread->joinable())
        {
            _loc_thread->join();
        }
    }
}

}
