#ifndef EVENTLOOP_DUMPTESTHELPER_HEADER
#define EVENTLOOP_DUMPTESTHELPER_HEADER

namespace EventLoop
{
    namespace SomeNamespace
    {
        class DumpTestHelper: public EventLoop::Dumpable
        {
        public:
            void dump(std::ostream &out) const
            {
                DUMP_BEGIN(out);
                out << "blaa\n";
                DUMP_END(out);
            }
        };
    }
}

#endif /* !EVENTLOOP_DUMPTESTHELPER_HEADER */
