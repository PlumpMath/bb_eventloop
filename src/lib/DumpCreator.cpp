#include <EventLoop/DumpCreator.hpp>

#include <ostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <EventLoop/Dumpable.hpp>

#include "private/DumpFormatter.hpp"

using namespace EventLoop;

namespace
{
    void outputPid(std::ostream &os)
    {
        os << "Pid: " << getpid() << '\n';
    }

    void outputTimestamp(std::ostream &os)
    {
        const boost::posix_time::ptime t(boost::posix_time::microsec_clock::universal_time());
        os << "Time: " << boost::posix_time::to_iso_extended_string(t) << '\n';
    }

    void outputDumpable(std::ostream    &os,
                        const Dumpable  *d)
    {
        if (d == nullptr)
        {
            os << "null\n";
        }
        else
        {
            d->dump(os);
        }
    }

    void outputDumpables(std::ostream       &os,
                         const Dumpables    &ds)
    {
        for (Dumpables::const_iterator i = ds.begin();
             i != ds.end();
             ++i)
        {
            outputDumpable(os, *i);
        }
    }
}

void EventLoop::dumpToStream(std::ostream       &os,
                             const Dumpables    &ds)
{
    os << std::boolalpha;
    outputPid(os);
    outputTimestamp(os);
    os << "BEGIN\n";
    {
        DumpFormatter df(os);
        outputDumpables(df, ds);
    }
    os << "END\n";
}

bool EventLoop::dumpToFile(const std::string    &name,
                           const Dumpables      ds)
{
    std::ofstream of(name.c_str());
    if (of)
    {
        dumpToStream(of, ds);
        return true;
    }
    else
    {
        return false;
    }
}
