#include "private/SigPipeSuppressor.hpp"

#include "private/System.hpp"

using namespace EventLoop;

SigPipeSuppressor::SigPipeSuppressor(System &system):
    mask(SIGPIPE),
    system(system),
    wasPending(false),
    doUnblock(false)
{
    wasPending = isPending();
    if (!wasPending)
    {
        SignalSet blocked;
        system.sigMask(SIG_BLOCK, &mask.getSet(), &blocked.getSet());
        doUnblock = !blocked.isSet(SIGPIPE);
    }
}

SigPipeSuppressor::~SigPipeSuppressor() noexcept
{
    if (!wasPending)
    {
        if (isPending())
        {
            static const struct timespec timeout = { 0, 0 };
            system.sigTimedWait(&mask.getSet(), nullptr, &timeout);
        }
        if (doUnblock)
        {
            system.sigMask(SIG_UNBLOCK, &mask.getSet(), nullptr);
        }
    }
}

bool SigPipeSuppressor::isPending()
{
    SignalSet pending;
    system.sigPending(&pending.getSet());
    return pending.isSet(SIGPIPE);
}
