#include <EventLoop/UncaughtExceptionHandler.hpp>

#include <cxxabi.h>
#include <exception>
#include <cstdlib>
#include <syslog.h>
#include <boost/thread/mutex.hpp>

#include <EventLoop/TypeName.hpp>

#include "private/Abort.hpp"

using namespace EventLoop;

namespace
{
    std::terminate_handler originalUnexpected(nullptr);

    std::terminate_handler originalTerminate(nullptr);

    int referenceCount(0);

    boost::mutex referenceCountMutex;

    void analyzeStdException(bool                   unexpected,
                             const std::exception   &e)
    {
        char buffer[EVENTLOOP_MAX_TYPE_NAME];
        syslog(LOG_ERR,
               "%s called after throwing an instance of \'%s\': %s\n",
               unexpected ? "unexpected" : "terminate",
               getTypeName(e, buffer, sizeof(buffer)),
               e.what());
    }

    void analyzeUnknownException(bool unexpected)
    {
        std::type_info *t(abi::__cxa_current_exception_type());
        if (t != nullptr)
        {
            char buffer[EVENTLOOP_MAX_TYPE_NAME];
            syslog(LOG_ERR,
                   "%s called after throwing an instance of \'%s\'\n",
                   unexpected ? "unexpected" : "terminate",
                   Detail::demangleName(t->name(), buffer, sizeof(buffer)));
        }
        else
        {
            syslog(LOG_ERR,
                   "%s called after throwing uknown exception\n",
                   unexpected ? "unexpected" : "terminate");
        }
    }

    void getAndAnalyzeException(bool unexpected)
    {
        try
        {
            throw;
        }
        catch (const std::exception &e)
        {
            analyzeStdException(unexpected, e);
        }
        catch (...)
        {
            analyzeUnknownException(unexpected);
        }
    }

    void handleUnexpected()
    {
        getAndAnalyzeException(true);
        ::originalUnexpected();
        EVENTLOOP_ABORT();
    }

    void handleTerminate()
    {
        getAndAnalyzeException(false);
        ::originalTerminate();
        EVENTLOOP_ABORT();
    }
}

void UncaughtExceptionHandler::enable()
{
    boost::mutex::scoped_lock lock(referenceCountMutex);
    originalUnexpected = std::set_unexpected(handleUnexpected);
    originalTerminate = std::set_terminate(handleTerminate);
    ++referenceCount;
}

void UncaughtExceptionHandler::disable()
{
    boost::mutex::scoped_lock lock(referenceCountMutex);
    if (--referenceCount == 0)
    {
        std::set_unexpected(originalUnexpected);
        originalUnexpected = nullptr;
        std::set_terminate(originalTerminate);
        originalTerminate = nullptr;
    }
}

int UncaughtExceptionHandler::getReferenceCount()
{
    boost::mutex::scoped_lock lock(referenceCountMutex);
    return referenceCount;
}
