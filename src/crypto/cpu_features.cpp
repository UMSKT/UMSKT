/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "cpu_features.h"

#if defined(UMSKT_FREESTANDING) || defined(_WIN32)
#ifndef WINVER
#define WINVER 0x0400
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>
#ifndef PF_XMMI64_INSTRUCTIONS_AVAILABLE
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE 10
#endif
#ifndef PF_AVX2_INSTRUCTIONS_AVAILABLE
#define PF_AVX2_INSTRUCTIONS_AVAILABLE 40
#endif
#endif

static int g_cpu_ready = 0;
static int g_has_sse2 = 0;
static int g_has_avx2 = 0;

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)

#if defined(__GNUC__) || defined(__clang__)
static int cpuid_supported(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return 1;
#else
    unsigned int original;
    unsigned int modified;
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        "movl %0, %1\n\t"
        "xorl $0x200000, %0\n\t"
        "pushl %0\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %0\n\t"
        "pushl %1\n\t"
        "popfl\n\t"
        : "=&r"(modified), "=&r"(original)
        :
        : "cc");
    return ((original ^ modified) & 0x200000u) != 0;
#endif
}

static void do_cpuid(unsigned int leaf, unsigned int subleaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    __asm__ __volatile__(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf));
}

static unsigned long long do_xgetbv(unsigned int index) {
    unsigned int eax;
    unsigned int edx;
    __asm__ __volatile__(
        ".byte 0x0f, 0x01, 0xd0"
        : "=a"(eax), "=d"(edx)
        : "c"(index));
    return ((unsigned long long)edx << 32) | eax;
}
#else
static int cpuid_supported(void) {
    return 0;
}

static void do_cpuid(unsigned int leaf, unsigned int subleaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    (void)leaf;
    (void)subleaf;
    *eax = *ebx = *ecx = *edx = 0;
}

static unsigned long long do_xgetbv(unsigned int index) {
    (void)index;
    return 0;
}
#endif

static int cpu_has_sse2_bit(void) {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    if (!cpuid_supported()) {
        return 0;
    }
    do_cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax < 1) {
        return 0;
    }
    do_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (edx & (1u << 26)) != 0;
}

static int cpu_has_avx2_bits(void) {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned long long xcr0;
    if (!cpuid_supported()) {
        return 0;
    }
    do_cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax < 1) {
        return 0;
    }
    do_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    /* OSXSAVE (27) and AVX (28) */
    if ((ecx & ((1u << 27) | (1u << 28))) != ((1u << 27) | (1u << 28))) {
        return 0;
    }
    xcr0 = do_xgetbv(0);
    /* XMM (bit 1) and YMM (bit 2) state must be enabled by the OS. */
    if ((xcr0 & 0x6ull) != 0x6ull) {
        return 0;
    }
    do_cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax < 7) {
        return 0;
    }
    do_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    return (ebx & (1u << 5)) != 0;
}

static int os_supports_sse2(void) {
#if defined(UMSKT_FREESTANDING) || defined(_WIN32)
    return IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) ? 1 : 0;
#else
    return 1;
#endif
}

static int os_supports_avx2(void) {
#if defined(UMSKT_FREESTANDING) || defined(_WIN32)
    return IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE) ? 1 : 0;
#else
    return 1;
#endif
}

#else

static int cpu_has_sse2_bit(void) {
    return 0;
}

static int cpu_has_avx2_bits(void) {
    return 0;
}

static int os_supports_sse2(void) {
    return 0;
}

static int os_supports_avx2(void) {
    return 0;
}

#endif

void umskt_cpu_init(void) {
    if (g_cpu_ready) {
        return;
    }
    g_has_sse2 = cpu_has_sse2_bit() && os_supports_sse2();
    g_has_avx2 = cpu_has_avx2_bits() && os_supports_avx2();
    g_cpu_ready = 1;
}

int umskt_cpu_has_sse2(void) {
    umskt_cpu_init();
    return g_has_sse2;
}

int umskt_cpu_has_avx2(void) {
    umskt_cpu_init();
    return g_has_avx2;
}
