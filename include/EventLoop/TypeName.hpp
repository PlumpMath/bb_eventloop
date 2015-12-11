#ifndef EVENTLOOP_TYPENAME_HEADER
#define EVENTLOOP_TYPENAME_HEADER

#include <typeinfo>
#include <cstdlib>

#define EVENTLOOP_MAX_TYPE_NAME 512

namespace EventLoop
{
    namespace Detail
    {
        const char *demangleName(const char *name,
                                 char       *buffer,
                                 size_t     size);
    }

    template <typename T>
    const char *getTypeName(const T &t,
                            char    *buffer,
                            size_t  size)
    {
        return Detail::demangleName(typeid(t).name(), buffer, size);
    }
}

#endif /* !EVENTLOOP_TYPENAME_HEADER */
