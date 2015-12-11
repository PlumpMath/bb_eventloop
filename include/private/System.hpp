#ifndef EVENTLOOP_SYSTEM_HEADER
#define EVENTLOOP_SYSTEM_HEADER

#include <ctime>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>

extern "C"
{
    struct epoll_event;
}

namespace EventLoop
{
    /**
     * Thin wrapper for used system calls. All functions are virtual, so they
     * can easily be mocked in unit tests. System class itself is not expected
     * to be unit tested.
     *
     * All functions take care of handling EINTR error, but other errors are
     * reported with SystemException (unless otherwise mentioned in the
     * documentation).
     */
    class System
    {
    public:
        System() = default;

        System(const System &) = delete;

        System &operator = (const System &) = delete;

        virtual ~System() { }

        virtual boost::posix_time::milliseconds getMonotonicClock();

        virtual int epollCreate(int flags);

        virtual void epollCtl(int                   epfd,
                              int                   op,
                              int                   fd,
                              struct epoll_event    *event);

        virtual int epollWait(int                   epfd,
                              struct epoll_event    *events,
                              int                   maxevents,
                              int                   timeout);

        virtual void pipe(int   fd[2],
                          int   flags);

        virtual int timerFDCreate(int   clockid,
                                  int   flags);

        virtual void timerFDSetTime(int                     fd,
                                    int                     flags,
                                    const struct itimerspec *new_value,
                                    struct itimerspec       *old_value);

        virtual int socket(int  domain,
                           int  type,
                           int  protocol);

        virtual void setCloseOnExec(int fd);

        virtual void setNonBlock(int fd);

        virtual void setReuseAddr(int fd);

        virtual void bind(int                   sockfd,
                          const struct sockaddr *addr,
                          socklen_t             addrlen);

        virtual void getSockName(int                sockfd,
                                 struct sockaddr    *addr,
                                 socklen_t          *addrlen);

        virtual void getPeerName(int                sockfd,
                                 struct sockaddr    *addr,
                                 socklen_t          *addrlen);

        /** @note EINPROGRESS doesn't throw an exception. */
        virtual void connect(int                    sockfd,
                             const struct sockaddr  *addr,
                             socklen_t              addrlen);

        virtual void listen(int sockfd,
                            int backlog);

        /** @note EAGAIN doesn't throw an exception. */
        virtual int accept(int              sockfd,
                           struct sockaddr  *addr,
                           socklen_t        *addrlen);

        /** @note EAGAIN doesn't throw an exception. */
        virtual ssize_t write(int           fd,
                              const void    *buf,
                              size_t        count);

        /** @note EAGAIN doesn't throw an exception. */
        virtual ssize_t read(int    fd,
                             void   *buf,
                             size_t count);

        /** @note EAGAIN doesn't throw an exception. */
        virtual ssize_t sendmsg(int                 sockfd,
                                const struct msghdr *msg,
                                int                 flags);

        /** @note EAGAIN doesn't throw an exception. */
        virtual ssize_t recvmsg(int             sockfd,
                                struct msghdr   *msg,
                                int             flags);

        virtual int getSocketError(int fd);

        /** @note No exceptions, instead calls syslog() & abort(). */
        virtual void close(int fd) noexcept;

        virtual void sigMask(int            how,
                             const sigset_t *set,
                             sigset_t       *oldset);

        virtual void sigPending(sigset_t *set);

        virtual int sigTimedWait(const sigset_t         *set,
                                 siginfo_t              *info,
                                 const struct timespec  *timeout);

        virtual int signalFD(int            fd,
                             const sigset_t *mask,
                             int            flags);

        virtual pid_t fork();

        /**
         * Return true if at least one signal was sent.
         */
        virtual bool kill(pid_t pid,
                          int   sig);

        virtual pid_t waitpid(pid_t pid,
                              int   *status,
                              int   options);

        static System &getSystem() noexcept;
    };
}

#endif /* !EVENTLOOP_SYSTEM_HEADER */
