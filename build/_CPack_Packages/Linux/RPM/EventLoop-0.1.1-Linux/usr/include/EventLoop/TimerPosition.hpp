#ifndef EVENTLOOP_TIMERPOSITION_HEADER
#define EVENTLOOP_TIMERPOSITION_HEADER

#include <map>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace EventLoop
{
    class Timer;
    typedef std::multimap<boost::posix_time::time_duration, Timer *>::iterator TimerPosition;
}

#endif /* !EVENTLOOP_TIMERPOSITION_HEADER */
