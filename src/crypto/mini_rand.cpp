/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Kernel32 entropy. CryptGenRandom is optional and resolved at runtime so
 * advapi32 never becomes a static import (it is not guaranteed on NT 4).
 */

#ifndef WINVER
#define WINVER 0x0400
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x04000000
#endif

#include "mini_openssl.h"

#include <windows.h>
#include <stdint.h>

static uint32_t rng_state[4] = {0xA3C59AC3u, 0x6F372C1Du, 0x19E8B2A5u, 0xC0FFEE11u};
static int rng_ready = 0;

typedef int (WINAPI *CryptAcquireContextA_fn)(uintptr_t *prov, const char *container, const char *provider, unsigned long type, unsigned long flags);
typedef int (WINAPI *CryptGenRandom_fn)(uintptr_t prov, unsigned long len, unsigned char *buf);
typedef int (WINAPI *CryptReleaseContext_fn)(uintptr_t prov, unsigned long flags);

static uint32_t rng_step(void) {
    uint32_t t = rng_state[3];
    uint32_t s = rng_state[0];
    rng_state[3] = rng_state[2];
    rng_state[2] = rng_state[1];
    rng_state[1] = s;
    t ^= t << 11;
    t ^= t >> 8;
    rng_state[0] = t ^ s ^ (s >> 19);
    return rng_state[0];
}

static void rng_mix(uint32_t extra) {
    rng_state[0] ^= extra;
    rng_state[1] ^= extra * 0x9E3779B9u;
    rng_step();
    rng_step();
}

static void rng_stir(void) {
    LARGE_INTEGER counter;
    SYSTEMTIME time;
    counter.QuadPart = 0;
    QueryPerformanceCounter(&counter);
    GetSystemTime(&time);
    rng_mix(GetTickCount());
    rng_mix((uint32_t)counter.LowPart);
    rng_mix((uint32_t)counter.HighPart);
    rng_mix(((uint32_t)time.wMilliseconds << 16) ^ ((uint32_t)time.wSecond << 8) ^ time.wMinute);
    rng_mix(GetCurrentProcessId() ^ ((uint32_t)GetCurrentThreadId() << 16));
}

static int rng_capi(unsigned char *buf, int num) {
    HMODULE advapi;
    CryptAcquireContextA_fn acquire;
    CryptGenRandom_fn generate;
    CryptReleaseContext_fn release;
    uintptr_t prov = 0;
    int ok;
    advapi = LoadLibraryA("advapi32.dll");
    if (!advapi) {
        return 0;
    }
    acquire = (CryptAcquireContextA_fn)GetProcAddress(advapi, "CryptAcquireContextA");
    generate = (CryptGenRandom_fn)GetProcAddress(advapi, "CryptGenRandom");
    release = (CryptReleaseContext_fn)GetProcAddress(advapi, "CryptReleaseContext");
    if (!acquire || !generate || !release) {
        FreeLibrary(advapi);
        return 0;
    }
    /* PROV_RSA_FULL = 1, CRYPT_VERIFYCONTEXT = 0xF0000000 */
    if (!acquire(&prov, NULL, NULL, 1, 0xF0000000u)) {
        FreeLibrary(advapi);
        return 0;
    }
    ok = generate(prov, (unsigned long)num, buf);
    release(prov, 0);
    FreeLibrary(advapi);
    return ok ? 1 : 0;
}

int RAND_bytes(unsigned char *buf, int num) {
    unsigned char extra[256];
    int filled = 0;
    int i;
    if (!buf || num < 0) {
        return 0;
    }
    if (!rng_ready) {
        rng_stir();
        rng_ready = 1;
    }
    rng_stir();
    if (num <= (int)sizeof(extra) && rng_capi(extra, num)) {
        for (i = 0; i < num; i++) {
            buf[i] = (unsigned char)(extra[i] ^ (rng_step() & 0xFF));
        }
        return 1;
    }
    for (i = 0; i < num; i++) {
        if ((i & 3) == 0) {
            filled = (int)rng_step();
        }
        buf[i] = (unsigned char)(filled & 0xFF);
        filled >>= 8;
    }
    return 1;
}
