#ifndef EVENTLOOP_FDLISTENERMOCK_HEADER
#define EVENTLOOP_FDLISTENERMOCK_HEADER

#include <gmock/gmock.h>

#include <EventLoop/FDListener.hpp>

namespace EventLoop
{
    namespace Test
    {
        class FDListenerMock: public FDListener
        {
        public:
            MOCK_METHOD2(handleEventsCb, void(int fd, unsigned int events));
        };
    }
}

#endif /* !EVENTLOOP_FDLISTENERMOCK_HEADER */
