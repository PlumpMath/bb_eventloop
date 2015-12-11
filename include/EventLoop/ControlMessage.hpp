#ifndef EVENTLOOP_CONTROLMESSAGE_HEADER
#define EVENTLOOP_CONTROLMESSAGE_HEADER

#include <type_traits>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <boost/iterator/iterator_facade.hpp>

#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    /**
     * Control message to be used with sendmsg(2) and recvmsg(2). Also called
     * ancillary data.
     *
     * Sending control messages:
     * \code
     * ControlMessage cm;
     * cm.append(SOL_SOCKET, SCM_RIGHTS, fdToSend);
     * struct msghdr msg;
     * msg.msg_control = cm.data();
     * msg.msg_controllen = cm.size();
     * ...
     * sendmsg(fd, &msg, flags);
     * \endcode
     *
     * Receiving control messages:
     * \code
     * ControlMessage cm;
     * cm.reserve(1024U);
     * struct msghdr msg;
     * msg.msg_control = cm.data();
     * msg.msg_controllen = cm.capacity();
     * ...
     * recvmsg(fd, &msg, flags);
     * cm.markUsed(msg.msg_controllen);
     * for (ControlMessage::const_iterator i = cm.begin(); i != cm.end(); ++i)
     * {
     *     if ((i->cmsg_level == SOL_SOCKET) &&
     *         (i->cmsg_type == SCM_RIGHTS))
     *     {
     *          int receivedFD(ControlMessage::getData<int>(*i));
     *           ... do something with receivedFD ...
     *     }
     * }
     * \endcode
     */
    class ControlMessage
    {
    public:
        class TruncatedCMSG: public Exception
        {
        public:
            TruncatedCMSG(): Exception("truncated cmsghdr") { }
        };

        class BadCMSG: public Exception
        {
        public:
            BadCMSG(): Exception("bad cmsghdr") { }
        };

        class Overflow: public Exception
        {
        public:
            Overflow(): Exception("control message overflow") { }
        };

        typedef size_t size_type;

        /**
         * Default constructor. Creates zero size empty control message.
         */
        ControlMessage(): usedSize(0U) { }

        /**
         * Create control message by copying the given data of the given size.
         */
        ControlMessage(const void   *data,
                       size_type    size);

        /**
         * Create control message containing ancillary data with the given
         * level, type and value.
         */
        template <typename T>
        ControlMessage(int      level,
                       int      type,
                       const T  &value);

        virtual ~ControlMessage() noexcept = default;

        /**
         * Get control message size in bytes.
         */
        size_type size() const { return usedSize; }

        /**
         * Get pointer to the data.
         */
        const void *data() const { return buffer.data(); }

        /**
         * Get pointer to the data.
         */
        void *data() { return buffer.data(); }

        bool empty() const { return buffer.empty(); }

        /**
         * Reserve capacity of the given size.
         */
        void reserve(size_type size) { buffer.resize(size); }

        /**
         * Check how much capacity has been reserved.
         */
        size_type capacity() const { return buffer.size(); }

        /**
         * Mark the given size to be used of previously allocated capacity.
         *
         * @exception Overflow if the given size exceeds the capacity.
         */
        void markUsed(size_type size);

        /**
         * Append ancillary data with the given level, type and value.
         */
        template <typename T>
        void append(int     level,
                    int     type,
                    const T &value);

        /**
         * Get ancillary data of the given type from the given cmsg.
         *
         * @exception TruncatedCMSG if the given cmsg is not big enough to
         *            contain the needed type.
         */
        template <typename T>
        static const T &getData(const struct cmsghdr &cmsg);

        template <typename T>
        class IteratorImpl: public boost::iterator_facade<IteratorImpl<T>, T, boost::forward_traversal_tag>
        {
            friend class ControlMessage;

        public:
            IteratorImpl(): ptr(nullptr), size(0U) { }

            template <typename Other>
            IteratorImpl(const IteratorImpl<Other> &other): ptr(other.ptr), size(other.size) { }

            template <typename Other>
            bool equal(const IteratorImpl<Other> &other) const { return ptr == other.ptr; }

            void increment()
            {
                typedef typename std::remove_const<T>::type Type;
                const size_type asize(CMSG_ALIGN(ptr->cmsg_len));
                size -= asize;
                ptr = reinterpret_cast<T *>(reinterpret_cast<char *>(const_cast<Type *>(ptr)) + asize);
            }

            T &dereference() const
            {
                if ((size < sizeof(*ptr)) ||
                    (ptr->cmsg_len < sizeof(*ptr)) ||
                    (size < static_cast<size_type>(ptr->cmsg_len)))
                {
                    throw BadCMSG();
                }
                return *ptr;
            }

        private:
            IteratorImpl(T *ptr, size_type size): ptr(ptr), size(size) { }

            T *ptr;
            size_type size;
        };

        typedef IteratorImpl<struct cmsghdr> iterator;

        typedef IteratorImpl<struct cmsghdr const> const_iterator;

        iterator begin() { return iterator(reinterpret_cast<struct cmsghdr *>(buffer.data()), usedSize); }

        const_iterator begin() const { return const_iterator(reinterpret_cast<const struct cmsghdr *>(buffer.data()), usedSize); }

        iterator end() { return iterator(reinterpret_cast<struct cmsghdr *>(&buffer[0] + usedSize), 0U); }

        const_iterator end() const { return const_iterator(reinterpret_cast<const struct cmsghdr *>(&buffer[0] + usedSize), 0U); }

    private:
        /* buffer.size() is the capacity */
        std::vector<char> buffer;
        size_type usedSize;
    };

    template <typename T>
    ControlMessage::ControlMessage(int      level,
                                   int      type,
                                   const T  &value):
        usedSize(0)
    {
        append(level, type, value);
    }

    template <typename T>
    void ControlMessage::append(int     level,
                                int     type,
                                const T &value)
    {
        const size_type neededSize(CMSG_SPACE(sizeof(value)));
        buffer.resize(usedSize + neededSize, 0);
        const size_type oldUsedSize(usedSize);
        usedSize += neededSize;
        struct cmsghdr *cmsg(reinterpret_cast<struct cmsghdr *>(&buffer[oldUsedSize]));
        cmsg->cmsg_level = level;
        cmsg->cmsg_type = type;
        cmsg->cmsg_len = CMSG_LEN(sizeof(value));
        memcpy(CMSG_DATA(cmsg), &value, sizeof(value));
    }

    template <typename T>
    const T &ControlMessage::getData(const struct cmsghdr &cmsg)
    {
        if (cmsg.cmsg_len < CMSG_LEN(sizeof(T)))
        {
            throw TruncatedCMSG();
        }
        return *reinterpret_cast<const T *>(CMSG_DATA(&cmsg));
    }
}

#endif /* !EVENTLOOP_CONTROLMESSAGE_HEADER */
