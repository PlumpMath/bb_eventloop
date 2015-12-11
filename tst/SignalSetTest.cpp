#include <boost/lexical_cast.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/SignalSet.hpp>

using namespace testing;
using namespace EventLoop;

TEST(SignalSet, defaultConstructor)
{
    const SignalSet sset;
    for (int i = 1; i <= SignalSet::MAX_SIGNAL; ++i)
    {
        EXPECT_FALSE(sset.isSet(i));
    }
    EXPECT_TRUE(sset.empty());
}

TEST(SignalSet, signalConstructor)
{
    const SignalSet sset(SIGHUP);
    EXPECT_TRUE(sset.isSet(SIGHUP));
    EXPECT_TRUE(!sset.empty());
}

TEST(SignalSet, signalListConstructor)
{
    const SignalSet sset({ SIGHUP, SIGTERM });
    EXPECT_TRUE(sset.isSet(SIGHUP));
    EXPECT_TRUE(sset.isSet(SIGTERM));
    EXPECT_FALSE(sset.isSet(SIGINT));
}

TEST(SignalSet, set)
{
    SignalSet sset;
    sset.set(SIGINT);
    EXPECT_TRUE(sset.isSet(SIGINT));
}

TEST(SignalSet, unset)
{
    SignalSet sset;
    sset.set(SIGINT);
    sset.unset(SIGINT);
    EXPECT_FALSE(sset.isSet(SIGINT));
}

TEST(SignalSet, invalidSetThrows)
{
    SignalSet sset;
    EXPECT_THROW(sset.set(-1), SystemException);
}

TEST(SignalSet, invalidUnsetThrows)
{
    SignalSet sset;
    EXPECT_THROW(sset.unset(-1), SystemException);
}

TEST(SignalSet, maxSignal)
{
    EXPECT_LT(0, SignalSet::MAX_SIGNAL);
#ifdef SIGUNUSED
    EXPECT_LE(SIGUNUSED, SignalSet::MAX_SIGNAL);
#endif /* SIGUNUSED */
    EXPECT_LE(SIGSYS, SignalSet::MAX_SIGNAL);
    EXPECT_LE(SIGUSR1, SignalSet::MAX_SIGNAL);
    EXPECT_LE(SIGUSR2, SignalSet::MAX_SIGNAL);
    EXPECT_LE(SIGXCPU, SignalSet::MAX_SIGNAL);
    EXPECT_LE(SIGXFSZ, SignalSet::MAX_SIGNAL);
#ifdef SIGPWR
    EXPECT_LE(SIGPWR, SignalSet::MAX_SIGNAL);
#endif /* SIGPWR */
}

TEST(SignalSet, validSignalToString)
{
    EXPECT_EQ(std::string("SIGHUP"), signalToString(SIGHUP));
    EXPECT_EQ(std::string("SIGABRT"), signalToString(SIGABRT));
    EXPECT_EQ(std::string("unknown-signal"), signalToString(100));
}

TEST(SignalSet, outputEmptySet)
{
    const SignalSet sset;
    EXPECT_EQ(std::string(""), boost::lexical_cast<std::string>(sset));
}

TEST(SignalSet, outputFilledSet)
{
    SignalSet sset;
    sset.set(SIGHUP);
    sset.set(SIGABRT);
    EXPECT_EQ(std::string("SIGHUP,SIGABRT"), boost::lexical_cast<std::string>(sset));
}

TEST(SignalSet, behavesLikeStandardContainer)
{
    SignalSet sset;
    EXPECT_THAT(sset, IsEmpty());
    sset.set(SIGTERM);
    sset.set(SIGINT);
    EXPECT_THAT(sset, SizeIs(2));
    EXPECT_THAT(sset, UnorderedElementsAre(SIGTERM, SIGINT));
}

TEST(SignalSet, iteratorHandlesMinAndMaxSignalsCorrectly)
{
    const SignalSet sset({ 1, SignalSet::MAX_SIGNAL });
    EXPECT_THAT(sset, ElementsAre(1, SignalSet::MAX_SIGNAL));
}

TEST(SignalSet, equalOperator)
{
    EXPECT_TRUE(SignalSet({ SIGHUP, SIGTERM }) == SignalSet({ SIGHUP, SIGTERM }));
    EXPECT_FALSE(SignalSet({ SIGHUP, SIGTERM }) == SignalSet({ SIGHUP, SIGINT }));
}
