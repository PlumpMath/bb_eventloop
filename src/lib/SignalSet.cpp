#include <EventLoop/SignalSet.hpp>

#include <ostream>
#include <map>

using namespace EventLoop;

namespace
{
    typedef std::map<SignalSet::value_type, std::string> SignalNameMap;
    const SignalNameMap signalNameMap = {
        { SIGHUP, "SIGHUP" },
        { SIGINT, "SIGINT" },
        { SIGQUIT, "SIGQUIT" },
        { SIGILL, "SIGILL" },
        { SIGTRAP, "SIGTRAP" },
        { SIGABRT, "SIGABRT" },
        { SIGIOT, "SIGIOT" },
        { SIGBUS, "SIGBUS" },
        { SIGFPE, "SIGFPE" },
        { SIGKILL, "SIGKILL" },
        { SIGUSR1, "SIGUSR1" },
        { SIGSEGV, "SIGSEGV" },
        { SIGUSR2, "SIGUSR2" },
        { SIGPIPE, "SIGPIPE" },
        { SIGALRM, "SIGALRM" },
        { SIGTERM, "SIGTERM" },
#ifdef SIGSTKFLT
        { SIGSTKFLT, "SIGSTKFLT" },
#endif /* !SIGSTKFLT */
        { SIGCHLD, "SIGCHLD" },
        { SIGCONT, "SIGCONT" },
        { SIGSTOP, "SIGSTOP" },
        { SIGTSTP, "SIGTSTP" },
        { SIGTTIN, "SIGTTIN" },
        { SIGTTOU, "SIGTTOU" },
        { SIGURG, "SIGURG" },
        { SIGXCPU, "SIGXCPU" },
        { SIGXFSZ, "SIGXFSZ" },
        { SIGVTALRM, "SIGVTALRM" },
        { SIGPROF, "SIGPROF" },
        { SIGWINCH, "SIGWINCH" },
        { SIGIO, "SIGIO" },
        { SIGPOLL, "SIGPOLL" },
        { SIGPWR, "SIGPWR" },
        { SIGSYS, " SIGSYS" },
#ifdef SIGUNUSED
        { SIGUNUSED, "SIGUNUSED" },
#endif /* SIGUNUSED */
    };
}

const SignalSet::value_type SignalSet::MAX_SIGNAL(signalNameMap.size());

SignalSet::size_type SignalSet::size() const
{
    size_type s(0U);
    for (value_type i = 1; i <= SignalSet::MAX_SIGNAL; ++i)
    {
        if (isSet(i)) ++s;
    }
    return s;
}

SignalSet::value_type SignalSet::getNext(value_type current) const
{
    for (value_type i = current + 1; i <= SignalSet::MAX_SIGNAL; ++i)
    {
        if (isSet(i)) return i;
    }
    return SignalSet::MAX_SIGNAL + 1;
}

bool SignalSet::operator == (const SignalSet &other) const
{
    for (value_type i = 0U; i <= SignalSet::MAX_SIGNAL; ++i)
    {
        if (isSet(i) != other.isSet(i))
        {
            return false;
        }
    }
    return true;
}

std::string EventLoop::signalToString(SignalSet::value_type sig)
{
    const SignalNameMap::const_iterator i(signalNameMap.find(sig));
    if (i != signalNameMap.end())
    {
        return i->second;
    }
    return ("unknown-signal");
}

std::ostream & EventLoop::operator << (std::ostream     &out,
                                       const SignalSet  &sset)
{
    bool first(true);
    for (SignalSet::const_iterator i = sset.begin();
         i != sset.end();
         ++i)
    {
        if (!first) out << ',';
        out << signalToString(*i);
        first = false;
    }
    return out;
}
