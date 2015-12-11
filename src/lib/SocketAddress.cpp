#include <EventLoop/SocketAddress.hpp>

#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <netdb.h>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

using namespace EventLoop;

namespace
{
    std::string buildError(const std::string &address,
                           const std::string &service,
                           const std::string &error)
    {
        std::ostringstream os;
        os << "getaddrinfo(" << address << ", " << service << "): " << error;
        return os.str();
    }

    std::string buildError(const std::string &service,
                           const std::string &error)
    {
        std::ostringstream os;
        os << "getservbyname(" << service << "): " << error;
        return os.str();
    }
}

SocketAddress::TranslationError::TranslationError(const std::string &address,
                                                  const std::string &service,
                                                  const std::string &error):
    Exception(buildError(address, service, error))
{
}

SocketAddress::TranslationError::TranslationError(const std::string &service,
                                                  const std::string &error):
    Exception(buildError(service, error))
{
}

SocketAddress::SocketAddress():
    usedSize(0)
{
    memset(&sa, 0, sizeof(sa));
}

namespace
{
    size_t memCopy(const void   *src,
                   size_t       srcSize,
                   void         *dst,
                   size_t       dstSize)
    {
        const size_t min(std::min(srcSize, dstSize));
        memcpy(dst, src, min);
        if (dstSize > srcSize)
        {
            memset(static_cast<char *>(dst) + srcSize, 0, dstSize - srcSize);
        }
        return min;
    }

    template <typename Src, typename Dst>
    size_t memCopy(const Src    &src,
                   Dst          &dst)
    {
        return memCopy(&src, sizeof(src), &dst, sizeof(dst));
    }
}

SocketAddress::SocketAddress(const struct sockaddr  *s,
                             socklen_t              size):
    usedSize(static_cast<socklen_t>(memCopy(s, size, &sa, sizeof(sa))))
{
}

SocketAddress::SocketAddress(const struct sockaddr_un   &sun,
                             socklen_t                  size):
    usedSize(static_cast<socklen_t>(memCopy(&sun, size, &sa, sizeof(sa))))
{
}

SocketAddress::SocketAddress(const struct sockaddr_in &sin):
    usedSize(static_cast<socklen_t>(memCopy(sin, sa)))
{
}

SocketAddress::SocketAddress(const struct in_addr   &addr,
                             uint16_t               port):
    usedSize(static_cast<socklen_t>(sizeof(struct sockaddr_in)))
{
    memset(&sa, 0, sizeof(sa));
    sa.sin.sin_family = AF_INET;
    sa.sin.sin_addr.s_addr = addr.s_addr;
    sa.sin.sin_port = port;
}

SocketAddress::SocketAddress(const struct sockaddr_in6 &sin6):
    usedSize(static_cast<socklen_t>(memCopy(sin6, sa)))
{
}

SocketAddress::SocketAddress(const struct in6_addr  &addr,
                             uint16_t               port):
    SocketAddress(addr, 0U, port) { }

SocketAddress::SocketAddress(const struct in6_addr  &addr,
                             uint32_t               scopeId,
                             uint16_t               port):
    usedSize(static_cast<socklen_t>(sizeof(struct sockaddr_in6)))
{
    memset(&sa, 0, sizeof(sa));
    sa.sin6.sin6_family = AF_INET6;
    sa.sin6.sin6_addr.s6_addr32[0] = addr.s6_addr32[0];
    sa.sin6.sin6_addr.s6_addr32[1] = addr.s6_addr32[1];
    sa.sin6.sin6_addr.s6_addr32[2] = addr.s6_addr32[2];
    sa.sin6.sin6_addr.s6_addr32[3] = addr.s6_addr32[3];
    sa.sin6.sin6_port = port;
    sa.sin6.sin6_scope_id = scopeId;
}

SocketAddress SocketAddress::createUnix(const std::string &path)
{
    SocketAddress sa;
    if (path.size() >= sizeof(sa.sa.sun.sun_path))
    {
        throw TooLongUnixSocketPath();
    }
    sa.sa.sun.sun_family = AF_UNIX;
    memcpy(sa.sa.sun.sun_path, path.data(), path.size());
    sa.usedSize = static_cast<socklen_t>(offsetof(struct sockaddr_un, sun_path) + path.size() + 1U);
    return sa;
}

SocketAddress SocketAddress::createUnixAny()
{
    SocketAddress sa;
    sa.sa.sun.sun_family = AF_UNIX;
    sa.usedSize = static_cast<socklen_t>(offsetof(struct sockaddr_un, sun_path));
    return sa;
}

SocketAddress SocketAddress::createAbstractUnix(const std::string &path)
{
    SocketAddress sa;
    if (path.size() > sizeof(sa.sa.sun.sun_path) - 1)
    {
        throw TooLongUnixSocketPath();
    }
    sa.sa.sun.sun_family = AF_UNIX;
    memcpy(sa.sa.sun.sun_path + 1, path.data(), path.size());
    sa.usedSize = static_cast<socklen_t>(sizeof(sa_family_t) + path.size() + 1U);
    return sa;
}

namespace
{
    socklen_t getAddrInfoImpl(int               flags,
                              int               family,
                              const std::string &node,
                              const std::string &service,
                              struct sockaddr   *sa)
    {
        struct addrinfo hints = { flags, family };
        struct addrinfo *res(nullptr);
        int status(getaddrinfo(node.c_str(),
                               service.c_str(),
                               &hints,
                               &res));
        if (status)
        {
            throw SocketAddress::TranslationError(node,
                                                  service,
                                                  gai_strerror(status));
        }
        const socklen_t size(res->ai_addrlen);
        memcpy(sa, res->ai_addr, size);
        freeaddrinfo(res);
        return size;
    }

    socklen_t getAddrInfoWithService(int                family,
                                     const std::string  &node,
                                     const std::string  &service,
                                     struct sockaddr    *sa)
    {
        return getAddrInfoImpl(0, family, node, service, sa);
    }

    socklen_t getAddrInfoWithPort(int               family,
                                  const std::string &node,
                                  uint16_t          port,
                                  struct sockaddr   *sa)
    {
        return getAddrInfoImpl(AI_NUMERICSERV, family, node, boost::lexical_cast<std::string>(ntohs(port)), sa);
    }

    uint16_t getServByName(const std::string &service)
    {
        try
        {
            return htons(boost::lexical_cast<uint16_t>(service));
        }
        catch (const boost::bad_lexical_cast &)
        {
            struct servent result = { };
            char buffer[1024];
            struct servent *ptr(nullptr);
            const int status(getservbyname_r(service.c_str(), nullptr, &result, buffer, sizeof(buffer), &ptr));
            if (status)
            {
                throw SocketAddress::TranslationError(service, strerror(status));
            }
            else if (ptr == nullptr)
            {
                throw SocketAddress::TranslationError(service, strerror(ENOENT));
            }
            return result.s_port;
        }
    }
}

SocketAddress SocketAddress::createIPv4(const std::string   &address,
                                        const std::string   &service)
{
    SocketAddress sa;
    sa.usedSize = getAddrInfoWithService(AF_INET, address, service, &sa.sa.s);
    return sa;
}

SocketAddress SocketAddress::createIPv4(const std::string   &address,
                                        uint16_t            port)
{
    SocketAddress sa;
    sa.usedSize = getAddrInfoWithPort(AF_INET, address, port, &sa.sa.s);
    return sa;
}

SocketAddress SocketAddress::createIPv4(const struct in_addr    &addr,
                                        const std::string       &service)
{
    return SocketAddress(addr, getServByName(service));
}

SocketAddress SocketAddress::createIPv4(const struct in_addr    &addr,
                                        uint16_t                port)
{
    return SocketAddress(addr, port);
}

SocketAddress SocketAddress::createIPv4Any()
{
    static const struct sockaddr_in sin = { AF_INET };
    return SocketAddress(sin);
}

SocketAddress SocketAddress::createIPv6(const std::string   &address,
                                        const std::string   &service)
{
    SocketAddress sa;
    sa.usedSize = getAddrInfoWithService(AF_INET6, address, service, &sa.sa.s);
    return sa;
}

SocketAddress SocketAddress::createIPv6(const std::string   &address,
                                        uint16_t            port)
{
    SocketAddress sa;
    sa.usedSize = getAddrInfoWithPort(AF_INET6, address, port, &sa.sa.s);
    return sa;
}

SocketAddress SocketAddress::createIPv6(const struct in6_addr   &addr,
                                        const std::string       &service)
{
    return createIPv6(addr, 0U, getServByName(service));
}

SocketAddress SocketAddress::createIPv6(const struct in6_addr   &addr,
                                        uint16_t                port)
{
    return createIPv6(addr, 0U, port);
}

SocketAddress SocketAddress::createIPv6(const struct in6_addr   &addr,
                                        uint32_t                scopeId,
                                        const std::string       &service)
{
    return SocketAddress(addr, scopeId, getServByName(service));
}

SocketAddress SocketAddress::createIPv6(const struct in6_addr   &addr,
                                        uint32_t                scopeId,
                                        uint16_t                port)
{
    return SocketAddress(addr, scopeId, port);
}

SocketAddress SocketAddress::createIPv6Any()
{
    static const struct sockaddr_in6 sin6 = { AF_INET6 };
    return SocketAddress(sin6);
}

SocketAddress SocketAddress::createIPv4OrIPv6(const std::string &address,
                                              const std::string &service)
{
    SocketAddress sa;
    sa.usedSize = getAddrInfoWithService(AF_UNSPEC, address, service, &sa.sa.s);
    return sa;
}

SocketAddress SocketAddress::createIPv4OrIPv6(const std::string &address,
                                              uint16_t          port)
{
    SocketAddress sa;
    sa.usedSize = getAddrInfoWithPort(AF_UNSPEC, address, port, &sa.sa.s);
    return sa;
}

socklen_t SocketAddress::getAvailableSize() const
{
    switch (getFamily())
    {
        case (AF_UNIX): return static_cast<socklen_t>(sizeof(sa.sun));
        case (AF_INET): return static_cast<socklen_t>(sizeof(sa.sin));
        case (AF_INET6): return static_cast<socklen_t>(sizeof(sa.sin6));
        default: return sizeof(sa);
    }
}

socklen_t *SocketAddress::getSizePointer()
{
    usedSize = getAvailableSize();
    return &usedSize;
}

bool SocketAddress::isAny() const
{
    switch (getFamily())
    {
        case (AF_UNSPEC):
            return true;
        case (AF_UNIX):
            return (usedSize == offsetof(struct sockaddr_un, sun_path));
        case (AF_INET):
            return ((sa.sin.sin_port == 0) &&
                    (sa.sin.sin_addr.s_addr == 0));
        case (AF_INET6):
            return ((sa.sin6.sin6_port == 0) &&
                    (sa.sin6.sin6_addr.s6_addr32[0] == 0) &&
                    (sa.sin6.sin6_addr.s6_addr32[1] == 0) &&
                    (sa.sin6.sin6_addr.s6_addr32[2] == 0) &&
                    (sa.sin6.sin6_addr.s6_addr32[3] == 0));
            return true;
        default:
            return false;
    }
}

bool SocketAddress::operator == (const SocketAddress &other) const
{
    return ((usedSize == other.usedSize) &&
            (memcmp(&sa, &other.sa, usedSize) == 0));

}

bool SocketAddress::operator < (const SocketAddress &other) const
{
    if (usedSize < other.usedSize)
    {
        return true;
    }
    else if (usedSize == other.usedSize)
    {
        return memcmp(&sa, &other.sa, usedSize) < 0;
    }
    else
    {
        return false;
    }
}

namespace
{
    void output(std::ostream    &out,
                int             family,
                const void      *addr)
    {
        char buffer[INET6_ADDRSTRLEN + 1] = { };
        inet_ntop(family, addr, buffer, sizeof(buffer) - 1U);
        out << buffer;
    }

    void outputUnix(std::ostream    &out,
                    const char      *path,
                    size_t          size)
    {
        out << "unix:";
        if (size > 0)
        {
            size_t offset(0U);
            if ((path[0] == '\0') &&
                (path[1] != '\0'))
            {
                out << '@';
                offset = 1U;
            }
            for (size_t i = offset;
                 (i < size) && (path[i] != '\0');
                 ++i)
            {
                out << path[i];
            }
        }
    }

    void outputIPv4(std::ostream                &out,
                    const struct sockaddr_in    &sin)
    {
        out << "ipv4:";
        output(out, AF_INET, &sin.sin_addr);
        out << ':' << ntohs(sin.sin_port);
    }

    void outputIPv6(std::ostream                &out,
                    const struct sockaddr_in6   &sin6)
    {
        out << "ipv6:[";
        output(out, AF_INET6, &sin6.sin6_addr);
        if (sin6.sin6_scope_id != 0)
        {
            out << '%' << sin6.sin6_scope_id;
        }
        out << "]:" << ntohs(sin6.sin6_port);
    }
}

std::ostream & EventLoop::operator << (std::ostream         &out,
                                       const SocketAddress  &sa)
{
    switch (sa.getFamily())
    {
        case (AF_UNIX):
            outputUnix(out,
                       sa.getUnixAddress().sun_path,
                       sa.getUsedSize() - offsetof(struct sockaddr_un, sun_path));
            break;
        case (AF_INET):
            outputIPv4(out, sa.getIPv4Address());
            break;
        case (AF_INET6):
            outputIPv6(out, sa.getIPv6Address());
            break;
        default:
            out << "unspecified";
            break;
    }
    return out;
}

namespace
{
    const boost::regex ABSTRACT_UNIX_REGEX("^unix:@(.*)$");

    const boost::regex UNIX_REGEX("^unix:(.*)$");

    const boost::regex IPV4_REGEX("^ipv4:([^:]+):(.+)$");

    const boost::regex IPV6_ADDRESS_REGEX("^ipv6:\\[(.+)\\]:([[:alnum:]_]+)$");

    const boost::regex IPV6_NAME_REGEX("^ipv6:([^:]+):([[:alnum:]_]+)$");

    bool input(const std::string    &str,
               SocketAddress        &sa)
    {
        boost::match_results<std::string::const_iterator> what;
        if (boost::regex_search(str, what, ABSTRACT_UNIX_REGEX))
        {
            sa = SocketAddress::createAbstractUnix(std::string(what[1].first, what[1].second));
            return true;
        }
        else if (boost::regex_search(str, what, UNIX_REGEX))
        {
            if (what[1].first == what[1].second)
            {
                sa = SocketAddress::createUnixAny();
            }
            else
            {
                sa = SocketAddress::createUnix(std::string(what[1].first, what[1].second));
            }
            return true;
        }
        else if (boost::regex_search(str, what, IPV4_REGEX))
        {
            sa = SocketAddress::createIPv4(std::string(what[1].first, what[1].second),
                                           std::string(what[2].first, what[2].second));
            return true;
        }
        else if (boost::regex_search(str, what, IPV6_ADDRESS_REGEX))
        {
            sa = SocketAddress::createIPv6(std::string(what[1].first, what[1].second),
                                           std::string(what[2].first, what[2].second));
            return true;
        }
        else if (boost::regex_search(str, what, IPV6_NAME_REGEX))
        {
            sa = SocketAddress::createIPv6(std::string(what[1].first, what[1].second),
                                           std::string(what[2].first, what[2].second));
            return true;
        }
        return false;
    }
}

std::istream & EventLoop::operator >> (std::istream     &in,
                                       SocketAddress    &sa)
{
    std::string str;
    in >> str;
    try
    {
        if (!input(str, sa))
        {
            in.clear(std::ios_base::failbit);
        }
    }
    catch (const SocketAddress::TranslationError &)
    {
        in.clear(std::ios_base::failbit);
    }
    return in;
}
