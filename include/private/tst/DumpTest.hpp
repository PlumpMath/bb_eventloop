#ifndef EVENTLOOP_DUMPTEST_HEADER
#define EVENTLOOP_DUMPTEST_HEADER

namespace EventLoop
{
    class Dumpable;

    namespace Test
    {
        void EXPECT_VALID_DUMP(const Dumpable &d);
    }
}

#endif /* !EVENTLOOP_DUMPTEST_HEADER */
