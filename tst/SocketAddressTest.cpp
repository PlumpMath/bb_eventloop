#include <sstream>
#include <algorithm>
#include <sys/un.h>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>
#include <gtest/gtest.h>

#include <EventLoop/SocketAddress.hpp>

using namespace EventLoop;

namespace
{
    const std::string maxPath("12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567");
    const std::string tooLongPath("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678");

    void EXPECT_EQ_SA(const SocketAddress &sa1, const SocketAddress &sa2)
    {
        EXPECT_EQ(sa1.getFamily(), sa2.getFamily());
        EXPECT_EQ(sa1.getUsedSize(), sa2.getUsedSize());
        EXPECT_EQ(sa1.getAvailableSize(), sa2.getAvailableSize());
        EXPECT_EQ(0, memcmp(sa1.getPointer(), sa2.getPointer(), sa1.getAvailableSize()));
    }
}

TEST(SocketAddress, defaultConstructorCreatesUnspecAddress)
{
    SocketAddress sa;
    EXPECT_EQ(AF_UNSPEC, sa.getFamily());
}

TEST(SocketAddress, createUnixAddress)
{
    const std::string path("foo");
    SocketAddress sa(SocketAddress::createUnix(path));
    EXPECT_EQ(AF_UNIX, sa.getFamily());
    EXPECT_EQ(path, std::string(sa.getUnixAddress().sun_path));
}

TEST(SocketAddress, tooLongUnixPathThrows)
{
    SocketAddress::createUnix(maxPath);
    EXPECT_THROW(SocketAddress::createUnix(tooLongPath), SocketAddress::TooLongUnixSocketPath);
}

TEST(SocketAddress, createUnixAddressAny)
{
    SocketAddress sa(SocketAddress::createUnixAny());
    EXPECT_EQ(AF_UNIX, sa.getFamily());
    EXPECT_EQ(sizeof(sa_family_t), sa.getUsedSize());
}

TEST(SocketAddress, createAbstractUnixAddress)
{
    const std::string path("foo");
    SocketAddress sa(SocketAddress::createAbstractUnix(path));
    EXPECT_EQ(AF_UNIX, sa.getFamily());
    EXPECT_EQ(0, sa.getUnixAddress().sun_path[0]);
    EXPECT_EQ(path, std::string(&sa.getUnixAddress().sun_path[0] + 1));
}

TEST(SocketAddress, tooLongAbstractUnixPathThrows)
{
    SocketAddress::createAbstractUnix(maxPath);
    EXPECT_THROW(SocketAddress::createAbstractUnix(tooLongPath), SocketAddress::TooLongUnixSocketPath);
}

TEST(SocketAddress, createIPv4AddressWithServiceName)
{
    SocketAddress sa(SocketAddress::createIPv4("127.0.0.1", "ssh"));
    EXPECT_EQ(AF_INET, sa.getFamily());
    uint32_t addr(htonl((127U << 24U) | 1));
    uint16_t port(htons(22));
    EXPECT_EQ(addr, sa.getIPv4Address().sin_addr.s_addr);
    EXPECT_EQ(port, sa.getIPv4Address().sin_port);
}

TEST(SocketAddress, createIPv4AddressWithServiceNumber)
{
    uint16_t port(htons(22));
    SocketAddress sa(SocketAddress::createIPv4("127.0.0.1", port));
    EXPECT_EQ(AF_INET, sa.getFamily());
    EXPECT_EQ(port, sa.getIPv4Address().sin_port);
}

TEST(SocketAddress, createIPv4AddressWithBinaryAddressAndServiceName)
{
    const SocketAddress tmp(SocketAddress::createIPv4("127.0.0.1", "ssh"));
    const SocketAddress sa(SocketAddress::createIPv4(tmp.getIPv4Address().sin_addr, "ssh"));
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createIPv4AddressWithBinaryAddressAndServiceNumberAsString)
{
    const SocketAddress tmp(SocketAddress::createIPv4("127.0.0.1", "9000"));
    const SocketAddress sa(SocketAddress::createIPv4(tmp.getIPv4Address().sin_addr, "9000"));
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createIPv4AddressWithBinaryAddressAndServiceNumber)
{
    const SocketAddress tmp(SocketAddress::createIPv4("127.0.0.1", "9000"));
    const SocketAddress sa(SocketAddress::createIPv4(tmp.getIPv4Address().sin_addr, tmp.getIPv4Address().sin_port));
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createIPv4AddressWithNonExistingNameThrows)
{
    EXPECT_THROW(SocketAddress::createIPv4("nosuchname.invalid", "nosuchservice"), SocketAddress::TranslationError);
}

TEST(SocketAddress, createIPv4AddressWithNonExistingServiceThrows)
{
    const struct in_addr addr = { };
    EXPECT_THROW(SocketAddress::createIPv4(addr, "nosuchservice"), SocketAddress::TranslationError);
}

TEST(SocketAddress, createIPv4AddressAny)
{
    SocketAddress sa(SocketAddress::createIPv4Any());
    EXPECT_EQ(AF_INET, sa.getFamily());
    EXPECT_EQ(0U, sa.getIPv4Address().sin_addr.s_addr);
    EXPECT_EQ(0U, sa.getIPv4Address().sin_port);
}

TEST(SocketAddress, createIPv6AddressWithServiceName)
{
    SocketAddress sa(SocketAddress::createIPv6("2000::1", "ssh"));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    struct in6_addr addr;
    addr.s6_addr32[0] = htonl(0x20000000);
    addr.s6_addr32[1] = 0;
    addr.s6_addr32[2] = 0;
    addr.s6_addr32[3] = htonl(1);
    EXPECT_TRUE(IN6_ARE_ADDR_EQUAL(&addr, &sa.getIPv6Address().sin6_addr));
    uint16_t port(htons(22));
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, createIPv6AddressWithServiceNumber)
{
    uint16_t port(htons(22));
    SocketAddress sa(SocketAddress::createIPv6("::1", port));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, createIPv6AddressWithScopeIdAsNumber)
{
    uint16_t port(htons(22));
    SocketAddress sa(SocketAddress::createIPv6("fe80::5054:ff:fe12:3456%10", port));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    struct in6_addr addr;
    addr.s6_addr32[0] = htonl(0xfe800000);
    addr.s6_addr32[1] = 0;
    addr.s6_addr32[2] = htonl(0x505400ff);
    addr.s6_addr32[3] = htonl(0xfe123456);
    EXPECT_TRUE(IN6_ARE_ADDR_EQUAL(&addr, &sa.getIPv6Address().sin6_addr));
    EXPECT_EQ(10U, sa.getIPv6Address().sin6_scope_id);
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, createIPv6AddressWithScopeIdAsName)
{
    uint16_t port(htons(22));
    SocketAddress sa(SocketAddress::createIPv6("fe80::5054:ff:fe12:3456%lo", port));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    struct in6_addr addr;
    addr.s6_addr32[0] = htonl(0xfe800000);
    addr.s6_addr32[1] = 0;
    addr.s6_addr32[2] = htonl(0x505400ff);
    addr.s6_addr32[3] = htonl(0xfe123456);
    EXPECT_TRUE(IN6_ARE_ADDR_EQUAL(&addr, &sa.getIPv6Address().sin6_addr));
    EXPECT_NE(0U, sa.getIPv6Address().sin6_scope_id); /* "lo" interface should always exist, but we don't know its index */
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, createIPv6AddressWithBinaryAddressAndServiceName)
{
    const SocketAddress tmp(SocketAddress::createIPv6("::1", "ssh"));
    const SocketAddress sa(SocketAddress::createIPv6(tmp.getIPv6Address().sin6_addr, "ssh"));
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createIPv6AddressWithBinaryAddressAndServiceNumber)
{
    const SocketAddress tmp(SocketAddress::createIPv6("::1", "9000"));
    const SocketAddress sa(SocketAddress::createIPv6(tmp.getIPv6Address().sin6_addr, tmp.getIPv6Address().sin6_port));
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createIPv6AddressWithBinaryAddressAndScopeIdAndServiceName)
{
    const SocketAddress tmp(SocketAddress::createIPv6("fe80::1%1", "ssh"));
    const SocketAddress sa(SocketAddress::createIPv6(tmp.getIPv6Address().sin6_addr, tmp.getIPv6Address().sin6_scope_id, "ssh"));
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createIPv6AddressWithBinaryAddressAndScopeIdAndServiceNumber)
{
    const SocketAddress tmp(SocketAddress::createIPv6("fe80::1%1", "9000"));
    const SocketAddress sa(SocketAddress::createIPv6(tmp.getIPv6Address().sin6_addr, tmp.getIPv6Address().sin6_scope_id, tmp.getIPv6Address().sin6_port));
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createIPv6AddressWithNonExistingNameThrows)
{
    EXPECT_THROW(SocketAddress::createIPv6("nosuchname.invalid", "nosuchservice"), SocketAddress::TranslationError);
}

TEST(SocketAddress, createIPv6AddressAny)
{
    SocketAddress sa(SocketAddress::createIPv6Any());
    EXPECT_EQ(AF_INET6, sa.getFamily());
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_addr.s6_addr32[0]);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_addr.s6_addr32[1]);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_addr.s6_addr32[2]);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_addr.s6_addr32[3]);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_scope_id);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, createIPv4WithCreateIPv4OrIPv6AndServiceName)
{
    const SocketAddress sa(SocketAddress::createIPv4OrIPv6("127.0.0.1", "ssh"));
    EXPECT_EQ(AF_INET, sa.getFamily());
    const uint32_t addr(htonl((127U << 24U) | 1));
    const uint16_t port(htons(22));
    EXPECT_EQ(addr, sa.getIPv4Address().sin_addr.s_addr);
    EXPECT_EQ(port, sa.getIPv4Address().sin_port);
}

TEST(SocketAddress, createIPv4WithCreateIPv4OrIPv6AndServiceNumber)
{
    const uint16_t port(htons(22));
    const SocketAddress sa(SocketAddress::createIPv4OrIPv6("127.0.0.1", port));
    EXPECT_EQ(AF_INET, sa.getFamily());
    const uint32_t addr(htonl((127U << 24U) | 1));
    EXPECT_EQ(addr, sa.getIPv4Address().sin_addr.s_addr);
    EXPECT_EQ(port, sa.getIPv4Address().sin_port);
}

TEST(SocketAddress, createIPv6WithCreateIPv4OrIPv6AndServiceName)
{
    const SocketAddress sa(SocketAddress::createIPv4OrIPv6("2000::1", "ssh"));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    struct in6_addr addr;
    addr.s6_addr32[0] = htonl(0x20000000);
    addr.s6_addr32[1] = 0;
    addr.s6_addr32[2] = 0;
    addr.s6_addr32[3] = htonl(1);
    EXPECT_TRUE(IN6_ARE_ADDR_EQUAL(&addr, &sa.getIPv6Address().sin6_addr));
    const uint16_t port(htons(22));
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, createIPv6WithCreateIPv4OrIPv6AndServiceNumber)
{
    const uint16_t port(htons(22));
    const SocketAddress sa(SocketAddress::createIPv4OrIPv6("2000::1", port));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    struct in6_addr addr;
    addr.s6_addr32[0] = htonl(0x20000000);
    addr.s6_addr32[1] = 0;
    addr.s6_addr32[2] = 0;
    addr.s6_addr32[3] = htonl(1);
    EXPECT_TRUE(IN6_ARE_ADDR_EQUAL(&addr, &sa.getIPv6Address().sin6_addr));
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, createFromPointer)
{
    const SocketAddress tmp(SocketAddress::createUnix("foo"));
    const SocketAddress sa(tmp.getPointer(), tmp.getUsedSize());
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createFromPointerWithTooBigSizeDoesntCrash)
{
    const SocketAddress tmp(SocketAddress::createUnix(maxPath));
    const SocketAddress sa(tmp.getPointer(), 1024);
    EXPECT_EQ(0, memcmp(&tmp.getUnixAddress(), &sa.getUnixAddress(), sizeof(tmp.getUnixAddress())));
}

TEST(SocketAddress, createFromUnix)
{
    const SocketAddress tmp(SocketAddress::createUnix("foo"));
    const SocketAddress sa(tmp.getUnixAddress(), tmp.getUsedSize());
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createFromIPv4)
{
    const SocketAddress tmp(SocketAddress::createIPv4("127.0.0.1", "9000"));
    const SocketAddress sa(tmp.getIPv4Address());
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createFromIPv4AddressAndPort)
{
    const SocketAddress tmp(SocketAddress::createIPv4("127.0.0.1", "9000"));
    const SocketAddress sa(tmp.getIPv4Address().sin_addr, tmp.getIPv4Address().sin_port);
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createFromIPv6)
{
    const SocketAddress tmp(SocketAddress::createIPv6("::1", "9000"));
    const SocketAddress sa(tmp.getIPv6Address());
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createFromIPv6AddressAndPort)
{
    const SocketAddress tmp(SocketAddress::createIPv6("::1", "9000"));
    const SocketAddress sa(tmp.getIPv6Address().sin6_addr, tmp.getIPv6Address().sin6_port);
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, createFromIPv6AddressScopeIdAndPort)
{
    const SocketAddress tmp(SocketAddress::createIPv6("fe80::11%1", "9000"));
    const SocketAddress sa(tmp.getIPv6Address().sin6_addr, 1U, tmp.getIPv6Address().sin6_port);
    EXPECT_EQ_SA(tmp, sa);
}

TEST(SocketAddress, getUnspecAddressUsedSize)
{
    const SocketAddress sa;
    EXPECT_EQ(0U, sa.getUsedSize());
}

TEST(SocketAddress, getUnspecAddressAvailableSize)
{
    const SocketAddress sa;
    const size_t combinedSize(std::max(std::max(sizeof(struct sockaddr),
                                                sizeof(struct sockaddr_un)),
                                       std::max(sizeof(struct sockaddr_in),
                                                sizeof(struct sockaddr_in6))));
    EXPECT_LE(combinedSize, sa.getAvailableSize());
}

TEST(SocketAddress, getUnixAddressUsedSize)
{
    const std::string path("/var/run/socket");
    const size_t size(offsetof(struct sockaddr_un, sun_path) + path.size() + 1U);
    SocketAddress sa(SocketAddress::createUnix(path));
    EXPECT_EQ(size, sa.getUsedSize());
}

TEST(SocketAddress, getUnixAddressAvailableSize)
{
    SocketAddress sa(SocketAddress::createUnix("/var/run/socket"));
    EXPECT_EQ(sizeof(struct sockaddr_un), sa.getAvailableSize());
}

TEST(SocketAddress, getAbstractUnixAddressUsedSize)
{
    const std::string path("foo");
    const size_t size(sizeof(sa_family_t) + path.size() + 1U);
    SocketAddress sa(SocketAddress::createAbstractUnix(path));
    EXPECT_EQ(size, sa.getUsedSize());
}

TEST(SocketAddress, getAbstractUnixAddressAvailableSize)
{
    SocketAddress sa(SocketAddress::createAbstractUnix("foo"));
    EXPECT_EQ(sizeof(struct sockaddr_un), sa.getAvailableSize());
}

TEST(SocketAddress, getIPv4AddressUsedSize)
{
    SocketAddress sa(SocketAddress::createIPv4("127.0.0.1", "ssh"));
    EXPECT_EQ(sizeof(struct sockaddr_in), sa.getUsedSize());
}

TEST(SocketAddress, getIPv4AddressAvailableSize)
{
    SocketAddress sa(SocketAddress::createIPv4("127.0.0.1", "ssh"));
    EXPECT_EQ(sizeof(struct sockaddr_in), sa.getAvailableSize());
}

TEST(SocketAddress, getIPv6AddressUsedSize)
{
    SocketAddress sa(SocketAddress::createIPv6("::1", "ssh"));
    EXPECT_EQ(sizeof(struct sockaddr_in6), sa.getUsedSize());
}

TEST(SocketAddress, getIPv6AddressAvailableSize)
{
    SocketAddress sa(SocketAddress::createIPv6("::1", "ssh"));
    EXPECT_EQ(sizeof(struct sockaddr_in6), sa.getAvailableSize());
}

TEST(SocketAddress, setUsedSize)
{
    const socklen_t size(4U);
    SocketAddress sa(SocketAddress::createUnix("foo"));
    sa.setUsedSize(size);
    EXPECT_EQ(size, sa.getUsedSize());
    EXPECT_EQ("unix:fo", boost::lexical_cast<std::string>(sa));
}

TEST(SocketAddress, outputUnspecified)
{
    EXPECT_EQ("unspecified", boost::lexical_cast<std::string>(SocketAddress()));
}

TEST(SocketAddress, outputUnix)
{
    EXPECT_EQ("unix:/var/run/socket", boost::lexical_cast<std::string>(SocketAddress::createUnix("/var/run/socket")));
    EXPECT_EQ(std::string("unix:") + maxPath, boost::lexical_cast<std::string>(SocketAddress::createUnix(maxPath)));
}

TEST(SocketAddress, outputUnixAny)
{
    EXPECT_EQ("unix:", boost::lexical_cast<std::string>(SocketAddress::createUnixAny()));
}

TEST(SocketAddress, outputUnixWithTooLongPath)
{
    SocketAddress sa(SocketAddress::createUnix(maxPath));
    sa.getUnixAddress().sun_path[maxPath.size()] = 'a';
    EXPECT_EQ(std::string("unix:") + maxPath + std::string("a"), boost::lexical_cast<std::string>(sa));
}

TEST(SocketAddress, outputAbstractUnix)
{
    EXPECT_EQ("unix:@NetworkManager", boost::lexical_cast<std::string>(SocketAddress::createAbstractUnix("NetworkManager")));
    EXPECT_EQ(std::string("unix:@") + maxPath, boost::lexical_cast<std::string>(SocketAddress::createAbstractUnix(maxPath)));
}

TEST(SocketAddress, outputEmptyUnix)
{
    EXPECT_EQ("unix:", boost::lexical_cast<std::string>(SocketAddress::createUnix("")));
}

TEST(SocketAddress, outputIPv4)
{
    EXPECT_EQ("ipv4:10.0.0.1:450", boost::lexical_cast<std::string>(SocketAddress::createIPv4("10.0.0.1", "450")));
}

TEST(SocketAddress, outputIPv6)
{
    EXPECT_EQ("ipv6:[2001::123]:9000", boost::lexical_cast<std::string>(SocketAddress::createIPv6("2001::123", "9000")));
}

TEST(SocketAddress, outputIPv6WithScopeId)
{
    EXPECT_EQ("ipv6:[fe80::5054:ff:fe12:3456%10]:9000", boost::lexical_cast<std::string>(SocketAddress::createIPv6("fe80::5054:ff:fe12:3456%10", "9000")));
}

TEST(SocketAddress, inputWithoutColonThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("unix"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputUnknownProtocolThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("something:blaa"), boost::bad_lexical_cast);
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("something:blaa:foobar"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputUnix)
{
    const SocketAddress sa(boost::lexical_cast<SocketAddress>("unix:/var/run/socket"));
    EXPECT_EQ(AF_UNIX, sa.getFamily());
    EXPECT_EQ("/var/run/socket", std::string(sa.getUnixAddress().sun_path));
}

TEST(SocketAddress, inputAbstractUnix)
{
    const SocketAddress sa(boost::lexical_cast<SocketAddress>("unix:@NetworkManager"));
    EXPECT_EQ(AF_UNIX, sa.getFamily());
    EXPECT_EQ(AF_UNIX, sa.getFamily());
    EXPECT_EQ(0, sa.getUnixAddress().sun_path[0]);
    EXPECT_EQ("NetworkManager", std::string(&sa.getUnixAddress().sun_path[0] + 1));
}

TEST(SocketAddress, inputEmptyUnix)
{
    const SocketAddress sa(boost::lexical_cast<SocketAddress>("unix:"));
    EXPECT_EQ(AF_UNIX, sa.getFamily());
    EXPECT_EQ("", std::string(sa.getUnixAddress().sun_path));
}

TEST(SocketAddress, inputIPv4Address)
{
    const SocketAddress sa(boost::lexical_cast<SocketAddress>("ipv4:127.0.0.1:450"));
    EXPECT_EQ(AF_INET, sa.getFamily());
    uint32_t addr(htonl((127U << 24U) | 1));
    uint16_t port(htons(450));
    EXPECT_EQ(addr, sa.getIPv4Address().sin_addr.s_addr);
    EXPECT_EQ(port, sa.getIPv4Address().sin_port);
}

TEST(SocketAddress, inputIPv4Name)
{
    const SocketAddress sa(boost::lexical_cast<SocketAddress>("ipv4:localhost:ssh"));
    EXPECT_EQ(AF_INET, sa.getFamily());
    uint32_t addr(htonl((127U << 24U) | 1));
    uint16_t port(htons(22));
    EXPECT_EQ(addr, sa.getIPv4Address().sin_addr.s_addr);
    EXPECT_EQ(port, sa.getIPv4Address().sin_port);
}

TEST(SocketAddress, inputInvalidIPv4NameThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv4:localhost:nosuchprotocol"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputIPv4AddressWithoutPortThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv4:127.0.0.1"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputIPv6Address)
{
    const SocketAddress sa(boost::lexical_cast<SocketAddress>("ipv6:[2000::1]:22"));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    struct in6_addr addr;
    addr.s6_addr32[0] = htonl(0x20000000);
    addr.s6_addr32[1] = 0;
    addr.s6_addr32[2] = 0;
    addr.s6_addr32[3] = htonl(1);
    EXPECT_TRUE(IN6_ARE_ADDR_EQUAL(&addr, &sa.getIPv6Address().sin6_addr));
    uint16_t port(htons(22));
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, DISABLED_inputIPv6Name)
{
    const SocketAddress sa(boost::lexical_cast<SocketAddress>("ipv6:ip6-localhost:ssh"));
    EXPECT_EQ(AF_INET6, sa.getFamily());
    struct in6_addr addr;
    addr.s6_addr32[0] = 0;
    addr.s6_addr32[1] = 0;
    addr.s6_addr32[2] = 0;
    addr.s6_addr32[3] = htonl(1);
    EXPECT_TRUE(IN6_ARE_ADDR_EQUAL(&addr, &sa.getIPv6Address().sin6_addr));
    uint16_t port(htons(22));
    EXPECT_EQ(port, sa.getIPv6Address().sin6_port);
    EXPECT_EQ(0U, sa.getIPv6Address().sin6_flowinfo);
}

TEST(SocketAddress, inputInvalidIPv6NameThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv6:localhost:nosuchprotocol"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputInvalidIPv6AddressThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv6:[::1]:nosuchprotocol"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputIPv6AddressWithoutPortThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv6:[fe80::1]"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputIPv6AddressWithoutSquareBracketsThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv6:fe80::1"), boost::bad_lexical_cast);
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv6:fe80::1:1"), boost::bad_lexical_cast);
}

TEST(SocketAddress, inputIPv6AddressWithoutAddressThrows)
{
    EXPECT_THROW(boost::lexical_cast<SocketAddress>("ipv6:[]:1"), boost::bad_lexical_cast);
}

TEST(SocketAddress, isAny)
{
    EXPECT_TRUE(SocketAddress().isAny());
    EXPECT_TRUE(SocketAddress::createUnixAny().isAny());
    EXPECT_TRUE(SocketAddress::createIPv4("0.0.0.0", 0).isAny());
    EXPECT_TRUE(SocketAddress::createIPv4Any().isAny());
    EXPECT_TRUE(SocketAddress::createIPv6("::", 0).isAny());
    EXPECT_TRUE(SocketAddress::createIPv6Any().isAny());
    EXPECT_TRUE(boost::lexical_cast<SocketAddress>("unix:").isAny());
    EXPECT_TRUE(boost::lexical_cast<SocketAddress>("ipv4:0.0.0.0:0").isAny());
    EXPECT_TRUE(boost::lexical_cast<SocketAddress>("ipv6:[::]:0").isAny());
    EXPECT_FALSE(SocketAddress::createIPv4("0.0.0.0", 1).isAny());
    EXPECT_FALSE(SocketAddress::createIPv4("0.0.0.1", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("::", 1).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("1:0:0:0:0:0:0:0", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("0:1:0:0:0:0:0:0", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("0:0:1:0:0:0:0:0", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("0:0:0:1:0:0:0:0", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("0:0:0:0:1:0:0:0", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("0:0:0:0:0:1:0:0", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("0:0:0:0:0:0:1:0", 0).isAny());
    EXPECT_FALSE(SocketAddress::createIPv6("0:0:0:0:0:0:0:1", 0).isAny());
    EXPECT_FALSE(SocketAddress::createUnix("/var/run/socket").isAny());
    EXPECT_FALSE(SocketAddress::createAbstractUnix("path").isAny());
}

TEST(SocketAddress, getSizePointer)
{
    SocketAddress sa;
    socklen_t *ptr(sa.getSizePointer());
    ASSERT_NE(nullptr, ptr);
    EXPECT_EQ(sa.getAvailableSize(), *ptr);
    *ptr = 10U;
    EXPECT_EQ(10U, sa.getUsedSize());
}

TEST(SocketAddress, getIPv4Port)
{
    const uint16_t PORT(6789);
    EXPECT_EQ(PORT, SocketAddress::createIPv4("127.0.0.1", PORT).getIPPort());
}

TEST(SocketAddress, getIPv6Port)
{
    const uint16_t PORT(6789);
    EXPECT_EQ(PORT, SocketAddress::createIPv6("::1", PORT).getIPPort());
}

TEST(SocketAddress, equalOperator)
{
    const char path[] = "1235\0\0\0";
    EXPECT_TRUE(SocketAddress::createUnix(std::string(path, sizeof(path))) == SocketAddress::createUnix(std::string(path, sizeof(path))));
    EXPECT_FALSE(SocketAddress::createUnix(std::string(path, sizeof(path) - 1U)) == SocketAddress::createUnix(std::string(path, sizeof(path))));
    EXPECT_TRUE(SocketAddress::createIPv4("127.0.0.1", 123) == SocketAddress::createIPv4("127.0.0.1", 123));
    EXPECT_FALSE(SocketAddress::createIPv4("127.0.0.1", 123) == SocketAddress::createIPv4("127.0.0.1", 333));
    EXPECT_FALSE(SocketAddress::createIPv4("127.0.0.1", 123) == SocketAddress::createIPv6("::1", 123));
}

TEST(SocketAddress, lessThanOperator)
{
    const SocketAddress none;
    const SocketAddress aunix1(SocketAddress::createAbstractUnix("aa"));
    const SocketAddress aunix2(SocketAddress::createAbstractUnix(std::string("aa\0", 3)));
    const SocketAddress unix1(SocketAddress::createUnix("aaa"));
    const SocketAddress unix2(SocketAddress::createUnix("bbb"));
    const SocketAddress ipv41(SocketAddress::createIPv4("10.0.0.20", htons(1000)));
    const SocketAddress ipv42(SocketAddress::createIPv4("20.0.0.10", htons(1000)));
    const SocketAddress ipv43(SocketAddress::createIPv4("20.0.0.10", htons(1001)));
    const SocketAddress ipv61(SocketAddress::createIPv6("10::20", htons(1000)));
    const SocketAddress ipv62(SocketAddress::createIPv6("20::10", htons(1000)));
    const SocketAddress ipv63(SocketAddress::createIPv6("20::10", htons(1001)));

    EXPECT_FALSE(none < none);
    EXPECT_TRUE(none < aunix1);
    EXPECT_FALSE(aunix1 < none);

    EXPECT_FALSE(aunix1 < aunix1);
    EXPECT_TRUE(aunix1 < aunix2);
    EXPECT_FALSE(aunix2 < aunix1);
    EXPECT_TRUE(aunix2 < unix1);
    EXPECT_FALSE(unix1 < aunix2);

    EXPECT_FALSE(unix1 < unix1);
    EXPECT_TRUE(unix1 < unix2);
    EXPECT_FALSE(unix2 < unix1);
    EXPECT_TRUE(unix2 < ipv41);
    EXPECT_FALSE(ipv41 < unix2);

    EXPECT_FALSE(ipv41 < ipv41);
    EXPECT_TRUE(ipv41 < ipv42);
    EXPECT_FALSE(ipv42 < ipv41);
    EXPECT_TRUE(ipv42 < ipv43);
    EXPECT_FALSE(ipv43 < ipv42);
    EXPECT_TRUE(ipv43 < ipv61);
    EXPECT_FALSE(ipv61 < ipv43);

    EXPECT_FALSE(ipv61 < ipv61);
    EXPECT_TRUE(ipv61 < ipv62);
    EXPECT_FALSE(ipv62 < ipv61);
    EXPECT_TRUE(ipv62 < ipv63);
    EXPECT_FALSE(ipv63 < ipv62);
}
