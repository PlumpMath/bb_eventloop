#ifndef EVENTLOOP_TEST_MOCKABLERUNNER_HEADER
#define EVENTLOOP_TEST_MOCKABLERUNNER_HEADER

#include <memory>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <EventLoop/Runner.hpp>
#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    namespace Test
    {
        /**
         * Helper class for other subsystems for mocking Runner.
         */
        class MockableRunner: public Runner
        {
        public:
            class NoTimers: public Exception
            {
            public:
                NoTimers();
            };

            MockableRunner();

            virtual ~MockableRunner();

            MockableRunner(const MockableRunner &) = delete;

            MockableRunner &operator = (const MockableRunner &) = delete;

            virtual void addFDListener(FDListener &, int, unsigned int);

            virtual void modifyFDListener(int, unsigned int);

            virtual void delFDListener(int);

            virtual void addCallback(const std::function<void()> &);

            virtual void dump(std::ostream &) const;

            /**
             * Get count of currently scheduled timers.
             */
            size_t getScheduledTimerCount() const;

            /**
             * Get timeout for the first scheduled timer.
             */
            boost::posix_time::time_duration getTimeoutForFirstScheduledTimer() const;

            /**
             * Pop and execute the first scheduled timer.
             *
             * @exception NoTimers if there are no scheduled timers.
             */
            void popAndExecuteFirstScheduledTimer();

        protected:
            TimerQueue &getTimerQueue();

        private:
            std::unique_ptr<TimerQueue> tq;
        };
    }
}

#endif /* !EVENTLOOP_TEST_MOCKABLERUNNER_HEADER */
