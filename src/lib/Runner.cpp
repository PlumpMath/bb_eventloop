#include <EventLoop/Runner.hpp>

#include <string>
#include <sstream>

using namespace EventLoop;

namespace
{
    std::string buildFDAlreadyAddedError(int fd)
    {
        std::ostringstream os;
        os << "fd " << fd << " already added";
        return os.str();
    }

    std::string buildNonExistingFDListenerError(int fd)
    {
        std::ostringstream os;
        os << "non-existing FDListener: " << fd;
        return os.str();
    }
}

Runner::FDAlreadyAdded::FDAlreadyAdded(int fd):
    Exception(buildFDAlreadyAddedError(fd))
{
}

Runner::NonExistingFDListener::NonExistingFDListener(int fd):
    Exception(buildNonExistingFDListenerError(fd))
{
}

Runner::Runner()
{
}

Runner::~Runner()
{
}
