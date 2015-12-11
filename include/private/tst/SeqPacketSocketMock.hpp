#ifndef EVENTLOOP_SEQPACKETSOCKETMOCK_HEADER
#define EVENTLOOP_SEQPACKETSOCKETMOCK_HEADER

#include <gmock/gmock.h>

#include <EventLoop/SeqPacketSocket.hpp>

namespace EventLoop
{
    namespace Test
    {
        class SeqPacketSocketMock: public SeqPacketSocket
        {
        public:
            static const size_t MAX_SIZE;

            SeqPacketSocketMock(System              &system,
                                Runner              &runner,
                                int                 protocol,
                                const SocketAddress &local,
                                const SocketAddress &remote):
                SeqPacketSocket(system,
                                runner,
                                protocol,
                                MAX_SIZE,
                                local,
                                remote,
                                std::bind(&SeqPacketSocketMock::connectionEstablishedCb, this),
                                std::bind(&SeqPacketSocketMock::connectionClosedCb, this, std::placeholders::_1),
                                std::bind(&SeqPacketSocketMock::dataReceivedCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                std::bind(&SeqPacketSocketMock::allDataSentCb, this)) { }

            SeqPacketSocketMock(System              &system,
                                Runner              &runner,
                                int                 protocol,
                                const SocketAddress &remote):
                SeqPacketSocket(system,
                                runner,
                                protocol,
                                MAX_SIZE,
                                remote,
                                std::bind(&SeqPacketSocketMock::connectionEstablishedCb, this),
                                std::bind(&SeqPacketSocketMock::connectionClosedCb, this, std::placeholders::_1),
                                std::bind(&SeqPacketSocketMock::dataReceivedCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                std::bind(&SeqPacketSocketMock::allDataSentCb, this)) { }

            SeqPacketSocketMock(System          &system,
                                Runner          &runner,
                                FileDescriptor  &fd):
                SeqPacketSocket(system,
                                runner,
                                fd,
                                MAX_SIZE,
                                std::bind(&SeqPacketSocketMock::connectionClosedCb, this, std::placeholders::_1),
                                std::bind(&SeqPacketSocketMock::dataReceivedCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                std::bind(&SeqPacketSocketMock::allDataSentCb, this)) { }

            MOCK_METHOD0(connectionEstablishedCb, void());

            MOCK_METHOD1(connectionClosedCb, void(int error));

            MOCK_METHOD3(dataReceivedCb, void(const void *data, size_t size, const ControlMessage &cm));

            MOCK_METHOD0(allDataSentCb, void());
        };
    }
}

#endif /* !EVENTLOOP_SEQPACKETSOCKETMOCK_HEADER */
