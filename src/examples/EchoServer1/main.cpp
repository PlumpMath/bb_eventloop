#include <iostream>
#include <string>
#include <exception>
#include <cstdlib>
#include <boost/lexical_cast.hpp>

#include <EventLoop/EpollRunner.hpp>
#include <EventLoop/SignalSet.hpp>
#include <EventLoop/UncaughtExceptionHandler.hpp>

#include "examples/EchoServer.hpp"

using namespace EchoServer;

int main(int argc, char **argv)
{
    EventLoop::UncaughtExceptionHandler::Guard uehg;

    /* Create runner and server */
    bool doContinue(true);
    EventLoop::EpollRunner runner(EventLoop::SignalSet(SIGINT), [&doContinue] (int sig) { if (sig == SIGINT) doContinue = false; });
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

    /* Run until SIGINT (Ctrl+C) or exception */
    runner.run([&doContinue] () { return doContinue; });
    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
