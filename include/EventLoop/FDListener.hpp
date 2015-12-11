#ifndef EVENTLOOP_FDLISTENER_HEADER
#define EVENTLOOP_FDLISTENER_HEADER

#include <EventLoop/Dumpable.hpp>

namespace EventLoop
{
    class FDListener: public Dumpable
    {
    public:
        /** Event: There is data to read. */
        static const unsigned int EVENT_IN;
        /** Event: Writing now will not block. */
        static const unsigned int EVENT_OUT;
        /** Event: Error condition (output only). */
        static const unsigned int EVENT_ERR;
        /** Event: Hang up (output only). */
        static const unsigned int EVENT_HUP;

        FDListener() = default;

        virtual ~FDListener();

        FDListener(const FDListener &) = delete;

        FDListener &operator = (const FDListener &) = delete;

        /**
         * Handle the given events.
         *
         * @param fd        File descriptor where the events happened.
         * @param events    One or more EVENT_* values bit-wise or-red.
         */
        void handleEvents(int           fd,
                          unsigned int  events);

        virtual void dump(std::ostream &os) const;

    protected:
        /**
         * Handle the given events.
         *
         * @param fd        File descriptor where the events happened.
         * @param events    One or more EVENT_* values bit-wise or-red.
         * @return          Result telling for caller what to do.
         */
        virtual void handleEventsCb(int             fd,
                                    unsigned int    events) = 0;
    };
}

#endif /* !EVENTLOOP_FDLISTENER_HEADER */
