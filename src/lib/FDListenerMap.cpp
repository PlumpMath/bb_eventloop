#include "private/FDListenerMap.hpp"

#include <ostream>
#include <iomanip>

#include <EventLoop/Runner.hpp>
#include <EventLoop/FDListener.hpp>
#include <EventLoop/DumpUtils.hpp>

using namespace EventLoop;

FDListenerMap::FDListenerMap(Callback   addCallback,
                             Callback   modifyCallback,
                             Callback   delCallback):
    addCallback(addCallback),
    modifyCallback(modifyCallback),
    delCallback(delCallback)
{
}

FDListenerMap::~FDListenerMap()
{
}

const FDListenerMap::MapEntry &FDListenerMap::find(int fd) const
{
    Map::const_iterator i(map.find(fd));
    if (i == map.end())
    {
        throw Runner::NonExistingFDListener(fd);
    }
    return i->second;
}

void FDListenerMap::add(FDListener      &fl,
                        int             fd,
                        unsigned int    events)
{
    const std::pair<Map::iterator, bool> ret(map.insert(std::make_pair(fd, MapEntry(&fl, events))));
    if (!ret.second)
    {
        throw Runner::FDAlreadyAdded(fd);
    }
    addCallback(fl, fd, events);
}

void FDListenerMap::modify(int          fd,
                           unsigned int events)
{
    MapEntry &m(const_cast<MapEntry &>(find(fd)));
    if (m.events != events)
    {
        modifyCallback(*(m.fl), fd, events);
        m.events = events;
    }
}

FDListener &FDListenerMap::get(int fd) const
{
    return *(find(fd).fl);
}

unsigned int FDListenerMap::getEvents(int fd) const
{
    return find(fd).events;
}

void FDListenerMap::del(int fd)
{
    Map::iterator i(map.find(fd));
    if (i == map.end())
    {
        throw Runner::NonExistingFDListener(fd);
    }
    delCallback(*(i->second.fl), i->first, i->second.events);
    map.erase(i);
}

void FDListenerMap::dump(std::ostream &os) const
{
    DUMP_BEGIN(os);
    DUMP_VALUE_OF(os, addCallback);
    DUMP_VALUE_OF(os, modifyCallback);
    DUMP_VALUE_OF(os, delCallback);
    DUMP_BLOCK_BEGIN(os, "map");
    for (Map::const_iterator i = map.begin();
         i != map.end();
         ++i)
    {
        {
            const boost::io::ios_flags_saver ifs(os);
            os << "fd: " << i->first << " events: 0x" << std::hex << i->second.events;
        }
        os << '\n';
        i->second.fl->dump(os);
    }
    DUMP_BLOCK_END(os);
    DUMP_END(os);
}
