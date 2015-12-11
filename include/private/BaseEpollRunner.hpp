#ifndef EVENTLOOP_BASEEPOLLRUNNER_HEADER
#define EVENTLOOP_BASEEPOLLRUNNER_HEADER

#include <EventLoop/Dumpable.hpp>

#include "TimerQueue.hpp"
#include "CallbackQueue.hpp"
#include "FDListenerMap.hpp"
#include "Epoll.hpp"

namespace EventLoop
{
    class System;
    class Epoll;

    /**
     * Basic implementation for epoll based runners.
     */
    class BaseEpollRunner: public Dumpable
    {
    public:
        BaseEpollRunner() = delete;

        explicit BaseEpollRunner(System &system);

        virtual ~BaseEpollRunner();

        BaseEpollRunner(const BaseEpollRunner &) = delete;

        BaseEpollRunner &operator = (const BaseEpollRunner &) = delete;

        virtual void dump(std::ostream &os) const;

        CallbackQueue cbq;
        TimerQueue tq;
        FDListenerMap fdListenerMap;
        Epoll epoll;

        void executeFirstTimer();

    private:
        void addFDListener(int fd, unsigned int events);

        void modifyFDListener(int fd, unsigned int events);

        void delFDListener(int fd);
    };
}

#endif /* !EVENTLOOP_BASEEPOLLRUNNER_HEADER */
