/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2024 UMSKT Contributors (et.al.)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @FileCreated by Neo on 12/05/2023
 * @Maintainer Neo
 */

#include "confid.h"

#if defined(__x86_64__) || defined(_M_AMD64) || defined(__aarch64__) || (defined(__arm64__) && defined(__APPLE__))
#ifdef __GNUC__
QWORD ConfirmationID::Residue::__umul128(QWORD multiplier, QWORD multiplicand, QWORD *product_hi)
{
    OWORD r = (OWORD)multiplier * (OWORD)multiplicand;
    *product_hi = r >> 64;
    return (QWORD)r;
}
#else // basically msvc
/**
 *
 * @param multiplier
 * @param multiplicand
 * @param product_hi
 * @return
 */
QWORD ConfirmationID::Residue::__umul128(QWORD multiplier, QWORD multiplicand, QWORD *product_hi)
{
    return _umul128(multiplier, multiplicand, product_hi);
}
#endif
#elif defined(__i386__) || defined(_M_IX86) || defined(__arm__) || defined(__EMSCRIPTEN__)
QWORD ConfirmationID::Residue::__umul128(QWORD multiplier, QWORD multiplicand, QWORD *product_hi)
{
    // multiplier   = ab = a * 2^32 + b
    // multiplicand = cd = c * 2^32 + d
    // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
    QWORD a = multiplier >> 32;
    QWORD b = multiplier & 0xFFFFFFFF;
    QWORD c = multiplicand >> 32;
    QWORD d = multiplicand & 0xFFFFFFFF;

    // QWORD ac = a * c;
    QWORD ad = a * d;
    // QWORD bc = b * c;
    QWORD bd = b * d;

    QWORD adbc = ad + (b * c);
    QWORD adbc_carry = adbc < ad ? 1 : 0;

    // multiplier * multiplicand = product_hi * 2^64 + product_lo
    QWORD product_lo = bd + (adbc << 32);
    QWORD product_lo_carry = product_lo < bd ? 1 : 0;
    *product_hi = (a * c) + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;

    return product_lo;
}
#else
#error Unknown architecture detected - please edit confid.cpp to tailor __umul128() your architecture
#endif

/**
 *
 * @param lo
 * @param hi
 * @return
 */
QWORD ConfirmationID::Residue::ui128_quotient_mod(QWORD lo, QWORD hi)
{
    // hi:lo * ceil(2**170/MOD) >> (64 + 64 + 42)
    QWORD prod1;
    __umul128(lo, parent->privateKey.qword[0], &prod1);

    QWORD part1hi, part1lo = __umul128(lo, parent->privateKey.qword[1], &part1hi);
    QWORD part2hi, part2lo = __umul128(hi, parent->privateKey.qword[0], &part2hi);

    QWORD sum1 = part1lo + part2lo;
    unsigned sum1carry = (sum1 < part1lo);
    sum1 += prod1;
    sum1carry += (sum1 < prod1);
    QWORD prod2 = part1hi + part2hi + sum1carry;

    QWORD prod3hi, prod3lo = __umul128(hi, parent->privateKey.qword[1], &prod3hi);

    prod3lo += prod2;
    prod3hi += (prod3lo < prod2);
    return (prod3lo >> 42) | (prod3hi << 22);
}

/**
 *
 * @param x
 * @param y
 * @return
 */
QWORD ConfirmationID::Residue::mul(QWORD x, QWORD y)
{
    // * ceil(2**170/MOD) = 0x2d351 c6d04f8b|604fa6a1 c6346a87 for (p-1)*(p-1) max
    QWORD hi, lo = __umul128(x, y, &hi);
    QWORD quotient = ui128_quotient_mod(lo, hi);
    return lo - quotient * parent->MOD;
}

/**
 *
 * @param x
 * @param y
 * @return
 */
QWORD ConfirmationID::Residue::pow(QWORD x, QWORD y)
{
    if (y == 0)
    {
        return 1;
    }

    QWORD cur = x;
    while (!(y & 1))
    {
        cur = mul(cur, cur);
        y >>= 1;
    }

    QWORD res = cur;
    while ((y >>= 1) != 0)
    {
        cur = mul(cur, cur);
        if (y & 1)
        {
            res = mul(res, cur);
        }
    }

    return res;
}

/**
 *
 * @param x
 * @param y
 * @return
 */
QWORD ConfirmationID::Residue::add(QWORD x, QWORD y)
{
    QWORD z = x + y;
    // z = z - (z >= MOD ? MOD : 0);
    if (z >= parent->MOD)
    {
        z -= parent->MOD;
    }
    return z;
}

/**
 *
 * @param x
 * @param y
 * @return
 */
QWORD ConfirmationID::Residue::sub(QWORD x, QWORD y)
{
    QWORD z = x - y;
    // z += (x < y ? MOD : 0);
    if (x < y)
    {
        z += parent->MOD;
    }
    return z;
}

/**
 *
 * @param u
 * @param v
 * @return
 */
QWORD ConfirmationID::Residue::inverse(QWORD u, QWORD v)
{
    // assert(u);
    int64_t tmp;
    int64_t xu = 1, xv = 0;
    QWORD v0 = v;
    while (u > 1)
    {
        QWORD d = v / u;
        QWORD remainder = v % u;
        tmp = u;
        u = remainder;
        v = tmp;
        tmp = xu;
        xu = xv - d * xu;
        xv = tmp;
    }
    xu += (xu < 0 ? v0 : 0);
    return xu;
}

/**
 *
 * @param x
 * @return
 */
QWORD ConfirmationID::Residue::inv(QWORD x)
{
    return inverse(x, parent->MOD);
    // return residue_pow(x, MOD - 2);
}

/**
 *
 * @param what
 * @return
 */
QWORD ConfirmationID::Residue::sqrt(QWORD what)
{
    if (!what)
    {
        return 0;
    }

    QWORD g = parent->NON_RESIDUE, z, y, r, x, b, t;
    QWORD e = 0, q = parent->MOD - 1;

    while (!(q & 1))
    {
        e++, q >>= 1;
    }

    z = pow(g, q);
    y = z;
    r = e;
    x = pow(what, (q - 1) / 2);
    b = mul(mul(what, x), x);
    x = mul(what, x);

    while (b != 1)
    {
        QWORD m = 0, b2 = b;

        do
        {
            m++;
            b2 = mul(b2, b2);
        } while (b2 != 1);

        if (m == r)
        {
            return BAD;
        }

        t = pow(y, 1 << (r - m - 1));
        y = mul(t, t);
        r = m;
        x = mul(x, t);
        b = mul(b, y);
    }

    if (mul(x, x) != what)
    {
        // printf("internal error in sqrt\n");
        return BAD;
    }

    return x;
}
