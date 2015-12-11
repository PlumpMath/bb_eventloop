#include "examples/EchoServer.hpp"

#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>

#include <EventLoop/DumpUtils.hpp>

using namespace EchoServer;

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
       SOCK_STREAM,
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
       std::bind(&AcceptedSocket::connectionClosedCb, this, std::placeholders::_1),
       std::bind(&AcceptedSocket::dataReceivedCb, this),
       EventLoop::StreamSocket::AllDataSentCb()),
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

void AcceptedSocket::dataReceivedCb()
{
    resetTimeout();
    ss.getOutput().appendStreamBuffer(EventLoop::StreamBuffer(ss.getInput().spliceHead(ss.getInput().getUsedSize())));
    ss.startSending();
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
