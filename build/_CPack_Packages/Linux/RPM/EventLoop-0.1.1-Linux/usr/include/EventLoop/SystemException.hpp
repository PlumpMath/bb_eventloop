#ifndef EVENTLOOP_SYSTEMEXCEPTION_HEADER
#define EVENTLOOP_SYSTEMEXCEPTION_HEADER

#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    class SystemException: public Exception
    {
    public:
        SystemException() = delete;

        explicit SystemException(const std::string &function);

        SystemException(const std::string   &function,
                        int                 error);

        virtual ~SystemException() noexcept { }

        const std::string function;
        const int error;
    };
}

#endif /* !EVENTLOOP_SYSTEMEXCEPTION_HEADER */
