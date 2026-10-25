/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Closed OpenSSL-compatible surface used by the freestanding Windows build.
 * Only the BN / EC / SHA-1 calls UMSKT actually makes are declared here.
 */

#ifndef UMSKT_MINI_OPENSSL_H
#define UMSKT_MINI_OPENSSL_H

#include <stddef.h>
#include <stdint.h>

#define BN_RAND_TOP_ANY    (-1)
#define BN_RAND_TOP_ONE    0
#define BN_RAND_TOP_TWO    1
#define BN_RAND_BOTTOM_ANY 0
#define BN_RAND_BOTTOM_ODD 1

#define SHA_DIGEST_LENGTH 20

typedef uint32_t BN_ULONG;

struct bignum_st;
struct bn_ctx_st;
struct ec_group_st;
struct ec_point_st;

typedef struct bignum_st BIGNUM;
typedef struct bn_ctx_st BN_CTX;
typedef struct ec_group_st EC_GROUP;
typedef struct ec_point_st EC_POINT;

BIGNUM *BN_new(void);
BIGNUM *BN_dup(const BIGNUM *a);
void BN_free(BIGNUM *a);
int BN_zero(BIGNUM *a);
int BN_set_word(BIGNUM *a, BN_ULONG w);
int BN_copy(BIGNUM *to, const BIGNUM *from);
int BN_ucmp(const BIGNUM *a, const BIGNUM *b);
int BN_is_zero(const BIGNUM *a);
int BN_is_odd(const BIGNUM *a);
int BN_num_bits(const BIGNUM *a);
int BN_num_bytes(const BIGNUM *a);
int BN_set_bit(BIGNUM *a, int n);
int BN_is_bit_set(const BIGNUM *a, int n);
int BN_mask_bits(BIGNUM *a, int n);
int BN_lshift(BIGNUM *r, const BIGNUM *a, int n);
int BN_rshift1(BIGNUM *r, const BIGNUM *a);
int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b);
int BN_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b);
int BN_mul_word(BIGNUM *a, BN_ULONG w);
int BN_add_word(BIGNUM *a, BN_ULONG w);
int BN_sub_word(BIGNUM *a, BN_ULONG w);
BN_ULONG BN_div_word(BIGNUM *a, BN_ULONG w);
int BN_dec2bn(BIGNUM **a, const char *str);
char *BN_bn2dec(const BIGNUM *a);
int BN_bn2bin(const BIGNUM *a, unsigned char *to);
BIGNUM *BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret);
BIGNUM *BN_lebin2bn(const unsigned char *s, int len, BIGNUM *ret);
int BN_bn2lebinpad(const BIGNUM *a, unsigned char *to, int tolen);
int BN_mod_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m, BN_CTX *ctx);
int BN_mod_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m, BN_CTX *ctx);
int BN_mod_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m, BN_CTX *ctx);
int BN_mod_sqr(BIGNUM *r, const BIGNUM *a, const BIGNUM *m, BN_CTX *ctx);
BIGNUM *BN_mod_sqrt(BIGNUM *r, const BIGNUM *a, const BIGNUM *p, BN_CTX *ctx);
int BN_rand(BIGNUM *rnd, int bits, int top, int bottom);

BN_CTX *BN_CTX_new(void);
void BN_CTX_free(BN_CTX *ctx);

EC_GROUP *EC_GROUP_new_curve_GFp(const BIGNUM *p, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx);
void EC_GROUP_free(EC_GROUP *group);
EC_POINT *EC_POINT_new(const EC_GROUP *group);
void EC_POINT_free(EC_POINT *point);
int EC_POINT_set_affine_coordinates(const EC_GROUP *group, EC_POINT *point, const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx);
int EC_POINT_get_affine_coordinates(const EC_GROUP *group, const EC_POINT *point, BIGNUM *x, BIGNUM *y, BN_CTX *ctx);
int EC_POINT_is_on_curve(const EC_GROUP *group, const EC_POINT *point, BN_CTX *ctx);
int EC_POINT_add(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, const EC_POINT *b, BN_CTX *ctx);
int EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx);

unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);
int RAND_bytes(unsigned char *buf, int num);

#endif
