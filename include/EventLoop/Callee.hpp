#ifndef EVENTLOOP_CALLEE_HEADER
#define EVENTLOOP_CALLEE_HEADER

#include <memory>

#include <EventLoop/Dumpable.hpp>

namespace EventLoop
{
    class WeakCallback;

    /**
     * Simple callee to be used with WeakCallback.
     *
     * @see WeakCallback
     */
    class Callee: public Dumpable
    {
        friend class WeakCallback;

    public:
        struct Life
        {
            bool alive;

            Life(): alive(true) { }
            ~Life() { alive = false; }
        };

        Callee(): life(new Life()) { }

        Callee(const Callee &) = default;

        Callee &operator = (const Callee &) = default;

        virtual ~Callee() = default;

        virtual void dump(std::ostream &os) const;

    private:
        const std::shared_ptr<Life> &getLife() const { return life; }

        std::shared_ptr<Life> life;
    };
}

#endif /* !EVENTLOOP_CALLEE_HEADER */
