
#pragma once


#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <functional>
#include <future>
#include <memory>

#include <log.h>

namespace co2
{
struct io_callback
{
    //文件读写通知
    virtual int on_fd_event(int)
    {
        return 0;
    }

    //定时器通知
    virtual void on_time_out()
    {
    }

    virtual ~io_callback() noexcept = default;
};

struct alignas(64) io_loop : io_callback //for-eventfd
{
    enum
    {
        EM_READ = 1,
        EM_WRITE = 2
    };

    enum
    {
        EV_IN = EPOLLIN,
        EV_OUT = EPOLLOUT,
        EV_ERR = EPOLLHUP | EPOLLERR | EPOLLRDHUP //only for test
    };

    volatile int m_breaking{0};
    alignas(64) std::thread::id m_thread_id;
    int _epoll_fd, _event_fd;
    int _g_timer_id = 0;

    struct alignas(32) fd_task
    {
        int _fd = -1;
        uint32_t _event = 0;
        int16_t _enable_read = 0;
        int16_t _enable_write = 0;
        uint16_t _read_func_version = 0;
        uint16_t _write_func_version = 0;
        io_callback * _func_w = nullptr;
        io_callback * _func_r = nullptr;
    };

    std::vector<fd_task> _fds;

    struct timer_task
    {
        int _id;
        std::chrono::time_point<std::chrono::system_clock> _when;
        io_callback * _func = nullptr;
    };

    std::vector<timer_task> _timer;
    std::vector<std::function<void()>> _async_funcs_tmp;


    alignas(64) std::mutex _async_mutex;
    std::vector<std::function<void()>> _async_funcs;

    static bool compare_timer(const timer_task & l, const timer_task & r)
    {
        return l._when > r._when;
    }

    io_loop() noexcept
    {
        _fds.reserve(256);
        _async_funcs.reserve(64);
        _async_funcs_tmp.reserve(64);
        _timer.reserve(64);

        _epoll_fd = epoll_create(256);
        _event_fd = eventfd(0, 0);
        fcntl(_event_fd, F_SETFL, O_NONBLOCK);

        this->wait_read(_event_fd, this);
    }

    io_loop(const io_loop &) = delete;
    io_loop(io_loop &&) = delete;

    ~io_loop() noexcept
    {
        Epoll_ctl_del(_event_fd);
        close(_event_fd);
        close(_epoll_fd);
    }

    auto now() const noexcept
    {
        return std::chrono::system_clock::now();
    }

    int on_fd_event(int)
    {
        uint64_t val;
        eventfd_read(_event_fd, &val);
        {
            std::unique_lock lock(this->_async_mutex);
            _async_funcs_tmp.swap(_async_funcs);
        }

        for (auto & func : _async_funcs_tmp)
        {
            func();
        }

        _async_funcs_tmp.clear();
        return 1;
    }

    int run()
    {
        m_thread_id = std::this_thread::get_id();

        struct epoll_event events[64];
        while (!m_breaking) [[likely]]
        {
            int timeout;
            if (!_timer.empty())
            {
                const int left = std::chrono::duration_cast<std::chrono::milliseconds>(_timer[0]._when - now()).count();

                timeout = std::min(100, std::max(0, left));
            }
            else
            {
                timeout = 100;
            }

            int ret = epoll_wait(_epoll_fd, events, sizeof(events) / sizeof(events[0]), timeout);

            if (ret < 0)
            {
                if (errno == EINTR)
                    continue;

                log_errno("epoll error:");
                break;
            }

            for (int n = 0; n < ret; n++)
            {
                int fd = events[n].data.fd;

                if (fd >= (int)_fds.size()) [[unlikely]]
                    continue;

                auto & fe = _fds[fd];

                if (events[n].events & EV_ERR) [[unlikely]]
                {
                    int e = 0;
                    socklen_t elen = sizeof(e);
                    int rc = ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &e, &elen);
                    if (rc == -1 && errno == ENOTSOCK)
                    {
                        e = 0;
                    }

                    if (fe._enable_write)
                    {
                        fe._enable_write = 0;
                        //fe._write_func_version++;
                        fe._func_w->on_fd_event(-e);
                    }
                    if (fe._enable_read)
                    {
                        fe._enable_read = 0;
                        //fe._read_func_version++;
                        fe._func_r->on_fd_event(-e);
                    }
                }
                else
                {
                    if (fe._enable_write && (events[n].events & EV_OUT))
                    {
                        assert(fe._event & EPOLLOUT);
                        const auto ver = fe._write_func_version;
                        auto rc = fe._func_w->on_fd_event(io_loop::EM_WRITE);
                        if (ver == fe._write_func_version)
                        {
                            fe._enable_write = rc;
                        }
                    }

                    if (fe._enable_read && (events[n].events & EV_IN))
                    {
                        assert(fe._event & EPOLLIN);
                        auto ver = fe._read_func_version;
                        auto rc = fe._func_r->on_fd_event(io_loop::EM_READ);
                        if (ver == fe._read_func_version)
                        {
                            fe._enable_read = rc;
                        }
                    }
                }
            }

            while (!_timer.empty() && _timer[0]._when <= now())
            {
                {
                    std::pop_heap(_timer.begin(), _timer.end(), compare_timer);
                    auto t = std::move(_timer.back());
                    _timer.pop_back();

                    t._func->on_time_out();
                }
            }
        }

        return 0;
    }

    void break_run() noexcept
    {
        this->m_breaking = 1;
    }

    int cancel_ev(int fd, int ev) noexcept
    {
        assert(is_local_thread());

        if ((int)_fds.size() <= fd)
            return -1;

        auto & fe = _fds[fd];

        assert(fe._fd != -1);

        fe._event &= ~ev;

        if (fe._event & (EPOLLIN | EPOLLOUT))
        {
            return Epoll_ctl_mod(fd, fe._event);
        }
        else
        {
            fe._fd = -1;
            return Epoll_ctl_del(fd);
        }
    }

    void cancel_read(int fd) noexcept
    {
#if 0
        _fds_pendding.emplace_back(fd, 2, EPOLLIN, [](int) -> int { return 0; });
#else
        cancel_ev(fd, EPOLLIN);
#endif
    }

    void cancel_write(int fd) noexcept
    {
#if 0
        _fds_pendding.emplace_back(fd, 2, EPOLLOUT, [](int) -> int { return 0; });
#else
        cancel_ev(fd, EPOLLOUT);
#endif
    }

    void cancel_fdall(int fd) noexcept
    {
#if 0
        _fds_pendding.emplace_back(fd, 2, EPOLLIN | EPOLLOUT, [](int) -> int { return 0; });
#else
        cancel_ev(fd, EPOLLIN | EPOLLOUT);
#endif
    }

    int Epoll_ctl_add(int fd, int type) noexcept
    {
        log_debug("epoll-add %d", fd);

        struct epoll_event ev;
        ev.data.u64 = 0;
        ev.data.fd = fd;
        ev.events = type;

        return epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    }

    int Epoll_ctl_mod(int fd, int type) noexcept
    {
        log_debug("epoll-mod %d", fd);
        struct epoll_event ev;
        ev.data.u64 = 0;
        ev.data.fd = fd;
        ev.events = type;

        return epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    }

    int Epoll_ctl_del(int fd) noexcept
    {
        log_debug("epoll-del %d", fd);
        return epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    }

    int wait_fd(int fd, int type, io_callback * func) noexcept
    {
        if ((int)_fds.size() <= fd)
        {
            _fds.resize(fd + 1);
        }

        auto & fe = _fds[fd];

        if (fe._fd == -1)
        {
            fe._fd = fd;
            fe._enable_write = fe._enable_read = 0;
            fe._event = type;

            if (type & EPOLLIN)
            {
                fe._enable_read = 1;
                fe._read_func_version++;
                fe._func_r = func;
            }
            else
            {
                fe._enable_write = 1;
                fe._write_func_version++;
                fe._func_w = func;
            }

            return Epoll_ctl_add(fd, type);
        }
        else
        {
            if (type & EPOLLIN)
            {
                fe._enable_read = 1;
                ++fe._read_func_version;
                fe._func_r = func;

                if (fe._event & EPOLLIN)
                    return 0;
            }
            else
            {
                fe._enable_write = 1;
                ++fe._write_func_version;
                fe._func_w = func;

                if (fe._event & EPOLLOUT)
                    return 0;
            }

            fe._event |= type;

            Epoll_ctl_mod(fd, fe._event);
        }

        return 0;
    }

    void wait_read(int fd, io_callback * f) noexcept
    {
        wait_fd(fd, EPOLLIN, f);
    }

    void wait_write(int fd, io_callback * f) noexcept
    {
        wait_fd(fd, EPOLLOUT, f);
    }

    int delay_call(double after, io_callback * func) noexcept
    {
        auto when = now() + std::chrono::nanoseconds(uint64_t(after * 1e9));
        return delay_call(when, func);
    }

    int delay_call(const std::chrono::time_point<std::chrono::system_clock> & when, io_callback * func) noexcept
    {
        timer_task tt;

        tt._id = ++_g_timer_id;
        tt._when = when;
        tt._func = func;

        this->_timer.push_back(tt);
        std::push_heap(this->_timer.begin(), this->_timer.end(), compare_timer);

        return _g_timer_id;
    }

    void cancel_delay(int timer_id) noexcept
    {
        assert(is_local_thread());
        auto it = std::find_if(_timer.begin(), _timer.end(), [timer_id](auto & task) { return task._id == timer_id; });
        if (it != _timer.end())
        {
            _timer.erase(it);
            std::make_heap(this->_timer.begin(), this->_timer.end(), compare_timer);
        }
    }

    template <typename T>
    std::future<T> async(std::function<T()> func) noexcept
    {
        assert(!is_local_thread());

        auto prom = std::make_shared<std::promise<T>>();
        auto fu = prom->get_future();

        {
            std::unique_lock lock(_async_mutex);
            _async_funcs.emplace_back(
                [f = std::move(func), p = std::move(prom)]()
                {
                    if constexpr (std::is_void_v<T>)
                    {
                        f();
                        p->set_value();
                    }
                    else
                        p->set_value(f());
                });
        }

        eventfd_write(this->_event_fd, 1);

        return fu;
    }

    std::future<void> async_call(std::function<void()> func) noexcept
    {
        if (is_local_thread()) [[unlikely]]
        {
            std::promise<void> res;
            func();
            res.set_value();
            return res.get_future();
        }
        else
        {
            return async(std::move(func));
        }
    }

    bool is_local_thread() const noexcept
    {
        return std::this_thread::get_id() == m_thread_id;
    }
};
}
