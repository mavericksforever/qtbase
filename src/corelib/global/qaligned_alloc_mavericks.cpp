// Copyright (C) mavericksforever
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Polyfills for C++/compiler-rt symbols missing on older macOS.
// C/POSIX polyfills (clock_gettime, getentropy, openat, clonefile, etc.)
// are provided by macports-legacy-support library instead.

#include <new>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/sysctl.h>

#if defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101500

// __availability_version_check is used by @available() on macOS 10.15+.
// On older systems, provide a polyfill that checks the OS version via sysctl.
extern "C" __attribute__((visibility("default")))
int __availability_version_check(uint32_t count, const uint32_t versions[])
{
    char osversion[32];
    size_t len = sizeof(osversion);
    if (sysctlbyname("kern.osproductversion", osversion, &len, NULL, 0) != 0)
        strlcpy(osversion, "10.9.0", sizeof(osversion));

    unsigned major = 0, minor = 0, patch = 0;
    sscanf(osversion, "%u.%u.%u", &major, &minor, &patch);
    uint32_t current = (major << 16) | (minor << 8) | patch;

    // versions array: [platform, version, platform, version, ...]
    // platform 1 = macOS
    for (uint32_t i = 0; i + 1 < count * 2; i += 2) {
        if (versions[i] == 1) { // macOS
            return current >= versions[i + 1];
        }
    }
    return 1; // Unknown platform, assume available
}

#endif // 101500

// __ulock_wait2 (macOS 11+) and __ulock_wake (10.12+) — used by libc++ mutex
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

// renameatx_np (10.12+) — not in macports-legacy-support
extern "C" __attribute__((visibility("default")))
int renameatx_np(int fromfd, const char *from, int tofd, const char *to, unsigned int flags)
{
    (void)fromfd; (void)from; (void)tofd; (void)to; (void)flags;
    errno = ENOTSUP;
    return -1;
}

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
