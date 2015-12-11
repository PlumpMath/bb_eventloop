#ifndef EVENTLOOP_LISTENINGSOCKET_HEADER
#define EVENTLOOP_LISTENINGSOCKET_HEADER

#include <functional>
#include <exception>

#include <EventLoop/FDListener.hpp>
#include <EventLoop/FileDescriptor.hpp>

namespace EventLoop
{
    class System;
    class Runner;
    class SocketAddress;

    /**
     * Listening socket. Takes care of creating socket, setting usual settings,
     * calling bind(), listen() and accept().
     */
    class ListeningSocket: public FDListener
    {
    public:
        /**
         * Callback function for new accepted connections.
         *
         * @param fd    File descriptor holding the newly accepted connection.
         * @param sa    Socket address of the newly accepted connection.
         */
        typedef std::function<void(FileDescriptor &fd, const SocketAddress &sa)> NewAcceptedSocketCb;

        /**
         * Callback function for handling exceptions while accepting new
         * connections.
         *
         * @param e Exception thrown while accepting new connections.
         */
        typedef std::function<void(const std::exception &e)> HandleExceptionCb;

        ListeningSocket() = delete;

        ListeningSocket(Runner                      &runner,
                        int                         type,
                        int                         protocol,
                        const SocketAddress         &sa,
                        const NewAcceptedSocketCb   &newAcceptedSocketCb);

        ListeningSocket(Runner                      &runner,
                        int                         type,
                        int                         protocol,
                        const SocketAddress         &sa,
                        const NewAcceptedSocketCb   &newAcceptedSocketCb,
                        const HandleExceptionCb     &handleExceptionCb);

        ListeningSocket(Runner                      &runner,
                        FileDescriptor              &fd,
                        const NewAcceptedSocketCb   &newAcceptedSocketCb);

        ListeningSocket(Runner                      &runner,
                        FileDescriptor              &fd,
                        const NewAcceptedSocketCb   &newAcceptedSocketCb,
                        const HandleExceptionCb     &handleExceptionCb);

        virtual ~ListeningSocket();

        int getFD() const { return fd; }

        /**
         * Get socket name (i.e. listening address).
         */
        SocketAddress getName() const;

        virtual void dump(std::ostream &os) const;

    protected:
        ListeningSocket(System                      &system,
                        Runner                      &runner,
                        int                         type,
                        int                         protocol,
                        const SocketAddress         &sa,
                        const NewAcceptedSocketCb   &newAcceptedSocketCb,
                        const HandleExceptionCb     &handleExceptionCb);

        ListeningSocket(System                      &system,
                        Runner                      &runner,
                        FileDescriptor              &fd,
                        const NewAcceptedSocketCb   &newAcceptedSocketCb,
                        const HandleExceptionCb     &handleExceptionCb);

    private:
        void handleEventsCb(int             fd,
                            unsigned int    events);

        bool handleEventsOrException();

        bool accept();

        System &system;
        Runner &runner;
        FileDescriptor fd;
        NewAcceptedSocketCb newAcceptedSocketCb;
        HandleExceptionCb handleExceptionCb;
    };
}

#endif /* !EVENTLOOP_LISTENINGSOCKET_HEADER */
