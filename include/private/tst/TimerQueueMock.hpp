#ifndef EVENTLOOP_TIMERQUEUEMOCK_HEADER
#define EVENTLOOP_TIMERQUEUEMOCK_HEADER

#include <EventLoop/tst/MockableRunner.hpp>

#include "private/TimerQueue.hpp"

namespace EventLoop
{
    namespace Test
    {
        class TimerQueueMock: public MockableRunner
        {
        public:
            TimerQueueMock() = default;

            ~TimerQueueMock() = default;

            explicit TimerQueueMock(System &system): tq(system) { }

            TimerQueue tq;

            void addFDListener(FDListener &, int, unsigned int) { }

            void modifyFDListener(int, unsigned int) { }

            void delFDListener(int) { }

            void addCallback(const std::function<void()> &) { }

        protected:
            TimerQueue &getTimerQueue() { return tq; }
        };
    }
}

#endif /* !EVENTLOOP_TIMERQUEUEMOCK_HEADER */
