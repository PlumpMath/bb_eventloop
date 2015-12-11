#ifndef EVENTLOOP_EXCEPTION_HEADER
#define EVENTLOOP_EXCEPTION_HEADER

#include <exception>
#include <string>

namespace EventLoop
{
    class Exception: public std::exception
    {
    public:
        Exception() = delete;

        explicit Exception(const std::string &msg): msg(msg) { }

        virtual ~Exception() noexcept { }

        const char *what() const noexcept { return msg.c_str(); }

        const std::string msg;
    };
}

#endif /* !EVENTLOOP_EXCEPTION_HEADER */
