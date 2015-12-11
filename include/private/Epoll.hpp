#ifndef EVENTLOOP_EPOLL_HEADER
#define EVENTLOOP_EPOLL_HEADER

#include <vector>
#include <sys/epoll.h>

#include <EventLoop/FileDescriptor.hpp>
#include <EventLoop/Dumpable.hpp>

namespace EventLoop
{
    class System;
    class SignalSet;

    /**
     * Abstraction for epoll logic. Provides easier to use functions for doing
     * epoll_create(), epoll_ctl() and epoll_wait() than simply using those
     * system functions directly and wraps the returned events behind nice
     * iterator interface.
     */
    class Epoll: public Dumpable
    {
    public:
        Epoll() = delete;

        explicit Epoll(System &system);

        ~Epoll();

        Epoll(const Epoll &) = delete;

        Epoll &operator = (const Epoll &) = delete;

        int getFD() const { return epfd; }

        void add(int                fd,
                 struct epoll_event *event);

        void modify(int                 fd,
                    struct epoll_event  *event);

        void del(int fd);

        /**
         * Wait for file descriptor events with the given timeout.
         *
         * @param timeout   Timeout as milliseconds.
         * @return          True, if wait ended in timeout.
         *
         * @note Calling wait invalidates all iterators.
         */
        bool wait(int timeout);

        /** Iterator for iterating trough events got by calling wait(). */
        typedef std::vector<struct epoll_event>::const_iterator event_iterator;

        /**
         * Get iterator to the beginning of events got by calling wait().
         *
         * @note Calling wait invalidates all iterators.
         */
        event_iterator events_begin() const { return eventArray.begin(); }

        /**
         * Get iterator to the end of events got by calling wait().
         *
         * @note Calling wait invalidates all iterators.
         */
        event_iterator events_end() const { return eventArray.end(); }

        void dump(std::ostream &os) const;

    private:
        System &system;
        FileDescriptor epfd;
        size_t addCount;
        std::vector<struct epoll_event> eventArray;
    };
}

#endif /* !EVENTLOOP_EPOLL_HEADER */
