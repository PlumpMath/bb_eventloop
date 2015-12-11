#include <sstream>
#include <iomanip>
#include <gtest/gtest.h>

#include "private/DumpFormatter.hpp"

using namespace testing;
using namespace EventLoop;

TEST(DumpFormatter, simpleOutputIsNotFormatted)
{
    std::ostringstream target;
    DumpFormatter d(target);
    d << "testing" << std::flush;
    EXPECT_EQ("testing", target.str());
}

TEST(DumpFormatter, indentationIncreasesWithOpenBraceAndDecreasesWithCloseBrace)
{
    std::ostringstream target;
    DumpFormatter d(target);
    d << "{\nindent me\n{\nindent me more\n}\n}\nbut not me\n" << std::flush;
    EXPECT_EQ("{\n    indent me\n    {\n        indent me more\n    }\n}\nbut not me\n", target.str());
}

TEST(DumpFormatter, emptyLinesAreNotIndented)
{
    std::ostringstream target;
    DumpFormatter d(target);
    d << "{\n\n}\n" << std::flush;
    EXPECT_EQ("{\n\n}\n", target.str());
}

TEST(DumpFormatter, formatterUsesFlagsFromTheOriginalTarget)
{
    std::ostringstream target;
    target << std::boolalpha;
    DumpFormatter d(target);
    d << true << std::flush;
    EXPECT_EQ("true", target.str());
}
