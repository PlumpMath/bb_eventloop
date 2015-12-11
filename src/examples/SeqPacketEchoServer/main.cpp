#include <iostream>
#include <string>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/lexical_cast.hpp>

#include <EventLoop/EpollRunner.hpp>
#include <EventLoop/Timer.hpp>
#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/ListeningSocket.hpp>
#include <EventLoop/SeqPacketSocket.hpp>
#include <EventLoop/DumpUtils.hpp>
#include <EventLoop/SignalSet.hpp>
#include <EventLoop/UncaughtExceptionHandler.hpp>

namespace SeqPacketEchoServer
{
    class ListeningSocket;
    class AcceptedSocket;

    typedef boost::ptr_list<ListeningSocket> ListeningSockets;
    typedef boost::ptr_list<AcceptedSocket> AcceptedSockets;

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

        void dataReceivedCb(const void  *data,
                            size_t      size);

        void resetTimeout();

        void remove();

        static const boost::posix_time::time_duration TIMEOUT;

        Server &server;
        EventLoop::SeqPacketSocket ss;
        EventLoop::Timer timeout;
    };

    Server::Server(EventLoop::Runner &runner):
        runner(runner)
    {
    }

    void Server::newListeningSocket(const EventLoop::SocketAddress &sa)
    {
        lss.push_back(new ListeningSocket(*this, sa));
    }

    void Server::newAcceptedSocket(EventLoop::FileDescriptor &fd)
    {
        ass.push_back(new AcceptedSocket(*this, fd));
    }

    void Server::removeAcceptedSocket(AcceptedSocket &as)
    {
        for (AcceptedSockets::iterator i = ass.begin();
             i != ass.end();
             ++i)
        {
            if (&(*i) == &as)
            {
                ass.release(i);
                break;
            }
        }
    }

    ListeningSocket::ListeningSocket(Server                         &server,
                                     const EventLoop::SocketAddress &sa):
        server(server),
        ls(server.runner,
           SOCK_SEQPACKET,
           0,
           sa,
           std::bind(&ListeningSocket::newAcceptedSocketCb,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2),
           std::bind(&ListeningSocket::handleExceptionCb,
                     this,
                     std::placeholders::_1))
    {
        std::cout << "Listening " << sa << std::endl;
    }

    ListeningSocket::~ListeningSocket()
    {
    }

    void ListeningSocket::newAcceptedSocketCb(EventLoop::FileDescriptor         &fd,
                                              const EventLoop::SocketAddress    &sa)
    {
        std::cout << "New accepted connection from " << sa << std::endl;
        server.newAcceptedSocket(fd);
    }

    void ListeningSocket::handleExceptionCb(const std::exception &e)
    {
        std::cerr << "Error while accepting new connection: " << e.what() << std::endl;
    }

    void ListeningSocket::dump(std::ostream &os) const
    {
        DUMP_BEGIN(os);
        DUMP_ADDRESS_OF(os, server);
        DUMP_VALUE_OF(os, ls);
        DUMP_END(os);
    }

    const boost::posix_time::time_duration AcceptedSocket::TIMEOUT(boost::posix_time::seconds(10));

    AcceptedSocket::AcceptedSocket(Server                       &server,
                                   EventLoop::FileDescriptor    &fd):
        server(server),
        ss(server.runner,
           fd,
           1024U,
           std::bind(&AcceptedSocket::connectionClosedCb, this, std::placeholders::_1),
           std::bind(&AcceptedSocket::dataReceivedCb, this, std::placeholders::_1, std::placeholders::_2),
           EventLoop::SeqPacketSocket::AllDataSentCb()),
        timeout(server.runner)
    {
        std::cout << "New accepted socket" << std::endl;
        resetTimeout();
    }

    AcceptedSocket::~AcceptedSocket()
    {
        std::cout << "Deleting accepted socket" << std::endl;
    }

    void AcceptedSocket::connectionClosedCb(int error)
    {
        if (error)
        {
            std::cerr << "Error in accepted connection: " << strerror(error) << std::endl;
        }
        else
        {
            std::cout << "Accepted socket closed" << std::endl;
        }
        remove();
    }

    void AcceptedSocket::dataReceivedCb(const void  *data,
                                        size_t      size)
    {
        resetTimeout();
        ss.send(data, size);
    }

    void AcceptedSocket::remove()
    {
        server.runner.addCallback(std::bind(&Server::removeAcceptedSocket, &server, std::ref(*this)));
    }

    void AcceptedSocket::resetTimeout()
    {
        timeout.schedule(TIMEOUT, std::bind(&AcceptedSocket::remove, this));
    }

    void AcceptedSocket::dump(std::ostream &os) const
    {
        DUMP_BEGIN(os);
        DUMP_ADDRESS_OF(os, server);
        DUMP_VALUE_OF(os, ss);
        DUMP_VALUE_OF(os, timeout);
        DUMP_END(os);
    }
}

int main(int argc, char **argv)
{
    EventLoop::UncaughtExceptionHandler::Guard uehg;

    bool doContinue(true);
    EventLoop::EpollRunner runner(EventLoop::SignalSet(SIGINT), [&doContinue] (int sig) { if (sig == SIGINT) doContinue = false; });
    SeqPacketEchoServer::Server server(runner);

    for (int i = 1; i < argc; ++i)
    {
        server.newListeningSocket(boost::lexical_cast<EventLoop::SocketAddress>(argv[i]));
    }

    runner.run([&doContinue] () { return doContinue; });
    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
