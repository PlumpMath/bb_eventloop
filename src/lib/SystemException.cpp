#include <EventLoop/SystemException.hpp>

#include <sstream>
#include <cstring>
#include <cerrno>

using namespace EventLoop;

namespace
{
    std::string buildMessage(const std::string  &function,
                             int                error)
    {
        std::ostringstream os;
        os << function << ": " << strerror(error);
        return os.str();
    }
}

SystemException::SystemException(const std::string &function):
    SystemException(function, errno) { }

SystemException::SystemException(const std::string  &function,
                                 int                error):
    Exception(buildMessage(function, error)),
    function(function),
    error(error)
{
}
