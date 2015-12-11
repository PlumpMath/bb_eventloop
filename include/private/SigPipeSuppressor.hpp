#ifndef EVENTLOOP_SIGPIPESUPPRESSOR_HEADER
#define EVENTLOOP_SIGPIPESUPPRESSOR_HEADER

#include <EventLoop/SignalSet.hpp>

namespace EventLoop
{
    class System;

    class SigPipeSuppressor
    {
    public:
        SigPipeSuppressor() = delete;

        SigPipeSuppressor(const SigPipeSuppressor &) = delete;

        SigPipeSuppressor &operator = (const SigPipeSuppressor &) = delete;

        explicit SigPipeSuppressor(System &system);

        ~SigPipeSuppressor() noexcept;

    private:
        bool isPending();

        const SignalSet mask;
        System &system;
        bool wasPending;
        bool doUnblock;
    };
}

#endif /* !EVENTLOOP_SIGPIPESUPPRESSOR_HEADER */
