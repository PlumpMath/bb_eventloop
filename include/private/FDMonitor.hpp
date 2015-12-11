#ifndef EVENTLOOP_FDMONITOR_HEADER
#define EVENTLOOP_FDMONITOR_HEADER

#include <set>

#include <EventLoop/Dumpable.hpp>

namespace EventLoop
{
    class FDMonitor: public Dumpable
    {
    public:
        class Guard
        {
        public:
            explicit Guard(FDMonitor &m);

            ~Guard();

            Guard() = delete;

            Guard(const Guard &) = delete;

            Guard &operator = (const Guard &) = delete;

        private:
            FDMonitor &m;
        };

        FDMonitor();

        ~FDMonitor();

        FDMonitor(const FDMonitor &) = delete;

        FDMonitor &operator = (const FDMonitor &) = delete;

        void startMonitoring();

        void stopMonitoring();

        void add(int fd);

        bool has(int fd) const;

        void dump(std::ostream &os) const;

    private:
        typedef std::set<int> Set;

        bool monitoring;
        Set set;
    };
}

#endif /* !EVENTLOOP_FDMONITOR_HEADER */
