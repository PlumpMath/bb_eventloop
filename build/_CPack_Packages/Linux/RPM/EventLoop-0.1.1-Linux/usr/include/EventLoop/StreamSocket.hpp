#ifndef EVENTLOOP_STREAMSOCKET_HEADER
#define EVENTLOOP_STREAMSOCKET_HEADER

#include <functional>

#include <EventLoop/FDListener.hpp>
#include <EventLoop/StreamBuffer.hpp>
#include <EventLoop/FileDescriptor.hpp>

namespace EventLoop
{
    class System;
    class Runner;
    class SocketAddress;

    /**
     * Stream socket. Takes care of optionally creating socket, setting usual
     * settings, optionally calling bind(), optionally calling connect() and
     * sendmsg() and recvmsg() when needed.
     */
    class StreamSocket: public FDListener
    {
    public:
        /**
         * Callback for informing when connection has been established.
         */
        typedef std::function<void()> ConnectionEstablishedCb;

        /**
         * Callback for informing when connection has been closed. The given
         * error is zero if connection was normally terminated, otherwise it
         * tells the reason for connection being closed (for example
         * ECONNREFUSED).
         */
        typedef std::function<void(int error)> ConnectionClosedCb;

        /**
         * Callback for informing about received data. The data is available in
         * the input buffer.
         */
        typedef std::function<void()> DataReceivedCb;

        /**
         * Callback for informing that all data in output buffer has been sent.
         */
        typedef std::function<void()> AllDataSentCb;

        StreamSocket() = delete;

        /**
         * Bind into the given local address and connect to the given remote
         * address.
         */
        StreamSocket(Runner                         &runner,
                     int                            protocol,
                     const SocketAddress            &local,
                     const SocketAddress            &remote,
                     const ConnectionEstablishedCb  &connectionEstablishedCb,
                     const ConnectionClosedCb       &connectionClosedCb,
                     const DataReceivedCb           &dataReceivedCb,
                     const AllDataSentCb            &allDataSentCb);

        /**
         * Connect to the given remote address.
         */
        StreamSocket(Runner                         &runner,
                     int                            protocol,
                     const SocketAddress            &remote,
                     const ConnectionEstablishedCb  &connectionEstablishedCb,
                     const ConnectionClosedCb       &connectionClosedCb,
                     const DataReceivedCb           &dataReceivedCb,
                     const AllDataSentCb            &allDataSentCb);

        /**
         * Take the already connected socket file descriptor. This is usable for
         * example with fds returned by accept().
         */
        StreamSocket(Runner                         &runner,
                     FileDescriptor                 &fd,
                     const ConnectionClosedCb       &connectionClosedCb,
                     const DataReceivedCb           &dataReceivedCb,
                     const AllDataSentCb            &allDataSentCb);

        virtual ~StreamSocket();

        int getFD() const { return fd; }

        /**
         * Get socket name (i.e. local address).
         */
        SocketAddress getName() const;

        /**
         * Get peer name (i.e. remote address).
         */
        SocketAddress getPeer() const;

        /**
         * Start sending output buffer.
         */
        void startSending();

        /**
         * Get input buffer.
         */
        StreamBuffer &getInput() { return input; }

        /**
         * Get output buffer.
         */
        StreamBuffer &getOutput() { return output; }

        virtual void dump(std::ostream &os) const;

    protected:
        StreamSocket(System                         &system,
                     Runner                         &runner,
                     int                            protocol,
                     const SocketAddress            &local,
                     const SocketAddress            &remote,
                     const ConnectionEstablishedCb  &connectionEstablishedCb,
                     const ConnectionClosedCb       &connectionClosedCb,
                     const DataReceivedCb           &dataReceivedCb,
                     const AllDataSentCb            &allDataSentCb);

        StreamSocket(System                         &system,
                     Runner                         &runner,
                     int                            protocol,
                     const SocketAddress            &remote,
                     const ConnectionEstablishedCb  &connectionEstablishedCb,
                     const ConnectionClosedCb       &connectionClosedCb,
                     const DataReceivedCb           &dataReceivedCb,
                     const AllDataSentCb            &allDataSentCb);

        StreamSocket(System                         &system,
                     Runner                         &runner,
                     FileDescriptor                 &fd,
                     const ConnectionClosedCb       &connectionClosedCb,
                     const DataReceivedCb           &dataReceivedCb,
                     const AllDataSentCb            &allDataSentCb);

    private:
        void handleEventsCb(int             fd,
                            unsigned int    events);

        void checkConnectionEstablished(unsigned int events);

        void checkIncomingData(unsigned int events);

        void checkOutgoingData(unsigned int events);

        void checkConnectionClosed(unsigned int events);

        void allocateReceiveBuffer();

        void receive();

        void trySend();

        void send();

        void close(int error);

        System &system;
        Runner &runner;
        FileDescriptor fd;
        bool connecting;
        bool sending;
        StreamBuffer input;
        StreamBuffer output;
        ConnectionEstablishedCb connectionEstablishedCb;
        ConnectionClosedCb connectionClosedCb;
        DataReceivedCb dataReceivedCb;
        AllDataSentCb allDataSentCb;
    };
}

#endif /* !EVENTLOOP_STREAMSOCKET_HEADER */
