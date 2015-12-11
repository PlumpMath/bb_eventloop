#ifndef EVENTLOOP_DUMPCREATOR_HEADER
#define EVENTLOOP_DUMPCREATOR_HEADER

#include <iosfwd>
#include <vector>
#include <string>

namespace EventLoop
{
    class Dumpable;

    typedef std::vector<const Dumpable *> Dumpables;

    /**
     * Dump all the given Dumpables and decorations to the given stream.
     */
    void dumpToStream(std::ostream      &os,
                      const Dumpables   &ds);

    /**
     * Dump all the given Dumpables and decorations to a file with the given
     * name.
     *
     * @return  True, if file was successfully created. False, if file couldn't
     *          be created.
     */
    bool dumpToFile(const std::string   &name,
                    const Dumpables     ds);
}

#endif /* !EVENTLOOP_DUMPCREATOR_HEADER */
