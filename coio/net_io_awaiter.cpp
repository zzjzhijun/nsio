#include "io_context.h"
#include "io_loop.h"
#include "log.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <inet_address.h>
#include <sys/socket.h>
#include <utility>
#include <zio.h>

#include <io_awaiter.h>

namespace co2
{

int accept::on_fd_event(int)
{
    const auto serv_fd = _serv_fd;

    int fd;
    for (;;)
    {
        fd = (int)::accept(serv_fd, nullptr, nullptr);
        if (fd < 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN)
                return 1;

            if (errno == ECONNABORTED)
            {
                log_warn("accept socket:%d, will again...", serv_fd);
                return 1;
            }

            log_errno("accept socket:%d", serv_fd);
        }

        _result = fd;
        _handle.resume();
        return 0;
    }
}

accept::accept(int serv_fd) noexcept : _serv_fd(serv_fd)
{
    this_context->wait_read(serv_fd, this);
}

void connect::on_time_out()
{
}

int connect::on_fd_event(int event)
{
    if (event == io_loop::EM_WRITE)
    {
        int e = 0;
        socklen_t elen = sizeof(e);
        ::getsockopt(_sockfd, SOL_SOCKET, SO_ERROR, &e, &elen);
        if (e == 0)
        {
            this->_result = 0;
        }
    }

    this->_handle.resume();
    return 0;
}

connect::connect(int sockfd, const char * ip, uint16_t port) noexcept
{
    _sockfd = sockfd;

    inet_address ad(ip, port);
    zio::setblocking(sockfd, false);

    for (;;)
    {
        auto rc = ::connect(sockfd, ad.get_sockaddr(), ad.length());
        if (rc < 0) [[likely]]
        {
            if (errno == EINTR)
                continue;

            if (errno == EINPROGRESS)
            {
                errno = 0;
                this_context->wait_write(sockfd, this);
                break;
            }

            log_errno("recv_some");
            break;
        }
        else if (rc == 0)
        {
            _result = 0;
            return;
        }
    }
}


int recv_some::on_fd_event(int event)
{
    if (event <= 0) [[unlikely]]
    {
        log_warn("socket:%d error:%d %s", _sockfd, -event, strerror(-event));
        _handle.resume();
        return 0;
    }

    for (;;)
    {
        auto rc = ::recv(_sockfd, _buff.data(), _buff.size(), _flags);

        if (rc < 0) [[unlikely]]
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN)
                return 1;

            log_errno("recv_some");
        }

        _result = rc;
        _handle.resume();
        return 0;
    }
}

recv_some::recv_some(int sockfd, std::span<char> buf, int flags) noexcept : _sockfd(sockfd), _flags(flags), _buff(buf)
{
    this_context->wait_read(sockfd, this);
}


void recv_lazy::on_time_out()
{
    this->_is_timeout = 1;
    this->_result = 0;
    this_context->cancel_read(_sockfd);

    this->_handle.resume();
}

int recv_lazy::on_fd_event(int event)
{
    if (event <= 0) [[unlikely]]
    {
        log_warn("socket:%d error:%d %s", _sockfd, -event, strerror(-event));

        cancel_delay();
        _handle.resume();
        return 0;
    }

    for (; _recved_bytes < (int)_buff.size();)
    {
        auto rc = ::recv(_sockfd, _buff.data() + _recved_bytes, _buff.size() - _recved_bytes, _flags);

        if (rc > 0)
        {
            _recved_bytes += rc;
            continue;
        }
        else if (rc < 0)
        {
            if (errno == EAGAIN)
                return 1;

            if (errno == EINTR)
                continue;

            log_errno("recv_lazy");
        }
        else
        {
            log_info("socket:%d remote closed", _sockfd);
        }

        _result = rc;
        cancel_delay();
        _handle.resume();

        return 0;
    }

    cancel_delay();

    _result = _recved_bytes;
    _handle.resume();

    return 0;
}

recv_lazy::recv_lazy(int sockfd, std::span<char> buf, double time_out_sec, int flags) noexcept
{
    if (time_out_sec > 0)
    {
        _timer_id = this_context->delay_call(time_out_sec, this);
    }

    _sockfd = sockfd;
    _flags = flags;
    _buff = buf;

    this_context->wait_read(sockfd, this);
}


int send_some::on_fd_event(int event)
{
    const auto sockfd = _sockfd;
    const auto flags = _flags;

    if (event <= 0) [[unlikely]]
    {
        log_warn("socket:%d error:%d %s", sockfd, -event, strerror(-event));
        this_context->cancel_write(sockfd);
        _handle.resume();
    }

    for (;;)
    {
        auto rc = ::send(sockfd, _buff.data(), _buff.size(), flags);

        if (rc < 0)
        {
            if (errno == EAGAIN)
                return 1;

            if (errno == EINTR)
                continue;

            log_errno("send_some");
        }

        _result = rc;
        _handle.resume();
        return 0;
    }
}

send_some::send_some(int sockfd, std::span<const char> buf, int flags) noexcept
    : _sockfd(sockfd), _flags(flags), _buff(buf)
{
    this_context->wait_write(sockfd, this);
}

void send_lazy::on_time_out()
{
    this->_is_timeout = 1;
    this->_result = 0;
    this_context->cancel_write(_sockfd);
    this->_handle.resume();
}

int send_lazy::on_fd_event(int event)
{
    if (event <= 0) [[unlikely]]
    {
        log_warn("socket:%d error:%d %s", _sockfd, -event, strerror(-event));
        cancel_delay();
        this_context->cancel_delay(_timer_id);
        _handle.resume();
        return 0;
    }

    for (; _sended_bytes < (int)_buff.size();)
    {
        auto rc = ::send(_sockfd, _buff.data() + _sended_bytes, _buff.size() - _sended_bytes, _flags);

        if (rc > 0)
        {
            _sended_bytes += rc;
            continue;
        }
        else if (rc < 0)
        {
            if (errno == EAGAIN)
                return 1;

            if (errno == EINTR)
                continue;

            log_errno("send_lazy");
        }

        cancel_delay();
        _handle.resume();
        return 0;
    }

    cancel_delay();
    _result = _sended_bytes;
    _handle.resume();
    return 0;
}

send_lazy::send_lazy(int sockfd, std::span<char> buf, double time_out_sec, int flags) noexcept
    : _sockfd(sockfd), _flags(flags), _buff(buf)
{
    if (time_out_sec > 0)
    {
        _timer_id = this_context->delay_call(time_out_sec, this);
    }

    this_context->wait_write(sockfd, this);
}
}
