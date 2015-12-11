#include <cstring>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/ControlMessage.hpp>

using namespace testing;
using namespace EventLoop;

TEST(ControlMessage, defaultConstructorCreatesEmptyContainer)
{
    const ControlMessage cm;
    EXPECT_EQ(0U, cm.size());
    EXPECT_THAT(cm, IsEmpty());
    EXPECT_EQ(nullptr, cm.data());
}

TEST(ControlMessage, appendIncreasesSize)
{
    ControlMessage cm;
    const char data1(3);
    cm.append(1, 2, data1);
    EXPECT_EQ(CMSG_SPACE(sizeof(data1)), cm.size());
    EXPECT_NE(nullptr, cm.data());
    const char data2[9] = { 4 };
    cm.append(5, 6, data2);
    EXPECT_EQ(CMSG_SPACE(sizeof(data1)) + CMSG_SPACE(sizeof(data2)), cm.size());
}

TEST(ControlMessage, mutableIterator)
{
    ControlMessage cm;
    cm.append(1, 2, 3);
    cm.append(4, 5, 6);
    ControlMessage::iterator i(cm.begin());
    ASSERT_NE(cm.end(), i);
    EXPECT_EQ(1, i->cmsg_level);
    EXPECT_EQ(2, i->cmsg_type);
    EXPECT_EQ(3, ControlMessage::getData<int>(*i));
    ++i;
    ASSERT_NE(cm.end(), i);
    EXPECT_EQ(4, i->cmsg_level);
    EXPECT_EQ(5, i->cmsg_type);
    EXPECT_EQ(6, ControlMessage::getData<int>(*i));
    ++i;
    EXPECT_EQ(cm.end(), i);
}

TEST(ControlMessage, constIterator)
{
    ControlMessage mcm;
    mcm.append(1, 2, 3);
    mcm.append(4, 5, 6);
    const ControlMessage &cm(mcm);
    ControlMessage::const_iterator i(cm.begin());
    ASSERT_NE(cm.end(), i);
    EXPECT_EQ(1, i->cmsg_level);
    EXPECT_EQ(2, i->cmsg_type);
    EXPECT_EQ(3, ControlMessage::getData<int>(*i));
    ++i;
    ASSERT_NE(cm.end(), i);
    EXPECT_EQ(4, i->cmsg_level);
    EXPECT_EQ(5, i->cmsg_type);
    EXPECT_EQ(6, ControlMessage::getData<int>(*i));
    ++i;
    EXPECT_EQ(cm.end(), i);
}

TEST(ControlMessage, constIteratorFromMutableControlMessage)
{
    ControlMessage cm;
    for (ControlMessage::const_iterator i = cm.begin(); i != cm.end(); ++i)
    {
        const struct cmsghdr &c(*i);
        static_cast<void>(c);
    }
}

TEST(ControlMessage, constructorTakingDataAndSizeMakesCopyOfTheGivenData)
{
    struct msghdr msg = { };
    struct cmsghdr *cmsg;
    int fd(123);
    char buf[CMSG_SPACE(sizeof fd)];

    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
    msg.msg_controllen = cmsg->cmsg_len;

    const ControlMessage cm(msg.msg_control, msg.msg_controllen);
    EXPECT_EQ(msg.msg_controllen, cm.size());
    EXPECT_EQ(0, memcmp(cm.data(), msg.msg_control, cm.size()));
}

TEST(ControlMessage, dataGivenWithAppendCreatesSimilarMessageThanDataGivenInConstructor)
{
    ControlMessage other;
    other.append(1, 2, 3);
    const ControlMessage cm(1, 2, 3);
    EXPECT_EQ(other.size(), cm.size());
    EXPECT_EQ(0, memcmp(cm.data(), other.data(), cm.size()));
}

TEST(ControlMessage, capacityAndMarkUsed)
{
    ControlMessage orig;
    orig.append(1, 2, 3);
    ControlMessage cm;
    cm.reserve(orig.size() + 100U);
    EXPECT_LE(orig.size() + 100U, cm.capacity());
    memcpy(cm.data(), orig.data(), orig.size());
    cm.markUsed(orig.size());
    EXPECT_EQ(orig.size(), cm.size());
    EXPECT_EQ(0, memcmp(cm.data(), orig.data(), cm.size()));
}

TEST(ControlMessage, markingTooMuchUsedThrows)
{
    ControlMessage cm;
    EXPECT_THROW(cm.markUsed(1U), ControlMessage::Overflow);
}

TEST(ControlMessage, getDataThrowsIfTheGivenCMSGIsTooSmallForContainingTheGivenType)
{
    const struct cmsghdr cmsg = { };
    EXPECT_THROW(ControlMessage::getData<int>(cmsg), ControlMessage::TruncatedCMSG);
}

TEST(ControlMessage, iteratorDereferenceThrowsIfTheSizeCantHoldTheCmsg)
{
    ControlMessage orig;
    orig.append(1, 2, 3);
    {
        const ControlMessage cm(orig.data(), 1U);
        EXPECT_THROW(*cm.begin(), ControlMessage::BadCMSG);
    }
    {
        ControlMessage cm(orig.data(), orig.size());
        cm.begin()->cmsg_len = sizeof(struct cmsghdr) - 1U;
        EXPECT_THROW(*cm.begin(), ControlMessage::BadCMSG);
    }
    {
        const ControlMessage cm(orig.data(), sizeof(struct cmsghdr));
        EXPECT_THROW(*cm.begin(), ControlMessage::BadCMSG);
    }
}
