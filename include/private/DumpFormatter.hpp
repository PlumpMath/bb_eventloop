#ifndef EVENTLOOP_DUMPFORMATTER_HEADER
#define EVENTLOOP_DUMPFORMATTER_HEADER

#include <ostream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/concepts.hpp>

namespace EventLoop
{
    namespace Detail
    {
        class DumpFormatterSink: public boost::iostreams::sink
        {
        public:
            DumpFormatterSink() = delete;

            explicit DumpFormatterSink(std::ostream &target);

            ~DumpFormatterSink();

            std::streamsize write(const char *s, std::streamsize n);

        private:
            void indentIfNeeded();

            std::ostream &target;
            int level;
            bool indentNext;
        };
    };

    class DumpFormatter: public boost::iostreams::stream<Detail::DumpFormatterSink>
    {
    public:
        DumpFormatter() = delete;

        explicit DumpFormatter(std::ostream &target);

        ~DumpFormatter();

        DumpFormatter(const DumpFormatter &) = delete;

        DumpFormatter &operator = (const DumpFormatter &) = delete;

    private:
        Detail::DumpFormatterSink sink;
    };
}

#endif /* !EVENTLOOP_DUMPFORMATTER_HEADER */
