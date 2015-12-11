#include <string>
#include <boost/lexical_cast.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtest/gtest.h>

#include <EventLoop/FileDescriptor.hpp>

#include "private/tst/SystemMock.hpp"

using namespace testing;
using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class FileDescriptorTest: public testing::Test
    {
    public:
        SystemMock sm;
    };
}

TEST_F(FileDescriptorTest, defaultConstructorCreatesInvalidFD)
{
    FileDescriptor fd;
    EXPECT_FALSE(static_cast<bool>(fd));
}

TEST_F(FileDescriptorTest, systemConstructorCreatesInvalidFD)
{
    FileDescriptor fd(sm);
    EXPECT_FALSE(static_cast<bool>(fd));
}

TEST_F(FileDescriptorTest, realConstructorWithInvalidFD)
{
    FileDescriptor fd(-1);
    EXPECT_FALSE(static_cast<bool>(fd));
}

TEST_F(FileDescriptorTest, realConstructorWithValidFD)
{
    /* We need some real fd, otherwise destructor throws while calling close() */
    FileDescriptor fd(open("/dev/null", O_RDONLY));
    EXPECT_TRUE(static_cast<bool>(fd));
}

TEST_F(FileDescriptorTest, destructorDoesntCallCloseIfFDIsInvalid)
{
    FileDescriptor fd(sm, -1);
    EXPECT_CALL(sm, close(_))
        .Times(0);
}

TEST_F(FileDescriptorTest, destructorCallsCloseIfFDIsValid)
{
    FileDescriptor fd(sm, 9);
    EXPECT_CALL(sm, close(9))
        .Times(1);
}

TEST_F(FileDescriptorTest, invalidFDIsFalse)
{
    EXPECT_FALSE(static_cast<bool>(FileDescriptor(sm, -1)));
}

TEST_F(FileDescriptorTest, validFDIsTrue)
{
    EXPECT_TRUE(static_cast<bool>(FileDescriptor(sm, 0)));
    EXPECT_TRUE(static_cast<bool>(FileDescriptor(sm, 1)));
}

TEST_F(FileDescriptorTest, validConversionToInt)
{
    FileDescriptor fd(sm, 9);
    EXPECT_EQ(9, static_cast<int>(fd));
}

TEST_F(FileDescriptorTest, invalidConversionToIntThrows)
{
    FileDescriptor fd(-1);
    EXPECT_THROW(static_cast<void>(static_cast<int>(fd)), FileDescriptor::InvalidFD);
}

TEST_F(FileDescriptorTest, closeValidFD)
{
    FileDescriptor fd(sm, 9);
    EXPECT_CALL(sm, close(9))
        .Times(1);
    fd.close();
    EXPECT_FALSE(static_cast<bool>(fd));
}

TEST_F(FileDescriptorTest, closeInvalidFDThrows)
{
    FileDescriptor fd(-1);
    EXPECT_THROW(fd.close(), FileDescriptor::InvalidFD);
}

TEST_F(FileDescriptorTest, releaseValidFD)
{
    FileDescriptor fd(sm, 9);
    EXPECT_CALL(sm, close(_))
        .Times(0);
    int tmp(fd.release());
    EXPECT_EQ(9, tmp);
    EXPECT_FALSE(static_cast<bool>(fd));
}

TEST_F(FileDescriptorTest, releaseInvalidFDThrows)
{
    FileDescriptor fd(-1);
    EXPECT_THROW(fd.release(), FileDescriptor::InvalidFD);
}

TEST_F(FileDescriptorTest, assigmentTakesOwnership)
{
    FileDescriptor fd(sm);
    fd = 99;
    EXPECT_CALL(sm, close(99))
        .Times(1);
}

TEST_F(FileDescriptorTest, assigmentFirstClosesTheOldFD)
{
    FileDescriptor fd(sm, 88);
    EXPECT_CALL(sm, close(88))
        .Times(1);
    fd = 99;
    EXPECT_CALL(sm, close(99))
        .Times(1);
}

TEST_F(FileDescriptorTest, swapSwapsFileDescriptors)
{
    FileDescriptor fd1(sm, 1);
    FileDescriptor fd2(sm, 2);
    fd1.swap(fd2);
    EXPECT_EQ(2, static_cast<int>(fd1));
    EXPECT_EQ(1, static_cast<int>(fd2));
}

TEST_F(FileDescriptorTest, compareTwoFileDescriptors)
{
    const FileDescriptor one(sm, 1);
    const FileDescriptor two(sm, 2);
    const FileDescriptor invalid(sm);

    EXPECT_TRUE(one == one);
    EXPECT_FALSE(one == two);
    EXPECT_FALSE(one == invalid);

    EXPECT_FALSE(one != one);
    EXPECT_TRUE(one != two);
    EXPECT_TRUE(one != invalid);

    EXPECT_TRUE(one < two);
    EXPECT_FALSE(two < one);
    EXPECT_FALSE(two < invalid);

    EXPECT_FALSE(one > two);
    EXPECT_TRUE(two > one);
    EXPECT_TRUE(two > invalid);

    EXPECT_TRUE(one <= one);
    EXPECT_TRUE(one <= two);
    EXPECT_FALSE(two <= one);
    EXPECT_FALSE(two <= invalid);

    EXPECT_TRUE(one >= one);
    EXPECT_FALSE(one >= two);
    EXPECT_TRUE(two >= one);
    EXPECT_TRUE(two >= invalid);
}

TEST_F(FileDescriptorTest, compareFileDescriptorAndInt)
{
    const FileDescriptor fdOne(sm, 1);
    const FileDescriptor fdTwo(sm, 2);
    const FileDescriptor invalid(sm);
    const int iOne(1);
    const int iTwo(2);

    EXPECT_FALSE(fdOne == iTwo);
    EXPECT_TRUE(fdTwo == iTwo);
    EXPECT_FALSE(invalid == iTwo);

    EXPECT_TRUE(fdOne != iTwo);
    EXPECT_FALSE(fdTwo != iTwo);
    EXPECT_TRUE(invalid != iTwo);

    EXPECT_TRUE(fdOne < iTwo);
    EXPECT_FALSE(fdTwo < iTwo);
    EXPECT_TRUE(invalid < iTwo);

    EXPECT_FALSE(fdOne > iTwo);
    EXPECT_FALSE(fdTwo > iTwo);
    EXPECT_TRUE(fdTwo > iOne);
    EXPECT_FALSE(invalid > iTwo);

    EXPECT_TRUE(fdOne <= iTwo);
    EXPECT_TRUE(fdTwo <= iTwo);
    EXPECT_FALSE(fdTwo <= iOne);
    EXPECT_TRUE(invalid <= iTwo);

    EXPECT_FALSE(fdOne >= iTwo);
    EXPECT_TRUE(fdTwo >= iTwo);
    EXPECT_TRUE(fdTwo >= iOne);
    EXPECT_FALSE(invalid >= iTwo);
}

TEST_F(FileDescriptorTest, compareIntAndFileDescriptor)
{
    const FileDescriptor fdOne(sm, 1);
    const FileDescriptor fdTwo(sm, 2);
    const FileDescriptor invalid(sm);
    const int iOne(1);
    const int iTwo(2);

    EXPECT_TRUE(iOne == fdOne);
    EXPECT_FALSE(iOne == fdTwo);
    EXPECT_FALSE(iOne == invalid);

    EXPECT_FALSE(iOne != fdOne);
    EXPECT_TRUE(iOne != fdTwo);
    EXPECT_TRUE(iOne != invalid);

    EXPECT_FALSE(iOne > fdOne);
    EXPECT_TRUE(iTwo > fdOne);
    EXPECT_TRUE(iOne > invalid);

    EXPECT_FALSE(iOne < fdOne);
    EXPECT_TRUE(iOne < fdTwo);
    EXPECT_FALSE(iOne < invalid);

    EXPECT_TRUE(iTwo >= fdOne);
    EXPECT_TRUE(iTwo >= fdTwo);
    EXPECT_FALSE(iOne >= fdTwo);
    EXPECT_TRUE(iOne >= invalid);

    EXPECT_TRUE(iOne <= fdTwo);
    EXPECT_TRUE(iTwo <= fdTwo);
    EXPECT_FALSE(iTwo <= fdOne);
    EXPECT_FALSE(iOne <= invalid);
}

TEST_F(FileDescriptorTest, output)
{
    FileDescriptor fd(sm, 17);
    EXPECT_EQ("17", boost::lexical_cast<std::string>(fd));
}
