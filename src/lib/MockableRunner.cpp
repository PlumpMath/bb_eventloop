#include <EventLoop/tst/MockableRunner.hpp>

#include <boost/thread/once.hpp>

#include <EventLoop/Timer.hpp>

#include "private/System.hpp"
#include "private/TimerQueue.hpp"

using namespace EventLoop;
using namespace EventLoop::Test;

namespace
{
    class FakeTimeSystem: public System
    {
    public:
        FakeTimeSystem() { }
        ~FakeTimeSystem() { }
        boost::posix_time::milliseconds getMonotonicClock() { return boost::posix_time::milliseconds(0); }

        static FakeTimeSystem &getFakeTimeSystem() noexcept;
    };

    std::unique_ptr<FakeTimeSystem> fakeTimeSystemInstance;
    boost::once_flag once = BOOST_ONCE_INIT;

    void initializeFakeTimeSystem() noexcept
    {
        fakeTimeSystemInstance.reset(new FakeTimeSystem());
    }

    FakeTimeSystem &FakeTimeSystem::getFakeTimeSystem() noexcept
    {
        boost::call_once(&initializeFakeTimeSystem, once);
        return *fakeTimeSystemInstance;
    }
}

MockableRunner::NoTimers::NoTimers():
    Exception("No timers scheduled")
{
}

MockableRunner::MockableRunner():
    tq(new TimerQueue(FakeTimeSystem::getFakeTimeSystem()))
{
}

MockableRunner::~MockableRunner()
{
}

void MockableRunner::addFDListener(FDListener &, int, unsigned int)
{
}

void MockableRunner::modifyFDListener(int, unsigned int)
{
}

void MockableRunner::delFDListener(int)
{
}

void MockableRunner::addCallback(const std::function<void()> &)
{
}

size_t MockableRunner::getScheduledTimerCount() const
{
    return tq->getScheduledTimerCount();
}

boost::posix_time::time_duration MockableRunner::getTimeoutForFirstScheduledTimer() const
{
    return tq->getTimeout();
}

void MockableRunner::popAndExecuteFirstScheduledTimer()
{
    Timer *timer(tq->popFirstTimerToExecute());
    if (timer == nullptr)
    {
        throw NoTimers();
    }
    timer->execute();
}

TimerQueue &MockableRunner::getTimerQueue()
{
    return *tq;
}

void MockableRunner::dump(std::ostream &) const
{
}
