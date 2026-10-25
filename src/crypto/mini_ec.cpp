/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Short Weierstrass curves over a prime field. Scalar multiplication uses
 * Jacobian coordinates so a 384/512-bit multiply does one inversion.
 */

#include "mini_openssl.h"

#include <stdlib.h>
#include <string.h>

struct ec_group_st {
    BIGNUM *p;
    BIGNUM *a;
    BIGNUM *b;
};

struct ec_point_st {
    const EC_GROUP *group;
    BIGNUM *x;
    BIGNUM *y;
    int infinity;
};

struct jpoint {
    BIGNUM *X;
    BIGNUM *Y;
    BIGNUM *Z;
    int infinity;
};

static int field_mul(const EC_GROUP *g, BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    return BN_mod_mul(r, a, b, g->p, NULL);
}

static int field_sqr(const EC_GROUP *g, BIGNUM *r, const BIGNUM *a) {
    return BN_mod_sqr(r, a, g->p, NULL);
}

static int field_add(const EC_GROUP *g, BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    return BN_mod_add(r, a, b, g->p, NULL);
}

static int field_sub(const EC_GROUP *g, BIGNUM *r, const BIGNUM *a, const BIGNUM *b) {
    return BN_mod_sub(r, a, b, g->p, NULL);
}

static int field_reduce(const EC_GROUP *g, BIGNUM *r, const BIGNUM *a) {
    BIGNUM *zero = BN_new();
    int ok;
    if (!zero) {
        return 0;
    }
    BN_zero(zero);
    ok = BN_mod_add(r, a, zero, g->p, NULL);
    BN_free(zero);
    return ok;
}

static int field_mul_small(const EC_GROUP *g, BIGNUM *r, const BIGNUM *a, BN_ULONG w) {
    BIGNUM *tmp = BN_dup(a);
    int ok = tmp && BN_mul_word(tmp, w) && field_reduce(g, r, tmp);
    BN_free(tmp);
    return ok;
}

static int field_inv(const EC_GROUP *g, BIGNUM *r, const BIGNUM *a) {
    return BN_mod_inverse(r, a, g->p);
}

static void jfree(struct jpoint *p) {
    if (!p) {
        return;
    }
    BN_free(p->X);
    BN_free(p->Y);
    BN_free(p->Z);
}

static int jinit(struct jpoint *p) {
    p->X = BN_new();
    p->Y = BN_new();
    p->Z = BN_new();
    p->infinity = 1;
    if (!p->X || !p->Y || !p->Z) {
        jfree(p);
        return 0;
    }
    return 1;
}

static int jset_affine(const EC_GROUP *g, struct jpoint *p, const EC_POINT *src) {
    if (src->infinity) {
        p->infinity = 1;
        return 1;
    }
    p->infinity = 0;
    return field_reduce(g, p->X, src->x) && field_reduce(g, p->Y, src->y) && BN_set_word(p->Z, 1);
}

static int jdouble(const EC_GROUP *g, struct jpoint *p) {
    BIGNUM *YY;
    BIGNUM *YYYY;
    BIGNUM *XX;
    BIGNUM *S;
    BIGNUM *M;
    BIGNUM *Z2;
    BIGNUM *Z4;
    BIGNUM *X3;
    BIGNUM *Y3;
    BIGNUM *Z3;
    BIGNUM *tmp;
    int ok;
    if (p->infinity) {
        return 1;
    }
    if (BN_is_zero(p->Y)) {
        p->infinity = 1;
        return 1;
    }
    YY = BN_new();
    YYYY = BN_new();
    XX = BN_new();
    S = BN_new();
    M = BN_new();
    Z2 = BN_new();
    Z4 = BN_new();
    X3 = BN_new();
    Y3 = BN_new();
    Z3 = BN_new();
    tmp = BN_new();
    ok = YY && YYYY && XX && S && M && Z2 && Z4 && X3 && Y3 && Z3 && tmp;
    if (ok) {
        ok = field_sqr(g, YY, p->Y) && field_sqr(g, YYYY, YY) && field_sqr(g, XX, p->X);
    }
    if (ok) {
        ok = field_mul(g, S, p->X, YY) && field_mul_small(g, S, S, 4);
    }
    if (ok) {
        ok = field_mul_small(g, M, XX, 3) && field_sqr(g, Z2, p->Z) && field_sqr(g, Z4, Z2) && field_mul(g, tmp, g->a, Z4) && field_add(g, M, M, tmp);
    }
    if (ok) {
        ok = field_sqr(g, X3, M) && field_mul_small(g, tmp, S, 2) && field_sub(g, X3, X3, tmp);
    }
    if (ok) {
        ok = field_mul(g, Z3, p->Y, p->Z) && field_mul_small(g, Z3, Z3, 2);
    }
    if (ok) {
        ok = field_sub(g, tmp, S, X3) && field_mul(g, tmp, M, tmp) && field_mul_small(g, YYYY, YYYY, 8) && field_sub(g, Y3, tmp, YYYY);
    }
    if (ok) {
        ok = BN_copy(p->X, X3) && BN_copy(p->Y, Y3) && BN_copy(p->Z, Z3);
    }
    BN_free(YY);
    BN_free(YYYY);
    BN_free(XX);
    BN_free(S);
    BN_free(M);
    BN_free(Z2);
    BN_free(Z4);
    BN_free(X3);
    BN_free(Y3);
    BN_free(Z3);
    BN_free(tmp);
    return ok;
}

static int jadd_mixed(const EC_GROUP *g, struct jpoint *p, const EC_POINT *q) {
    BIGNUM *Z2;
    BIGNUM *Z3;
    BIGNUM *U2;
    BIGNUM *S2;
    BIGNUM *H;
    BIGNUM *R;
    BIGNUM *HH;
    BIGNUM *HHH;
    BIGNUM *U1HH;
    BIGNUM *X3;
    BIGNUM *Y3;
    BIGNUM *Zout;
    BIGNUM *tmp;
    int ok;
    if (q->infinity) {
        return 1;
    }
    if (p->infinity) {
        return jset_affine(g, p, q);
    }
    Z2 = BN_new();
    Z3 = BN_new();
    U2 = BN_new();
    S2 = BN_new();
    H = BN_new();
    R = BN_new();
    HH = BN_new();
    HHH = BN_new();
    U1HH = BN_new();
    X3 = BN_new();
    Y3 = BN_new();
    Zout = BN_new();
    tmp = BN_new();
    ok = Z2 && Z3 && U2 && S2 && H && R && HH && HHH && U1HH && X3 && Y3 && Zout && tmp;
    if (ok) {
        ok = field_sqr(g, Z2, p->Z) && field_mul(g, Z3, Z2, p->Z) && field_mul(g, U2, q->x, Z2) && field_mul(g, S2, q->y, Z3);
    }
    if (ok) {
        ok = field_sub(g, H, U2, p->X) && field_sub(g, R, S2, p->Y);
    }
    if (ok && BN_is_zero(H)) {
        int double_it = BN_is_zero(R);
        BN_free(Z2);
        BN_free(Z3);
        BN_free(U2);
        BN_free(S2);
        BN_free(H);
        BN_free(R);
        BN_free(HH);
        BN_free(HHH);
        BN_free(U1HH);
        BN_free(X3);
        BN_free(Y3);
        BN_free(Zout);
        BN_free(tmp);
        if (double_it) {
            return jdouble(g, p);
        }
        p->infinity = 1;
        return 1;
    }
    if (ok) {
        ok = field_sqr(g, HH, H) && field_mul(g, HHH, H, HH) && field_mul(g, U1HH, p->X, HH);
    }
    if (ok) {
        ok = field_sqr(g, X3, R) && field_sub(g, X3, X3, HHH) && field_mul_small(g, tmp, U1HH, 2) && field_sub(g, X3, X3, tmp);
    }
    if (ok) {
        ok = field_sub(g, tmp, U1HH, X3) && field_mul(g, tmp, R, tmp) && field_mul(g, Y3, p->Y, HHH) && field_sub(g, Y3, tmp, Y3);
    }
    if (ok) {
        ok = field_mul(g, Zout, p->Z, H) && BN_copy(p->X, X3) && BN_copy(p->Y, Y3) && BN_copy(p->Z, Zout);
    }
    BN_free(Z2);
    BN_free(Z3);
    BN_free(U2);
    BN_free(S2);
    BN_free(H);
    BN_free(R);
    BN_free(HH);
    BN_free(HHH);
    BN_free(U1HH);
    BN_free(X3);
    BN_free(Y3);
    BN_free(Zout);
    BN_free(tmp);
    return ok;
}

static int jto_affine(const EC_GROUP *g, EC_POINT *out, const struct jpoint *p) {
    BIGNUM *inv = BN_new();
    BIGNUM *z2 = BN_new();
    BIGNUM *z3 = BN_new();
    int ok = inv && z2 && z3;
    if (p->infinity || BN_is_zero(p->Z)) {
        out->infinity = 1;
        BN_free(inv);
        BN_free(z2);
        BN_free(z3);
        return 1;
    }
    if (ok) {
        ok = field_inv(g, inv, p->Z) && field_sqr(g, z2, inv) && field_mul(g, z3, z2, inv) && field_mul(g, out->x, p->X, z2) && field_mul(g, out->y, p->Y, z3);
    }
    if (ok) {
        out->infinity = 0;
    }
    BN_free(inv);
    BN_free(z2);
    BN_free(z3);
    return ok;
}

EC_GROUP *EC_GROUP_new_curve_GFp(const BIGNUM *p, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx) {
    EC_GROUP *group;
    (void)ctx;
    if (!p || BN_is_zero(p)) {
        return NULL;
    }
    group = (EC_GROUP *)malloc(sizeof(EC_GROUP));
    if (!group) {
        return NULL;
    }
    group->p = BN_dup(p);
    group->a = BN_dup(a);
    group->b = BN_dup(b);
    if (!group->p || !group->a || !group->b) {
        EC_GROUP_free(group);
        return NULL;
    }
    return group;
}

void EC_GROUP_free(EC_GROUP *group) {
    if (!group) {
        return;
    }
    BN_free(group->p);
    BN_free(group->a);
    BN_free(group->b);
    free(group);
}

EC_POINT *EC_POINT_new(const EC_GROUP *group) {
    EC_POINT *point = (EC_POINT *)malloc(sizeof(EC_POINT));
    if (!point) {
        return NULL;
    }
    point->group = group;
    point->x = BN_new();
    point->y = BN_new();
    point->infinity = 1;
    if (!point->x || !point->y) {
        EC_POINT_free(point);
        return NULL;
    }
    return point;
}

void EC_POINT_free(EC_POINT *point) {
    if (!point) {
        return;
    }
    BN_free(point->x);
    BN_free(point->y);
    free(point);
}

int EC_POINT_set_affine_coordinates(const EC_GROUP *group, EC_POINT *point, const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx) {
    (void)ctx;
    if (!group || !point || !x || !y) {
        return 0;
    }
    point->group = group;
    point->infinity = 0;
    return field_reduce(group, point->x, x) && field_reduce(group, point->y, y);
}

int EC_POINT_get_affine_coordinates(const EC_GROUP *group, const EC_POINT *point, BIGNUM *x, BIGNUM *y, BN_CTX *ctx) {
    (void)group;
    (void)ctx;
    if (!point || point->infinity) {
        return 0;
    }
    return BN_copy(x, point->x) && BN_copy(y, point->y);
}

int EC_POINT_is_on_curve(const EC_GROUP *group, const EC_POINT *point, BN_CTX *ctx) {
    BIGNUM *y2 = BN_new();
    BIGNUM *x3 = BN_new();
    BIGNUM *ax = BN_new();
    BIGNUM *rhs = BN_new();
    int ok;
    (void)ctx;
    if (!point || point->infinity) {
        BN_free(y2);
        BN_free(x3);
        BN_free(ax);
        BN_free(rhs);
        return 0;
    }
    ok = y2 && x3 && ax && rhs && field_sqr(group, y2, point->y) && field_sqr(group, x3, point->x) && field_mul(group, x3, x3, point->x) && field_mul(group, ax, group->a, point->x) && field_add(group, rhs, x3, ax) && field_add(group, rhs, rhs, group->b);
    if (ok) {
        ok = BN_ucmp(y2, rhs) == 0;
    }
    BN_free(y2);
    BN_free(x3);
    BN_free(ax);
    BN_free(rhs);
    return ok;
}

int EC_POINT_add(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, const EC_POINT *b, BN_CTX *ctx) {
    struct jpoint sum;
    int ok;
    (void)ctx;
    if (!jinit(&sum)) {
        return 0;
    }
    ok = jset_affine(group, &sum, a) && jadd_mixed(group, &sum, b) && jto_affine(group, r, &sum);
    jfree(&sum);
    return ok;
}

int EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx) {
    struct jpoint acc;
    int bits;
    int i;
    int ok;
    (void)ctx;
    if (n) {
        return 0;
    }
    if (!q || !m) {
        return 0;
    }
    if (q->infinity || BN_is_zero(m)) {
        r->infinity = 1;
        return 1;
    }
    if (!jinit(&acc)) {
        return 0;
    }
    bits = BN_num_bits(m);
    ok = 1;
    for (i = bits - 1; ok && i >= 0; i--) {
        if (!acc.infinity) {
            ok = jdouble(group, &acc);
        }
        if (ok && BN_is_bit_set(m, i)) {
            ok = jadd_mixed(group, &acc, q);
        }
    }
    if (ok) {
        ok = jto_affine(group, r, &acc);
    }
    jfree(&acc);
    return ok;
}

unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md) {
    uint32_t h0 = 0x67452301u;
    uint32_t h1 = 0xEFCDAB89u;
    uint32_t h2 = 0x98BADCFEu;
    uint32_t h3 = 0x10325476u;
    uint32_t h4 = 0xC3D2E1F0u;
    size_t bit_len_hi;
    size_t bit_len_lo;
    size_t total;
    size_t offset = 0;
    unsigned char block[64];
    size_t i;
    if (!md) {
        return NULL;
    }
    bit_len_lo = (n << 3) & 0xFFFFFFFFu;
    bit_len_hi = (n >> 29);
    while (offset + 64 <= n) {
        uint32_t w[80];
        uint32_t a, b, c, e, f, temp;
        uint32_t k;
        for (i = 0; i < 16; i++) {
            w[i] = ((uint32_t)d[offset + i * 4] << 24) | ((uint32_t)d[offset + i * 4 + 1] << 16) | ((uint32_t)d[offset + i * 4 + 2] << 8) | (uint32_t)d[offset + i * 4 + 3];
        }
        for (i = 16; i < 80; i++) {
            uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (x << 1) | (x >> 31);
        }
        a = h0;
        b = h1;
        c = h2;
        e = h3;
        f = h4;
        for (i = 0; i < 80; i++) {
            uint32_t fmix;
            if (i < 20) {
                fmix = (b & c) | ((~b) & e);
                k = 0x5A827999u;
            } else if (i < 40) {
                fmix = b ^ c ^ e;
                k = 0x6ED9EBA1u;
            } else if (i < 60) {
                fmix = (b & c) | (b & e) | (c & e);
                k = 0x8F1BBCDCu;
            } else {
                fmix = b ^ c ^ e;
                k = 0xCA62C1D6u;
            }
            temp = ((a << 5) | (a >> 27)) + fmix + f + k + w[i];
            f = e;
            e = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += e;
        h4 += f;
        offset += 64;
    }
    total = n - offset;
    memset(block, 0, sizeof(block));
    if (total) {
        memcpy(block, d + offset, total);
    }
    block[total] = 0x80;
    if (total >= 56) {
        uint32_t w[80];
        uint32_t a, b, c, e, f, temp;
        for (i = 0; i < 16; i++) {
            w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) | ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
        }
        for (i = 16; i < 80; i++) {
            uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (x << 1) | (x >> 31);
        }
        a = h0;
        b = h1;
        c = h2;
        e = h3;
        f = h4;
        for (i = 0; i < 80; i++) {
            uint32_t fmix;
            uint32_t k;
            if (i < 20) {
                fmix = (b & c) | ((~b) & e);
                k = 0x5A827999u;
            } else if (i < 40) {
                fmix = b ^ c ^ e;
                k = 0x6ED9EBA1u;
            } else if (i < 60) {
                fmix = (b & c) | (b & e) | (c & e);
                k = 0x8F1BBCDCu;
            } else {
                fmix = b ^ c ^ e;
                k = 0xCA62C1D6u;
            }
            temp = ((a << 5) | (a >> 27)) + fmix + f + k + w[i];
            f = e;
            e = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += e;
        h4 += f;
        memset(block, 0, sizeof(block));
    }
    block[56] = (unsigned char)((bit_len_hi >> 24) & 0xFF);
    block[57] = (unsigned char)((bit_len_hi >> 16) & 0xFF);
    block[58] = (unsigned char)((bit_len_hi >> 8) & 0xFF);
    block[59] = (unsigned char)(bit_len_hi & 0xFF);
    block[60] = (unsigned char)((bit_len_lo >> 24) & 0xFF);
    block[61] = (unsigned char)((bit_len_lo >> 16) & 0xFF);
    block[62] = (unsigned char)((bit_len_lo >> 8) & 0xFF);
    block[63] = (unsigned char)(bit_len_lo & 0xFF);
    {
        uint32_t w[80];
        uint32_t a, b, c, e, f, temp;
        for (i = 0; i < 16; i++) {
            w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) | ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
        }
        for (i = 16; i < 80; i++) {
            uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (x << 1) | (x >> 31);
        }
        a = h0;
        b = h1;
        c = h2;
        e = h3;
        f = h4;
        for (i = 0; i < 80; i++) {
            uint32_t fmix;
            uint32_t k;
            if (i < 20) {
                fmix = (b & c) | ((~b) & e);
                k = 0x5A827999u;
            } else if (i < 40) {
                fmix = b ^ c ^ e;
                k = 0x6ED9EBA1u;
            } else if (i < 60) {
                fmix = (b & c) | (b & e) | (c & e);
                k = 0x8F1BBCDCu;
            } else {
                fmix = b ^ c ^ e;
                k = 0xCA62C1D6u;
            }
            temp = ((a << 5) | (a >> 27)) + fmix + f + k + w[i];
            f = e;
            e = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += e;
        h4 += f;
    }
    md[0] = (unsigned char)(h0 >> 24);
    md[1] = (unsigned char)(h0 >> 16);
    md[2] = (unsigned char)(h0 >> 8);
    md[3] = (unsigned char)h0;
    md[4] = (unsigned char)(h1 >> 24);
    md[5] = (unsigned char)(h1 >> 16);
    md[6] = (unsigned char)(h1 >> 8);
    md[7] = (unsigned char)h1;
    md[8] = (unsigned char)(h2 >> 24);
    md[9] = (unsigned char)(h2 >> 16);
    md[10] = (unsigned char)(h2 >> 8);
    md[11] = (unsigned char)h2;
    md[12] = (unsigned char)(h3 >> 24);
    md[13] = (unsigned char)(h3 >> 16);
    md[14] = (unsigned char)(h3 >> 8);
    md[15] = (unsigned char)h3;
    md[16] = (unsigned char)(h4 >> 24);
    md[17] = (unsigned char)(h4 >> 16);
    md[18] = (unsigned char)(h4 >> 8);
    md[19] = (unsigned char)h4;
    return md;
}
