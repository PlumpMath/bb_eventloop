#ifndef EVENTLOOP_SYSTEMMOCK_HEADER
#define EVENTLOOP_SYSTEMMOCK_HEADER

#include <gmock/gmock.h>

#include "private/System.hpp"

namespace EventLoop
{
    namespace Test
    {
        class SystemMock: public System
        {
        public:
            MOCK_METHOD0(getMonotonicClock, boost::posix_time::milliseconds());
            MOCK_METHOD1(epollCreate, int(int flags));
            MOCK_METHOD4(epollCtl, void(int epfd, int op, int fd, struct epoll_event *event));
            MOCK_METHOD4(epollWait, int(int epfd, struct epoll_event *events, int maxevents, int timeout));
            MOCK_METHOD2(pipe, void(int fd[2], int flags));
            MOCK_METHOD2(timerFDCreate, int(int clockid, int flags));
            MOCK_METHOD4(timerFDSetTime, void(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value));
            MOCK_METHOD3(socket, int(int domain, int type, int protocol));
            MOCK_METHOD1(setCloseOnExec, void(int fd));
            MOCK_METHOD1(setNonBlock, void(int fd));
            MOCK_METHOD1(setReuseAddr, void(int fd));
            MOCK_METHOD1(setTCPNoDelay, void(int fd));
            MOCK_METHOD3(bind, void(int sockfd, const struct sockaddr *addr, socklen_t addrlen));
            MOCK_METHOD3(getSockName, void(int sockfd, struct sockaddr *addr, socklen_t *addrlen));
            MOCK_METHOD3(getPeerName, void(int sockfd, struct sockaddr *addr, socklen_t *addrlen));
            MOCK_METHOD3(connect, void(int sockfd, const struct sockaddr *addr, socklen_t addrlen));
            MOCK_METHOD2(listen, void(int sockfd, int backlog));
            MOCK_METHOD3(accept, int(int sockfd, struct sockaddr *addr, socklen_t *addrlen));
            MOCK_METHOD3(write, ssize_t(int fd, const void *buf, size_t count));
            MOCK_METHOD3(read, ssize_t(int fd, void *buf, size_t count));
            MOCK_METHOD3(sendmsg, ssize_t(int sockfd, const struct msghdr *msg, int flags));
            MOCK_METHOD3(recvmsg, ssize_t(int sockfd, struct msghdr *msg, int flags));
            MOCK_METHOD1(getSocketError, int(int fd));
            #define MOCK_NOEXCEPT_METHOD1(m, ...) GMOCK_METHOD1_(, noexcept, , m, __VA_ARGS__)
            MOCK_NOEXCEPT_METHOD1(close, void(int));
            #undef MOCK_NOEXCEPT_METHOD1
            MOCK_METHOD3(sigMask, void(int how, const sigset_t *set, sigset_t *oldset));
            MOCK_METHOD1(sigPending, void(sigset_t *set));
            MOCK_METHOD3(sigTimedWait, int(const sigset_t *set, siginfo_t *info, const struct timespec *timeout));
            MOCK_METHOD3(signalFD, int(int fd, const sigset_t *mask, int flags));
            MOCK_METHOD0(fork, pid_t());
            MOCK_METHOD2(kill, bool(pid_t pid, int sig));
            MOCK_METHOD3(waitpid, pid_t(pid_t pid, int *status, int options));
        };
    }
}

#endif /* !EVENTLOOP_SYSTEMMOCK_HEADER */
