#include <iostream>
#include <string>
#include <exception>
#include <cstdlib>
#include <poll.h>
#include <boost/lexical_cast.hpp>

#include <EventLoop/AsyncRunner.hpp>
#include <EventLoop/SystemException.hpp>
#include <EventLoop/UncaughtExceptionHandler.hpp>

#include "examples/EchoServer.hpp"

using namespace EchoServer;

int main(int argc, char **argv)
{
    EventLoop::UncaughtExceptionHandler::Guard uehg;

    /* Create runner and server */
    EventLoop::AsyncRunner runner;
    Server server(runner);

    /*
     * Parse arguments. No error checking - this is bad example :-(
     *
     * @note This is the only place where we have any domain or protocol
     * specific handling. The whole EchoServer namespace is completely
     * protocol independent (of course other than assuming that the
     * used protocol is 'stream protocol' - SOCK_STREAM).
     */
    for (int i = 1; i < argc; ++i)
    {
        server.newListeningSocket(boost::lexical_cast<EventLoop::SocketAddress>(argv[i]));
    }

    /* Start looping */
    struct pollfd pollfd[1] = { { runner.getEventFD(), POLLIN, 0 } };
    while (true)
    {
        int status(poll(pollfd, sizeof(pollfd) / sizeof(pollfd[0]), -1));
        if (status < 0)
        {
            throw EventLoop::SystemException("poll");
        }
        else
        {
            runner.step();
        }
    }

    return EXIT_SUCCESS;
}
