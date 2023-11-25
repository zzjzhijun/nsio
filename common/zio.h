#ifndef _zio_hpp_z_
#define _zio_hpp_z_

#ifdef _MSC_VER
#    include <Windows.h>
#    include <WinSock2.h>
#else
#    include <errno.h>
#    include <fcntl.h>
#    include <netdb.h>
#    include <unistd.h>
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <netinet/tcp.h>
#    include <sys/socket.h>
#    include <sys/types.h>
#    include <sys/un.h>

#endif

#include <string>
#include <string.h>

#include "log.h"
struct zio
{
private:
    template <typename T>
    static int setoption(int fd, int level, int name, const T & opt)
    {
        //struct sockaddr_storage ss;
        return setsockopt(fd, level, name, (const char *)&opt, sizeof(T));
    }

    static sockaddr * build_addr(sockaddr * addr1, const char * host, const char * port)
    {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;

        struct addrinfo * result;

        int s = getaddrinfo(host, port, &hints, &result);
        if (s != 0)
        {
            return nullptr;
        }

        memcpy(addr1, result->ai_addr, sizeof(sockaddr_in));

        freeaddrinfo(result);

        return addr1;
    }

    static sockaddr_in * build_addr(sockaddr_in * addr, const char * ip, int port)
    {
        return build_addr(addr, ntohl(inet_addr(ip)), port);
    }

    static sockaddr_in * build_addr(sockaddr_in * addr, int ip, int port)
    {
        memset(addr, 0, sizeof(sockaddr_in));

        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = htonl(ip);
        addr->sin_port = htons(port);

        return addr;
    }

    static int bind(int fd, sockaddr_un * sa)
    {
        return ::bind(fd, (sockaddr *)sa, sizeof(*sa));
    }

    static int bind(int fd, sockaddr_in * sa)
    {
        return ::bind(fd, (sockaddr *)sa, sizeof(*sa));
    }

    static int connect(int fd, sockaddr * sa)
    {
        int rc = ::connect(fd, (sockaddr *)sa, sizeof(sockaddr));
        if (rc < 0)
        {
            log_errno("zio:socket connect to failed..");
        }

        return rc;
    }
    static int connect(int fd, sockaddr_in * sa)
    {
        int rc = ::connect(fd, (sockaddr *)sa, sizeof(sockaddr_in));
        if (rc < 0)
        {
            char name[128];
            sprintf(name, "%s:%d", inet_ntoa(((sockaddr_in *)sa)->sin_addr), ntohs(((sockaddr_in *)sa)->sin_port));
            log_errno("zio:socket connect to %s", name);
        }

        return rc;
    }

    static int connect(int fd, sockaddr_un * sa)
    {
        int rc = ::connect(fd, (sockaddr *)sa, sizeof(sockaddr_un));
        if (rc < 0)
        {
            log_errno("zio:socket connect to unix socket:%s", sa->sun_path);
        }

        return rc;
    }

    static int bind(int fd, const char * ip, int port)
    {
        sockaddr_in si;
        return zio::bind(fd, build_addr(&si, ip, port));
    }

    static int bind(int fd, int inaddr, int port)
    {
        sockaddr_in si;
        return zio::bind(fd, build_addr(&si, inaddr, port));
    }

    static int connect(int fd, int inaddr, int port)
    {
        sockaddr_in si;
        return zio::connect(fd, build_addr(&si, inaddr, port));
    }

public:
    static int connect(int fd, const char * host, const char * port)
    {
        sockaddr si;
        return zio::connect(fd, build_addr(&si, host, port));
    }

    static int connect(int fd, const char * host, int port)
    {
        sockaddr_in si;
        return zio::connect(fd, build_addr(&si, host, port));
    }

    static int connect_domain(int fd, const char * socket_path)
    {
        sockaddr_un server_addr{};
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, socket_path);

        return zio::connect(fd, &server_addr);
    }

    static int listen(int fd, const char * ip, int port, int backlog = 1024)
    {
        setnodelay(fd);
        setreuseaddr(fd);

        if (zio::bind(fd, ip, port) < 0)
        {
            log_errno("zio:socket bind (%d,'%s',%d)", fd, ip, port);
            return -1;
        }

        return ::listen(fd, backlog);
    }

    static int setnodelay(int fd, bool nodelay = true)
    {
        int v = nodelay ? 1 : 0;

        return setoption(fd, IPPROTO_TCP, TCP_NODELAY, v);
    }

    static int setiobuf(int fd, int send_size, int recv_size)
    {
        if (send_size && setoption(fd, SOL_SOCKET, SO_SNDBUF, send_size) < 0)
        {
            log_errno("zio:setoption(%d,SO_SNDBUF,%d)", fd, send_size);
            return -1;
        }

        if (recv_size && setoption(fd, SOL_SOCKET, SO_RCVBUF, recv_size) < 0)
        {
            log_errno("zio:setoption(%d,SO_RECBUF,%d)", fd, recv_size);
            return -1;
        }

        return 0;
    }

    static int setreuseaddr(int fd, bool reuse = true)
    {
        int v = reuse ? 1 : 0;

        return setoption(fd, SOL_SOCKET, SO_REUSEADDR, v);
    }

    static int setrecvtimeo(int fd, int tmout_ms)
    {
#ifdef _MSC_VER
        int tv = tmout_ms;
#else
        struct timeval tv = {tmout_ms / 1000, tmout_ms % 1000 * 1000};
#endif

        return setoption(fd, SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    static int setsendtimeo(int fd, int tmout_ms)
    {
#ifdef _MSC_VER
        int tv = tmout_ms;
#else
        struct timeval tv = {tmout_ms / 1000, tmout_ms % 1000 * 1000};
#endif

        return setoption(fd, SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    static int setblocking(int fd, bool b = true)
    {
#ifdef _MSC_VER
#else
        int opts = fcntl(fd, F_GETFL);
        if (opts < 0)
            return -1;

        opts |= b ? (opts & ~O_NONBLOCK) : (opts | O_NONBLOCK);

        if (fcntl(fd, F_SETFL, opts) < 0)
            return -1;

        return 0;
#endif
    }

    static int close(int fd)
    {
        if (fd == -1)
            return 0;

#ifdef _MSC_VER
        return ::closesocket(fd);
#else
        return ::close(fd);
#endif
    }

    static int accept(int serv_fd, char * name)
    {
        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        int fd = -1;
        for (;;)
        {
            socklen_t addrlen = sizeof(struct sockaddr_in);
            fd = (int)::accept(serv_fd, (sockaddr *)&addr, &addrlen);
            if (fd < 0)
            {
#ifdef _MSC_VER
#else
                if (errno == EINTR || errno == ECONNABORTED)
                    continue;
#endif
            }

            if (name && fd >= 0)
            {
                sprintf(name, "%s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                log_debug("zio:client %d(%s) connected", fd, name);
            }
            break;
        }
        return fd;
    }

    static int build_stream_unix_domain()
    {
        return (int)::socket(AF_UNIX, SOCK_STREAM, 0);
    }

    static int build_stream()
    {
        return (int)::socket(AF_INET, SOCK_STREAM, 0);
    }

    static int write(int fd, const void * buff, int count)
    {
        const char * buf = (const char *)buff;
        for (;;)
        {
            int rc = ::write(fd, buf, count);
            if (rc == -1)
            {
                switch (errno)
                {
#ifndef _MSC_VER
                    case EINTR:
                        continue;
#endif
                    case EAGAIN:
#ifdef _MSC_VER
                    case EWOULDBLOCK:
#endif
                        return -2;
                    default:
                        log_errno("zio:socket %d write", fd);
                        return -1;
                }
            }

            return rc;
        }
    }

    static int read(int fd, void * buff, int count)
    {
        char * buf = (char *)buff;
        for (;;)
        {
            int rc = ::read(fd, buf, count);
            if (rc == -1)
            {
                switch (errno)
                {
#ifndef _MSC_VER
                    case EINTR:
                        continue;
#endif
                    case EAGAIN:
#ifdef _MSC_VER
                    case EWOULDBLOCK:
#endif
                        return -2;
                    default:
                        log_errno("zio:socket %d read", fd);
                        return -1;
                }
            }

            return rc;
        }
    }
    static int writev(int fd, const void * buff, int count)
    {
        const char * buf = (const char *)buff;
        int rc, pos = 0;
        while (pos != count)
        {
            if ((rc = zio::write(fd, buf, count - pos)) > 0)
            {
                pos += rc;
                buf += rc;
                continue;
            }

            if (rc == -1)
                break;
        }

        return pos == 0 ? -1 : pos;
    }

    static int readv(int fd, void * buff, int count)
    {
        char * buf = (char *)buff;
        int rc, pos = 0;
        while (pos != count)
        {
            if ((rc = zio::read(fd, buf, count - pos)) > 0)
            {
                pos += rc;
                buf += rc;
                continue;
            }

            if (rc == 0)
                return pos > 0 ? pos : 0;

            if (rc == -1)
                break;
        }

        return pos == 0 ? -1 : pos;
    }

    static int listen_on(int port)
    {
        int fd = zio::build_stream();
        zio::setreuseaddr(fd, true);
        int rc = zio::listen(fd, "0.0.0.0", port, 1024);
        if (rc < 0)
        {
            zio::close(fd);
            return -1;
        }

        return fd;
    }

    static std::string get_local_ip(int fd)
    {
        struct sockaddr addr;
        socklen_t addr_len = sizeof(sockaddr);

        memset(&addr, 0, addr_len);

        int rc = getsockname(fd, &addr, &addr_len);

        if (rc < 0)
        {
            log_errno("zio::get_local_ip");
            return "";
        }

        struct sockaddr_in * a = ((sockaddr_in *)&addr);

        return inet_ntoa(a->sin_addr);
    }
};

#endif
