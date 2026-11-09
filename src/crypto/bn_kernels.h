/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef UMSKT_BN_KERNELS_H
#define UMSKT_BN_KERNELS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void umskt_mont_mul_scalar(uint32_t *r, const uint32_t *a, const uint32_t *b, const uint32_t *n, uint32_t n0, int nw);
void umskt_bn_mul_limbs_scalar(uint32_t *r, const uint32_t *a, int an, const uint32_t *b, int bn);

#if defined(UMSKT_HAVE_SSE2_KERNELS)
void umskt_mont_mul_sse2(uint32_t *r, const uint32_t *a, const uint32_t *b, const uint32_t *n, uint32_t n0, int nw);
void umskt_bn_mul_limbs_sse2(uint32_t *r, const uint32_t *a, int an, const uint32_t *b, int bn);
#endif

#if defined(UMSKT_HAVE_AVX2_KERNELS)
void umskt_mont_mul_avx2(uint32_t *r, const uint32_t *a, const uint32_t *b, const uint32_t *n, uint32_t n0, int nw);
void umskt_bn_mul_limbs_avx2(uint32_t *r, const uint32_t *a, int an, const uint32_t *b, int bn);
#endif

void umskt_bn_dispatch_init(void);

#ifdef __cplusplus
}
#endif

#endif
