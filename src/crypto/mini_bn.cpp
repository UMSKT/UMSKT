/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Schoolbook bignum. Not constant-time: the Windows x86 image must stay
 * free of CMOV and SIMD, and UMSKT does not need a side-channel barrier.
 */

#include "mini_openssl.h"

#include <stdlib.h>
#include <string.h>

struct bignum_st {
    uint32_t *d;
    int top;
    int dmax;
    int neg;
};

struct bn_ctx_st {
    int unused;
};

static int bn_wexpand(BIGNUM *a, int words) {
    int next;
    uint32_t *grown;
    if (words <= a->dmax) {
        return 1;
    }
    next = a->dmax > 0 ? a->dmax : 4;
    while (next < words) {
        next *= 2;
    }
    grown = (uint32_t *)realloc(a->d, (size_t)next * sizeof(uint32_t));
    if (!grown) {
        return 0;
    }
    memset(grown + a->dmax, 0, (size_t)(next - a->dmax) * sizeof(uint32_t));
    a->d = grown;
    a->dmax = next;
    return 1;
}

static void bn_fix_top(BIGNUM *a) {
    while (a->top > 0 && a->d[a->top - 1] == 0) {
        a->top--;
    }
    if (a->top == 0) {
        a->neg = 0;
    }
}

BIGNUM *BN_new(void) {
    BIGNUM *a = (BIGNUM *)malloc(sizeof(BIGNUM));
    if (!a) {
        return NULL;
    }
    a->d = (uint32_t *)calloc(4, sizeof(uint32_t));
    if (!a->d) {
        free(a);
        return NULL;
    }
    a->dmax = 4;
    a->top = 0;
    a->neg = 0;
    return a;
}

void BN_free(BIGNUM *a) {
    if (!a) {
        return;
    }
    free(a->d);
    free(a);
}

BIGNUM *BN_dup(const BIGNUM *a) {
    BIGNUM *r = BN_new();
    if (!r) {
        return NULL;
    }
    if (!BN_copy(r, a)) {
        BN_free(r);
        return NULL;
    }
    return r;
}

int BN_zero(BIGNUM *a) {
    a->top = 0;
    a->neg = 0;
    return 1;
}

int BN_is_zero(const BIGNUM *a) {
    return a->top == 0;
}

int BN_set_word(BIGNUM *a, BN_ULONG w) {
    if (w == 0) {
        return BN_zero(a);
    }
    if (!bn_wexpand(a, 1)) {
        return 0;
    }
    a->d[0] = w;
    a->top = 1;
    a->neg = 0;
    return 1;
}

int BN_copy(BIGNUM *to, const BIGNUM *from) {
    int i;
    if (to == from) {
        return 1;
    }
    if (!bn_wexpand(to, from->top > 0 ? from->top : 1)) {
        return 0;
    }
    for (i = 0; i < from->top; i++) {
        to->d[i] = from->d[i];
    }
    to->top = from->top;
    to->neg = from->neg;
    return 1;
}

int BN_ucmp(const BIGNUM *a, const BIGNUM *b) {
    int i;
    if (a->top != b->top) {
        return a->top > b->top ? 1 : -1;
    }
    for (i = a->top - 1; i >= 0; i--) {
        if (a->d[i] != b->d[i]) {
            return a->d[i] > b->d[i] ? 1 : -1;
        }
    }
    return 0;
}

int BN_is_odd(const BIGNUM *a) {
    return a->top > 0 && (a->d[0] & 1u);
}

int BN_num_bits(const BIGNUM *a) {
    uint32_t word;
    int bits;
    if (a->top == 0) {
        return 0;
    }
    word = a->d[a->top - 1];
    bits = (a->top - 1) * 32;
    while (word) {
        bits++;
        word >>= 1;
    }
    return bits;
}

int BN_num_bytes(const BIGNUM *a) {
    return (BN_num_bits(a) + 7) / 8;
}

int BN_set_bit(BIGNUM *a, int n) {
    int word = n / 32;
    int bit = n % 32;
    int i;
    if (n < 0) {
        return 0;
    }
    if (!bn_wexpand(a, word + 1)) {
        return 0;
    }
    for (i = a->top; i <= word; i++) {
        a->d[i] = 0;
    }
    if (a->top <= word) {
        a->top = word + 1;
    }
    a->d[word] |= 1u << bit;
    return 1;
}

int BN_is_bit_set(const BIGNUM *a, int n) {
    int word = n / 32;
    if (n < 0 || word >= a->top) {
        return 0;
    }
    return (a->d[word] >> (n % 32)) & 1;
}

int BN_mask_bits(BIGNUM *a, int n) {
    int words;
    int bits;
    if (n < 0) {
        return 0;
    }
    if (n == 0) {
        return BN_zero(a);
    }
    words = n / 32;
    bits = n % 32;
    if (words >= a->top) {
        return 1;
    }
    if (bits == 0) {
        a->top = words;
    } else {
        a->d[words] &= (1u << bits) - 1u;
        a->top = words + 1;
    }
    bn_fix_top(a);
    return 1;
}

static int bn_lshift_raw(BIGNUM *r, const BIGNUM *a, int n) {
    int words;
    int bits;
    int i;
    uint32_t carry;
    if (a->top == 0 || n == 0) {
        return BN_copy(r, a);
    }
    words = n / 32;
    bits = n % 32;
    if (!bn_wexpand(r, a->top + words + 1)) {
        return 0;
    }
    for (i = 0; i < a->top + words + 1; i++) {
        r->d[i] = 0;
    }
    if (bits == 0) {
        for (i = 0; i < a->top; i++) {
            r->d[i + words] = a->d[i];
        }
        r->top = a->top + words;
    } else {
        carry = 0;
        for (i = 0; i < a->top; i++) {
            uint32_t value = a->d[i];
            r->d[i + words] = (value << bits) | carry;
            carry = value >> (32 - bits);
        }
        r->d[a->top + words] = carry;
        r->top = a->top + words + (carry ? 1 : 0);
    }
    r->neg = a->neg;
    bn_fix_top(r);
    return 1;
}

int BN_lshift(BIGNUM *r, const BIGNUM *a, int n) {
    BIGNUM *tmp;
    int ok;
    if (n < 0) {
        return 0;
    }
    if (r != a) {
        return bn_lshift_raw(r, a, n);
    }
    tmp = BN_dup(a);
    if (!tmp) {
        return 0;
    }
    ok = bn_lshift_raw(r, tmp, n);
    BN_free(tmp);
    return ok;
}

int BN_rshift1(BIGNUM *r, const BIGNUM *a) {
    int i;
    uint32_t carry = 0;
    if (r != a && !BN_copy(r, a)) {
        return 0;
    }
    for (i = r->top - 1; i >= 0; i--) {
        uint32_t next = r->d[i] & 1u;
        r->d[i] = (r->d[i] >> 1) | (carry << 31);
        carry = next;
    }
    bn_fix_top(r);
    return 1;
}

static int bn_uadd(BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    const BIGNUM *longer = a;
    const BIGNUM *shorter = b;
    int i;
    uint32_t carry = 0;
    if (longer->top < shorter->top) {
        longer = b;
        shorter = a;
    }
    if (!bn_wexpand(r, longer->top + 1)) {
        return 0;
    }
    for (i = 0; i < shorter->top; i++) {
        uint64_t sum = (uint64_t)longer->d[i] + shorter->d[i] + carry;
        r->d[i] = (uint32_t)sum;
        carry = (uint32_t)(sum >> 32);
    }
    for (; i < longer->top; i++) {
        uint64_t sum = (uint64_t)longer->d[i] + carry;
        r->d[i] = (uint32_t)sum;
        carry = (uint32_t)(sum >> 32);
    }
    r->d[i] = carry;
    r->top = longer->top + (carry ? 1 : 0);
    r->neg = 0;
    bn_fix_top(r);
    return 1;
}

static int bn_usub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    BIGNUM *tmp;
    int i;
    uint32_t borrow = 0;
    if (r == a || r == b) {
        int ok;
        tmp = BN_new();
        if (!tmp) {
            return 0;
        }
        ok = bn_usub(tmp, a, b);
        if (ok) {
            ok = BN_copy(r, tmp);
        }
        BN_free(tmp);
        return ok;
    }
    if (!bn_wexpand(r, a->top > 0 ? a->top : 1)) {
        return 0;
    }
    for (i = 0; i < b->top; i++) {
        uint32_t av = a->d[i];
        uint32_t diff = av - b->d[i];
        uint32_t borrow1 = av < b->d[i];
        uint32_t diff2 = diff - borrow;
        uint32_t borrow2 = diff < borrow;
        r->d[i] = diff2;
        borrow = borrow1 | borrow2;
    }
    for (; i < a->top; i++) {
        uint32_t av = a->d[i];
        r->d[i] = av - borrow;
        borrow = av < borrow;
    }
    r->top = a->top;
    r->neg = 0;
    bn_fix_top(r);
    return 1;
}

int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    BIGNUM *left;
    BIGNUM *right;
    int ok;
    if (r == a || r == b) {
        left = BN_dup(a);
        right = BN_dup(b);
        ok = left && right && BN_add(r, left, right);
        BN_free(left);
        BN_free(right);
        return ok;
    }
    if (a->neg == b->neg) {
        ok = bn_uadd(r, a, b);
        r->neg = a->neg && r->top != 0;
        return ok;
    }
    if (BN_ucmp(a, b) >= 0) {
        ok = bn_usub(r, a, b);
        r->neg = a->neg && r->top != 0;
    } else {
        ok = bn_usub(r, b, a);
        r->neg = b->neg && r->top != 0;
    }
    return ok;
}

int BN_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    BIGNUM *flipped;
    int ok;
    if (r == a || r == b) {
        BIGNUM *left = BN_dup(a);
        BIGNUM *right = BN_dup(b);
        ok = left && right && BN_sub(r, left, right);
        BN_free(left);
        BN_free(right);
        return ok;
    }
    flipped = BN_dup(b);
    if (!flipped) {
        return 0;
    }
    flipped->neg = b->top != 0 ? !b->neg : 0;
    ok = BN_add(r, a, flipped);
    BN_free(flipped);
    return ok;
}

int BN_mul_word(BIGNUM *a, BN_ULONG w) {
    int i;
    uint32_t carry = 0;
    if (w == 0 || a->top == 0) {
        return BN_zero(a);
    }
    for (i = 0; i < a->top; i++) {
        uint64_t prod = (uint64_t)a->d[i] * w + carry;
        a->d[i] = (uint32_t)prod;
        carry = (uint32_t)(prod >> 32);
    }
    if (carry) {
        if (!bn_wexpand(a, a->top + 1)) {
            return 0;
        }
        a->d[a->top++] = carry;
    }
    return 1;
}

int BN_add_word(BIGNUM *a, BN_ULONG w) {
    int i;
    uint32_t carry;
    if (w == 0) {
        return 1;
    }
    if (a->neg) {
        BIGNUM *word = BN_new();
        int ok;
        if (!word) {
            return 0;
        }
        BN_set_word(word, w);
        ok = BN_sub(a, a, word);
        BN_free(word);
        return ok;
    }
    if (a->top == 0) {
        return BN_set_word(a, w);
    }
    carry = w;
    for (i = 0; i < a->top && carry; i++) {
        uint64_t sum = (uint64_t)a->d[i] + carry;
        a->d[i] = (uint32_t)sum;
        carry = (uint32_t)(sum >> 32);
    }
    if (carry) {
        if (!bn_wexpand(a, a->top + 1)) {
            return 0;
        }
        a->d[a->top++] = carry;
    }
    return 1;
}

int BN_sub_word(BIGNUM *a, BN_ULONG w) {
    int i;
    uint32_t borrow;
    if (w == 0) {
        return 1;
    }
    if (a->top == 0 || a->neg) {
        BIGNUM *word = BN_new();
        int ok;
        if (!word) {
            return 0;
        }
        BN_set_word(word, w);
        ok = BN_sub(a, a, word);
        BN_free(word);
        return ok;
    }
    if (a->top == 1 && a->d[0] < w) {
        a->d[0] = w - a->d[0];
        a->neg = 1;
        return 1;
    }
    borrow = w;
    for (i = 0; i < a->top && borrow; i++) {
        uint32_t value = a->d[i];
        a->d[i] = value - borrow;
        borrow = value < borrow;
    }
    bn_fix_top(a);
    return 1;
}

BN_ULONG BN_div_word(BIGNUM *a, BN_ULONG w) {
    int i;
    uint32_t rem = 0;
    if (w == 0) {
        return (BN_ULONG)-1;
    }
    for (i = a->top - 1; i >= 0; i--) {
        uint64_t cur = ((uint64_t)rem << 32) | a->d[i];
        a->d[i] = (uint32_t)(cur / w);
        rem = (uint32_t)(cur % w);
    }
    bn_fix_top(a);
    return rem;
}

static int bn_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    int i;
    int j;
    int top = a->top + b->top;
    if (a->top == 0 || b->top == 0) {
        return BN_zero(r);
    }
    if (!bn_wexpand(r, top + 1)) {
        return 0;
    }
    for (i = 0; i < top + 1; i++) {
        r->d[i] = 0;
    }
    for (i = 0; i < a->top; i++) {
        uint32_t carry = 0;
        for (j = 0; j < b->top; j++) {
            union {
                uint64_t v;
                uint32_t p[2];
            } acc;
            acc.v = (uint64_t)a->d[i] * b->d[j] + r->d[i + j] + carry;
            r->d[i + j] = acc.p[0];
            carry = acc.p[1];
        }
        r->d[i + b->top] = carry;
    }
    r->top = top;
    r->neg = (a->neg ^ b->neg) && 1;
    bn_fix_top(r);
    return 1;
}

static int bn_mod(BIGNUM *r, const BIGNUM *a, const BIGNUM *m) {
    BIGNUM *shifted;
    int max_shift;
    int shift;
    if (BN_is_zero(m)) {
        return 0;
    }
    if (r != a && !BN_copy(r, a)) {
        return 0;
    }
    r->neg = 0;
    if (BN_ucmp(r, m) < 0) {
        return 1;
    }
    shifted = BN_new();
    if (!shifted) {
        return 0;
    }
    max_shift = BN_num_bits(r) - BN_num_bits(m);
    if (!BN_lshift(shifted, m, max_shift)) {
        BN_free(shifted);
        return 0;
    }
    for (shift = max_shift; shift >= 0; shift--) {
        if (BN_ucmp(r, shifted) >= 0) {
            if (!bn_usub(r, r, shifted)) {
                BN_free(shifted);
                return 0;
            }
        }
        if (shift > 0 && !BN_rshift1(shifted, shifted)) {
            BN_free(shifted);
            return 0;
        }
    }
    BN_free(shifted);
    return 1;
}

static int bn_mod_abs(BIGNUM *r, const BIGNUM *a, const BIGNUM *m) {
    BIGNUM *tmp;
    int ok;
    if (!a->neg) {
        return bn_mod(r, a, m);
    }
    tmp = BN_dup(a);
    if (!tmp) {
        return 0;
    }
    tmp->neg = 0;
    ok = bn_mod(tmp, tmp, m);
    if (ok && !BN_is_zero(tmp)) {
        ok = bn_usub(r, m, tmp);
    } else if (ok) {
        ok = BN_zero(r);
    }
    BN_free(tmp);
    return ok;
}

int BN_mod_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m, BN_CTX *ctx) {
    BIGNUM *sum;
    int ok;
    (void)ctx;
    sum = BN_new();
    if (!sum) {
        return 0;
    }
    ok = BN_add(sum, a, b) && bn_mod_abs(r, sum, m);
    BN_free(sum);
    return ok;
}

int BN_mod_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m, BN_CTX *ctx) {
    BIGNUM *diff;
    int ok;
    (void)ctx;
    diff = BN_new();
    if (!diff) {
        return 0;
    }
    ok = BN_sub(diff, a, b) && bn_mod_abs(r, diff, m);
    BN_free(diff);
    return ok;
}

int BN_mod_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m, BN_CTX *ctx) {
    BIGNUM *prod;
    int ok;
    (void)ctx;
    prod = BN_new();
    if (!prod) {
        return 0;
    }
    ok = bn_mul(prod, a, b) && bn_mod_abs(r, prod, m);
    BN_free(prod);
    return ok;
}

int BN_mod_sqr(BIGNUM *r, const BIGNUM *a, const BIGNUM *m, BN_CTX *ctx) {
    return BN_mod_mul(r, a, a, m, ctx);
}

int BN_mod_inverse(BIGNUM *r, const BIGNUM *a, const BIGNUM *m) {
    BIGNUM *u;
    BIGNUM *v;
    BIGNUM *x;
    BIGNUM *y;
    BIGNUM *tmp;
    int ok;
    int guard;
    if (!r || !a || !m || BN_is_zero(m) || (m->d[0] & 1u) == 0) {
        return 0;
    }
    u = BN_new();
    v = BN_dup(m);
    x = BN_new();
    y = BN_new();
    tmp = BN_new();
    ok = u && v && x && y && tmp && BN_set_word(x, 1) && BN_zero(y) && bn_mod_abs(u, a, m);
    if (ok && BN_is_zero(u)) {
        ok = 0;
    }
    guard = BN_num_bits(m) * 4 + 8;
    while (ok && !BN_is_zero(u) && guard-- > 0) {
        while (ok && !BN_is_zero(u) && !BN_is_odd(u)) {
            ok = BN_rshift1(u, u);
            if (ok && BN_is_odd(x)) {
                ok = bn_uadd(tmp, x, m) && BN_rshift1(x, tmp);
            } else if (ok) {
                ok = BN_rshift1(x, x);
            }
        }
        while (ok && !BN_is_odd(v)) {
            ok = BN_rshift1(v, v);
            if (ok && BN_is_odd(y)) {
                ok = bn_uadd(tmp, y, m) && BN_rshift1(y, tmp);
            } else if (ok) {
                ok = BN_rshift1(y, y);
            }
        }
        if (!ok || BN_is_zero(u)) {
            break;
        }
        if (BN_ucmp(u, v) >= 0) {
            ok = bn_usub(tmp, u, v) && BN_copy(u, tmp);
            if (ok && BN_ucmp(x, y) >= 0) {
                ok = bn_usub(tmp, x, y) && BN_copy(x, tmp);
            } else if (ok) {
                ok = bn_usub(tmp, y, x) && bn_usub(x, m, tmp);
            }
        } else {
            ok = bn_usub(tmp, v, u) && BN_copy(v, tmp);
            if (ok && BN_ucmp(y, x) >= 0) {
                ok = bn_usub(tmp, y, x) && BN_copy(y, tmp);
            } else if (ok) {
                ok = bn_usub(tmp, x, y) && bn_usub(y, m, tmp);
            }
        }
    }
    if (ok && (BN_is_zero(u) == 0 || v->top != 1 || v->d[0] != 1)) {
        ok = 0;
    }
    if (ok) {
        ok = BN_copy(r, y);
    }
    BN_free(u);
    BN_free(v);
    BN_free(x);
    BN_free(y);
    BN_free(tmp);
    return ok;
}

static uint32_t mont_n0(uint32_t n0) {
    uint32_t inv = n0;
    inv *= 2u - n0 * inv;
    inv *= 2u - n0 * inv;
    inv *= 2u - n0 * inv;
    inv *= 2u - n0 * inv;
    inv *= 2u - n0 * inv;
    return 0u - inv;
}

static void mont_load(uint32_t *dst, const BIGNUM *src, int nw) {
    int i;
    for (i = 0; i < nw; i++) {
        dst[i] = (src && i < src->top) ? src->d[i] : 0;
    }
}

static void mont_mul(uint32_t *r, const uint32_t *a, const uint32_t *b, const uint32_t *n, uint32_t n0, int nw) {
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

static int bn_to_mont_rr(BIGNUM *rr, const BIGNUM *m, int nw) {
    return BN_set_word(rr, 1) && BN_lshift(rr, rr, nw * 64) && bn_mod(rr, rr, m);
}

static int bn_mod_exp(BIGNUM *r, const BIGNUM *base, const BIGNUM *exp, const BIGNUM *m) {
    BIGNUM *reduced;
    BIGNUM *rr;
    uint32_t n[32];
    uint32_t base_m[32];
    uint32_t acc[32];
    uint32_t cur[32];
    uint32_t rr_w[32];
    uint32_t n0;
    int nw;
    int bits;
    int i;
    int ok;
    if (!m || m->top == 0 || m->top > 32 || (m->d[0] & 1u) == 0) {
        return 0;
    }
    if (BN_is_zero(exp)) {
        return BN_set_word(r, 1);
    }
    nw = m->top;
    reduced = BN_new();
    rr = BN_new();
    ok = reduced && rr && bn_mod(reduced, base, m) && bn_to_mont_rr(rr, m, nw);
    if (!ok) {
        BN_free(reduced);
        BN_free(rr);
        return 0;
    }
    mont_load(n, m, nw);
    mont_load(rr_w, rr, nw);
    mont_load(base_m, reduced, nw);
    n0 = mont_n0(n[0]);
    mont_mul(base_m, base_m, rr_w, n, n0, nw);
    mont_mul(acc, rr_w, rr_w, n, n0, nw);
    /* acc was R^2 * R^2 * R^{-1} = R^3. Montgomery form of 1 is R, from 1 * R^2 * R^{-1}. */
    {
        uint32_t one[32];
        for (i = 0; i < nw; i++) {
            one[i] = 0;
        }
        one[0] = 1;
        mont_mul(acc, one, rr_w, n, n0, nw);
    }
    bits = BN_num_bits(exp);
    for (i = bits - 1; i >= 0; i--) {
        mont_mul(cur, acc, acc, n, n0, nw);
        if (BN_is_bit_set(exp, i)) {
            mont_mul(acc, cur, base_m, n, n0, nw);
        } else {
            for (int w = 0; w < nw; w++) {
                acc[w] = cur[w];
            }
        }
    }
    {
        uint32_t one[32];
        for (i = 0; i < nw; i++) {
            one[i] = i == 0 ? 1u : 0u;
        }
        mont_mul(cur, acc, one, n, n0, nw);
    }
    if (!bn_wexpand(r, nw)) {
        BN_free(reduced);
        BN_free(rr);
        return 0;
    }
    for (i = 0; i < nw; i++) {
        r->d[i] = cur[i];
    }
    r->top = nw;
    r->neg = 0;
    bn_fix_top(r);
    BN_free(reduced);
    BN_free(rr);
    return 1;
}

static int bn_legendre_is_one(const BIGNUM *a, const BIGNUM *p) {
    BIGNUM *exp = BN_dup(p);
    BIGNUM *result = BN_new();
    int status = -1;
    if (!exp || !result || !BN_sub_word(exp, 1) || !BN_rshift1(exp, exp)) {
        BN_free(exp);
        BN_free(result);
        return -1;
    }
    if (!bn_mod_exp(result, a, exp, p)) {
        BN_free(exp);
        BN_free(result);
        return -1;
    }
    if (BN_is_zero(result)) {
        status = 0;
    } else if (result->top == 1 && result->d[0] == 1 && !result->neg) {
        status = 1;
    } else {
        status = -1;
    }
    BN_free(exp);
    BN_free(result);
    return status;
}

BIGNUM *BN_mod_sqrt(BIGNUM *r, const BIGNUM *a, const BIGNUM *p, BN_CTX *ctx) {
    BIGNUM *aa;
    BIGNUM *out;
    int legendre;
    (void)ctx;
    if (!r) {
        r = BN_new();
        if (!r) {
            return NULL;
        }
    }
    aa = BN_new();
    out = BN_new();
    if (!aa || !out || !bn_mod(aa, a, p)) {
        BN_free(aa);
        BN_free(out);
        return NULL;
    }
    if (BN_is_zero(aa)) {
        BN_zero(r);
        BN_free(aa);
        BN_free(out);
        return r;
    }
    legendre = bn_legendre_is_one(aa, p);
    if (legendre != 1) {
        BN_free(aa);
        BN_free(out);
        return NULL;
    }
    if ((p->d[0] & 3u) == 3u) {
        BIGNUM *exp = BN_dup(p);
        int ok = exp && BN_add_word(exp, 1) && BN_rshift1(exp, exp) && BN_rshift1(exp, exp) && bn_mod_exp(out, aa, exp, p);
        BN_free(exp);
        if (!ok) {
            BN_free(aa);
            BN_free(out);
            return NULL;
        }
    } else {
        /* Tonelli-Shanks. p - 1 = q * 2^s, q odd. */
        BIGNUM *q = BN_dup(p);
        BIGNUM *z = BN_new();
        BIGNUM *c = BN_new();
        BIGNUM *t = BN_new();
        BIGNUM *root = BN_new();
        BIGNUM *b = BN_new();
        BIGNUM *one = BN_new();
        int s = 0;
        int m;
        int ok = q && z && c && t && root && b && one;
        if (!ok) {
            BN_free(aa);
            BN_free(out);
            BN_free(q);
            BN_free(z);
            BN_free(c);
            BN_free(t);
            BN_free(root);
            BN_free(b);
            BN_free(one);
            return NULL;
        }
        BN_sub_word(q, 1);
        while (!BN_is_odd(q)) {
            BN_rshift1(q, q);
            s++;
        }
        BN_set_word(one, 1);
        BN_set_word(z, 2);
        while (bn_legendre_is_one(z, p) != -1) {
            BN_add_word(z, 1);
        }
        m = s;
        ok = bn_mod_exp(c, z, q, p);
        if (ok) {
            BIGNUM *half = BN_dup(q);
            ok = half && BN_add_word(half, 1) && BN_rshift1(half, half) && bn_mod_exp(root, aa, half, p) && bn_mod_exp(t, aa, q, p);
            BN_free(half);
        }
        while (ok) {
            int i = 0;
            BIGNUM *ti = BN_dup(t);
            if (!ti) {
                ok = 0;
                break;
            }
            while (ok && BN_ucmp(ti, one) != 0 && i < m) {
                ok = BN_mod_sqr(ti, ti, p, NULL);
                i++;
            }
            BN_free(ti);
            if (!ok || i == m) {
                ok = 0;
                break;
            }
            if (i == 0) {
                BN_copy(out, root);
                break;
            }
            {
                BIGNUM *shift = BN_new();
                if (!shift) {
                    ok = 0;
                    break;
                }
                BN_set_word(shift, 1);
                if (m - i - 1 > 0) {
                    ok = BN_lshift(shift, shift, m - i - 1);
                }
                /* b = c^(2^(m-i-1)) */
                if (ok) {
                    ok = bn_mod_exp(b, c, shift, p);
                }
                BN_free(shift);
            }
            if (!ok) {
                break;
            }
            ok = BN_mod_mul(root, root, b, p, NULL) && BN_mod_sqr(b, b, p, NULL) && BN_mod_mul(t, t, b, p, NULL);
            BN_copy(c, b);
            m = i;
        }
        BN_free(q);
        BN_free(z);
        BN_free(c);
        BN_free(t);
        BN_free(root);
        BN_free(b);
        BN_free(one);
        if (!ok || BN_is_zero(out)) {
            BN_free(aa);
            BN_free(out);
            return NULL;
        }
    }
    {
        BIGNUM *check = BN_new();
        int ok = check && BN_mod_sqr(check, out, p, NULL) && BN_ucmp(check, aa) == 0;
        BN_free(check);
        BN_free(aa);
        if (!ok) {
            BN_free(out);
            return NULL;
        }
    }
    BN_copy(r, out);
    BN_free(out);
    return r;
}

int BN_dec2bn(BIGNUM **a, const char *str) {
    int digits = 0;
    if (!a) {
        return 0;
    }
    if (!*a) {
        *a = BN_new();
        if (!*a) {
            return 0;
        }
    }
    BN_zero(*a);
    if (!str) {
        return 0;
    }
    if (*str == '+') {
        str++;
    }
    while (*str == '0') {
        str++;
        digits++;
    }
    if (*str < '0' || *str > '9') {
        return digits;
    }
    while (*str >= '0' && *str <= '9') {
        if (!BN_mul_word(*a, 10) || !BN_add_word(*a, (BN_ULONG)(*str - '0'))) {
            return 0;
        }
        str++;
        digits++;
    }
    return digits;
}

char *BN_bn2dec(const BIGNUM *a) {
    BIGNUM *tmp;
    int cap;
    int count = 0;
    int neg;
    int i;
    char *buf;
    if (BN_is_zero(a)) {
        buf = (char *)malloc(2);
        if (buf) {
            buf[0] = '0';
            buf[1] = 0;
        }
        return buf;
    }
    tmp = BN_dup(a);
    if (!tmp) {
        return NULL;
    }
    neg = tmp->neg;
    tmp->neg = 0;
    cap = BN_num_bits(tmp) / 3 + 4 + neg;
    buf = (char *)malloc((size_t)cap);
    if (!buf) {
        BN_free(tmp);
        return NULL;
    }
    while (!BN_is_zero(tmp)) {
        BN_ULONG rem = BN_div_word(tmp, 10);
        buf[count++] = (char)('0' + rem);
    }
    if (neg) {
        buf[count++] = '-';
    }
    for (i = 0; i < count / 2; i++) {
        char swap = buf[i];
        buf[i] = buf[count - 1 - i];
        buf[count - 1 - i] = swap;
    }
    buf[count] = 0;
    BN_free(tmp);
    return buf;
}

int BN_bn2bin(const BIGNUM *a, unsigned char *to) {
    int n = BN_num_bytes(a);
    int i;
    int b;
    memset(to, 0, (size_t)n);
    for (i = 0; i < a->top; i++) {
        uint32_t value = a->d[i];
        for (b = 0; b < 4; b++) {
            int index = n - 1 - (i * 4 + b);
            if (index >= 0 && index < n) {
                to[index] = (unsigned char)((value >> (8 * b)) & 0xFF);
            }
        }
    }
    return n;
}

BIGNUM *BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret) {
    int words;
    int i;
    if (!ret) {
        ret = BN_new();
        if (!ret) {
            return NULL;
        }
    }
    BN_zero(ret);
    if (!s || len <= 0) {
        return ret;
    }
    words = (len + 3) / 4;
    if (!bn_wexpand(ret, words)) {
        return NULL;
    }
    for (i = 0; i < words; i++) {
        ret->d[i] = 0;
    }
    ret->top = words;
    for (i = 0; i < len; i++) {
        int from_right = len - 1 - i;
        ret->d[from_right / 4] |= ((uint32_t)s[i]) << ((from_right % 4) * 8);
    }
    bn_fix_top(ret);
    return ret;
}

BIGNUM *BN_lebin2bn(const unsigned char *s, int len, BIGNUM *ret) {
    int words;
    int i;
    if (!ret) {
        ret = BN_new();
        if (!ret) {
            return NULL;
        }
    }
    BN_zero(ret);
    if (!s || len <= 0) {
        return ret;
    }
    words = (len + 3) / 4;
    if (!bn_wexpand(ret, words)) {
        return NULL;
    }
    for (i = 0; i < words; i++) {
        ret->d[i] = 0;
    }
    ret->top = words;
    for (i = 0; i < len; i++) {
        ret->d[i / 4] |= ((uint32_t)s[i]) << ((i % 4) * 8);
    }
    bn_fix_top(ret);
    return ret;
}

int BN_bn2lebinpad(const BIGNUM *a, unsigned char *to, int tolen) {
    int i;
    int b;
    if (tolen < 0 || BN_num_bytes(a) > tolen) {
        return -1;
    }
    memset(to, 0, (size_t)tolen);
    for (i = 0; i < a->top; i++) {
        uint32_t value = a->d[i];
        for (b = 0; b < 4; b++) {
            int index = i * 4 + b;
            if (index < tolen) {
                to[index] = (unsigned char)((value >> (8 * b)) & 0xFF);
            }
        }
    }
    return tolen;
}

int BN_rand(BIGNUM *rnd, int bits, int top, int bottom) {
    int bytes;
    int excess;
    unsigned char *buf;
    if (!rnd || bits < 0 || (bits == 1 && top > 0)) {
        return 0;
    }
    if (bits == 0) {
        return BN_zero(rnd);
    }
    bytes = (bits + 7) / 8;
    excess = bytes * 8 - bits;
    buf = (unsigned char *)malloc((size_t)bytes);
    if (!buf) {
        return 0;
    }
    if (RAND_bytes(buf, bytes) != 1) {
        free(buf);
        return 0;
    }
    buf[0] = (unsigned char)(buf[0] & (0xFFu >> excess));
    if (top == BN_RAND_TOP_ONE || top == BN_RAND_TOP_TWO) {
        int bit = (bits - 1) % 8;
        buf[0] = (unsigned char)(buf[0] | (1u << bit));
        if (top == BN_RAND_TOP_TWO && bits > 1) {
            if (bit == 0) {
                buf[1] = (unsigned char)(buf[1] | 0x80);
            } else {
                buf[0] = (unsigned char)(buf[0] | (1u << (bit - 1)));
            }
        }
    }
    if (bottom == BN_RAND_BOTTOM_ODD) {
        buf[bytes - 1] = (unsigned char)(buf[bytes - 1] | 1u);
    }
    if (!BN_bin2bn(buf, bytes, rnd)) {
        free(buf);
        return 0;
    }
    free(buf);
    return 1;
}

BN_CTX *BN_CTX_new(void) {
    BN_CTX *ctx = (BN_CTX *)malloc(sizeof(BN_CTX));
    if (ctx) {
        ctx->unused = 0;
    }
    return ctx;
}

void BN_CTX_free(BN_CTX *ctx) {
    free(ctx);
}
