#ifndef EVENTLOOP_STREAMBUFFER_HEADER
#define EVENTLOOP_STREAMBUFFER_HEADER

#include <list>
#include <string>
#include <exception>
#include <cstdlib>
#include <stdint.h>
#include <sys/uio.h>
#include <boost/shared_array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/type_traits/alignment_of.hpp>

#include <EventLoop/Exception.hpp>
#include <EventLoop/Dumpable.hpp>

namespace EventLoop
{
    /**
     * StreamBuffer provides nice functions for handling IO vectors with as
     * little memory copies as possible. The most obvious use case is to send
     * and receive with non-blocking stream socket using recvmsg() and
     * sendmsg().
     *
     * Received data needs to copied only if it has been scattered into multiple
     * IO vectors but reader expects to have one continuous buffer, or if data
     * alignment is not OK. In such cases StreamBuffer automatically linearizes
     * all IO vectors into one IO vector with proper alignment.
     *
     * All non-const functions invalidate all references and pointers to the data.
     */
    class StreamBuffer: public Dumpable
    {
    public:
        class Underflow: public Exception
        {
        public:
            Underflow(): Exception("stream buffer underflow") { }
        };

        class Overflow: public Exception
        {
        public:
            Overflow(): Exception("stream buffer overflow") { }
        };

        StreamBuffer();

        StreamBuffer(const StreamBuffer &sb);

        StreamBuffer &operator = (const StreamBuffer &sb);

        virtual ~StreamBuffer();

        /**
         * Allocate at least the given size of free space, if not already
         * allocated. After this the getFreeSize() is guaranteed to return
         * at least the given size.
         *
         * @param size  Minimum size to allocate free space.
         */
        void allocate(size_t size);

        size_t getFreeSize() const { return totalSize - usedSize; }

        size_t getUsedSize() const { return usedSize; }

        size_t getTotalSize() const { return totalSize; }

        /**
         * Increase the amount of used data.
         */
        void increaseUsed(size_t size) { if (usedSize + size > totalSize) throw Underflow(); usedSize += size;  }

        /**
         * Splice this StreamBuffer into two StreamBuffers.
         *
         * @param size  Size of the data to remove from the beginning of this
         *              StreamBuffer.
         * @return      New StreamBuffer, holding the given amount of the
         *              original data.
         */
        StreamBuffer spliceHead(size_t size);

        /**
         * Remove the given amount of data from the beginning of this
         * StreamBuffer.
         *
         * @param size  Size of the data to remove from the beginning of this
         *              StreamBuffer.
         */
        void cutHead(size_t size);

        /** Pair holding iovec pointer and count. */
        typedef std::pair<struct iovec *, size_t> IOVecArray;

        /**
         * Build IO vector of the used data. The built IO vector can be used
         * with standard writev(2) and sendmsg(2) functions.
         *
         * \code
         * StreamBuffer sb;
         * ... add data into sb ...
         * StreamBuffer::IOVecArray v(sb.getUsedIOVec());
         * ssize_t ssize = writev(fd, v.fist(), v.second);
         * ... handle errors ...
         * sb.cutHead(ssize);
         * \endcode
         */
        IOVecArray getUsedIOVec();

        /**
         * Build IO vector of the free data. The built IO vector can be used
         * with standard readv(2) and recvmsg(2) functions.
         *
         * \code
         * StreamBuffer sb;
         * sb.allocate(goodGuessForSize);
         * StreamBuffer::IOVecArray v(sb.getFreeIOVec());
         * ssize_t ssize = readv(fd, v.first, v.second);
         * ... handle errors ...
         * sb.increaseUsed(ssize);
         * \endcode
         */
        IOVecArray getFreeIOVec();

        /**
         * Append the given data into this StreamBuffer. Allocation and
         * increaseUsed() is automatically handled.
         *
         * @param data  Pointer to the beginning of the data.
         * @param size  Size of the data.
         */
        void appendData(const void  *data,
                        size_t      size);

        /**
         * Wrapper for appending std::string data.
         */
        void appendData(const std::string &s) { appendData(s.data(), s.size()); }

        /**
         * Wrapper for appending C string data.
         *
         * @param s Normal '\0' terminated C string.
         */
        void appendData(const char *s) { appendData(std::string(s)); }

        /**
         * Template wrapper for appendData(const void *, size_t).
         */
        template <typename T>
        void appendData(const T &t) { appendData(&t, sizeof(T)); }

        /**
         * Append used data from another stream buffer.
         *
         * @param sb    Stream buffer whose used data to append.
         */
        void appendStreamBuffer(const StreamBuffer &sb);

        /**
         * Allocate and get mutable array. This function is useful if you have
         * a function which can be used to build a message into given array, but
         * you don't want to first build and then copy the data.
         *
         * @param size The size of the array to allocate.
         *
         * \code
         * StreamBuffer sb;
         * Object objectToBeSent;
         * ...
         * size_t size(objectToBeSent.byteSize());
         * objectToBeSent.serializeToArray(db.getMutableArray(size));
         * \endcode
         */
        void *getMutableArray(size_t size);

        /**
         * Linearize this StreamBuffer so that all used data is only in one IO
         * vector and its starting address is properly aligned.
         */
        void linearize();

        /**
         * Get data as the provided type. Linearization and alignment is
         * handled if needed, so this is safe to call for any kind of
         * data type.
         */
        template <typename T>
        const T &getData();

        /**
         * Get const array. This function is useful if you have a function
         * which can be used to parse a message from given array, but you don't
         * want to use memcpy or play around with the getData(). This function
         * linearizes this StreamBuffer if needed.
         *
         * @param offset    Offset from beginning of data.
         * @param size      The size of the array to expect.
         *
         * \code
         * StreamBuffer db;
         * ... fill db ...
         * uint32_t size(db.getData<uint32_t>()); // Protocol sends first 32-bit length
         * if (db.getUsedSize() >= sizeof(size) + size)
         * {
         *     Object obectReceived;
         *     objectReceived.parseFromArray(db.getConstArray(sizeof(size), size), size);
         *     ...
         * }
         * \endcode
         */
        const void *getConstArray(size_t    offset,
                                  size_t    size);

        virtual void dump(std::ostream &os) const;

    private:
        struct Buffer
        {
            boost::shared_array<uint64_t> base;
            void *data;
            size_t size;

            Buffer() = delete;

            explicit Buffer(size_t needSize);
        };

        typedef std::list<Buffer> Buffers;

        static inline void *incrementPointer(void   *ptr,
                                             size_t size)
        {
            return static_cast<void *>(static_cast<char *>(ptr) + size);
        }

        /**
         * first: Iterator to buffer.
         * second: Offset in buffer.
         *
         * Use the static helper functions for accessing.
         */
        typedef std::pair<Buffers::iterator, size_t> Location;

        static Buffers::iterator pointedBuffer(const Location &l) { return l.first; }

        static size_t offSetInBuffer(const Location &l) { return l.second; }

        static size_t sizeLeftInBuffer(const Location &l) { return l.first->size - l.second; }

        static void *pointerInBuffer(const Location &l) { return incrementPointer(l.first->data, l.second); }

        void *getMutableArrayNoFreeBuffers(size_t size);

        void *getMutableArrayFirstUnusedBufferTooSmall(const Location   &location,
                                                       size_t           size);

        void forceLinearize();

        Location findLocation(size_t size) const;

        Location findFirstUnused() const;

        void allocateIOVecArray();

        Buffers buffers;
        size_t usedSize;
        size_t totalSize;
        boost::scoped_array<struct iovec> iovecArray;
        size_t iovecArraySize;
    };

    template <typename T>
    const T &StreamBuffer::getData()
    {
        if (usedSize < sizeof(T))
        {
            throw Overflow();
        }
        if ((buffers.front().size < sizeof(T)) ||
            ((((uintptr_t)buffers.front().data) % boost::alignment_of<T>()) != 0))
        {
            forceLinearize();
        }
        return *static_cast<const T *>(buffers.front().data);
    }
}

#endif /* !EVENTLOOP_STREAMBUFFER_HEADER */
