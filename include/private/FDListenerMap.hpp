#ifndef EVENTLOOP_FDLISTENERMAP_HEADER
#define EVENTLOOP_FDLISTENERMAP_HEADER

#include <unordered_map>
#include <functional>

#include <EventLoop/Dumpable.hpp>

namespace EventLoop
{
    class FDListener;

    /**
     * Map of FDListeners mapping file descriptor to corresponding FDListener
     * and events interested by the FDListener. Several file descriptors can map
     * to the same FDListener, but each file descriptor has to be unique inside
     * this map.
     */
    class FDListenerMap: public Dumpable
    {
    public:
        typedef std::function<void(FDListener &fl, int fd, unsigned int events)> Callback;

        FDListenerMap() = delete;

        /**
         * Constructor. The given callback functions can be used as integration
         * with some select/poll/epoll logic.
         *
         * @param addCallback           Callback function to be called when new
         *                              file descriptor is successfully added.
         * @param modifyCallback        Callback function to be called when
         *                              existing file descriptor is successfully
         *                              modified.
         * @param delCallback           Callback function to be called when
         *                              existing file descriptor is successfully
         *                              deleted.
         */
        FDListenerMap(Callback  addCallback,
                      Callback  modifyCallback,
                      Callback  delCallback);

        virtual ~FDListenerMap();

        FDListenerMap(const FDListenerMap &) = delete;

        FDListenerMap &operator = (const FDListenerMap &) = delete;

        void add(FDListener     &fl,
                 int            fd,
                 unsigned int   events);

        void modify(int             fd,
                    unsigned int    events);

        FDListener &get(int fd) const;

        unsigned int getEvents(int fd) const;

        void del(int fd);

        virtual void dump(std::ostream &os) const;

    private:
        struct MapEntry
        {
            FDListener *fl;
            unsigned int events;

            MapEntry(): fl(nullptr), events(0) { }
            MapEntry(FDListener *fl, unsigned int events): fl(fl), events(events) { }
        };

        typedef std::unordered_map<int, MapEntry> Map;

        const MapEntry &find(int fd) const;

        Callback addCallback;
        Callback modifyCallback;
        Callback delCallback;
        Map map;
    };
}

#endif /* !EVENTLOOP_FDLISTENERMAP_HEADER */
