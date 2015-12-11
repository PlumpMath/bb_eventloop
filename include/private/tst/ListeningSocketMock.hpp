#ifndef EVENTLOOP_LISTENINGSOCKETMOCK_HEADER
#define EVENTLOOP_LISTENINGSOCKETMOCK_HEADER

#include <EventLoop/ListeningSocket.hpp>

namespace EventLoop
{
    namespace Test
    {
        class ListeningSocketMock: public ListeningSocket
        {
        public:
            ListeningSocketMock(System                      &system,
                                Runner                      &runner,
                                int                         type,
                                int                         protocol,
                                const SocketAddress         &sa,
                                const NewAcceptedSocketCb   &newAcceptedSocketCb):
                ListeningSocket(system, runner, type, protocol, sa, newAcceptedSocketCb, HandleExceptionCb()) { }

            ListeningSocketMock(System                      &system,
                                Runner                      &runner,
                                int                         type,
                                int                         protocol,
                                const SocketAddress         &sa,
                                const NewAcceptedSocketCb   &newAcceptedSocketCb,
                                const HandleExceptionCb     &handleExceptionCb):
                ListeningSocket(system, runner, type, protocol, sa, newAcceptedSocketCb, handleExceptionCb) { }

            ListeningSocketMock(System                      &system,
                                Runner                      &runner,
                                FileDescriptor              &fd,
                                const NewAcceptedSocketCb   &newAcceptedSocketCb):
                ListeningSocket(system, runner, fd, newAcceptedSocketCb, HandleExceptionCb()) { }
        };
    }
}

#endif /* !EVENTLOOP_LISTENINGSOCKETMOCK_HEADER */
