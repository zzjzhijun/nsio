#include "byte_buffer.h"
#include "io_context.h"
#include "io_loop.h"
#include "log.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <inet_address.h>
#include <memory>
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
                errno = 0;
                return;
            }

            if (errno == ECONNABORTED)
            {
                log_warn("accept socket:%d, will again...", serv_fd);
                es._func_r = this;
                errno = 0;
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
                errno = 0;
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


void recv_request::on_time_event(es_time &)
{
    this->_is_timeout = 1;
    this_context->cancel_read(_sockfd);

    this->_handle.resume();
}

void recv_request::on_file_event(es_file & es, int event)
{
    if (event <= 0) [[unlikely]]
    {
        errno = -event;
        log_warn("socket:%d error:%d %s", _sockfd, -event, strerror(-event));
        _handle.resume();
        return;
    }

    int rc;

    if (_recved_bytes < 6)
    {
        for (; _recved_bytes < 6;)
        {
            rc = ::recv(_sockfd, this->_buf + _recved_bytes, 6 - _recved_bytes, _flags);

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
                    errno = 0;
                    return;
                }

                if (errno == EINTR)
                    continue;

                log_errno("recv_request header.");
            }
            else
            {
                log_info("socket:%d remote closed", _sockfd);
            }
            break;
        }

        if (rc <= 0 || _recved_bytes != 6)
        {
            _result = rc;
            _handle.resume();
            return;
        }

        _cmd_len = ntohl(*(uint32_t *)_buf);
        auto align_off = (uint8_t)_buf[4];
        auto align_bit = (uint8_t)_buf[5];

        if (align_off == 0)
        {
            *_request = this_context->_buf_cache->malloc(_cmd_len + 4);
            (*_request)->set_data_offset(0, _cmd_len + 4);
        }
        else
        {
            *_request = this_context->_buf_cache->malloc(_cmd_len + 4 - align_off, align_bit, align_off);
            (*_request)->set_data_offset((*_request)->body_off() - align_off, _cmd_len + 4);
        }

        memcpy((*_request)->data(), _buf, 6);
        _cmd_len += 4;
    }

    for (; _recved_bytes < _cmd_len;)
    {
        rc = ::recv(_sockfd, (*_request)->data() + _recved_bytes, _cmd_len - _recved_bytes, _flags);

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
                errno = 0;
                return;
            }

            if (errno == EINTR)
                continue;

            log_errno("recv_request body.");
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

recv_request::recv_request(int sockfd, std::unique_ptr<byte_buffer> & req, double time_out_sec, int flags) noexcept
    : timeout(time_out_sec)
{
    _request = &req;
    _sockfd = sockfd;
    _flags = flags;
    this_context->wait_read(sockfd, this);
}


task<int> recv_request2(int sockfd, std::unique_ptr<byte_buffer> & req)
{
    char header[6];
    int rc;

    rc = co_await recv_lazy(sockfd, std::span(header));
    if (rc != 6)
        co_return rc;

    uint32_t req_len = ntohl(*(uint32_t *)&header[0]);
    uint8_t & align_off = *(uint8_t *)&header[4];
    uint8_t & align_bit = *(uint8_t *)&header[5];

    if (align_bit == 0)
        align_bit = 6;

    if (align_off == 0) //不需要特殊对齐
    {
        req = this_context->_buf_cache->malloc(req_len + 4);
        req->set_data_offset(0, req_len + 4);
    }
    else
    {
        req = this_context->_buf_cache->malloc(req_len + 4 - align_off, align_bit, align_off);
        req->set_data_offset(req->body_off() - align_off, req_len + 4);
    }

    memcpy(req->data(), header, 6);

    rc = co_await recv_lazy(sockfd, std::span(req->data() + 6, req_len - 2));

    if (rc <= 0)
        co_return rc;

    co_return req->data_len();
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
                errno = 0;
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
