#ifndef EVENTLOOP_EPOLLRUNNERMOCK_HEADER
#define EVENTLOOP_EPOLLRUNNERMOCK_HEADER

#include <gmock/gmock.h>

#include <EventLoop/EpollRunner.hpp>

namespace EventLoop
{
    namespace Test
    {
        class EpollRunnerMock: public EpollRunner
        {
        public:
            explicit EpollRunnerMock(System &system): EpollRunner(system) { }
            EpollRunnerMock(System &system, const SignalSet &signalSet, SignalHandler signalHandler): EpollRunner(system, signalSet, signalHandler) { }
        };
    }
}

#endif /* !EVENTLOOP_EPOLLRUNNERMOCK_HEADER */
