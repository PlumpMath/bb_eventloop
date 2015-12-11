#include <iostream>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <sys/uio.h>
#include <boost/lexical_cast.hpp>

#include <EventLoop/EpollRunner.hpp>
#include <EventLoop/FDListener.hpp>
#include <EventLoop/SocketAddress.hpp>
#include <EventLoop/SeqPacketSocket.hpp>
#include <EventLoop/UncaughtExceptionHandler.hpp>

namespace
{
    class Input: public EventLoop::FDListener
    {
    public:
        Input() = delete;

        Input(EventLoop::Runner             &runner,
              EventLoop::SeqPacketSocket    &ss,
              bool                          &doContinue);

        Input(const Input &) = delete;

        Input &operator = (const Input &) = delete;

        ~Input();

    private:
        void handleEventsCb(int, unsigned int events);

        EventLoop::Runner &runner;
        EventLoop::SeqPacketSocket &ss;
        bool &doContinue;
    };
}

Input::Input(EventLoop::Runner          &runner,
             EventLoop::SeqPacketSocket &ss,
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
        char buffer[1024];
        const ssize_t ssize(read(fd, buffer, sizeof(buffer)));
        if (ssize > 0)
        {
            ss.send(buffer, ssize);
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
    std::unique_ptr<EventLoop::SeqPacketSocket> ss;
    ss.reset(new EventLoop::SeqPacketSocket(runner,
                                            0,
                                            1024U,
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
                                            [&ss] (const void *data, size_t size, const EventLoop::ControlMessage &)
                                            {
                                                ssize_t tmp = write(STDOUT_FILENO, data, size);
                                                static_cast<void>(tmp);
                                            },
                                            EventLoop::SeqPacketSocket::AllDataSentCb()));
    Input input(runner, *ss, doContinue);

    runner.run([&doContinue] () { return doContinue; });

    return ret;
}
