#ifndef EVENTLOOP_BASEEPOLLRUNNERMOCK_HEADER
#define EVENTLOOP_BASEEPOLLRUNNERMOCK_HEADER

#include <gmock/gmock.h>

#include "private/BaseEpollRunner.hpp"

namespace EventLoop
{
    namespace Test
    {
        class BaseEpollRunnerMock: public BaseEpollRunner
        {
        public:
            explicit BaseEpollRunnerMock(System &system): BaseEpollRunner(system) { }
        };
    }
}

#endif /* !EVENTLOOP_BASEEPOLLRUNNERMOCK_HEADER */
