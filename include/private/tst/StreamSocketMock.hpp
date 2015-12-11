#ifndef EVENTLOOP_STREAMSOCKETMOCK_HEADER
#define EVENTLOOP_STREAMSOCKETMOCK_HEADER

#include <gmock/gmock.h>

#include <EventLoop/StreamSocket.hpp>

namespace EventLoop
{
    namespace Test
    {
        class StreamSocketMock: public StreamSocket
        {
        public:
            StreamSocketMock(System                 &system,
                             Runner                 &runner,
                             int                    protocol,
                             const SocketAddress    &local,
                             const SocketAddress    &remote):
                StreamSocket(system,
                             runner,
                             protocol,
                             local,
                             remote,
                             std::bind(&StreamSocketMock::connectionEstablishedCb, this),
                             std::bind(&StreamSocketMock::connectionClosedCb, this, std::placeholders::_1),
                             std::bind(&StreamSocketMock::dataReceivedCb, this),
                             std::bind(&StreamSocketMock::allDataSentCb, this)) { }

            StreamSocketMock(System                 &system,
                             Runner                 &runner,
                             int                    protocol,
                             const SocketAddress    &remote):
                StreamSocket(system,
                             runner,
                             protocol,
                             remote,
                             std::bind(&StreamSocketMock::connectionEstablishedCb, this),
                             std::bind(&StreamSocketMock::connectionClosedCb, this, std::placeholders::_1),
                             std::bind(&StreamSocketMock::dataReceivedCb, this),
                             std::bind(&StreamSocketMock::allDataSentCb, this)) { }

            StreamSocketMock(System         &system,
                             Runner         &runner,
                             FileDescriptor &fd):
                StreamSocket(system,
                             runner,
                             fd,
                             std::bind(&StreamSocketMock::connectionClosedCb, this, std::placeholders::_1),
                             std::bind(&StreamSocketMock::dataReceivedCb, this),
                             std::bind(&StreamSocketMock::allDataSentCb, this)) { }

            MOCK_METHOD0(connectionEstablishedCb, void());

            MOCK_METHOD1(connectionClosedCb, void(int error));

            MOCK_METHOD0(dataReceivedCb, void());

            MOCK_METHOD0(allDataSentCb, void());
        };
    }
}

#endif /* !EVENTLOOP_STREAMSOCKETMOCK_HEADER */
