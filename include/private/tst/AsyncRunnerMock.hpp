#ifndef EVENTLOOP_ASYNCRUNNERMOCK_HEADER
#define EVENTLOOP_ASYNCRUNNERMOCK_HEADER

#include <gmock/gmock.h>

#include <EventLoop/AsyncRunner.hpp>

namespace EventLoop
{
    namespace Test
    {
        class AsyncRunnerMock: public AsyncRunner
        {
        public:
            explicit AsyncRunnerMock(System &system): AsyncRunner(system) { }
        };
    }
}

#endif /* !EVENTLOOP_ASYNCRUNNERMOCK_HEADER */
