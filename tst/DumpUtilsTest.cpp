#include <functional>
#include <string>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <list>
#include <memory>
#include <tuple>
#include <arpa/inet.h>
#include <boost/optional.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EventLoop/Dumpable.hpp>
#include <EventLoop/DumpUtils.hpp>
#include <EventLoop/Exception.hpp>

#include "private/tst/DumpTestHelper.hpp"

using namespace testing;
using namespace EventLoop;

namespace
{
    class D: public EventLoop::Dumpable
    {
    public:
        D(): foo(123) { }
        int foo;
        void dump(std::ostream &os) const { os << "D\n{\nthis: 1\nfoo: " << foo << "\n}\n"; }
    };

    class O
    {
    public:
        O(): x(222) { }
        int x;
    };

    std::ostream &operator << (std::ostream &os, const O &o)
    {
        os << o.x;
        return os;
    }

    class DAO: public EventLoop::Dumpable
    {
    public:
        DAO(): foo(123) { }
        int foo;
        void dump(std::ostream &os) const { os << "DAO\n{\nthis: 1\nfoo: " << foo << "\n}\n"; }
    };

    std::ostream &operator << (std::ostream &os, const DAO &dao)
    {
        os << dao.foo;
        return os;
    }

    class C
    {
    public:
        C(): x(htonl(0x0102aabb)) { }
        int x;
    };

    enum class E
    {
        TWO = 2,
    };

    enum class EO
    {
        THREE = 3,
    };

    std::ostream &operator << (std::ostream &os, const EO &)
    {
        os << "three";
        return os;
    }

    class DumpUtilsTest: public testing::Test
    {
    public:
        std::ostringstream os;
        int i;
        O o;
        D d;
        DAO dao;
        C c;
        const O *optr;
        const D *dptr;
        std::function<void(void)> func;
        std::shared_ptr<int> sptr;
        std::weak_ptr<int> wptr;
        std::unique_ptr<int> uptr;
        std::list<int> iList;
        std::list<D> dList;
        std::list<C> cList;
        std::list<std::shared_ptr<int>> ptrList;
        std::map<int, int> iMap;
        std::map<int, D> dMap;
        std::map<int, std::shared_ptr<int>> ptrMap;
        boost::optional<int> bo;
        boost::optional<D> boD;
        E e;
        EO eo;
        std::string cppstr;
        const char *cstr;
        const char ch;

        DumpUtilsTest(): i(0), optr(&o), dptr(nullptr), e(E::TWO), eo(EO::THREE), cppstr("foo"), cstr("bar"), ch(0) { }
    };

    int noThrow()
    {
        return 42;
    }

    int doThrow()
    {
        throw Exception("test exception");
    }
}

TEST_F(DumpUtilsTest, dumpValueOfFundamental)
{
    const std::string EXPECTED_OUTPUT("i: 111\n");
    i = 111;
    DUMP_VALUE_OF(os, i);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfOutputable)
{
    const std::string EXPECTED_OUTPUT("o: 222\n");
    DUMP_VALUE_OF(os, o);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfNonOutputable)
{
    const std::string EXPECTED_OUTPUT("c: 0102aabb\n");
    DUMP_VALUE_OF(os, c);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfCppString)
{
    const std::string EXPECTED_OUTPUT("cppstr: \"foo\"\n");
    DUMP_VALUE_OF(os, cppstr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfCString)
{
    const std::string EXPECTED_OUTPUT("cstr: \"bar\"\n");
    DUMP_VALUE_OF(os, cstr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpAddressOfChar)
{
    const std::string EXPECTED_OUTPUT("ch: \"bar\"\n");
    DUMP_ADDRESS_OF(os, ch);
    EXPECT_THAT(os.str(), MatchesRegex("ch: 0x[[:xdigit:]]+\n"));
}

TEST_F(DumpUtilsTest, dumpHexValueOfFundamental)
{
    const std::string EXPECTED_OUTPUT("i: 0x6f\n");
    i = 0x6f;
    DUMP_HEX_VALUE_OF(os, i);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfDumpable)
{
    const std::string EXPECTED_OUTPUT(
        "d:\n"
        "{\n"
        "D\n"
        "{\n"
        "this: 1\n"
        "foo: 123\n"
        "}\n"
        "}\n");
    DUMP_VALUE_OF(os, d);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfDumpableAndOutputable)
{
    const std::string EXPECTED_OUTPUT(
        "dao:\n"
        "{\n"
        "DAO\n"
        "{\n"
        "this: 1\n"
        "foo: 123\n"
        "}\n"
        "}\n");
    DUMP_VALUE_OF(os, dao);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());

    /* Make sure 'dao' is outputable and that the output function is used */
    std::ostringstream tmp;
    tmp << dao;
}

TEST_F(DumpUtilsTest, dumpAddressOf)
{
    std::ostringstream EXPECTED_OUTPUT;
    EXPECTED_OUTPUT << "i: " << &i << '\n';
    DUMP_ADDRESS_OF(os, i);
    EXPECT_EQ(EXPECTED_OUTPUT.str(), os.str());
}

TEST_F(DumpUtilsTest, dumpValuePointedByPtr)
{
    const std::string EXPECTED_OUTPUT("optr: 222\n");
    DUMP_VALUE_POINTED_BY(os, optr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValuePointedBySharedPtr)
{
    const std::string EXPECTED_OUTPUT("sptr: 10\n");
    sptr = std::make_shared<int>(10);
    DUMP_VALUE_POINTED_BY(os, sptr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValuePointedByWeakPtr)
{
    const std::string EXPECTED_OUTPUT("wptr: 10\n");
    sptr = std::make_shared<int>(10);
    wptr = sptr;
    DUMP_VALUE_POINTED_BY(os, wptr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValuePointedByDanlingWeakPtr)
{
    const std::string EXPECTED_OUTPUT("wptr: null\n");
    DUMP_VALUE_POINTED_BY(os, wptr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValuePointedByUniquePtr)
{
    const std::string EXPECTED_OUTPUT("uptr: 10\n");
    uptr.reset(new int(10));
    DUMP_VALUE_POINTED_BY(os, uptr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValuePointedByNullPtr)
{
    const std::string EXPECTED_OUTPUT("dptr: null\n");
    DUMP_VALUE_POINTED_BY(os, dptr);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpStdFunction)
{
    /** @todo What we can and want to know about std::function? */
    const std::string EXPECTED_OUTPUT("func: 1\n");
    func = [] () { return; };
    DUMP_VALUE_OF(os, func);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfSharedPtr)
{
    std::ostringstream EXPECTED_OUTPUT;
    sptr = std::make_shared<int>(10);
    EXPECTED_OUTPUT << "sptr: " << &(*sptr) << " (use count: 1)\n";
    DUMP_VALUE_OF(os, sptr);
    EXPECT_EQ(EXPECTED_OUTPUT.str(), os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfWeakPtr)
{
    std::ostringstream EXPECTED_OUTPUT;
    sptr = std::make_shared<int>(10);
    wptr = sptr;
    EXPECTED_OUTPUT << "wptr: " << &(*sptr) << " (use count: 2)\n";
    DUMP_VALUE_OF(os, wptr);
    EXPECT_EQ(EXPECTED_OUTPUT.str(), os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfDanglingWeakPtr)
{
    std::ostringstream EXPECTED_OUTPUT;
    EXPECTED_OUTPUT << "wptr: 0 (use count: 0)\n";
    DUMP_VALUE_OF(os, wptr);
    EXPECT_EQ(EXPECTED_OUTPUT.str(), os.str());
}

TEST_F(DumpUtilsTest, dumpValueOfUniquePtr)
{
    std::ostringstream EXPECTED_OUTPUT;
    uptr.reset(new int(10));
    EXPECTED_OUTPUT << "uptr: " << &(*uptr) << '\n';
    DUMP_VALUE_OF(os, uptr);
    EXPECT_EQ(EXPECTED_OUTPUT.str(), os.str());
}

TEST_F(DumpUtilsTest, dumpContainerOfFundamentals)
{
    const std::string EXPECTED_OUTPUT(
        "iList:\n"
        "{\n"
        "100\n"
        "200\n"
        "}\n");
    iList.push_back(100);
    iList.push_back(200);
    DUMP_CONTAINER(os, iList);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpContainerOfDumpables)
{
    const std::string EXPECTED_OUTPUT(
        "dList:\n"
        "{\n"
        "D\n"
        "{\n"
        "this: 1\n"
        "foo: 123\n"
        "}\n"
        "D\n"
        "{\n"
        "this: 1\n"
        "foo: 123\n"
        "}\n"
        "}\n");
    dList.push_back(D());
    dList.push_back(D());
    DUMP_CONTAINER(os, dList);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpContainerOfNonOutputable)
{
    const std::string EXPECTED_OUTPUT(
        "cList:\n"
        "{\n"
        "0102aabb\n"
        "0102aabb\n"
        "}\n");
    cList.push_back(C());
    cList.push_back(C());
    DUMP_CONTAINER(os, cList);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpContainerOfPtrs)
{
    const std::string EXPECTED_OUTPUT(
        "ptrList:\n"
        "{\n"
        "1\n"
        "2\n"
        "null\n"
        "}\n");
    ptrList.push_back(std::make_shared<int>(1));
    ptrList.push_back(std::make_shared<int>(2));
    ptrList.push_back(nullptr);
    DUMP_PTR_CONTAINER(os, ptrList);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
};

TEST_F(DumpUtilsTest, dumpPairContainer)
{
    const std::string EXPECTED_OUTPUT(
        "iMap:\n"
        "{\n"
        "1: 100\n"
        "2: 200\n"
        "}\n");
    iMap.insert(std::make_pair(1, 100));
    iMap.insert(std::make_pair(2, 200));
    DUMP_PAIR_CONTAINER(os, iMap);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpPairContainerOfDumpable)
{
    const std::string EXPECTED_OUTPUT(
        "dMap:\n"
        "{\n"
        "1:\n"
        "{\n"
        "D\n"
        "{\n"
        "this: 1\n"
        "foo: 123\n"
        "}\n"
        "}\n"
        "2:\n"
        "{\n"
        "D\n"
        "{\n"
        "this: 1\n"
        "foo: 123\n"
        "}\n"
        "}\n"
        "}\n");
    dMap.insert(std::make_pair(1, D()));
    dMap.insert(std::make_pair(2, D()));
    DUMP_PAIR_CONTAINER(os, dMap);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpPtrPairContainer)
{
    const std::string EXPECTED_OUTPUT(
        "ptrMap:\n"
        "{\n"
        "1: 1\n"
        "2: 2\n"
        "3: null\n"
        "}\n");
    ptrMap.insert(std::make_pair(1, std::make_shared<int>(1)));
    ptrMap.insert(std::make_pair(2, std::make_shared<int>(2)));
    ptrMap.insert(std::make_pair(3, nullptr));
    DUMP_PTR_PAIR_CONTAINER(os, ptrMap);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpSetBoostOptional)
{
    const std::string EXPECTED_OUTPUT("bo: 333\n");
    bo = 333;
    DUMP_VALUE_OF(os, bo);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpUnsetBoostOptional)
{
    const std::string EXPECTED_OUTPUT("bo: <none>\n");
    DUMP_VALUE_OF(os, bo);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpSetBoostOptionalDumpable)
{
    const std::string EXPECTED_OUTPUT(
        "boD:\n"
        "{\n"
        "D\n"
        "{\n"
        "this: 1\n"
        "foo: 123\n"
        "}\n"
        "}\n");
    boD = D();
    DUMP_VALUE_OF(os, boD);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpTuple)
{
    const std::string EXPECTED_OUTPUT(
        "t1: (1)\n"
        "t2: (1, 2)\n"
        "t3: (\"foo\", \"bar\", 3)\n"
        "t4: (1, 2, 3, 4)\n"
        "t5: (1, 2, 3, 4, 5)\n");
    const std::tuple<int> t1(std::make_tuple(1));
    const std::tuple<int, int> t2(std::make_tuple(1, 2));
    const std::tuple<std::string, std::string, int> t3(std::make_tuple("foo", "bar", 3));
    const std::tuple<int, int, int, int> t4(std::make_tuple(1, 2, 3, 4));
    const std::tuple<int, int, int, int, int> t5(std::make_tuple(1, 2, 3, 4, 5));
    DUMP_VALUE_OF(os, t1);
    DUMP_VALUE_OF(os, t2);
    DUMP_VALUE_OF(os, t3);
    DUMP_VALUE_OF(os, t4);
    DUMP_VALUE_OF(os, t5);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpEnumWithoutOutputOperator)
{
    const std::string EXPECTED_OUTPUT("e: 2\n");
    DUMP_VALUE_OF(os, e);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpEnumWithOutputOperator)
{
    const std::string EXPECTED_OUTPUT("eo: three (3)\n");
    DUMP_VALUE_OF(os, eo);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, dumpMaintainsStreamFlags)
{
    const std::string EXPECTED_OUTPUT(
        "b: false\n"
        "v:\n"
        "{\n"
        "false\n"
        "}"
        "\n");
    const bool b(false);
    std::vector<bool> v;
    v.push_back(false);
    os << std::boolalpha;
    DUMP_VALUE_OF(os, b);
    DUMP_CONTAINER(os, v);
    EXPECT_EQ(EXPECTED_OUTPUT, os.str());
}

TEST_F(DumpUtilsTest, valuePointedBySharedPtrIncludesUseCount)
{
    std::shared_ptr<SomeNamespace::DumpTestHelper> p(std::make_shared<SomeNamespace::DumpTestHelper>());
    DUMP_VALUE_POINTED_BY(os, p);
    EXPECT_THAT(os.str(), MatchesRegex("p:\n"
                                       "\\{\n"
                                       "EventLoop::SomeNamespace::DumpTestHelper\n"
                                       "\\{\n"
                                       "this: 0x[[:xdigit:]]+\n"
                                       "blaa\n"
                                       "\\}\n"
                                       "use count: 1\n"
                                       "\\}\n"));
}

TEST_F(DumpUtilsTest, dumpResultOfWithOutException)
{
    DUMP_RESULT_OF(os, noThrow());
    EXPECT_EQ(os.str(), "noThrow(): 42\n");
}

TEST_F(DumpUtilsTest, dumpResultOfWithException)
{
    DUMP_RESULT_OF(os, doThrow());
    EXPECT_EQ(os.str(), "doThrow(): EventLoop::Exception: test exception\n");
}

TEST_F(DumpUtilsTest, outputEmptyString)
{
    DumpUtils::Detail::outputString(os, "");
    EXPECT_EQ("\"\"", os.str());
}

TEST_F(DumpUtilsTest, outputPlainText)
{
    DumpUtils::Detail::outputString(os, "testing 123");
    EXPECT_EQ("\"testing 123\"", os.str());
}

TEST_F(DumpUtilsTest, outputNilCharacter)
{
    const std::string s("a\0b", 3);
    DumpUtils::Detail::outputString(os, s);
    EXPECT_EQ("\"a\\0b\"", os.str());
}

TEST_F(DumpUtilsTest, outputSpecialCharacters)
{
    DumpUtils::Detail::outputString(os, " \f \n \r \t \v \' \" \\ ");
    EXPECT_EQ("\" \\f \\n \\r \\t \\v \\\' \\\" \\\\ \"", os.str());
}

TEST_F(DumpUtilsTest, outputNonPrintableCharacter)
{
    DumpUtils::Detail::outputString(os, "\xe \xff");
    EXPECT_EQ("\"\\xE \\xFF\"", os.str());
}

TEST_F(DumpUtilsTest, outputCppString)
{
    DumpUtils::Detail::outputString(os, std::string("hello world\n"));
    EXPECT_EQ("\"hello world\\n\"", os.str());
}

TEST_F(DumpUtilsTest, outputCString)
{
    DumpUtils::Detail::outputString(os, "hello world\n");
    EXPECT_EQ("\"hello world\\n\"", os.str());
}

TEST_F(DumpUtilsTest, outputNullCString)
{
    DumpUtils::Detail::outputString(os, nullptr);
    EXPECT_EQ("null", os.str());
}
