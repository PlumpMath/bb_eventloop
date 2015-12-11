#ifndef EVENTLOOP_SEQPACKETSOCKET_HEADER
#define EVENTLOOP_SEQPACKETSOCKET_HEADER

#include <functional>
#include <memory>
#include <vector>
#include <list>

#include <EventLoop/FDListener.hpp>
#include <EventLoop/FileDescriptor.hpp>
#include <EventLoop/ControlMessage.hpp>

namespace EventLoop
{
    class System;
    class Runner;
    class SocketAddress;

    /**
     * sequenced, reliable, two-way connection-based data transmission socket
     * for datagrams of fixed maximum length. Takes care of optionally creating
     * socket, setting usual settings, optionally calling bind(), optionally
     * calling connect() and sendmsg() and recvmsg() when needed.
     */
    class SeqPacketSocket: public FDListener
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
         * Callback for informing about received data.
         */
        typedef std::function<void(const void *data, size_t size, const ControlMessage &cm)> DataReceivedCb;

        /**
         * Callback for informing that all data in output buffer has been sent.
         */
        typedef std::function<void()> AllDataSentCb;

        SeqPacketSocket() = delete;

        /**
         * Bind into the given local address and connect to the given remote
         * address.
         *
         * @param maxSize   Maximum length for datagram.
         */
        SeqPacketSocket(Runner                          &runner,
                        int                             protocol,
                        size_t                          maxSize,
                        const SocketAddress             &local,
                        const SocketAddress             &remote,
                        const ConnectionEstablishedCb   &connectionEstablishedCb,
                        const ConnectionClosedCb        &connectionClosedCb,
                        const DataReceivedCb            &dataReceivedCb,
                        const AllDataSentCb             &allDataSentCb);

        /**
         * Connect to the given remote address.
         *
         * @param maxSize   Maximum length for datagram.
         */
        SeqPacketSocket(Runner                          &runner,
                        int                             protocol,
                        size_t                          maxSize,
                        const SocketAddress             &remote,
                        const ConnectionEstablishedCb   &connectionEstablishedCb,
                        const ConnectionClosedCb        &connectionClosedCb,
                        const DataReceivedCb            &dataReceivedCb,
                        const AllDataSentCb             &allDataSentCb);

        /**
         * Take the already connected socket file descriptor. This is usable for
         * example with fds returned by accept().
         *
         * @param maxSize   Maximum length for datagram.
         */
        SeqPacketSocket(Runner                      &runner,
                        FileDescriptor              &fd,
                        size_t                      maxSize,
                        const ConnectionClosedCb    &connectionClosedCb,
                        const DataReceivedCb        &dataReceivedCb,
                        const AllDataSentCb         &allDataSentCb);

        virtual ~SeqPacketSocket();

        int getFD() const { return fd; }

        /**
         * Get socket name (i.e. local address).
         */
        SocketAddress getName() const;

        /**
         * Get peer name (i.e. remote address).
         */
        SocketAddress getPeer() const;

        void send(const void                                    *data,
                  size_t                                        size,
                  const std::shared_ptr<const ControlMessage>   &cm);

        void send(const void    *data,
                  size_t        size);

        virtual void dump(std::ostream &os) const;

    protected:
        SeqPacketSocket(System                          &system,
                        Runner                          &runner,
                        int                             protocol,
                        size_t                          maxSize,
                        const SocketAddress             &local,
                        const SocketAddress             &remote,
                        const ConnectionEstablishedCb   &connectionEstablishedCb,
                        const ConnectionClosedCb        &connectionClosedCb,
                        const DataReceivedCb            &dataReceivedCb,
                        const AllDataSentCb             &allDataSentCb);

        SeqPacketSocket(System                          &system,
                        Runner                          &runner,
                        int                             protocol,
                        size_t                          maxSize,
                        const SocketAddress             &remote,
                        const ConnectionEstablishedCb   &connectionEstablishedCb,
                        const ConnectionClosedCb        &connectionClosedCb,
                        const DataReceivedCb            &dataReceivedCb,
                        const AllDataSentCb             &allDataSentCb);

        SeqPacketSocket(System                      &system,
                        Runner                      &runner,
                        FileDescriptor              &fd,
                        size_t                      maxSize,
                        const ConnectionClosedCb    &connectionClosedCb,
                        const DataReceivedCb        &dataReceivedCb,
                        const AllDataSentCb         &allDataSentCb);

    private:
        struct Data: public Dumpable
        {
            std::shared_ptr<std::vector<char>> buffer;
            std::shared_ptr<const ControlMessage> cm;

            Data(const void                                     *data,
                 size_t                                         size,
                 const std::shared_ptr<const ControlMessage>    &cm);

            void dump(std::ostream &os) const;
        };

        typedef std::list<Data> OutputQueue;

        void handleEventsCb(int             fd,
                            unsigned int    events);

        void checkConnectionEstablished(unsigned int events);

        void checkConnectionClosed(unsigned int events);

        void checkIncomingData(unsigned int events);

        void checkOutgoingData(unsigned int events);

        void allocateReceiveBuffers();

        void receive();

        void sendSlow();

        bool sendData(const Data &data);

        bool trySendFast(const void             *data,
                         size_t                 size,
                         const ControlMessage   &cm);

        bool sendFast(const void            *data,
                      size_t                size,
                      const ControlMessage  &cm);

        void queue(const void                                   *data,
                   size_t                                       size,
                   const std::shared_ptr<const ControlMessage>  &cm);

        void close(int error);

        System &system;
        Runner &runner;
        FileDescriptor fd;
        const size_t maxSize;
        bool connecting;
        bool sending;
        std::vector<char> buffer;
        ControlMessage cm;
        OutputQueue outputQueue;
        ConnectionEstablishedCb connectionEstablishedCb;
        ConnectionClosedCb connectionClosedCb;
        DataReceivedCb dataReceivedCb;
        AllDataSentCb allDataSentCb;
    };
}

#endif /* !EVENTLOOP_SEQPACKETSOCKET_HEADER */
