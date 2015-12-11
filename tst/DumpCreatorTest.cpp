#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/DumpCreator.hpp>
#include <EventLoop/Dumpable.hpp>

using namespace testing;
using namespace EventLoop;

namespace
{
    class DumpableMock: public Dumpable
    {
    public:
        void dump(std::ostream &os) const { os << "DumpMock\n"; }
    };
}

TEST(DumpCreator, dumpToStreamWritesTimestampAndDumpsAllGivenPointers)
{
    std::ostringstream os;
    DumpableMock dm1;
    DumpableMock dm2;
    Dumpables d { &dm1, &dm2, nullptr };
    dumpToStream(os, d);
    EXPECT_THAT(os.str(), MatchesRegex("Pid: [[:digit:]]+\n"
                                       "Time: [[:digit:]]{4}-[[:digit:]]{2}-[[:digit:]]{2}T[[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{2}\\.[[:digit:]]+\n"
                                       "BEGIN\n"
                                       "DumpMock\n"
                                       "DumpMock\n"
                                       "null\n"
                                       "END\n"));
}

TEST(DumpCreator, dumpToFileReturnsTrueOnSuccess)
{
    Dumpables d;
    EXPECT_TRUE(dumpToFile("/dev/null", d));
}

TEST(DumpCreator, dumpToFileReturnsFalseOnFailure)
{
    Dumpables d;
    EXPECT_FALSE(dumpToFile("/no/such/path/hopefully", d));
}
