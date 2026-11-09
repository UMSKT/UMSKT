/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Shared BN hot kernels. Included from scalar and SSE2 translation units
 * with BN_KERNEL_SUFFIX set to scalar or sse2.
 */

#ifndef BN_KERNEL_SUFFIX
#error "BN_KERNEL_SUFFIX must be defined before including bn_kernels.inc.cpp"
#endif

#define BN_KERNEL_PASTE2(a, b) a##_##b
#define BN_KERNEL_PASTE(a, b) BN_KERNEL_PASTE2(a, b)
#define BN_KERNEL_FN(name) BN_KERNEL_PASTE(name, BN_KERNEL_SUFFIX)

void BN_KERNEL_FN(umskt_mont_mul)(uint32_t *r, const uint32_t *a, const uint32_t *b, const uint32_t *n, uint32_t n0, int nw) {
    uint32_t tmp[34];
    int i;
    int j;
    int ge;
    for (i = 0; i < nw + 2; i++) {
        tmp[i] = 0;
    }
    for (i = 0; i < nw; i++) {
        uint64_t carry = 0;
        uint32_t m;
        for (j = 0; j < nw; j++) {
            carry += (uint64_t)tmp[j] + (uint64_t)a[j] * b[i];
            tmp[j] = (uint32_t)carry;
            carry >>= 32;
        }
        carry += tmp[nw];
        tmp[nw] = (uint32_t)carry;
        tmp[nw + 1] = (uint32_t)(carry >> 32);
        m = tmp[0] * n0;
        carry = 0;
        for (j = 0; j < nw; j++) {
            carry += (uint64_t)tmp[j] + (uint64_t)m * n[j];
            tmp[j] = (uint32_t)carry;
            carry >>= 32;
        }
        carry += tmp[nw];
        tmp[nw] = (uint32_t)carry;
        tmp[nw + 1] += (uint32_t)(carry >> 32);
        for (j = 0; j <= nw; j++) {
            tmp[j] = tmp[j + 1];
        }
        tmp[nw + 1] = 0;
    }
    ge = tmp[nw] != 0;
    if (!ge) {
        for (i = nw - 1; i >= 0; i--) {
            if (tmp[i] != n[i]) {
                ge = tmp[i] > n[i];
                break;
            }
            if (i == 0) {
                ge = 1;
            }
        }
    }
    if (ge) {
        uint32_t borrow = 0;
        for (i = 0; i < nw; i++) {
            uint64_t sub = (uint64_t)n[i] + borrow;
            r[i] = (uint32_t)((uint64_t)tmp[i] - sub);
            borrow = (uint64_t)tmp[i] < sub;
        }
    } else {
        for (i = 0; i < nw; i++) {
            r[i] = tmp[i];
        }
    }
}

void BN_KERNEL_FN(umskt_bn_mul_limbs)(uint32_t *r, const uint32_t *a, int an, const uint32_t *b, int bn) {
    int i;
    int j;
    int top = an + bn;
    for (i = 0; i < top + 1; i++) {
        r[i] = 0;
    }
    for (i = 0; i < an; i++) {
        uint32_t carry = 0;
        for (j = 0; j < bn; j++) {
            union {
                uint64_t v;
                uint32_t p[2];
            } acc;
            acc.v = (uint64_t)a[i] * b[j] + r[i + j] + carry;
            r[i + j] = acc.p[0];
            carry = acc.p[1];
        }
        r[i + bn] = carry;
    }
}

#undef BN_KERNEL_FN
#undef BN_KERNEL_PASTE
#undef BN_KERNEL_PASTE2
