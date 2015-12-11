#include "private/DumpFormatter.hpp"

using namespace EventLoop;

Detail::DumpFormatterSink::DumpFormatterSink(std::ostream &target):
    target(target),
    level(0),
    indentNext(false)
{
}

Detail::DumpFormatterSink::~DumpFormatterSink()
{
}

std::streamsize Detail::DumpFormatterSink::write(const char *s, std::streamsize n)
{
    for (auto i = 0U; i < n; ++i)
    {
        const char c(s[i]);
        switch (c)
        {
            case '{':
                indentIfNeeded();
                ++level;
                target.put('{');
                break;
            case '}':
                --level;
                indentIfNeeded();
                target.put('}');
                break;
            case '\n':
                indentNext = true;
                target.put('\n');
                break;
            default:
                indentIfNeeded();
                target.put(c);
        }
    }
    return n;
}

void Detail::DumpFormatterSink::indentIfNeeded()
{
    if (indentNext)
    {
        for (int i = 0; i < level; ++i)
        {
            static const char INDENT[] = "    ";
            target.write(INDENT, sizeof(INDENT) - 1U);
        }
        indentNext = false;
    }
}

DumpFormatter::DumpFormatter(std::ostream &target):
    sink(target)
{
    open(sink);
    flags(target.flags());
}

DumpFormatter::~DumpFormatter()
{
    close();
}
