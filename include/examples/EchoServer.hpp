#ifndef ECHOSERVER_HEADER
#define ECHOSERVER_HEADER

#include <boost/ptr_container/ptr_list.hpp>

#include <EventLoop/Runner.hpp>
#include <EventLoop/Timer.hpp>
#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/ListeningSocket.hpp>
#include <EventLoop/StreamSocket.hpp>

namespace EchoServer
{
    class ListeningSocket;
    class AcceptedSocket;

    typedef boost::ptr_list<ListeningSocket> ListeningSockets;
    typedef boost::ptr_list<AcceptedSocket> AcceptedSockets;

    /**
     * Server holds references to EventLoop::Runner object, but doesn't own it.
     * Server owns all instances of ListeningSocket and AcceptedSocket.
     *
     * @note Server knows only the abstract EventLoop::Runner, but not its
     * implementation. This way the actual 'runner' can be replaced without
     * touching EchoServer at all. Or EchoServer could easily be integrated
     * with some other application using EventLoop.
     */
    class Server
    {
    public:
        EventLoop::Runner   &runner;
        ListeningSockets    lss;
        AcceptedSockets     ass;

        Server() = delete;

        explicit Server(EventLoop::Runner &runner);

        Server(const Server &) = delete;

        Server &operator = (const Server &) = delete;

        void newListeningSocket(const EventLoop::SocketAddress &sa);

        void newAcceptedSocket(EventLoop::FileDescriptor &fd);

        void removeAcceptedSocket(AcceptedSocket &as);
    };

    /**
     * Thin wrapper on top of EventLoop::ListeningSocket. Mainly for creating
     * instances of AcceptedSocket when new connections are accepted.
     */
    class ListeningSocket: public EventLoop::Dumpable
    {
    public:
        ListeningSocket() = delete;

        ListeningSocket(Server                          &server,
                        const EventLoop::SocketAddress  &sa);

        ~ListeningSocket();

        void dump(std::ostream &os) const;

    private:
        void newAcceptedSocketCb(EventLoop::FileDescriptor      &fd,
                                 const EventLoop::SocketAddress &sa);

        void handleExceptionCb(const std::exception &e);

        Server &server;
        EventLoop::ListeningSocket ls;
    };

    /**
     * Accepted socket created by ListeningSocket. All data received is echoed
     * back to peer. If no data is received during timeout, connection is
     * closed. Timeout logic is done with EventLoop::Timer.
     */
    class AcceptedSocket: public EventLoop::Dumpable
    {
    public:
        AcceptedSocket() = delete;

        AcceptedSocket(Server                       &server,
                       EventLoop::FileDescriptor    &fd);

        ~AcceptedSocket();

        void dump(std::ostream &os) const;

    private:
        void connectionClosedCb(int error);

        void dataReceivedCb();

        void resetTimeout();

        void remove();

        static const boost::posix_time::time_duration TIMEOUT;

        Server &server;
        EventLoop::StreamSocket ss;
        EventLoop::Timer timeout;
    };
}

#endif /* !ECHOSERVER_HEADER */
