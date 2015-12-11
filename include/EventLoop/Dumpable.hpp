#ifndef EVENTLOOP_DUMPABLE_HEADER
#define EVENTLOOP_DUMPABLE_HEADER

#include <iosfwd>

namespace EventLoop
{
    /**
     * Base class for any class or struct that can be 'dumped'. Taking 'dump'
     * from ongoing process is very powerful debugging tool if implemented
     * correctly in application classes.
     *
     * EventLoop's own structs and classes all supports 'dump'.
     *
     * Utilities for easily implementing 'dump' in application code can be found
     * from <EventLoop/DumpUtils.hpp>.
     */
    class Dumpable
    {
    public:
        Dumpable() = default;

        virtual ~Dumpable() { }

        virtual void dump(std::ostream &os) const = 0;
    };
}

#endif /* !EVENTLOOP_DUMPABLE_HEADER */
