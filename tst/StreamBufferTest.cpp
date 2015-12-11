#include <string>
#include <sstream>
#include <cstring>
#include <climits>
#include <gtest/gtest.h>

#include <EventLoop/StreamBuffer.hpp>

#include "private/tst/DumpTest.hpp"

using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    template <typename T>
    T fromVoidPtr(const void *ptr)
    {
        T ret;
        memcpy(&ret, ptr, sizeof(T));
        return ret;
    }

    std::string stringFromVoidPtr(const void *ptr, size_t size)
    {
        return std::string(static_cast<const char *>(ptr), size);
    }

    std::string iovecToString(const struct iovec &iov)
    {
        return stringFromVoidPtr(iov.iov_base, iov.iov_len);
    }

    std::string iovecsToString(const StreamBuffer::IOVecArray &iovs)
    {
        std::string ret;
        for (size_t i = 0; i < iovs.second; ++i)
        {
            ret += iovecToString(iovs.first[i]);
        }
        return ret;
    }

    void EXPECT_NO_ZERO_LENGTH_IOVECS(const StreamBuffer::IOVecArray &iovs)
    {
        for (size_t i = 0; i < iovs.second; ++i)
        {
            EXPECT_NE(nullptr, iovs.first[i].iov_base);
            EXPECT_LT(0U, iovs.first[i].iov_len);
        }
    }

    void EXPECT_PROPER_IOVEC_SIZES(const StreamBuffer::IOVecArray   &iovs,
                                   size_t                           expectedSize)
    {
        size_t totalSize(0U);
        for (size_t i = 0; i < iovs.second; ++i)
        {
            EXPECT_NE(nullptr, iovs.first[i].iov_base);
            EXPECT_LT(0U, iovs.first[i].iov_len);
            totalSize += iovs.first[i].iov_len;
        }
        EXPECT_EQ(expectedSize, totalSize);
    }

    void EXPECT_PROPER_IOVEC_SIZES(StreamBuffer &sb)
    {
        EXPECT_PROPER_IOVEC_SIZES(sb.getUsedIOVec(), sb.getUsedSize());
        EXPECT_PROPER_IOVEC_SIZES(sb.getFreeIOVec(), sb.getFreeSize());
    }
}

TEST(StreamBuffer, constructorCreatesEmptyBuffer)
{
    StreamBuffer sb;
    EXPECT_EQ(0U, sb.getFreeSize());
    EXPECT_EQ(0U, sb.getUsedSize());
    EXPECT_EQ(0U, sb.getTotalSize());
}

TEST(StreamBuffer, allocateAllocatesAtLeastRequestedSize)
{
    StreamBuffer sb;
    sb.allocate(100);
    EXPECT_LE(100U, sb.getFreeSize());
    EXPECT_EQ(0U, sb.getUsedSize());
    EXPECT_LE(100U, sb.getTotalSize());
}

TEST(StreamBuffer, allocateRoundsUp8Bytes)
{
    for (size_t i = 1; i < 8; ++i)
    {
        StreamBuffer sb;
        sb.allocate(i);
        EXPECT_LE(8U, sb.getFreeSize());
    }
}

TEST(StreamBuffer, allocatingAlreadyExistingSizeDoesntChangeAnything)
{
    static const size_t size(100);
    StreamBuffer sb;
    sb.allocate(size);
    size_t freeSize(sb.getFreeSize());
    size_t usedSize(sb.getUsedSize());
    size_t totalSize(sb.getTotalSize());

    for (size_t i = 0; i < size; ++i)
    {
        sb.allocate(i);
        EXPECT_EQ(freeSize, sb.getFreeSize());
        EXPECT_EQ(usedSize, sb.getUsedSize());
        EXPECT_EQ(totalSize, sb.getTotalSize());
    }
}

TEST(StreamBuffer, increaseUsed)
{
    StreamBuffer sb;
    sb.allocate(80);
    EXPECT_EQ(0U, sb.getUsedSize());
    EXPECT_EQ(80U, sb.getFreeSize());
    sb.increaseUsed(50);
    EXPECT_EQ(50U, sb.getUsedSize());
    EXPECT_EQ(30U, sb.getFreeSize());
}

TEST(StreamBuffer, markTooMuchUsedThrows)
{
    StreamBuffer sb;
    sb.allocate(100);
    EXPECT_THROW(sb.increaseUsed(sb.getTotalSize() + 1), StreamBuffer::Underflow);
}

TEST(StreamBuffer, getUsedIOVecWithEmptyStreamBufferReturnsEmptyArray)
{
    StreamBuffer sb;
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(0U, v.second);
}

TEST(StreamBuffer, getUsedIOVecUnusedStreamBufferReturnsEmptyArray)
{
    StreamBuffer sb;
    sb.allocate(80);
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(0U, v.second);
}

TEST(StreamBuffer, getUsedIOVecWithPartialBuffer)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.increaseUsed(40);
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_NO_ZERO_LENGTH_IOVECS(v);
    ASSERT_EQ(1U, v.second);
    EXPECT_NE(static_cast<void *>(0), v.first[0].iov_base);
    EXPECT_EQ(40U, v.first[0].iov_len);
}

TEST(StreamBuffer, getUsedIOVecWithMultipleBuffers)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.increaseUsed(80);
    sb.allocate(160);
    sb.increaseUsed(160);
    sb.allocate(80);
    sb.increaseUsed(40);
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_NO_ZERO_LENGTH_IOVECS(v);
    ASSERT_EQ(3U, v.second);
    EXPECT_NE(static_cast<void *>(0), v.first[0].iov_base);
    EXPECT_EQ(80U, v.first[0].iov_len);
    EXPECT_NE(static_cast<void *>(0), v.first[1].iov_base);
    EXPECT_EQ(160U, v.first[1].iov_len);
    EXPECT_NE(static_cast<void *>(0), v.first[2].iov_base);
    EXPECT_EQ(40U, v.first[2].iov_len);
}

TEST(StreamBuffer, getUsedIOVecWithTooManyBuffers)
{
    StreamBuffer sb;
    for (int i = 0; i < (IOV_MAX + 3); ++i)
    {
        sb.appendData("1234567890123456");
    }
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    ASSERT_EQ(static_cast<size_t>(IOV_MAX), v.second);
}

TEST(StreamBuffer, getFreeIOVecWithEmptyStreamBufferReturnsEmptyArray)
{
    StreamBuffer sb;
    StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    EXPECT_NO_ZERO_LENGTH_IOVECS(v);
    EXPECT_EQ(nullptr, v.first);
    EXPECT_EQ(0U, v.second);
}

TEST(StreamBuffer, getFreeIOVecCompletelyUsedStreamBufferReturnsEmptyArray)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.increaseUsed(80);
    StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    EXPECT_EQ(nullptr, v.first);
    EXPECT_EQ(0U, v.second);
}

TEST(StreamBuffer, getFreeIOVecWithPartialBuffer)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.increaseUsed(40);
    StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    EXPECT_NO_ZERO_LENGTH_IOVECS(v);
    ASSERT_EQ(1U, v.second);
    EXPECT_NE(static_cast<void *>(0), v.first[0].iov_base);
    EXPECT_EQ(40U, v.first[0].iov_len);
}

TEST(StreamBuffer, getFreeIOVecWithCompleteBuffer)
{
    StreamBuffer sb;
    sb.allocate(80);
    StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    EXPECT_NO_ZERO_LENGTH_IOVECS(v);
    ASSERT_EQ(1U, v.second);
    EXPECT_NE(static_cast<void *>(0), v.first[0].iov_base);
    EXPECT_EQ(80U, v.first[0].iov_len);
}

TEST(StreamBuffer, getFreeIOVecWithMultipleBuffers)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.allocate(240);
    sb.allocate(320);
    sb.increaseUsed(40);
    StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    EXPECT_NO_ZERO_LENGTH_IOVECS(v);
    ASSERT_EQ(3U, v.second);
    EXPECT_NE(static_cast<void *>(0), v.first[0].iov_base);
    EXPECT_EQ(40U, v.first[0].iov_len);
    EXPECT_NE(static_cast<void *>(0), v.first[1].iov_base);
    EXPECT_EQ(160U, v.first[1].iov_len);
    EXPECT_NE(static_cast<void *>(0), v.first[2].iov_base);
    EXPECT_EQ(80U, v.first[2].iov_len);
}

TEST(StreamBuffer, getFreeIOVecWithTooManyBuffers)
{
    StreamBuffer sb;
    for (int i = 0; i < (IOV_MAX + 3); ++i)
    {
        sb.allocate(i * 16 + 16);
    }
    StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    ASSERT_EQ(static_cast<size_t>(IOV_MAX), v.second);
}

TEST(StreamBuffer, appendDataIntoEmptyStreamBuffer)
{
    static const std::string DATA("Hello world!");
    StreamBuffer sb;
    sb.appendData(DATA.data(), DATA.size());
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(DATA.size(), sb.getUsedSize());
    EXPECT_LE(0U, sb.getFreeSize());
    EXPECT_LE(DATA.size(), sb.getTotalSize());

    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    ASSERT_EQ(1U, v.second);
    EXPECT_EQ(DATA, iovecToString(v.first[0]));
}

TEST(StreamBuffer, appendDataMultipleTimes)
{
    static const std::string DATA("12345678");
    StreamBuffer sb;
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(4U * DATA.size(), sb.getUsedSize());
    EXPECT_LE(0U, sb.getFreeSize());
    EXPECT_LE(4U * DATA.size(), sb.getTotalSize());

    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    ASSERT_EQ(4U, v.second);
    EXPECT_EQ(DATA, iovecToString(v.first[0]));
    EXPECT_EQ(DATA, iovecToString(v.first[1]));
    EXPECT_EQ(DATA, iovecToString(v.first[2]));
    EXPECT_EQ(DATA, iovecToString(v.first[3]));
}

TEST(StreamBuffer, appendDataOnTopOfPartialBuffer)
{
    static const std::string DATA("12345678");
    StreamBuffer sb;
    sb.allocate(10);
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(3U * DATA.size(), sb.getUsedSize());
    EXPECT_LE(0U, sb.getFreeSize());
    EXPECT_LE(3U * DATA.size(), sb.getTotalSize());

    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(std::string("123456781234567812345678"), iovecsToString(v));
}

TEST(StreamBuffer, appendDataWithTemplate)
{
    static const uint64_t DATA(123);
    StreamBuffer sb;
    sb.appendData(DATA);
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(DATA, sb.getData<uint64_t>());
}

TEST(StreamBuffer, appendCppString)
{
    StreamBuffer sb;
    sb.appendData(std::string("Hello world!"));
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(std::string("Hello world!"), iovecsToString(sb.getUsedIOVec()));
}

TEST(StreamBuffer, appendCString)
{
    StreamBuffer sb;
    sb.appendData("Hello world!");
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(std::string("Hello world!"), iovecsToString(sb.getUsedIOVec()));
}

TEST(StreamBuffer, appendEmptyStreamBuffer)
{
    StreamBuffer sb1;
    sb1.allocate(80);
    sb1.appendData("11111111");

    StreamBuffer sb2;
    sb1.appendStreamBuffer(sb2);

    EXPECT_EQ(72U, sb1.getFreeSize());
    EXPECT_EQ(8U, sb1.getUsedSize());
    EXPECT_EQ(80U, sb1.getTotalSize());

    EXPECT_EQ(std::string("11111111"), iovecsToString(sb1.getUsedIOVec()));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
}

TEST(StreamBuffer, appendStreamBufferAtTheBeginning)
{
    StreamBuffer sb1;

    StreamBuffer sb2;
    sb2.appendData("11111111");
    sb2.allocate(80);
    sb2.appendData("22222222");
    sb1.appendStreamBuffer(sb2);

    EXPECT_EQ(0U, sb1.getFreeSize());
    EXPECT_EQ(16U, sb1.getUsedSize());
    EXPECT_EQ(16U, sb1.getTotalSize());

    EXPECT_EQ(std::string("1111111122222222"), iovecsToString(sb1.getUsedIOVec()));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
}

TEST(StreamBuffer, appendStreamBufferInTheMiddle)
{
    StreamBuffer sb1;
    sb1.appendData("11111111");
    sb1.allocate(80);
    sb1.appendData("22222222");

    StreamBuffer sb2;
    sb2.appendData("33333333");
    sb2.allocate(80);
    sb2.appendData("44444444");
    sb1.appendStreamBuffer(sb2);

    EXPECT_EQ(72U, sb1.getFreeSize());
    EXPECT_EQ(32U, sb1.getUsedSize());
    EXPECT_EQ(104U, sb1.getTotalSize());

    EXPECT_EQ(std::string("11111111222222223333333344444444"), iovecsToString(sb1.getUsedIOVec()));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
}

TEST(StreamBuffer, appendStreamBufferAtTheEnd)
{
    StreamBuffer sb1;
    sb1.appendData("11111111");
    sb1.appendData("22222222");

    StreamBuffer sb2;
    sb2.appendData("33333333");
    sb2.allocate(80);
    sb2.appendData("44444444");
    sb1.appendStreamBuffer(sb2);

    EXPECT_EQ(0U, sb1.getFreeSize());
    EXPECT_EQ(32U, sb1.getUsedSize());
    EXPECT_EQ(32U, sb1.getTotalSize());

    EXPECT_EQ(std::string("11111111222222223333333344444444"), iovecsToString(sb1.getUsedIOVec()));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
}

TEST(StreamBuffer, getMutableArrayUpdatesUsedSize)
{
    StreamBuffer sb;
    sb.getMutableArray(10);
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(10U, sb.getUsedSize());
    EXPECT_LE(0U, sb.getFreeSize());
    EXPECT_LE(10U, sb.getTotalSize());
}

TEST(StreamBuffer, getMutableArrayReturnsValidPointer)
{
    StreamBuffer sb;
    void *ptr(sb.getMutableArray(10));
    EXPECT_PROPER_IOVEC_SIZES(sb);
    ASSERT_NE(nullptr, ptr);
    *static_cast<char *>(ptr) = 'a';
    EXPECT_EQ('a', sb.getData<char>());
}

TEST(StreamBuffer, getMutableArrayToAlreadyAllocatesbuffer)
{
    StreamBuffer sb;
    sb.allocate(256);
    void *ptr(sb.getMutableArray(10));
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(10U, sb.getUsedSize());
    EXPECT_LE(246U, sb.getFreeSize());
    EXPECT_LE(256U, sb.getTotalSize());
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', 10);
    EXPECT_EQ('a', sb.getData<char>());
}

TEST(StreamBuffer, getMutableArrayToAlreadyAllocatesbutTooSmallUnusesbuffer)
{
    StreamBuffer sb;
    sb.allocate(8);
    void *ptr(sb.getMutableArray(32));
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(32U, sb.getUsedSize());
    EXPECT_LE(0U, sb.getFreeSize());
    EXPECT_LE(32U, sb.getTotalSize());
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', 32);
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(std::string("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), iovecsToString(v));
}

TEST(StreamBuffer, getMutableArrayToAlreadyAllocatesbutTooSmallUsesbuffer)
{
    StreamBuffer sb;
    sb.allocate(8);
    sb.appendData<char>('b');
    void *ptr(sb.getMutableArray(32));
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(33U, sb.getUsedSize());
    EXPECT_LE(0U, sb.getFreeSize());
    EXPECT_LE(33U, sb.getTotalSize());
    ASSERT_NE(nullptr, ptr);
    memset(ptr, 'a', 32);
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(std::string("baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), iovecsToString(v));
}

TEST(StreamBuffer, linearizeEmptyStreamBuffer)
{
    StreamBuffer sb;
    sb.linearize();
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(0U, sb.getUsedSize());
    EXPECT_EQ(0U, sb.getFreeSize());
    EXPECT_EQ(0U, sb.getTotalSize());
}

TEST(StreamBuffer, linearizeUnusedStreamBuffer)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.linearize();
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(0U, sb.getUsedSize());
    EXPECT_EQ(80U, sb.getFreeSize());
    EXPECT_EQ(80U, sb.getTotalSize());
}

TEST(StreamBuffer, linearizePartialBuffer)
{
    StreamBuffer sb;
    sb.allocate(100);
    sb.increaseUsed(11);
    sb.cutHead(1);
    sb.linearize();
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(10U, sb.getUsedSize());
    EXPECT_LE(0U, sb.getFreeSize());
    EXPECT_LE(10U, sb.getTotalSize());
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    ASSERT_EQ(1U, v.second);
    EXPECT_EQ(10U, v.first[0].iov_len);
    EXPECT_EQ(0U, ((uintptr_t)v.first[0].iov_base) % 8);
}

TEST(StreamBuffer, linearizeMultipleBuffers)
{
    static const std::string DATA("Hello world again!");
    StreamBuffer sb;
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    sb.linearize();
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(3U * DATA.size(), sb.getUsedSize());
    EXPECT_LE(3U * DATA.size(), sb.getTotalSize());
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    ASSERT_EQ(1U, v.second);
    EXPECT_EQ(3U * DATA.size(), v.first[0].iov_len);
    EXPECT_EQ(0U, ((uintptr_t)v.first[0].iov_base) % 8);
    EXPECT_EQ(std::string("Hello world again!Hello world again!Hello world again!"), iovecToString(v.first[0]));
}

TEST(StreamBuffer, linearizingAlreadyLinearizedDoesntChangeAnything)
{
    static const std::string DATA("Hello world again!");
    StreamBuffer sb;
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    sb.appendData(DATA.data(), DATA.size());
    sb.linearize();
    EXPECT_PROPER_IOVEC_SIZES(sb);
    StreamBuffer::IOVecArray v1(sb.getUsedIOVec());
    ASSERT_EQ(1U, v1.second);
    void *ptr1(v1.first[0].iov_base);
    sb.linearize();
    StreamBuffer::IOVecArray v2(sb.getUsedIOVec());
    ASSERT_EQ(1U, v2.second);
    void *ptr2(v2.first[0].iov_base);
    EXPECT_EQ(ptr1, ptr2);
}

TEST(StreamBuffer, linearizingTakesRoundingIntoAccountWhenAddingNewBuffer)
{
    StreamBuffer sb;
    sb.allocate(100);
    sb.increaseUsed(11);
    sb.cutHead(1);
    sb.linearize();

    size_t freeSize(0U);
    const StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    for (size_t i = 0; i < v.second; ++i)
    {
        freeSize += v.first[i].iov_len;
    }
    EXPECT_EQ(freeSize, sb.getFreeSize());
}

TEST(StreamBuffer, spliceHeadWithZeroFromEmptyStreamBuffer)
{
    StreamBuffer sb1;
    StreamBuffer sb2(sb1.spliceHead(0));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
    EXPECT_PROPER_IOVEC_SIZES(sb2);

    EXPECT_EQ(0U, sb1.getFreeSize());
    EXPECT_EQ(0U, sb1.getUsedSize());
    EXPECT_EQ(0U, sb1.getTotalSize());
    StreamBuffer::IOVecArray v1(sb1.getUsedIOVec());
    EXPECT_EQ(0U, v1.second);

    EXPECT_EQ(0U, sb2.getFreeSize());
    EXPECT_EQ(0U, sb2.getUsedSize());
    EXPECT_EQ(0U, sb2.getTotalSize());
    StreamBuffer::IOVecArray v2(sb1.getUsedIOVec());
    EXPECT_EQ(0U, v2.second);
}

TEST(StreamBuffer, spliceHeadLessThanUsedUpdatesSizesCorrectly)
{
    StreamBuffer sb1;
    sb1.allocate(80);
    sb1.increaseUsed(15);
    EXPECT_EQ(65U, sb1.getFreeSize());
    EXPECT_EQ(15U, sb1.getUsedSize());
    EXPECT_EQ(80U, sb1.getTotalSize());

    StreamBuffer sb2(sb1.spliceHead(10));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
    EXPECT_PROPER_IOVEC_SIZES(sb2);

    EXPECT_EQ(65U, sb1.getFreeSize());
    EXPECT_EQ(5U, sb1.getUsedSize());
    EXPECT_EQ(70U, sb1.getTotalSize());

    EXPECT_EQ(0U, sb2.getFreeSize());
    EXPECT_EQ(10U, sb2.getUsedSize());
    EXPECT_EQ(10U, sb2.getTotalSize());
}

TEST(StreamBuffer, spliceHeadMoreThanUsedUpdatesSizesCorrectly)
{
    StreamBuffer sb1;
    sb1.allocate(80);
    sb1.increaseUsed(15);
    EXPECT_EQ(65U, sb1.getFreeSize());
    EXPECT_EQ(15U, sb1.getUsedSize());
    EXPECT_EQ(80U, sb1.getTotalSize());

    StreamBuffer sb2(sb1.spliceHead(20));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
    EXPECT_PROPER_IOVEC_SIZES(sb2);

    EXPECT_EQ(60U, sb1.getFreeSize());
    EXPECT_EQ(0U, sb1.getUsedSize());
    EXPECT_EQ(60U, sb1.getTotalSize());

    EXPECT_EQ(5U, sb2.getFreeSize());
    EXPECT_EQ(15U, sb2.getUsedSize());
    EXPECT_EQ(20U, sb2.getTotalSize());
}

TEST(StreamBuffer, spliceHeadSplicesDataCorrectlyWithPartialBuffer)
{
    static const std::string DATA("Splice me!");
    StreamBuffer sb1;
    sb1.appendData(DATA.data(), DATA.size());

    StreamBuffer sb2(sb1.spliceHead(3));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
    EXPECT_PROPER_IOVEC_SIZES(sb2);

    StreamBuffer::IOVecArray v1(sb1.getUsedIOVec());
    EXPECT_EQ(std::string("ice me!"), iovecsToString(v1));

    StreamBuffer::IOVecArray v2(sb2.getUsedIOVec());
    EXPECT_EQ(std::string("Spl"), iovecsToString(v2));
}

TEST(StreamBuffer, spliceHeadSplicesDataCorrectlyWithCompleteBuffer)
{
    static const std::string DATA1("Complete block!");
    static const std::string DATA2("Splice me!");
    StreamBuffer sb1;
    sb1.appendData(DATA1.data(), DATA1.size());
    sb1.appendData(DATA2.data(), DATA2.size());

    StreamBuffer sb2(sb1.spliceHead(20));
    EXPECT_PROPER_IOVEC_SIZES(sb1);
    EXPECT_PROPER_IOVEC_SIZES(sb2);

    StreamBuffer::IOVecArray v1(sb1.getUsedIOVec());
    EXPECT_EQ(std::string("e me!"), iovecsToString(v1));
    StreamBuffer::IOVecArray v2(sb2.getUsedIOVec());
    EXPECT_EQ(std::string("Complete block!Splic"), iovecsToString(v2));
}

TEST(StreamBuffer, spliceHeadMoreThanTotalSizeThrows)
{
    StreamBuffer sb;
    sb.allocate(8);
    EXPECT_THROW(sb.spliceHead(9), StreamBuffer::Underflow);
}

TEST(StreamBuffer, cutHeadWithZeroFromEmptyStreamBuffer)
{
    StreamBuffer sb;
    sb.cutHead(0);
    EXPECT_PROPER_IOVEC_SIZES(sb);
    EXPECT_EQ(0U, sb.getFreeSize());
    EXPECT_EQ(0U, sb.getUsedSize());
    EXPECT_EQ(0U, sb.getTotalSize());
    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(0U, v.second);
}

TEST(StreamBuffer, cutHeadLessThanUsedUpdatesSizesCorrectly)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.increaseUsed(15);
    EXPECT_EQ(65U, sb.getFreeSize());
    EXPECT_EQ(15U, sb.getUsedSize());
    EXPECT_EQ(80U, sb.getTotalSize());

    sb.cutHead(10);
    EXPECT_PROPER_IOVEC_SIZES(sb);

    EXPECT_EQ(65U, sb.getFreeSize());
    EXPECT_EQ(5U, sb.getUsedSize());
    EXPECT_EQ(70U, sb.getTotalSize());
}

TEST(StreamBuffer, cutHeadMoreThanUsedUpdatesSizesCorrectly)
{
    StreamBuffer sb;
    sb.allocate(80);
    sb.increaseUsed(15);
    EXPECT_EQ(65U, sb.getFreeSize());
    EXPECT_EQ(15U, sb.getUsedSize());
    EXPECT_EQ(80U, sb.getTotalSize());

    sb.cutHead(20);
    EXPECT_PROPER_IOVEC_SIZES(sb);

    EXPECT_EQ(60U, sb.getFreeSize());
    EXPECT_EQ(0U, sb.getUsedSize());
    EXPECT_EQ(60U, sb.getTotalSize());
}

TEST(StreamBuffer, cutHeadCutsDataCorrectlyWithPartialBuffer)
{
    static const std::string DATA("Do cut me!");
    StreamBuffer sb;
    sb.appendData(DATA.data(), DATA.size());

    sb.cutHead(3);
    EXPECT_PROPER_IOVEC_SIZES(sb);

    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(std::string("cut me!"), iovecsToString(v));
}

TEST(StreamBuffer, cutHeadCutsDataCorrectlyWithCompleteBuffer)
{
    static const std::string DATA1("Complete block!");
    static const std::string DATA2("Do cut me!");
    StreamBuffer sb;
    sb.appendData(DATA1.data(), DATA1.size());
    sb.appendData(DATA2.data(), DATA2.size());

    sb.cutHead(20);
    EXPECT_PROPER_IOVEC_SIZES(sb);

    StreamBuffer::IOVecArray v(sb.getUsedIOVec());
    EXPECT_EQ(std::string("t me!"), iovecsToString(v));
}

TEST(StreamBuffer, cutHeadMoreThanTotalSizeThrows)
{
    StreamBuffer sb;
    sb.allocate(8);
    EXPECT_THROW(sb.cutHead(9), StreamBuffer::Underflow);
}

TEST(StreamBuffer, getDataInHappyCaseDoesntLinearize)
{
    StreamBuffer sb;
    sb.allocate(100);
    static const uint64_t DATA(123);
    const char *dataPtr(reinterpret_cast<const char *>(&DATA));

    sb.appendData(dataPtr, 1);
    const void *origPtr(&sb.getData<char>());
    sb.appendData(dataPtr + 1, sizeof(DATA) - 1);
    const void *newPtr(&sb.getData<uint64_t>());

    EXPECT_EQ(DATA, sb.getData<uint64_t>());
    EXPECT_EQ(origPtr, newPtr);
}

TEST(StreamBuffer, getDataLinearizesIfBufferIsNotAligned)
{
    StreamBuffer sb;
    sb.allocate(100);

    static const char TMP(0);
    static const uint64_t DATA(123);

    sb.appendData(&TMP, sizeof(TMP));
    sb.appendData(&DATA, sizeof(DATA));
    sb.cutHead(sizeof(TMP));

    const void *ptr(&sb.getData<uint64_t>());
    EXPECT_EQ(DATA, sb.getData<uint64_t>());
    EXPECT_EQ(0U, ((uintptr_t)ptr) % 8);
}

namespace
{
    struct BigData
    {
        uint64_t i0;
        uint64_t i1;
        uint64_t i2;
        uint64_t i3;

        BigData(): i0(0), i1(0), i2(0), i3(0) { }

        BigData(uint64_t i0,
                uint64_t i1,
                uint64_t i2,
                uint64_t i3):
            i0(i0), i1(i1), i2(i2), i3(i3) { }

        bool operator == (const BigData &bd) const { return ((i0 == bd.i0) &&
                                                             (i1 == bd.i1) &&
                                                             (i2 == bd.i2) &&
                                                             (i3 == bd.i3)); }
    };
}

TEST(StreamBuffer, getDataLinearizesIfDataIsntInOneBuffer)
{
    static const BigData bigData(1, 2, 3, 4);

    StreamBuffer sb;
    sb.appendData(&bigData.i0, sizeof(bigData.i0));
    sb.appendData(&bigData.i1, sizeof(bigData.i1));
    sb.appendData(&bigData.i2, sizeof(bigData.i2));
    sb.appendData(&bigData.i3, sizeof(bigData.i3));
    EXPECT_EQ(4U, sb.getUsedIOVec().second);
    EXPECT_EQ(bigData, sb.getData<BigData>());
}

TEST(StreamBuffer, getDataThrowsIfNotEnoughUsedData)
{
    StreamBuffer sb;
    sb.allocate(100);
    EXPECT_THROW(sb.getData<char>(), StreamBuffer::Overflow);
}

TEST(StreamBuffer, getConstArrayThrowsIfNotEnoughUsedData)
{
    StreamBuffer sb;
    sb.allocate(100);
    sb.increaseUsed(4);
    EXPECT_THROW(sb.getConstArray(3, 3), StreamBuffer::Overflow);
}

TEST(StreamBuffer, getConstArrayInOneBuffer)
{
    StreamBuffer sb;
    sb.allocate(100);
    sb.appendData<uint64_t>(1);
    sb.appendData<uint64_t>(2);

    uint64_t data1(fromVoidPtr<uint64_t>(sb.getConstArray(0, sizeof(uint64_t))));
    EXPECT_EQ(1U, data1);
    uint64_t data2(fromVoidPtr<uint64_t>(sb.getConstArray(sizeof(uint64_t), sizeof(uint64_t))));
    EXPECT_EQ(2U, data2);
}

TEST(StreamBuffer, getConstArrayInMultipleBuffers)
{
    StreamBuffer sb;
    sb.allocate(24);
    sb.appendData<uint64_t>(1);
    sb.appendData<uint64_t>(2);
    sb.appendData<uint64_t>(3);
    EXPECT_EQ(1U, sb.getUsedIOVec().second);
    sb.allocate(16);
    sb.appendData<uint64_t>(4);
    sb.appendData<uint64_t>(5);
    EXPECT_EQ(2U, sb.getUsedIOVec().second);

    BigData data(fromVoidPtr<BigData>(sb.getConstArray(sizeof(uint64_t), sizeof(BigData))));
    EXPECT_EQ(BigData(2, 3, 4, 5), data);
}

TEST(StreamBuffer, copyOfStreamBufferSharesTheSameRawData)
{
    static const std::string DATA("blaablaablaa");
    StreamBuffer sb1;
    sb1.appendData(DATA.data(), DATA.size());
    StreamBuffer sb2(sb1);

    const char *ptr1(&sb1.getData<char>());
    const char *ptr2(&sb2.getData<char>());

    EXPECT_EQ(ptr1, ptr2);
}

TEST(StreamBuffer, afterAssignmentSharesTheSameRawData)
{
    static const std::string DATA("blaablaablaa");
    StreamBuffer sb1;
    sb1.appendData(DATA.data(), DATA.size());
    StreamBuffer sb2;
    sb2 = sb1;

    const char *ptr1(&sb1.getData<char>());
    const char *ptr2(&sb2.getData<char>());

    EXPECT_EQ(ptr1, ptr2);
}

TEST(StreamBuffer, getMutableArrayTakesRoundingIntoAccountWhenAddingNewBuffer)
{
    StreamBuffer sb;
    sb.allocate(16U);
    sb.increaseUsed(15U);
    sb.getMutableArray(2U);

    size_t freeSize(0U);
    const StreamBuffer::IOVecArray v(sb.getFreeIOVec());
    for (size_t i = 0; i < v.second; ++i)
    {
        freeSize += v.first[i].iov_len;
    }
    EXPECT_EQ(freeSize, sb.getFreeSize());
}

TEST(StreamBuffer, dump)
{
    StreamBuffer sb;
    sb.allocate(10);
    sb.increaseUsed(10);
    sb.allocate(100);
    EXPECT_VALID_DUMP(sb);
}

TEST(StreamBuffer, dumpWhenThereAreExtraIOVecs)
{
    StreamBuffer sb;
    sb.allocate(10);
    sb.increaseUsed(10);
    sb.allocate(10);
    sb.getUsedIOVec();
    EXPECT_VALID_DUMP(sb);
}
