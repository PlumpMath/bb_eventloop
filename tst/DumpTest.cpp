#include "private/tst/DumpTest.hpp"

#include <string>
#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/Dumpable.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/tst/DumpTestHelper.hpp"

using namespace testing;
using namespace EventLoop;

namespace
{
    void checkNotEmpty(const std::string &dump)
    {
        EXPECT_NE(std::string(), dump);
    }

    void checkEvenBraces(const std::string &dump)
    {
        int open(0);
        for (std::string::const_iterator i = dump.begin();
             i != dump.end();
             ++i)
        {
            if (*i == '{')
            {
                ++open;
            }
            else if (*i == '}')
            {
                EXPECT_LT(0, open);
                --open;
            }
        }
        EXPECT_EQ(0, open);
    }

    void checkNewLineAtTheEnd(const std::string &dump)
    {
        EXPECT_EQ(dump.size() - 1, dump.rfind('\n'));
    }
}

void EventLoop::Test::EXPECT_VALID_DUMP(const Dumpable &d)
{
    std::ostringstream os;
    d.dump(os);
    checkNotEmpty(os.str());
    checkEvenBraces(os.str());
    checkNewLineAtTheEnd(os.str());
}

TEST(Dump, dumpBeginAndEnd)
{
    std::ostringstream out;

    SomeNamespace::DumpTestHelper dt;
    dt.dump(out);

    EXPECT_THAT(out.str(), MatchesRegex("EventLoop::SomeNamespace::DumpTestHelper\n"
                                        "\\{\n"
                                        "this: 0x[[:xdigit:]]+\n"
                                        "blaa\n"
                                        "\\}\n"));
}

TEST(Dump, dumpBlockBeginAndEnd)
{
    std::ostringstream out;

    DUMP_BLOCK_BEGIN(out, "TEST");
    out << "blaa\n";
    DUMP_BLOCK_END(out);

    EXPECT_THAT(out.str(), MatchesRegex("TEST:\n"
                                        "\\{\n"
                                        "blaa\n"
                                        "\\}\n"));
}
