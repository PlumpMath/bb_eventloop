#include <EventLoop/TypeName.hpp>

#include <cxxabi.h>

const char *EventLoop::Detail::demangleName(const char  *name,
                                            char        *buffer,
                                            size_t      size)
{
    int status(-1);
    size_t usedSize(size - 1U);
    abi::__cxa_demangle(name, buffer, &usedSize, &status);
    if (status)
    {
        return name;
    }
    else
    {
        buffer[usedSize] = '\0';
        return buffer;
    }
}
