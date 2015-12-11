#include <iostream>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <sys/uio.h>
#include <boost/lexical_cast.hpp>

#include "EventLoop/EpollRunner.hpp"
#include <EventLoop/FDListener.hpp>
#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/StreamSocket.hpp>
#include <EventLoop/StreamBuffer.hpp>
#include <EventLoop/UncaughtExceptionHandler.hpp>

namespace
{
    class Input: public EventLoop::FDListener
    {
    public:
        Input() = delete;

        Input(EventLoop::Runner         &runner,
              EventLoop::StreamSocket   &ss,
              bool                      &doContinue);

        Input(const Input &) = delete;

        Input &operator = (const Input &) = delete;

        ~Input();

    private:
        void handleEventsCb(int, unsigned int events);

        EventLoop::Runner &runner;
        EventLoop::StreamSocket &ss;
        bool &doContinue;
    };
}

Input::Input(EventLoop::Runner          &runner,
             EventLoop::StreamSocket    &ss,
             bool                       &doContinue):
    runner(runner),
    ss(ss),
    doContinue(doContinue)
{
    runner.addFDListener(*this, STDIN_FILENO, EventLoop::FDListener::EVENT_IN);
}

Input::~Input()
{
    runner.delFDListener(STDIN_FILENO);
}

void Input::handleEventsCb(int          fd,
                           unsigned int events)
{
    if (events & EventLoop::FDListener::EVENT_IN)
    {
        ss.getOutput().allocate(1024);
        EventLoop::StreamBuffer::IOVecArray iov(ss.getOutput().getFreeIOVec());
        const ssize_t ssize(readv(fd, iov.first, iov.second));
        if (ssize > 0)
        {
            ss.getOutput().increaseUsed(ssize);
            ss.startSending();
        }
        else
        {
            doContinue = false;
        }
    }
    else if (events & EventLoop::FDListener::EVENT_HUP)
    {
        doContinue = false;
    }
}

int main(int argc, char **argv)
{
    EventLoop::UncaughtExceptionHandler::Guard uehg;

    if (argc != 2)
    {
        std::cerr << "Usage: EchoClient ADDRESS" << std::endl;
        return EXIT_FAILURE;
    }

    bool doContinue(true);
    int ret(EXIT_SUCCESS);
    EventLoop::EpollRunner runner;
    std::unique_ptr<EventLoop::StreamSocket> ss;
    ss.reset(new EventLoop::StreamSocket(runner,
                                         0,
                                         boost::lexical_cast<EventLoop::SocketAddress>(argv[1]),
                                         [] ()
                                         {
                                             std::cout << "connection established" << std::endl;
                                         },
                                         [&doContinue, &ret] (int error)
                                         {
                                             if (error)
                                             {
                                                 std::cerr << strerror(error) << std::endl;
                                                 ret = EXIT_FAILURE;
                                             }
                                             doContinue = false;
                                         },
                                         [&ss] ()
                                         {
                                             const EventLoop::StreamBuffer::IOVecArray iov(ss->getInput().getUsedIOVec());
                                             const ssize_t ssize(writev(STDOUT_FILENO, iov.first, iov.second));
                                             ss->getInput().cutHead(ssize);
                                         },
                                         EventLoop::StreamSocket::AllDataSentCb()));
    Input input(runner, *ss, doContinue);

    runner.run([&doContinue] () { return doContinue; });

    return ret;
}
