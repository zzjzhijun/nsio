#include <deque>
#include <memory>
#include <memory>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <linux/aio_abi.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <functional>
#include <vector>

#include <byte_buffer.h>

#include "io_loop.h"
#include "log.h"
#include "fd_aio.h"

namespace co2
{
struct io_aio_impl : io_aio
{
    static constexpr uint32_t _max_events = 256;

    aio_context_t _ctx = 0;
    int _efd = -1;
    uint64_t _req=0,_res=0;

    io_aio_impl()
    {
        if (io_setup(_max_events, &_ctx))
        {
            log_errno("zaio::io_setup failed");
            return;
        }

        _efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    }

    ~io_aio_impl()
    {
        if (_ctx != 0)
            io_destroy(_ctx);

        if (_efd != -1)
            ::close(_efd);
    }

    virtual int event_fd() noexcept override
    {
        return _efd;
    }

    virtual int on_fd_event(int) noexcept override
    {
        eventfd_t v;
        ::eventfd_read(_efd, &v);
        skip_events();
        return 1;
    }

    void skip_events()
    {
        for(;;)
        {
            struct io_event e[_max_events];
            auto rc = io_getevents(_ctx, 1, _max_events, e, 0);

            if(rc>0)
            {
                for (int i = 0; i < rc; i++)
                {
                    io_callback * icb = (io_callback *)e[i].data;
                    icb->on_fd_event(io_loop::EM_READ);
                }

                _res+=rc;
                break;
            }

            if(rc<0)
            {
                if(errno==EINTR)
                    continue;

                log_errno("io_getevents");
            }
        }
    }

    virtual void submit(iocb*pb)
    {
        for(;;)
        {
            auto rc=io_submit(_ctx, 1, &pb);
            if(rc==1)
            {
                ++_req;
                break;
            }

            if(rc==-1)
            {
                if (errno == EAGAIN)
                {
                    this->skip_events();
                    continue;
                }

                if (errno == EINTR)
                    continue;

                log_errno("io_submit");
            }
        }
    }

    virtual int aio_read(
        int fd, off_t offset, size_t count, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept override
    {
        iocb b;

        buff->set_data_begin(0);
        buff->set_data_len(count);

        set_iocb(b, fd, false, offset, buff->data(), count, _efd, icb);

        submit(&b);
        return 0;
    }

    virtual int
    aio_write(int fd, off_t offset, std::unique_ptr<byte_buffer> & buff, io_callback * icb) noexcept override
    {
        iocb b;

        set_iocb(b, fd, true, offset, buff->data(), buff->data_len(), _efd, icb);

        submit(&b);

        return 0;
    }

    static void
    set_iocb(iocb & b, int fd, bool is_write, size_t off, const void * buff, size_t len, int efd, void * icb)
    {
        memset(&b, 0, sizeof(iocb));

        b.aio_data = (uint64_t)icb;
        b.aio_lio_opcode = is_write ? IOCB_CMD_PWRITE : IOCB_CMD_PREAD;
        b.aio_fildes = fd;
        b.aio_offset = off;
        b.aio_buf = (uint64_t)buff;
        b.aio_nbytes = len;
        b.aio_flags = IOCB_FLAG_RESFD;
        b.aio_resfd = efd;
    }

    static inline long io_setup(unsigned maxevents, aio_context_t * ctx)
    {
        return syscall(SYS_io_setup, maxevents, ctx);
    }

    static inline long io_submit(aio_context_t ctx, long nr, struct iocb ** iocbpp)
    {
        return syscall(SYS_io_submit, ctx, nr, iocbpp);
    }

    static inline long
    io_getevents(aio_context_t ctx, long min_nr, long nr, struct io_event * events, struct timespec * timeout)
    {
        return syscall(SYS_io_getevents, ctx, min_nr, nr, events, timeout);
    }

    static inline int io_destroy(aio_context_t ctx)
    {
        return syscall(SYS_io_destroy, ctx);
    }
};

std::unique_ptr<io_aio> io_aio::new_inst()
{
    return std::make_unique<io_aio_impl>();
}

#if 0
struct alignas(64) zaio
{
    static constexpr uint32_t _max_events = 256;
    static constexpr uint32_t _batch_n = 32;

    std::function<void(void *)> _func;
    iocb _iocb[_batch_n];
    iocb * _iv[_batch_n];
    size_t _submitted = 0, _completed = 0;
    size_t _cache_count = 0;

    zaio(const std::function<void(void *)> & func) : _func(func)
    {

        for (size_t i = 0; i < _batch_n; i++)
        {
            _iv[i] = &_iocb[i];
        }
    }

    ~zaio()
    {
    }

    int flighting_count();

    void write_add(int fd, size_t off, const void * buff, size_t len, const void * pv)
    {
        assert(_cache_count < _batch_n);

        log_debug("aio request:fd=%d, off=%lX, len=%lX", fd, off, len);
        set_iocb(_iocb[_cache_count++], fd, true, off, buff, len, pv);

        if (_cache_count == _batch_n)
            write_submit(true);
    }

    int write_submit(bool force = false)
    {
        if (_cache_count == 0 || (!force && _cache_count < _batch_n))
        {
            return 0;
        }

        if (_max_events < _cache_count + _submitted - _completed)
        {
            size_t i = 0;
            for (i = 0;; i++)
            {
                if (_max_events >= _cache_count + flighting_count())
                    break;
                rte_delay_us(500);
            }

            if (i > 2)
            {
                log_warn("wait disk io count=%ld.", i);
            }
        }

        int ret = io_submit(_ctx, _cache_count, &_iv[0]);

        if (ret == (int)_cache_count)
        {
            _submitted += _cache_count;
            _cache_count = 0;
            return 0;
        }

        assert(false);

        if (ret < 0)
        {
            log_errno("zaio::write_submit");
            return -1;
        }
        else if (ret != (int)_cache_count)
        {
            log_errno("zaio::write_submit,Partial success (%zd instead of %zd). Bailing ou", ret, _cache_count);
            return -1;
        }

        return -1;

        //        return flighting_count();
    }

};
#endif
}
