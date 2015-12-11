#include "private/System.hpp"

#include <type_traits>
#include <memory>
#include <ctime>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <boost/thread/once.hpp>

#include <EventLoop/SystemException.hpp>

#include "private/Abort.hpp"

namespace
{
    template<typename Func, typename... Args>
    auto tempFailureRetry(Func func, Args... args) noexcept -> typename std::result_of<Func(Args...)>::type
    {
        typename std::result_of<Func(Args...)>::type ret;
        do
        {
            ret = func(args...);
        }
        while ((ret < 0) &&
               (errno == EINTR));
        return ret;
    }
}

using namespace EventLoop;

boost::posix_time::milliseconds System::getMonotonicClock()
{
    struct timespec tp = { 0, 0 };
    ::clock_gettime(CLOCK_MONOTONIC, &tp);
    return boost::posix_time::milliseconds(tp.tv_sec * 1000L + tp.tv_nsec / 1000000L);
}

int System::epollCreate(int flags)
{
    const int fd(tempFailureRetry(::epoll_create1, flags));
    if (fd < 0)
    {
        throw SystemException("epoll_create1");
    }
    return fd;
}

void System::epollCtl(int                   epfd,
                      int                   op,
                      int                   fd,
                      struct epoll_event    *event)
{
    if (tempFailureRetry(::epoll_ctl, epfd, op, fd, event))
    {
        if (errno == EBADF)
        {
            EVENTLOOP_ABORT();
        }
        throw SystemException("epoll_ctl");
    }
}

int System::epollWait(int                   epfd,
                      struct epoll_event    *events,
                      int                   maxevents,
                      int                   timeout)
{
    const int ret(tempFailureRetry(::epoll_wait, epfd, events, maxevents, timeout));
    if (ret < 0)
    {
        if (errno == EBADF)
        {
            EVENTLOOP_ABORT();
        }
        throw SystemException("epoll_wait");
    }
    return ret;
}

void System::pipe(int   fd[2],
                  int   flags)
{
    const int ret(tempFailureRetry(::pipe2, fd, flags));
    if (ret)
    {
        throw SystemException("pipe2");
    }
}

int System::timerFDCreate(int   clockid,
                          int   flags)
{
    const int ret(tempFailureRetry(::timerfd_create, clockid, flags));
    if (ret < 0)
    {
        throw SystemException("timerfd_create");
    }
    return ret;
}

void System::timerFDSetTime(int                     fd,
                            int                     flags,
                            const struct itimerspec *new_value,
                            struct itimerspec       *old_value)
{
    const int ret(tempFailureRetry(::timerfd_settime, fd, flags, new_value, old_value));
    if (ret)
    {
        throw SystemException("timerfd_settime");
    }
}

int System::socket(int  domain,
                   int  type,
                   int  protocol)
{
    const int ret(tempFailureRetry(::socket, domain, type, protocol));
    if (ret < 0)
    {
        throw SystemException("socket");
    }
    return ret;
}

void System::setCloseOnExec(int fd)
{
    const int ret(tempFailureRetry(::fcntl, fd, F_SETFD, FD_CLOEXEC));
    if (ret)
    {
        throw SystemException("fcntl(FD_CLOEXEC)");
    }
}

void System::setNonBlock(int fd)
{
    const int ret(tempFailureRetry(::fcntl, fd, F_SETFL, O_NONBLOCK));
    if (ret)
    {
        throw SystemException("fcntl(O_NONBLOCK)");
    }
}

void System::setReuseAddr(int fd)
{
    static const int val(1);
    const int ret(tempFailureRetry(::setsockopt, fd, SOL_SOCKET, SO_REUSEADDR, &val, static_cast<socklen_t>(sizeof(val))));
    if (ret)
    {
        throw SystemException("setsockopt(SO_REUSEADDR)");
    }
}

void System::getSockName(int                sockfd,
                         struct sockaddr    *addr,
                         socklen_t          *addrlen)
{
    const int ret(tempFailureRetry(::getsockname, sockfd, addr, addrlen));
    if (ret)
    {
        throw SystemException("getsockname");
    }
}

void System::getPeerName(int                sockfd,
                         struct sockaddr    *addr,
                         socklen_t          *addrlen)
{
    const int ret(tempFailureRetry(::getpeername, sockfd, addr, addrlen));
    if (ret)
    {
        throw SystemException("getpeername");
    }
}

void System::bind(int                   sockfd,
                  const struct sockaddr *addr,
                  socklen_t             addrlen)
{
    const int ret(tempFailureRetry(::bind, sockfd, addr, addrlen));
    if (ret)
    {
        throw SystemException("bind");
    }
}

void System::connect(int                    sockfd,
                     const struct sockaddr  *addr,
                     socklen_t              addrlen)
{
    const int ret(tempFailureRetry(::connect, sockfd, addr, addrlen));
    if ((ret < 0) &&
        (errno != EINPROGRESS))
    {
        throw SystemException("connect");
    }
}

void System::listen(int sockfd,
                    int backlog)
{
    const int ret(tempFailureRetry(::listen, sockfd, backlog));
    if (ret)
    {
        throw SystemException("listen");
    }
}

int System::accept(int              sockfd,
                   struct sockaddr  *addr,
                   socklen_t        *addrlen)
{
    const int ret(tempFailureRetry(::accept, sockfd, addr, addrlen));
    if ((ret < 0) &&
        (errno != EAGAIN))
    {
        throw SystemException("accept");
    }
    return ret;
}

ssize_t System::write(int           fd,
                      const void    *buf,
                      size_t        count)
{
    const ssize_t ret(tempFailureRetry(::write, fd, buf, count));
    if ((ret < 0) &&
        (errno != EAGAIN))
    {
        throw SystemException("write");
    }
    return ret;
}

ssize_t System::read(int    fd,
                     void   *buf,
                     size_t count)
{
    const ssize_t ret(tempFailureRetry(::read, fd, buf, count));
    if ((ret < 0) &&
        (errno != EAGAIN))
    {
        throw SystemException("read");
    }
    return ret;
}

ssize_t System::sendmsg(int                 sockfd,
                        const struct msghdr *msg,
                        int                 flags)
{
    const ssize_t ret(tempFailureRetry(::sendmsg, sockfd, msg, flags));
    if ((ret < 0) &&
        (errno != EAGAIN))
    {
        throw SystemException("sendmsg");
    }
    return ret;
}

ssize_t System::recvmsg(int             sockfd,
                        struct msghdr   *msg,
                        int             flags)
{
    const ssize_t ret(tempFailureRetry(::recvmsg, sockfd, msg, flags));
    if ((ret < 0) &&
        (errno != EAGAIN))
    {
        throw SystemException("recvmsg");
    }
    return ret;
}

int System::getSocketError(int fd)
{
    int val(0);
    socklen_t size(sizeof(val));
    const int ret(tempFailureRetry(::getsockopt, fd, SOL_SOCKET, SO_ERROR, &val, &size));
    if (ret == 0)
    {
        return val;
    }
    return errno;
}

void System::close(int fd) noexcept
{
    if (tempFailureRetry(::close, fd))
    {
        /*
         * Two big reasons why not to throw an exception:
         *
         * 1) This function is usually called from destructor and throwing
         *    exceptions from destructor is bad idea and would lead to abort
         *    anyways. So better to have syslog telling why we are aborting
         *    and call abort() explicitly, than letting the default handler
         *    to abort.
         *
         * 2) EBADF (which is the most usual error code with close(2)) is a
         *    sign of programming error which would lead to extremely weird
         *    problems unless caught as early as possible.
         */
        syslog(LOG_ERR, "close(%d) failed: %s. Aborting now.\n", fd, strerror(errno));
        EVENTLOOP_ABORT();
    }
}

void System::sigMask(int            how,
                     const sigset_t *set,
                     sigset_t       *oldset)
{
    if (tempFailureRetry(::pthread_sigmask, how, set, oldset))
    {
        throw SystemException("pthread_sigmask");
    }
}

void System::sigPending(sigset_t *set)
{
    if (tempFailureRetry(::sigpending, set))
    {
        throw SystemException("sigpending");
    }
}

int System::sigTimedWait(const sigset_t         *set,
                         siginfo_t              *info,
                         const struct timespec  *timeout)
{
    const int ret(tempFailureRetry(::sigtimedwait, set, info, timeout));
    if (ret < 0)
    {
        throw SystemException("sigtimedwait");
    }
    return ret;
}

int System::signalFD(int            fd,
                     const sigset_t *mask,
                     int            flags)
{
    const int ret(tempFailureRetry(::signalfd, fd, mask, flags));
    if (ret < 0)
    {
        throw SystemException("signalfd");
    }
    return ret;
}

pid_t System::fork()
{
    const pid_t ret(::fork());
    if (ret < 0)
    {
        throw SystemException("fork");
    }
    return ret;
}

bool System::kill(pid_t pid,
                  int   sig)
{
    const int ret(::kill(pid, sig));
    if (ret)
    {
        if (errno == ESRCH)
        {
            return false;
        }
        throw SystemException("kill");
    }
    return true;
}

pid_t System::waitpid(pid_t pid,
                      int   *status,
                      int   options)
{
    const pid_t ret(tempFailureRetry(::waitpid, pid, status, options));
    if (ret < 0)
    {
        throw SystemException("waitpid");
    }
    return ret;
}

namespace
{
    std::unique_ptr<System> systemInstance;
    boost::once_flag once = BOOST_ONCE_INIT;

    void initializeSystem() noexcept
    {
        /*
         * boost::call_once documentation says that we can't throw exceptions,
         * so better to simply crash if the below line throws.
         */
        systemInstance.reset(new System());
    }
}

System &System::getSystem() noexcept
{
    /*
     * Allowing this function to throw would make implementing user classes
     * way too difficult. And since initializeSystem() can't throw either,
     * we might just as well declare this function as no throwing.
     */
    boost::call_once(&initializeSystem, once);
    return *systemInstance;
}
