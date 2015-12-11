#ifndef EVENTLOOP_DUMMYFDLISTENERMAP_HEADER
#define EVENTLOOP_DUMMYFDLISTENERMAP_HEADER

#include <EventLoop/tst/MockableRunner.hpp>

#include "private/FDListenerMap.hpp"

namespace EventLoop
{
    namespace Test
    {
        class DummyFDListenerMap: public MockableRunner
        {
        public:
            DummyFDListenerMap(): flm(doNothing, doNothing, doNothing) { }

            ~DummyFDListenerMap() = default;

            void addFDListener(FDListener   &fl,
                               int          fd,
                               unsigned int events)
            {
                flm.add(fl, fd, events);
            }

            void modifyFDListener(int           fd,
                                  unsigned int  events)
            {
                flm.modify(fd, events);
            }

            void delFDListener(int fd)
            {
                flm.del(fd);
            }

            FDListener &get(int fd) const
            {
                return flm.get(fd);
            }

            unsigned int getEvents(int fd) const
            {
                return flm.getEvents(fd);
            }

            void addCallback(const std::function<void()> &) { }

        private:
            static void doNothing(FDListener &, int, unsigned int) { }

            FDListenerMap flm;
        };
    }
}

#endif /* !EVENTLOOP_DUMMYFDLISTENERMAP_HEADER */
