#ifndef EVENTLOOP_FILEDESCRIPTOR_HEADER
#define EVENTLOOP_FILEDESCRIPTOR_HEADER

#include <iosfwd>

#include <EventLoop/Exception.hpp>

namespace EventLoop
{
    class System;

    /**
     * Simple wrapper for 'int file descriptor'. Takes care of calling close
     * once fd is no longer needed and makes sure invalid fd usage is noticed
     * by throwing InvalidFD.
     *
     * Example use case:
     * \code
     * void writeIntoFile(const char *path, void *data, size_t size)
     * {
     *     FileDescriptor fd(open(path, O_WRONLY));
     *     if (!fd)
     *     {
     *         throw SystemException("open");
     *     }
     *     if (write(fd, data, size) < 0)
     *     {
     *         throw SystemException("write");
     *     }
     * }
     * \endcode
     */
    class FileDescriptor
    {
    public:
        /** Exception for invalid file descriptor. */
        class InvalidFD: public Exception
        {
        public:
            InvalidFD(): Exception("invalid fd") { }
        };

        /**
         * Create invalid (-1) file descriptor.
         */
        FileDescriptor() noexcept;

        /**
         * Create file descriptor from the given raw fd.
         *
         * @param fd Raw fd.
         */
        explicit FileDescriptor(int fd) noexcept;

        /**
         * Create invalid (-1) file descriptor.
         */
        explicit FileDescriptor(System &system) noexcept;

        /**
         * Create file descriptor from the given raw fd.
         *
         * @param fd Raw fd.
         */
        FileDescriptor(System &system, int fd) noexcept;

        /**
         * Close the file descriptor if valid.
         */
        ~FileDescriptor() noexcept;

        FileDescriptor(const FileDescriptor &) = delete;

        FileDescriptor &operator = (const FileDescriptor &) = delete;

        /**
         * Close the file descriptor.
         *
         * @exception InvalidFD if the file descriptor is invalid.
         */
        void close();

        /**
         * Release the file descriptor and return it as raw fd.
         *
         * @return  Raw fd.
         *
         * @exception InvalidFD if the file descriptor is invalid.
         */
        int release();

        /**
         * Swap file descriptors held by this and the other objects.
         */
        void swap(FileDescriptor &other);

        /**
         * Check if the file descriptor is valid.
         *
         * @return  True, if the file descriptor is valid, otherwise false.
         *
         * @note This function can be used for checking whether this file
         *       descriptor is valid or not:
         * \code
         * FileDescriptor fd(openFile());
         * if (fd)
         * {
         *     fd is valid
         * }
         * else
         * {
         *     fd is invalid
         * }
         * \endcode
         */
        explicit operator bool () const noexcept { return fd >= 0; }

        /**
         * Convert the file descriptor to raw fd but keep ownership.
         *
         * @return  Raw fd (always equal or greater than zero).
         *
         * @exception InvalidFD if the file descriptor is invalid.
         */
        operator int () const { if (fd < 0) throw InvalidFD(); return fd; }

        /**
         * Take ownership of the given raw fd. If this file descriptor was
         * already valid, then this file descriptor is first closed.
         *
         * @param fd Raw fd.
         */
        FileDescriptor & operator = (int fd) noexcept;

        bool operator == (const FileDescriptor &fd) const noexcept { return this->fd == fd.fd; }

        bool operator == (int fd) const noexcept { return this->fd == fd; }

        bool operator != (const FileDescriptor &fd) const noexcept { return this->fd != fd.fd; }

        bool operator != (int fd) const noexcept { return this->fd != fd; }

        bool operator < (const FileDescriptor &fd) const noexcept { return this->fd < fd.fd; }

        bool operator < (int fd) const noexcept { return this->fd < fd; }

        bool operator <= (const FileDescriptor &fd) const noexcept { return this->fd <= fd.fd; }

        bool operator <= (int fd) const noexcept { return this->fd <= fd; }

        bool operator > (const FileDescriptor &fd) const noexcept { return this->fd > fd.fd; }

        bool operator > (int fd) const noexcept { return this->fd > fd; }

        bool operator >= (const FileDescriptor &fd) const noexcept { return this->fd >= fd.fd; }

        bool operator >= (int fd) const noexcept { return this->fd >= fd; }

    private:
        System &system;
        int fd;

        friend bool operator == (int raw, const FileDescriptor &fd);

        friend bool operator != (int raw, const FileDescriptor &fd);

        friend bool operator < (int raw, const FileDescriptor &fd);

        friend bool operator <= (int raw, const FileDescriptor &fd);

        friend bool operator > (int raw, const FileDescriptor &fd);

        friend bool operator >= (int raw, const FileDescriptor &fd);

        friend std::ostream & operator << (std::ostream         &out,
                                           const FileDescriptor &fd);
    };

    inline bool operator == (int raw, const FileDescriptor &fd)
    {
        return raw == fd.fd;
    }

    inline bool operator != (int raw, const FileDescriptor &fd)
    {
        return raw != fd.fd;
    }

    inline bool operator < (int raw, const FileDescriptor &fd)
    {
        return raw < fd.fd;
    }

    inline bool operator <= (int raw, const FileDescriptor &fd)
    {
        return raw <= fd.fd;
    }

    inline bool operator > (int raw, const FileDescriptor &fd)
    {
        return raw > fd.fd;
    }

    inline bool operator >= (int raw, const FileDescriptor &fd)
    {
        return raw >= fd.fd;
    }

    std::ostream & operator << (std::ostream            &out,
                                const FileDescriptor    &fd);
}

#endif /* !EVENTLOOP_FILEDESCRIPTOR_HEADER */
