#include <EventLoop/EpollRunner.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <boost/thread/mutex.hpp>

#include <EventLoop/FDListener.hpp>
#include <EventLoop/SignalSet.hpp>
#include <EventLoop/FileDescriptor.hpp>
#include <EventLoop/DumpUtils.hpp>

#include "private/System.hpp"
#include "private/BaseEpollRunner.hpp"
#include "private/FDMonitor.hpp"

using namespace EventLoop;

class EpollRunner::EventPipe: public FDListener
{
public:
    EventPipe() = delete;

    EventPipe(System        &system,
              FDListenerMap &fdListenerMap);

    ~EventPipe();

    int getWriteFD() const { return writeFD; }

    void dump(std::ostream &os) const;

protected:
    void handleEventsCb(int             fd,
                        unsigned int    events);

private:
    System &system;
    FDListenerMap &fdListenerMap;
    FileDescriptor readFD;
    FileDescriptor writeFD;
};

EpollRunner::EventPipe::EventPipe(System        &system,
                                  FDListenerMap &fdListenerMap):
    system(system),
    fdListenerMap(fdListenerMap),
    readFD(system),
    writeFD(system)
{
    int fd[2] = { -1, -1 };
    system.pipe(fd, O_CLOEXEC);
    readFD = fd[0];
    system.setNonBlock(readFD);
    writeFD = fd[1];

    fdListenerMap.add(*this, readFD, EVENT_IN);
}

EpollRunner::EventPipe::~EventPipe()
{
    fdListenerMap.del(readFD);
}

void EpollRunner::EventPipe::handleEventsCb(int,
                                            unsigned int)
{
    /* Just reading all the data is enough */
    ssize_t ssize;
    do
    {
        char buf[32];
        ssize = system.read(readFD, buf, sizeof(buf));
    }
    while (ssize > 0);
}

void EpollRunner::EventPipe::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_ADDRESS_OF(os, fdListenerMap);
    DUMP_VALUE_OF(os, readFD);
    DUMP_VALUE_OF(os, writeFD);
    DUMP_END(os);
}

class EpollRunner::SignalFD: public FDListener
{
public:
    SignalFD() = delete;

    SignalFD(System             &system,
             EpollRunner        &runner,
             const SignalSet    &signalSet,
             SignalHandler      signalHandler);

    ~SignalFD();

    void setSignals(const SignalSet &signalSet);

    void dump(std::ostream &os) const;

protected:
    void handleEventsCb(int             fd,
                        unsigned int    events);

private:
    System &system;
    EpollRunner &runner;
    FileDescriptor fd;
    SignalSet handledSignals;
    SignalSet originallyBlockedSignals;
    SignalHandler signalHandler;
};

EpollRunner::SignalFD::SignalFD(System          &system,
                                EpollRunner     &runner,
                                const SignalSet &signalSet,
                                SignalHandler   signalHandler):
    system(system),
    runner(runner),
    fd(system),
    handledSignals(signalSet),
    signalHandler(signalHandler)
{
    system.sigMask(SIG_BLOCK, &handledSignals.getSet(), &originallyBlockedSignals.getSet());
    fd = system.signalFD(-1, &signalSet.getSet(), SFD_NONBLOCK | SFD_CLOEXEC);
    runner.addFDListener(*this, fd, EVENT_IN);
}

EpollRunner::SignalFD::~SignalFD()
{
    runner.delFDListener(fd);
    fd.close();
    system.sigMask(SIG_SETMASK, &originallyBlockedSignals.getSet(), nullptr);
}

void EpollRunner::SignalFD::setSignals(const SignalSet &signalSet)
{
    SignalSet blockSignals;
    SignalSet unblockSignals(handledSignals);
    for (const auto &i : signalSet)
    {
        if (!originallyBlockedSignals.isSet(i))
        {
            if (!handledSignals.isSet(i))
            {
                blockSignals.set(i);
            }
            else
            {
                unblockSignals.unset(i);
            }
        }
    }
    for (const auto &i : originallyBlockedSignals)
    {
        unblockSignals.unset(i);
    }
    handledSignals = signalSet;
    system.sigMask(SIG_BLOCK, &blockSignals.getSet(), nullptr);
    system.sigMask(SIG_UNBLOCK, &unblockSignals.getSet(), nullptr);
    system.signalFD(fd, &signalSet.getSet(), 0);
}

void EpollRunner::SignalFD::handleEventsCb(int,
                                           unsigned int)
{
    ssize_t ssize;
    do
    {
        struct signalfd_siginfo siginfo;
        ssize = system.read(fd, &siginfo, sizeof(siginfo));
        if (ssize > 0)
        {
            runner.addCallback(std::bind(signalHandler, siginfo.ssi_signo));
        }
    }
    while (ssize > 0);
}

void EpollRunner::SignalFD::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_ADDRESS_OF(os, runner);
    DUMP_VALUE_OF(os, fd);
    DUMP_VALUE_OF(os, handledSignals);
    DUMP_VALUE_OF(os, originallyBlockedSignals);
    SignalSet currentlyBlockedSignals;
    system.sigMask(SIG_BLOCK, nullptr, &currentlyBlockedSignals.getSet());
    DUMP_VALUE_OF(os, currentlyBlockedSignals);
    DUMP_VALUE_OF(os, signalHandler);
    DUMP_END(os);
}

EpollRunner::EpollRunner():
    EpollRunner(System::getSystem()) { }

EpollRunner::EpollRunner(const SignalSet     &signalSet,
                         SignalHandler       signalHandler):
    EpollRunner(System::getSystem(), signalSet, signalHandler) { }

EpollRunner::EpollRunner(System &system):
    system(system),
    fdsDeletedWhileHandlingEvents(new FDMonitor()),
    baseEpollRunner(new BaseEpollRunner(system)),
    eventPipe(new EventPipe(system, baseEpollRunner->fdListenerMap))
{
    baseEpollRunner->cbq.setWakeUp(std::bind(&EpollRunner::wakeUp, this));
}

EpollRunner::EpollRunner(System             &system,
                         const SignalSet    &signalSet,
                         SignalHandler      signalHandler):
    system(system),
    fdsDeletedWhileHandlingEvents(new FDMonitor()),
    baseEpollRunner(new BaseEpollRunner(system)),
    eventPipe(new EventPipe(system, baseEpollRunner->fdListenerMap)),
    signalFD(new SignalFD(system, *this, signalSet, signalHandler))
{
    baseEpollRunner->cbq.setWakeUp(std::bind(&EpollRunner::wakeUp, this));
}

EpollRunner::~EpollRunner()
{
}

namespace
{
    class ResetThreadIdInDestructor
    {
    public:
        ResetThreadIdInDestructor() = delete;

        explicit ResetThreadIdInDestructor(boost::thread::id &id): id(id) { }

        ~ResetThreadIdInDestructor() { id = boost::thread::id(); }

        ResetThreadIdInDestructor(const ResetThreadIdInDestructor &) = delete;

        ResetThreadIdInDestructor &operator = (const ResetThreadIdInDestructor &) = delete;

    private:
        boost::thread::id &id;
    };
}

void EpollRunner::run(CanContinue canContinue)
{
    ResetThreadIdInDestructor rtid(runnerThreadId);
    runnerThreadId = boost::this_thread::get_id();
    runImpl(canContinue);
}

bool EpollRunner::isTimeout(const boost::posix_time::time_duration &timeout)
{
    static const boost::posix_time::time_duration MIN_TIMEOUT(boost::posix_time::milliseconds(1));

    return ((timeout != TimerQueue::NO_TIMEOUT) &&
            (timeout < MIN_TIMEOUT));
}

void EpollRunner::runImpl(CanContinue canContinue)
{
    do
    {
        if ((baseEpollRunner->cbq.execute()) &&
            (!canContinue()))
        {
            break;
        }

        const boost::posix_time::time_duration timeout(baseEpollRunner->tq.getTimeout());
        if ((isTimeout(timeout)) ||
            (wait(timeout)))
        {
            /* Timeout, execute the first timer */
            baseEpollRunner->executeFirstTimer();
        }
        else
        {
            /* Events in one or more fds, handle them */
            FDMonitor::Guard g(*fdsDeletedWhileHandlingEvents);
            for (Epoll::event_iterator i = baseEpollRunner->epoll.events_begin();
                 i != baseEpollRunner->epoll.events_end();
                 ++i)
            {
                const int fd(i->data.fd);
                if (!fdsDeletedWhileHandlingEvents->has(fd))
                {
                    FDListener &fl(baseEpollRunner->fdListenerMap.get(fd));
                    fl.handleEvents(fd, i->events);
                }
            }
        }
    }
    while (canContinue());
}

bool EpollRunner::wait(const boost::posix_time::time_duration &timeout)
{
    return baseEpollRunner->epoll.wait(static_cast<int>(timeout.total_milliseconds()));
}

void EpollRunner::wakeUp()
{
    static const boost::thread::id NO_ID;
    if ((runnerThreadId != NO_ID) &&
        (runnerThreadId != boost::this_thread::get_id()))
    {
        /*
         * Some thread is running the run() function but this is not that
         * thread, so we need to wake up that thread.
         */
        int event(0);
        system.write(eventPipe->getWriteFD(), &event, sizeof(event));
    }
}

void EpollRunner::setSignals(const SignalSet &signalSet)
{
    if (!signalFD)
    {
        throw NoSignalHandling();
    }
    signalFD->setSignals(signalSet);
}

void EpollRunner::addFDListener(FDListener      &fl,
                                int             fd,
                                unsigned int    events)
{
    baseEpollRunner->fdListenerMap.add(fl, fd, events);
}

void EpollRunner::modifyFDListener(int          fd,
                                   unsigned int events)
{
    baseEpollRunner->fdListenerMap.modify(fd, events);
}

void EpollRunner::delFDListener(int fd)
{
    fdsDeletedWhileHandlingEvents->add(fd);
    baseEpollRunner->fdListenerMap.del(fd);
}

void EpollRunner::addCallback(const std::function<void()> &cb)
{
    baseEpollRunner->cbq.add(cb);
}

TimerQueue &EpollRunner::getTimerQueue()
{
    return baseEpollRunner->tq;
}

void EpollRunner::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_ADDRESS_OF(os, system);
    DUMP_VALUE_POINTED_BY(os, fdsDeletedWhileHandlingEvents);
    DUMP_VALUE_POINTED_BY(os, baseEpollRunner);
    DUMP_VALUE_OF(os, eventPipe);
    DUMP_VALUE_OF(os, signalFD);
    DUMP_VALUE_OF(os, runnerThreadId);;
    DUMP_END(os);
}
