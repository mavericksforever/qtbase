// Copyright (C) mavericksforever
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Polyfills for C/C++ runtime symbols missing on older macOS:
// - __availability_version_check (10.15+ runtime, needed for @available)
// - clock_gettime, getentropy, clonefile, futimens, renameatx_np (10.12-10.13+)
// - aligned new/delete (10.13+)

#include <new>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

#if defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101500

// __availability_version_check is used by @available() on macOS 10.15+.
// On older systems, provide a polyfill that checks the OS version via sysctl.
typedef struct {
    uint32_t count;
    struct { uint32_t platform; uint32_t version[3]; } entries[];
} _AvailabilityVersionCheckInfo;

extern "C" __attribute__((visibility("default")))
int __availability_version_check(uint32_t count, const uint32_t versions[])
{
    // Parse macOS version
    char osversion[32];
    size_t len = sizeof(osversion);
    if (sysctlbyname("kern.osproductversion", osversion, &len, NULL, 0) != 0) {
        // Fallback: assume 10.9
        osversion[0] = '\0';
        strlcpy(osversion, "10.9.0", sizeof(osversion));
    }

    unsigned major = 0, minor = 0, patch = 0;
    sscanf(osversion, "%u.%u.%u", &major, &minor, &patch);
    uint32_t current = (major << 16) | (minor << 8) | patch;

    // versions array: [platform, version, platform, version, ...]
    // platform 1 = macOS
    for (uint32_t i = 0; i + 1 < count * 2; i += 2) {
        if (versions[i] == 1) { // macOS
            uint32_t required = versions[i + 1];
            return current >= required;
        }
    }
    return 1; // Unknown platform, assume available
}

#endif // 101500

// __ulock_wait2 (macOS 11+) and __ulock_wake (10.12+) — used by libc++ mutex
// Provide stubs that fall back to usleep-based spinning
#include <unistd.h>
extern "C" __attribute__((visibility("default")))
int __ulock_wait2(uint32_t operation, void *addr, uint64_t value, uint64_t timeout, uint64_t value2)
{
    (void)operation; (void)addr; (void)value; (void)timeout; (void)value2;
    usleep(1000);
    return 0;
}

extern "C" __attribute__((visibility("default")))
int __ulock_wake(uint32_t operation, void *addr, uint64_t wake_value)
{
    (void)operation; (void)addr; (void)wake_value;
    return 0;
}

#if defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200

// Various POSIX functions added in macOS 10.12-10.13
#include <mach/mach_time.h>
#include <sys/time.h>

// futimens (10.13+) — fallback to utimes
extern "C" __attribute__((visibility("default")))
int futimens(int fd, const struct timespec times[2])
{
    struct timeval tv[2];
    if (times) {
        tv[0].tv_sec = times[0].tv_sec;
        tv[0].tv_usec = times[0].tv_nsec / 1000;
        tv[1].tv_sec = times[1].tv_sec;
        tv[1].tv_usec = times[1].tv_nsec / 1000;
    }
    return futimes(fd, times ? tv : NULL);
}

// renameatx_np (10.12+) — just fail, Qt has fallback
extern "C" __attribute__((visibility("default")))
int renameatx_np(int fromfd, const char *from, int tofd, const char *to, unsigned int flags)
{
    (void)fromfd; (void)from; (void)tofd; (void)to; (void)flags;
    errno = ENOTSUP;
    return -1;
}

// clonefile (10.12+) — just fail, Qt has fallback via __builtin_available
extern "C" __attribute__((visibility("default")))
int clonefile(const char *src, const char *dst, uint32_t flags)
{
    (void)src; (void)dst; (void)flags;
    errno = ENOTSUP;
    return -1;
}
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
