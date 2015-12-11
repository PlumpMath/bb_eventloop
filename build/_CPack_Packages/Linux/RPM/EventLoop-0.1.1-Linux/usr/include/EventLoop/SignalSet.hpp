#ifndef SIGNALSET_HEADER
#define SIGNALSET_HEADER

#include <string>
#include <iosfwd>
#include <initializer_list>
#include <csignal>
#include <boost/iterator/iterator_facade.hpp>

#include <EventLoop/SystemException.hpp>

namespace EventLoop
{
    /** Simple wrapper for sigset_t. */
    class SignalSet
    {
        friend class const_iterator;

    public:
        typedef size_t size_type;

        typedef int value_type;

        SignalSet() { sigemptyset(&sset); }

        explicit SignalSet(value_type sig) { sigemptyset(&sset); set(sig); }

        SignalSet(const std::initializer_list<value_type> &sigs) { sigemptyset(&sset); for (auto i : sigs) set(i); }

        virtual ~SignalSet() = default;

        bool isSet(value_type sig) const { return sigismember(&sset, sig); }

        bool empty() const { return sigisemptyset(&sset); }

        size_type size() const;

        void set(value_type sig) { if (sigaddset(&sset, sig)) { throw SystemException("sigaddset"); } }

        void unset(value_type sig) { if (sigdelset(&sset, sig)) { throw SystemException("sigdelset"); } }

        const sigset_t &getSet() const { return sset; }

        sigset_t &getSet() { return sset; }

        bool operator == (const SignalSet &other) const;

        static const value_type MAX_SIGNAL;

        class const_iterator: public boost::iterator_facade<const_iterator, const value_type, boost::forward_traversal_tag>
        {
            friend class SignalSet;

        public:
            const_iterator(): s(nullptr), i(0) { }

            bool equal(const const_iterator &other) const { return ((s == other.s) && (i == other.i)); }

            void increment() { i = s->getNext(i); }

            const value_type &dereference() const { return i; }

        private:
            explicit const_iterator(const SignalSet *s,
                                    value_type      i): s(s), i(i) { }

            const SignalSet *s;
            value_type i;
        };

        const_iterator begin() const { return const_iterator(this, getNext(0)); }

        const_iterator end() const { return const_iterator(this, MAX_SIGNAL + 1); }

    private:
        value_type getNext(value_type current) const;

        sigset_t sset;
    };

    std::string signalToString(SignalSet::value_type sig);

    std::ostream & operator << (std::ostream    &out,
                                const SignalSet &sset);
}

#endif /* !SIGNALSET_HEADER */
