#include <EventLoop/Callee.hpp>

#include <EventLoop/DumpUtils.hpp>

using namespace EventLoop;

void Callee::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_VALUE_OF(os, life);
    DUMP_END(os);
}
