// Copyright (C) mavericksforever
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Polyfills for C/C++ runtime symbols missing on older macOS:
// - getentropy (10.12+)
// - aligned new/delete (10.13+)

#include <new>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#if defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200

// getentropy was added in macOS 10.12; fallback to /dev/urandom
// clock_gettime was added in macOS 10.12; fallback to gettimeofday/mach_absolute_time
#include <mach/mach_time.h>
#include <sys/time.h>
#include <time.h>

extern "C" __attribute__((visibility("default")))
int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    if (clk_id == CLOCK_REALTIME) {
        struct timeval tv;
        if (gettimeofday(&tv, nullptr) != 0)
            return -1;
        tp->tv_sec = tv.tv_sec;
        tp->tv_nsec = tv.tv_usec * 1000;
        return 0;
    } else if (clk_id == CLOCK_MONOTONIC) {
        static mach_timebase_info_data_t info = {};
        if (info.denom == 0)
            mach_timebase_info(&info);
        uint64_t t = mach_absolute_time();
        uint64_t ns = t * info.numer / info.denom;
        tp->tv_sec = ns / 1000000000ULL;
        tp->tv_nsec = ns % 1000000000ULL;
        return 0;
    }
    errno = EINVAL;
    return -1;
}

// getentropy was added in macOS 10.12; fallback to /dev/urandom
extern "C" __attribute__((visibility("default"))) int getentropy(void *buf, size_t buflen)
{
    if (buflen > 256) {
        errno = EIO;
        return -1;
    }
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return -1;
    size_t total = 0;
    while (total < buflen) {
        ssize_t n = read(fd, static_cast<char *>(buf) + total, buflen - total);
        if (n <= 0) {
            close(fd);
            errno = EIO;
            return -1;
        }
        total += n;
    }
    close(fd);
    return 0;
}

#endif // 101200

#if defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101300

void *operator new(std::size_t size, std::align_val_t alignment)
{
    void *ptr = nullptr;
    if (posix_memalign(&ptr, static_cast<size_t>(alignment), size) != 0)
        throw std::bad_alloc();
    return ptr;
}

void *operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept
{
    void *ptr = nullptr;
    if (posix_memalign(&ptr, static_cast<size_t>(alignment), size) != 0)
        return nullptr;
    return ptr;
}

void *operator new[](std::size_t size, std::align_val_t alignment)
{
    return operator new(size, alignment);
}

void *operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t &tag) noexcept
{
    return operator new(size, alignment, tag);
}

void operator delete(void *ptr, std::align_val_t) noexcept
{
    free(ptr);
}

void operator delete(void *ptr, std::size_t, std::align_val_t) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr, std::align_val_t alignment) noexcept
{
    operator delete(ptr, alignment);
}

void operator delete[](void *ptr, std::size_t size, std::align_val_t alignment) noexcept
{
    operator delete(ptr, size, alignment);
}

#endif
