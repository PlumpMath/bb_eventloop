#ifndef EVENTLOOP_UNCAUGHTEXCEPTIONHANDLER_HEADER
#define EVENTLOOP_UNCAUGHTEXCEPTIONHANDLER_HEADER

namespace EventLoop
{
    /**
     * Class for logging unexpected and uncaught exceptions to syslog without
     * actually catching them in the application code. The motivation is to
     * have both syslog about the exception *and* core dump showing where the
     * exception was thrown. By using normal try & catch logic backtrace in the
     * core dump is not from the original throw but from the catch making
     * debugging very difficult.
     */
    class UncaughtExceptionHandler
    {
    public:
        UncaughtExceptionHandler() = delete;

        ~UncaughtExceptionHandler() = delete;

        UncaughtExceptionHandler(const UncaughtExceptionHandler &) = delete;

        UncaughtExceptionHandler &operator = (const UncaughtExceptionHandler &) = delete;

        /**
         * Enable exception handling.
         */
        static void enable();

        /**
         * Disable exception handling.
         */
        static void disable();

        static int getReferenceCount();

        /**
         * RAII (Resource Acquisition Is Initialization) helper class for
         * enabling and disabling exception handling.
         */
        class Guard
        {
        public:
            Guard() { UncaughtExceptionHandler::enable(); }

            ~Guard() { UncaughtExceptionHandler::disable(); }

            Guard(const Guard &) = delete;

            Guard &operator = (const Guard &) = delete;
        };
    };
}

#endif /* !EVENTLOOP_UNCAUGHTEXCEPTIONHANDLER_HEADER */
