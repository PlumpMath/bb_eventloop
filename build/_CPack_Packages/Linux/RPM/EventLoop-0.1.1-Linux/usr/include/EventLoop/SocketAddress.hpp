#ifndef EVENTLOOP_SOCKETADDRESS_HEADER
#define EVENTLOOP_SOCKETADDRESS_HEADER

#include <iosfwd>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    /**
     * Handy class for representing any socket address.
     */
    class SocketAddress
    {
    public:
        class TooLongUnixSocketPath: public Exception
        {
        public:
            TooLongUnixSocketPath(): Exception("too long Unix socket path") { }
        };

        class TranslationError: public Exception
        {
        public:
            TranslationError() = delete;

            TranslationError(const std::string  &address,
                             const std::string  &service,
                             const std::string  &error);

            TranslationError(const std::string  &service,
                             const std::string  &error);
        };

        /** Default constructor. Creates unspecified (AF_UNSPEC) address. */
        SocketAddress();

        SocketAddress(const struct sockaddr *s,
                      socklen_t             size);

        SocketAddress(const struct sockaddr_un  &sun,
                      socklen_t                 size);

        SocketAddress(const struct sockaddr_in &sin);

        SocketAddress(const struct in_addr  &addr,
                      uint16_t              port);

        SocketAddress(const struct sockaddr_in6 &sin6);

        SocketAddress(const struct in6_addr &addr,
                      uint16_t              port);

        SocketAddress(const struct in6_addr &addr,
                      uint32_t              scopeId,
                      uint16_t              port);

        virtual ~SocketAddress() { }

        static SocketAddress createUnix(const std::string &path);

        static SocketAddress createAbstractUnix(const std::string &path);

        static SocketAddress createUnixAny();

        static SocketAddress createIPv4(const std::string   &address,
                                        const std::string   &service);

        static SocketAddress createIPv4(const std::string   &address,
                                        uint16_t            port);

        static SocketAddress createIPv4(const struct in_addr    &addr,
                                        const std::string       &service);

        static SocketAddress createIPv4(const struct in_addr    &addr,
                                        uint16_t                port);

        static SocketAddress createIPv4Any();

        static SocketAddress createIPv6(const std::string   &address,
                                        const std::string   &service);

        static SocketAddress createIPv6(const std::string   &address,
                                        uint16_t            port);

        static SocketAddress createIPv6(const struct in6_addr   &addr,
                                        const std::string       &service);

        static SocketAddress createIPv6(const struct in6_addr   &addr,
                                        uint16_t                port);

        static SocketAddress createIPv6(const struct in6_addr   &addr,
                                        uint32_t                scopeId,
                                        const std::string       &service);

        static SocketAddress createIPv6(const struct in6_addr   &addr,
                                        uint32_t                scopeId,
                                        uint16_t                port);

        static SocketAddress createIPv6Any();

        static SocketAddress createIPv4OrIPv6(const std::string &address,
                                              const std::string &service);

        static SocketAddress createIPv4OrIPv6(const std::string &address,
                                              uint16_t          port);

        int getFamily() const { return sa.s.sa_family; }

        const struct sockaddr *getPointer() const { return &sa.s; }

        struct sockaddr *getPointer() { return &sa.s; }

        const struct sockaddr &getAddress() const { return sa.s; }

        const sockaddr &getAddress() { return sa.s; }

        const struct sockaddr_un &getUnixAddress() const { return sa.sun; }

        struct sockaddr_un &getUnixAddress() { return sa.sun; }

        const struct sockaddr_in &getIPv4Address() const { return sa.sin; }

        struct sockaddr_in &getIPv4Address() { return sa.sin; }

        const struct sockaddr_in6 &getIPv6Address() const { return sa.sin6; }

        struct sockaddr_in6 &getIPv6Address() { return sa.sin6; }

        uint16_t getIPPort() const
        {
            /*
             * sockaddr_in and sockaddr_in6 are defined so that sin_port
             * and sin6_port have the same offset.
             */
            return sa.sin.sin_port;
        }

        socklen_t getUsedSize() const { return usedSize; }

        void setUsedSize(socklen_t size) { usedSize = size; }

        socklen_t getAvailableSize() const;

        /**
         * Get socklen_t pointer to be used with accept(2), getsockname(2),
         * getpeername(2) and other similar functions. The pointer is first
         * filled with available size and then expected to be filled with
         * used size.
         *
         * \code
         * SocketAddress sa;
         * int ret(getsockname(fd, sa.getPointer(), sa.getSizePointer()));
         * if (ret == 0)
         * {
         *     do something with sa
         * }
         * else
         * {
         *     sa is now unsable
         * }
         * \endcode
         */
        socklen_t *getSizePointer();

        /**
         * Is AF_UNSPEC, "unix:", "ipv4:0.0.0.0:0" or "ipv6:[::]:0"?
         */
        bool isAny() const;

        bool operator == (const SocketAddress &other) const;

        bool operator < (const SocketAddress &other) const;

    private:
        struct UnixWithPad { struct sockaddr_un sun; char pad; };
        union SAUnion
        {
            struct sockaddr     s;
            struct sockaddr_un  sun;
            struct sockaddr_in  sin;
            struct sockaddr_in6 sin6;
            UnixWithPad         pad;
        };
        SAUnion sa;
        socklen_t usedSize;
    };

    std::ostream & operator << (std::ostream        &out,
                                const SocketAddress &sa);

    std::istream & operator >> (std::istream    &in,
                                SocketAddress   &sa);
}

#endif /* !EVENTLOOP_SOCKETADDRESS_HEADER */
