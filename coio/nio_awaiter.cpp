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

#include <nio_awaiter.h>

namespace co2
{

void accept::on_file_event(es_file & es, int)
{
    const auto serv_fd = es._fd;
    int fd;
    for (;;)
    {
        fd = (int)::accept(serv_fd, nullptr, nullptr);
        if (fd < 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN)
            {
                es._func_r = this;
                return;
            }

            if (errno == ECONNABORTED)
            {
                log_warn("accept socket:%d, will again...", serv_fd);
                es._func_r = this;
                return;
            }

            log_errno("accept socket:%d", serv_fd);
        }

        _result = fd;

        log_debug("socket %d connected", fd);
        _handle.resume();
    }
}

accept::accept(int serv_fd) noexcept : _serv_fd(serv_fd)
{
    this_context->wait_read(serv_fd, this);
}

void connect::on_time_event(es_time &)
{
    this->_is_timeout = 1;
    this->_timer_id = 0;

    log_debug("socket %d do-connect timeout.", _sockfd);
    this_context->cancel_write(_sockfd);
    this->_handle.resume();
}

void connect::on_file_event(es_file & es, int event)
{
    if (event <= 0)
    {
        errno = -event;
        this_context->cancel_write(_sockfd);
        log_errno("connect");
    }
    else if (event == io_loop::EM_WRITE)
    {
        int e = 0;
        socklen_t elen = sizeof(e);
        ::getsockopt(es._fd, SOL_SOCKET, SO_ERROR, &e, &elen);
        if (e == 0)
        {
            this->_result = 0;
        }

        log_debug("socket %d do-connect ok.", _sockfd);
    }

    this->_handle.resume();
}

connect::connect(int sockfd, const char * ip, uint16_t port, double time_out_sec) noexcept : timeout(time_out_sec)
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
                this_context->wait_write(sockfd, this);
                break;
            }

            log_errno("connect");
        }

        break;
    }
}


void recv_lazy::on_time_event(es_time &)
{
    this->_is_timeout = 1;
    this_context->cancel_read(_sockfd);

    this->_handle.resume();
}

void recv_lazy::on_file_event(es_file & es, int event)
{
    if (event <= 0) [[unlikely]]
    {
        errno = -event;
        log_warn("socket:%d error:%d %s", _sockfd, -event, strerror(-event));
        _handle.resume();
        return;
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
            if (errno == EAGAIN) [[likely]]
            {
                es._func_r = this;
                return;
            }

            if (errno == EINTR)
                continue;

            log_errno("recv_lazy");
        }
        else
        {
            log_info("socket:%d remote closed", _sockfd);
        }

        _result = rc;
        _handle.resume();
        return;
    }

    _result = _recved_bytes;
    _handle.resume();
}

recv_lazy::recv_lazy(int sockfd, std::span<char> buf, double time_out_sec, int flags) noexcept : timeout(time_out_sec)
{
    _sockfd = sockfd;
    _flags = flags;
    _buff = buf;
    this_context->wait_read(sockfd, this);
}


void send_lazy::on_time_event(es_time &)
{
    this->_is_timeout = 1;
    this->_timer_id = 0;
    this->_result = 0;
    this_context->cancel_write(_sockfd);

    this->_handle.resume();
}

void send_lazy::on_file_event(es_file & es, int event)
{
    if (event <= 0) [[unlikely]]
    {
        log_warn("socket:%d error:%d %s", _sockfd, -event, strerror(-event));
        _handle.resume();
        return;
    }

    for (; _sended_bytes < (int)_buff.size();)
    {
        auto rc = ::send(_sockfd, _buff.data() + _sended_bytes, _buff.size() - _sended_bytes, _flags);

        if (rc < 0)
        {
            if (errno == EAGAIN)
            {
                es._func_w = this;
                return;
            }

            if (errno == EINTR)
                continue;

            log_errno("send_lazy");
        }
        else if (rc > 0)
        {
            _sended_bytes += rc;
            continue;
        }

        _handle.resume();
        return;
    }

    _result = _sended_bytes;
    _handle.resume();
    return;
}

send_lazy::send_lazy(int sockfd, std::span<char> buf, double time_out_sec, int flags) noexcept
    : timeout(time_out_sec), _sockfd(sockfd), _flags(flags), _buff(buf)
{
    this_context->wait_write(sockfd, this);
    if (time_out_sec > 0)
    {
        _timer_id = this_context->delay_call(time_out_sec, this);
    }
}
}
