// Copyright (C) mavericksforever
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Polyfill for C++17 exception types missing from libc++ on macOS < 10.14.
// This file is compiled with -mmacosx-version-min=10.15 and SKIP_PRECOMPILE_HEADERS
// so that the compiler emits symbols for these types.

#if defined(__APPLE__)

#include <optional>
#include <variant>

// --- bad_optional_access ---
// what() is inline in <optional>, so compiler won't emit vtable from it.
// Provide everything via assembly.

const char *std::bad_optional_access::what() const noexcept { return "bad optional access"; }

__asm__(".globl __ZNSt19bad_optional_accessD0Ev\n"
        ".globl __ZNSt19bad_optional_accessD1Ev\n"
        ".globl __ZNSt19bad_optional_accessD2Ev\n"
        "__ZNSt19bad_optional_accessD0Ev:\n"
        "__ZNSt19bad_optional_accessD1Ev:\n"
        "__ZNSt19bad_optional_accessD2Ev:\n"
        "  ret\n");

__asm__(".section __DATA,__const\n"
        ".globl __ZTSSt19bad_optional_access\n"
        "__ZTSSt19bad_optional_access:\n"
        "  .asciz \"St19bad_optional_access\"\n");

__asm__(".section __DATA,__const\n"
        ".globl __ZTISt19bad_optional_access\n"
        ".p2align 3\n"
        "__ZTISt19bad_optional_access:\n"
        "  .quad __ZTVN10__cxxabiv120__si_class_type_infoE + 16\n"
        "  .quad __ZTSSt19bad_optional_access\n"
        "  .quad __ZTISt9exception\n");

__asm__(".section __DATA,__const\n"
        ".globl __ZTVSt19bad_optional_access\n"
        ".p2align 3\n"
        "__ZTVSt19bad_optional_access:\n"
        "  .quad 0\n"
        "  .quad __ZTISt19bad_optional_access\n"
        "  .quad __ZNSt19bad_optional_accessD1Ev\n"
        "  .quad __ZNSt19bad_optional_accessD0Ev\n"
        "  .quad __ZNKSt19bad_optional_access4whatEv\n");

// --- bad_variant_access ---
// what() is the key function here — compiler emits vtable/typeinfo from it
const char *std::bad_variant_access::what() const noexcept { return "bad variant access"; }

#endif
