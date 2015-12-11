#include <sstream>
#include <cstring>
#include <cerrno>
#include <gtest/gtest.h>

#include <EventLoop/SystemException.hpp>

using namespace EventLoop;

TEST(SystemException, errnoConstructor)
{
    errno = EINVAL;
    SystemException e("read");
    const std::exception &s(e);
    std::ostringstream os;
    os << "read: " << strerror(EINVAL);
    EXPECT_EQ(os.str(), std::string(s.what()));
    EXPECT_EQ("read", e.function);
    EXPECT_EQ(EINVAL, e.error);
}

TEST(SystemException, errorConstructor)
{
    SystemException e("write", EAGAIN);
    const std::exception &s(e);
    std::ostringstream os;
    os << "write: " << strerror(EAGAIN);
    EXPECT_EQ(os.str(), std::string(s.what()));
    EXPECT_EQ("write", e.function);
    EXPECT_EQ(EAGAIN, e.error);
}
