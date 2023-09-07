/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2023 UMSKT Contributors (et.al.)
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
 * @FileCreated by WitherOrNot on 06/02/2023
 * @Maintainer WitherOrNot
 *
 * @History {
 *  This algorithm was provided to the UMSKT project by diamondggg
 *  the history provided by diamondggg is that they are the originator of the code
 *  and was created in tandem with an acquaintance who knows number theory.
 *  The file dates suggest this code was written sometime in 2017/2018
 * }
 */

#include "confid.h"

QWORD MOD = 0;
QWORD NON_RESIDUE = 0;
QWORD f[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
int productID[4];
int activationMode;

int ConfirmationID::calculateCheckDigit(int pid)
{
	unsigned int i = 0, j = 0, k = 0;
	for (j = pid; j; i += k)
	{
		k = j % 10;
		j /= 10;
	}
	return ((10 * pid) - (i % 7)) + 7;
}

QWORD ConfirmationID::residue_add(QWORD x, QWORD y)
{
	QWORD z = x + y;
	//z = z - (z >= MOD ? MOD : 0);
	if (z >= MOD)
		z -= MOD;
	return z;
}

QWORD ConfirmationID::residue_sub(QWORD x, QWORD y)
{
	QWORD z = x - y;
	//z += (x < y ? MOD : 0);
	if (x < y)
		z += MOD;
	return z;
}

#if defined(__x86_64__) || defined(_M_AMD64) || defined(__aarch64__) || (defined(__arm64__) && defined(__APPLE__))
#ifdef __GNUC__
inline QWORD ConfirmationID::__umul128(QWORD a, QWORD b, QWORD* hi)
{
    OWORD r = (OWORD)a * (OWORD)b;
    *hi = r >> 64;
    return (QWORD) r;
}
#else
#define __umul128 _umul128
#endif
#elif defined(__i386__) || defined(_M_IX86) || defined(__arm__) || defined(__EMSCRIPTEN__)
inline QWORD ConfirmationID::__umul128(QWORD multiplier, QWORD multiplicand, QWORD *product_hi) {
    // multiplier   = ab = a * 2^32 + b
    // multiplicand = cd = c * 2^32 + d
    // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
    QWORD a = multiplier >> 32;
    QWORD b = multiplier & 0xFFFFFFFF;
    QWORD c = multiplicand >> 32;
    QWORD d = multiplicand & 0xFFFFFFFF;

    //QWORD ac = a * c;
    QWORD ad = a * d;
    //QWORD bc = b * c;
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

QWORD ConfirmationID::ui128_quotient_mod(QWORD lo, QWORD hi)
{
	// hi:lo * ceil(2**170/MOD) >> (64 + 64 + 42)
	QWORD prod1;
	switch (activationMode) {
		case 0:
			__umul128(lo, 0x604FA6A1C6346A87, &prod1);
			break;
		case 1:
		case 2:
		case 3:
			__umul128(lo, 0x4FA8E4A40CDAE44A, &prod1);
			break;
		case 4:
			__umul128(lo, 0x2C5C4D3654A594F0, &prod1);
	}
	QWORD part1hi;
	QWORD part1lo;
        switch (activationMode) {
		case 0:
			part1lo = __umul128(lo, 0x2D351C6D04F8B, &part1hi);
			break;
		case 1:
		case 2:
		case 3:
			part1lo = __umul128(lo, 0x2CBAF12A59BBE, &part1hi);
			break;
		case 4:
			part1lo = __umul128(lo, 0x2D36C691A4EA5, &part1hi);
	}
	QWORD part2hi;
	QWORD part2lo;
	switch (activationMode) {
		case 0:
			part2lo = __umul128(hi, 0x604FA6A1C6346A87, &part2hi);
			break;
		case 1:
		case 2:
		case 3:
			part2lo = __umul128(hi, 0x4FA8E4A40CDAE44A, &part2hi);
			break;
		case 4:
			part2lo = __umul128(hi, 0x2C5C4D3654A594F0, &part2hi);
	}
	QWORD sum1 = part1lo + part2lo;
	unsigned sum1carry = (sum1 < part1lo);
	sum1 += prod1;
	sum1carry += (sum1 < prod1);
	QWORD prod2 = part1hi + part2hi + sum1carry;
	QWORD prod3hi;
	QWORD prod3lo;
        switch (activationMode) {
		case 0:
			prod3lo = __umul128(hi, 0x2D351C6D04F8B, &prod3hi);
			break;
		case 1:
		case 2:
		case 3:
			prod3lo = __umul128(hi, 0x2CBAF12A59BBE, &prod3hi);
			break;
		case 4:
			prod3lo = __umul128(hi, 0x2D36C691A4EA5, &prod3hi);
	}
	prod3lo += prod2;
	prod3hi += (prod3lo < prod2);
	return (prod3lo >> 42) | (prod3hi << 22);
}

QWORD ConfirmationID::residue_mul(QWORD x, QWORD y)
{
// * ceil(2**170/MOD) = 0x2d351 c6d04f8b|604fa6a1 c6346a87 for (p-1)*(p-1) max
	QWORD hi;
	QWORD lo = __umul128(x, y, &hi);
	QWORD quotient = ui128_quotient_mod(lo, hi);
	return lo - quotient * MOD;
}

QWORD ConfirmationID::residue_pow(QWORD x, QWORD y)
{
	if (y == 0)
		return 1;
	QWORD cur = x;
	while (!(y & 1)) {
		cur = residue_mul(cur, cur);
		y >>= 1;
	}
	QWORD res = cur;
	while ((y >>= 1) != 0) {
		cur = residue_mul(cur, cur);
		if (y & 1)
			res = residue_mul(res, cur);
	}
	return res;
}

QWORD ConfirmationID::inverse(QWORD u, QWORD v)
{
	//assert(u);
	int64_t tmp;
	int64_t xu = 1, xv = 0;
	QWORD v0 = v;
	while (u > 1) {
		QWORD d = v / u; QWORD remainder = v % u;
		tmp = u; u = remainder; v = tmp;
		tmp = xu; xu = xv - d * xu; xv = tmp;
	}
	xu += (xu < 0 ? v0 : 0);
	return xu;
}

QWORD ConfirmationID::residue_inv(QWORD x)
{
    return inverse(x, MOD);
    // return residue_pow(x, MOD - 2);
}

#define BAD 0xFFFFFFFFFFFFFFFFull

QWORD ConfirmationID::residue_sqrt(QWORD what)
{
	if (!what) {
        return 0;
    }

	QWORD g = NON_RESIDUE, z, y, r, x, b, t;
	QWORD e = 0, q = MOD - 1;

	while (!(q & 1)) {
        e++, q >>= 1;
    }

	z = residue_pow(g, q);
	y = z;
	r = e;
	x = residue_pow(what, (q - 1) / 2);
	b = residue_mul(residue_mul(what, x), x);
	x = residue_mul(what, x);
	while (b != 1) {
		QWORD m = 0, b2 = b;

        do {
			m++;
			b2 = residue_mul(b2, b2);
		} while (b2 != 1);

        if (m == r) {
            return BAD;
        }

		t = residue_pow(y, 1 << (r - m - 1));
		y = residue_mul(t, t);
		r = m;
		x = residue_mul(x, t);
		b = residue_mul(b, y);
	}

	if (residue_mul(x, x) != what) {
		//printf("internal error in sqrt\n");
		return BAD;
	}

	return x;
}

int ConfirmationID::find_divisor_v(TDivisor* d)
{
	// u | v^2 - f
	// u = u0 + u1*x + x^2
	// f%u = f0 + f1*x
	QWORD v1, f2[6];

    for (int i = 0; i < 6; i++) {
        f2[i] = f[i];
    }

	const QWORD u0 = d->u[0];
	const QWORD u1 = d->u[1];
	for (int j = 4; j--; ) {
		f2[j] = residue_sub(f2[j], residue_mul(u0, f2[j + 2]));
		f2[j + 1] = residue_sub(f2[j + 1], residue_mul(u1, f2[j + 2]));
		f2[j + 2] = 0;
	}
	// v = v0 + v1*x
	// u | (v0^2 - f0) + (2*v0*v1 - f1)*x + v1^2*x^2 = u0*v1^2 + u1*v1^2*x + v1^2*x^2
	// v0^2 - f0 = u0*v1^2
	// 2*v0*v1 - f1 = u1*v1^2
	// v0^2 = f0 + u0*v1^2 = (f1 + u1*v1^2)^2 / (2*v1)^2
	// (f1^2) + 2*(f1*u1-2*f0) * v1^2 + (u1^2-4*u0) * v1^4 = 0
	// v1^2 = ((2*f0-f1*u1) +- 2*sqrt(-f0*f1*u1 + f0^2 + f1^2*u0))) / (u1^2-4*u0)
	const QWORD f0 = f2[0];
	const QWORD f1 = f2[1];
	const QWORD u0double = residue_add(u0, u0);
	const QWORD coeff2 = residue_sub(residue_mul(u1, u1), residue_add(u0double, u0double));
	const QWORD coeff1 = residue_sub(residue_add(f0, f0), residue_mul(f1, u1));
	if (coeff2 == 0) {
		if (coeff1 == 0) {
			if (f1 == 0) {
				// impossible
				//printf("bad f(), double root detected\n");
			}
			return 0;
		}
		QWORD sqr = residue_mul(residue_mul(f1, f1), residue_inv(residue_add(coeff1, coeff1)));
		v1 = residue_sqrt(sqr);
		if (v1 == BAD) {
            return 0;
        }
	} else {
		QWORD d = residue_add(residue_mul(f0, f0), residue_mul(f1, residue_sub(residue_mul(f1, u0), residue_mul(f0, u1))));
		d = residue_sqrt(d);
		if (d == BAD) {
            return 0;
        }

		d = residue_add(d, d);
		QWORD inv = residue_inv(coeff2);
		QWORD root = residue_mul(residue_add(coeff1, d), inv);
		v1 = residue_sqrt(root);
		if (v1 == BAD) {
			root = residue_mul(residue_sub(coeff1, d), inv);
			v1 = residue_sqrt(root);
			if (v1 == BAD) {
                return 0;
            }
		}
	}

	QWORD v0 = residue_mul(residue_add(f1, residue_mul(u1, residue_mul(v1, v1))), residue_inv(residue_add(v1, v1)));
	d->v[0] = v0;
	d->v[1] = v1;
	return 1;
}

// generic short slow code
int ConfirmationID::polynomial_mul(int adeg, const QWORD a[], int bdeg, const QWORD b[], int resultprevdeg, QWORD result[])
{
	if (adeg < 0 || bdeg < 0)
		return resultprevdeg;
	int i, j;
	for (i = resultprevdeg + 1; i <= adeg + bdeg; i++)
		result[i] = 0;
	resultprevdeg = i - 1;
	for (i = 0; i <= adeg; i++)
		for (j = 0; j <= bdeg; j++)
			result[i + j] = residue_add(result[i + j], residue_mul(a[i], b[j]));
	while (resultprevdeg >= 0 && result[resultprevdeg] == 0)
		--resultprevdeg;
	return resultprevdeg;
}

int ConfirmationID::polynomial_div_monic(int adeg, QWORD a[], int bdeg, const QWORD b[], QWORD* quotient)
{
	assert(bdeg >= 0);
	assert(b[bdeg] == 1);
	int i, j;
	for (i = adeg - bdeg; i >= 0; i--) {
		QWORD q = a[i + bdeg];
		if (quotient)
			quotient[i] = q;
		for (j = 0; j < bdeg; j++)
			a[i + j] = residue_sub(a[i + j], residue_mul(q, b[j]));
		a[i + j] = 0;
	}
	i += bdeg;
	while (i >= 0 && a[i] == 0)
		i--;
	return i;
}
void ConfirmationID::polynomial_xgcd(int adeg, const QWORD a[3], int bdeg, const QWORD b[3], int* pgcddeg, QWORD gcd[3], int* pmult1deg, QWORD mult1[3], int* pmult2deg, QWORD mult2[3])
{
	int sdeg = -1;
	QWORD s[3] = {0, 0, 0};
	int mult1deg = 0;
	mult1[0] = 1; mult1[1] = 0; mult1[2] = 0;
	int tdeg = 0;
	QWORD t[3] = {1, 0, 0};
	int mult2deg = -1;
	mult2[0] = 0; mult2[1] = 0; mult2[2] = 0;
	int rdeg = bdeg;
	QWORD r[3] = {b[0], b[1], b[2]};
	int gcddeg = adeg;
	gcd[0] = a[0]; gcd[1] = a[1]; gcd[2] = a[2];
	// s*u1 + t*u2 = r
	// mult1*u1 + mult2*u2 = gcd
	while (rdeg >= 0) {
		if (rdeg > gcddeg) {
			unsigned tmp;
			int tmpi;
			tmp = rdeg; rdeg = gcddeg; gcddeg = tmp;
			tmpi = sdeg; sdeg = mult1deg; mult1deg = tmpi;
			tmpi = tdeg; tdeg = mult2deg; mult2deg = tmpi;
			QWORD tmp2;
			tmp2 = r[0]; r[0] = gcd[0]; gcd[0] = tmp2;
			tmp2 = r[1]; r[1] = gcd[1]; gcd[1] = tmp2;
			tmp2 = r[2]; r[2] = gcd[2]; gcd[2] = tmp2;
			tmp2 = s[0]; s[0] = mult1[0]; mult1[0] = tmp2;
			tmp2 = s[1]; s[1] = mult1[1]; mult1[1] = tmp2;
			tmp2 = s[2]; s[2] = mult1[2]; mult1[2] = tmp2;
			tmp2 = t[0]; t[0] = mult2[0]; mult2[0] = tmp2;
			tmp2 = t[1]; t[1] = mult2[1]; mult2[1] = tmp2;
			tmp2 = t[2]; t[2] = mult2[2]; mult2[2] = tmp2;
			continue;
		}
		int delta = gcddeg - rdeg;
		QWORD mult = residue_mul(gcd[gcddeg], residue_inv(r[rdeg]));
		// quotient = mult * x**delta
		assert(rdeg + delta < 3);
		for (int i = 0; i <= rdeg; i++)
			gcd[i + delta] = residue_sub(gcd[i + delta], residue_mul(mult, r[i]));
		while (gcddeg >= 0 && gcd[gcddeg] == 0)
			gcddeg--;
		assert(sdeg + delta < 3);
		for (int i = 0; i <= sdeg; i++)
			mult1[i + delta] = residue_sub(mult1[i + delta], residue_mul(mult, s[i]));
		if (mult1deg < sdeg + delta)
			mult1deg = sdeg + delta;
		while (mult1deg >= 0 && mult1[mult1deg] == 0)
			mult1deg--;
		assert(tdeg + delta < 3);
		for (int i = 0; i <= tdeg; i++)
			mult2[i + delta] = residue_sub(mult2[i + delta], residue_mul(mult, t[i]));
		if (mult2deg < tdeg + delta)
			mult2deg = tdeg + delta;
		while (mult2deg >= 0 && mult2[mult2deg] == 0)
			mult2deg--;
	}
	// d1 = gcd, e1 = mult1, e2 = mult2
	*pgcddeg = gcddeg;
	*pmult1deg = mult1deg;
	*pmult2deg = mult2deg;
}

int ConfirmationID::u2poly(const TDivisor* src, QWORD polyu[3], QWORD polyv[2])
{
	if (src->u[1] != BAD) {
		polyu[0] = src->u[0];
		polyu[1] = src->u[1];
		polyu[2] = 1;
		polyv[0] = src->v[0];
		polyv[1] = src->v[1];
		return 2;
	}
	if (src->u[0] != BAD) {
		polyu[0] = src->u[0];
		polyu[1] = 1;
		polyv[0] = src->v[0];
		polyv[1] = 0;
		return 1;
	}
	polyu[0] = 1;
	polyv[0] = 0;
	polyv[1] = 0;
	return 0;
}

void ConfirmationID::divisor_add(const TDivisor* src1, const TDivisor* src2, TDivisor* dst)
{
	QWORD u1[3], u2[3], v1[2], v2[2];
	int u1deg = u2poly(src1, u1, v1);
	int u2deg = u2poly(src2, u2, v2);
	// extended gcd: d1 = gcd(u1, u2) = e1*u1 + e2*u2
	int d1deg, e1deg, e2deg;
	QWORD d1[3], e1[3], e2[3];
	polynomial_xgcd(u1deg, u1, u2deg, u2, &d1deg, d1, &e1deg, e1, &e2deg, e2);
	assert(e1deg <= 1);
	assert(e2deg <= 1);
	// extended gcd again: d = gcd(d1, v1+v2) = c1*d1 + c2*(v1+v2)
	QWORD b[3] = {residue_add(v1[0], v2[0]), residue_add(v1[1], v2[1]), 0};
	int bdeg = (b[1] == 0 ? (b[0] == 0 ? -1 : 0) : 1);
	int ddeg, c1deg, c2deg;
	QWORD d[3], c1[3], c2[3];
	polynomial_xgcd(d1deg, d1, bdeg, b, &ddeg, d, &c1deg, c1, &c2deg, c2);
	assert(c1deg <= 0);
	assert(c2deg <= 1);
	assert(ddeg >= 0);
	QWORD dmult = residue_inv(d[ddeg]);
	int i;
	for (i = 0; i < ddeg; i++)
		d[i] = residue_mul(d[i], dmult);
	d[i] = 1;
	for (i = 0; i <= c1deg; i++)
		c1[i] = residue_mul(c1[i], dmult);
	for (i = 0; i <= c2deg; i++)
		c2[i] = residue_mul(c2[i], dmult);
	QWORD u[5];
	int udeg = polynomial_mul(u1deg, u1, u2deg, u2, -1, u);
	// u is monic
	QWORD v[7], tmp[7];
	int vdeg, tmpdeg;
	// c1*(e1*u1*v2 + e2*u2*v1) + c2*(v1*v2 + f)
	// c1*(e1*u1*(v2-v1) + d1*v1) + c2*(v1*v2 + f)
	v[0] = residue_sub(v2[0], v1[0]);
	v[1] = residue_sub(v2[1], v1[1]);
	tmpdeg = polynomial_mul(e1deg, e1, 1, v, -1, tmp);
	vdeg = polynomial_mul(u1deg, u1, tmpdeg, tmp, -1, v);
	vdeg = polynomial_mul(d1deg, d1, 1, v1, vdeg, v);
	for (i = 0; i <= vdeg; i++)
		v[i] = residue_mul(v[i], c1[0]);
	memcpy(tmp, f, 6 * sizeof(f[0]));
	tmpdeg = 5;
	tmpdeg = polynomial_mul(1, v1, 1, v2, tmpdeg, tmp);
	vdeg = polynomial_mul(c2deg, c2, tmpdeg, tmp, vdeg, v);
	if (ddeg > 0) {
		assert(udeg >= 2*ddeg);
		QWORD udiv[5];
		polynomial_div_monic(udeg, u, ddeg, d, udiv); udeg -= ddeg;
		polynomial_div_monic(udeg, udiv, ddeg, d, u); udeg -= ddeg;
		if (vdeg >= 0) {
			assert(vdeg >= ddeg);
			polynomial_div_monic(vdeg, v, ddeg, d, udiv); vdeg -= ddeg;
			memcpy(v, udiv, (vdeg + 1) * sizeof(v[0]));
		}
	}
	vdeg = polynomial_div_monic(vdeg, v, udeg, u, NULL);
	while (udeg > 2) {
		assert(udeg <= 4);
		assert(vdeg <= 3);
		// u' = monic((f-v^2)/u), v'=-v mod u'
		tmpdeg = polynomial_mul(vdeg, v, vdeg, v, -1, tmp);
		for (i = 0; i <= tmpdeg && i <= 5; i++)
			tmp[i] = residue_sub(f[i], tmp[i]);
		for (; i <= tmpdeg; i++)
			tmp[i] = residue_sub(0, tmp[i]);
		for (; i <= 5; i++)
			tmp[i] = f[i];
		tmpdeg = i - 1;
		QWORD udiv[5];
		polynomial_div_monic(tmpdeg, tmp, udeg, u, udiv);
		udeg = tmpdeg - udeg;
		QWORD mult = residue_inv(udiv[udeg]);
		for (i = 0; i < udeg; i++)
			u[i] = residue_mul(udiv[i], mult);
		u[i] = 1;
		for (i = 0; i <= vdeg; i++)
			v[i] = residue_sub(0, v[i]);
		vdeg = polynomial_div_monic(vdeg, v, udeg, u, NULL);
	}
	if (udeg == 2) {
		dst->u[0] = u[0];
		dst->u[1] = u[1];
		dst->v[0] = (vdeg >= 0 ? v[0] : 0);
		dst->v[1] = (vdeg >= 1 ? v[1] : 0);
	} else if (udeg == 1) {
		dst->u[0] = u[0];
		dst->u[1] = BAD;
		dst->v[0] = (vdeg >= 0 ? v[0] : 0);
		dst->v[1] = BAD;
	} else {
		assert(udeg == 0);
		dst->u[0] = BAD;
		dst->u[1] = BAD;
		dst->v[0] = BAD;
		dst->v[1] = BAD;
	}
}

#define divisor_double(src, dst) divisor_add(src, src, dst)

void ConfirmationID::divisor_mul(const TDivisor* src, QWORD mult, TDivisor* dst)
{
	if (mult == 0) {
		dst->u[0] = BAD;
		dst->u[1] = BAD;
		dst->v[0] = BAD;
		dst->v[1] = BAD;
		return;
	}
	TDivisor cur = *src;
	while (!(mult & 1)) {
		divisor_double(&cur, &cur);
		mult >>= 1;
	}
	*dst = cur;
	while ((mult >>= 1) != 0) {
		divisor_double(&cur, &cur);
		if (mult & 1)
			divisor_add(dst, &cur, dst);
	}
}

void ConfirmationID::divisor_mul128(const TDivisor* src, QWORD mult_lo, QWORD mult_hi, TDivisor* dst)
{
	if (mult_lo == 0 && mult_hi == 0) {
		dst->u[0] = BAD;
		dst->u[1] = BAD;
		dst->v[0] = BAD;
		dst->v[1] = BAD;
		return;
	}
	TDivisor cur = *src;
	while (!(mult_lo & 1)) {
		divisor_double(&cur, &cur);
		mult_lo >>= 1;
		if (mult_hi & 1)
			mult_lo |= (1ULL << 63);
		mult_hi >>= 1;
	}
	*dst = cur;
	for (;;) {
		mult_lo >>= 1;
		if (mult_hi & 1)
			mult_lo |= (1ULL << 63);
		mult_hi >>= 1;
		if (mult_lo == 0 && mult_hi == 0)
			break;
		divisor_double(&cur, &cur);
		if (mult_lo & 1)
			divisor_add(dst, &cur, dst);
	}
}

unsigned ConfirmationID::rol(unsigned x, int shift)
{
	//assert(shift > 0 && shift < 32);
	return (x << shift) | (x >> (32 - shift));
}

void ConfirmationID::sha1_single_block(unsigned char input[64], unsigned char output[20])
{
	unsigned a, b, c, d, e;
	a = 0x67452301;
	b = 0xEFCDAB89;
	c = 0x98BADCFE;
	d = 0x10325476;
	e = 0xC3D2E1F0;
	unsigned w[80];
	size_t i;
	for (i = 0; i < 16; i++)
		w[i] = input[4*i] << 24 | input[4*i+1] << 16 | input[4*i+2] << 8 | input[4*i+3];
	for (i = 16; i < 80; i++)
		w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
	for (i = 0; i < 20; i++) {
		unsigned tmp = rol(a, 5) + ((b & c) | (~b & d)) + e + w[i] + 0x5A827999;
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = tmp;
	}
	for (i = 20; i < 40; i++) {
		unsigned tmp = rol(a, 5) + (b ^ c ^ d) + e + w[i] + 0x6ED9EBA1;
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = tmp;
	}
	for (i = 40; i < 60; i++) {
		unsigned tmp = rol(a, 5) + ((b & c) | (b & d) | (c & d)) + e + w[i] + 0x8F1BBCDC;
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = tmp;
	}
	for (i = 60; i < 80; i++) {
		unsigned tmp = rol(a, 5) + (b ^ c ^ d) + e + w[i] + 0xCA62C1D6;
		e = d;
		d = c;
		c = rol(b, 30);
		b = a;
		a = tmp;
	}
	a += 0x67452301;
	b += 0xEFCDAB89;
	c += 0x98BADCFE;
	d += 0x10325476;
	e += 0xC3D2E1F0;
	output[0] = a >> 24; output[1] = a >> 16; output[2] = a >> 8; output[3] = a;
	output[4] = b >> 24; output[5] = b >> 16; output[6] = b >> 8; output[7] = b;
	output[8] = c >> 24; output[9] = c >> 16; output[10] = c >> 8; output[11] = c;
	output[12] = d >> 24; output[13] = d >> 16; output[14] = d >> 8; output[15] = d;
	output[16] = e >> 24; output[17] = e >> 16; output[18] = e >> 8; output[19] = e;
}

void ConfirmationID::decode_iid_new_version(unsigned char* iid, unsigned char* hwid, int* version)
{
    QWORD buffer[5];
    int i;
    for (i = 0; i < 5; i++)
        memcpy(&buffer[i], (iid + (4 * i)), 4);
    DWORD v1 = (buffer[3] & 0xFFFFFFF8) | 2;
    DWORD v2 = ((buffer[3] & 7) << 29) | (buffer[2] >> 3);
    QWORD hardwareIDVal = ((QWORD)v1 << 32) | v2;
    for (i = 0; i < 8; ++i)
        hwid[i] = (hardwareIDVal >> (8 * i)) & 0xFF;
    /*DWORD v3 = ((buffer[0] & 0xFFFFFF80) >> 7) & 0xFFFFFFFF;
    DWORD v4 = v3 & 0xFFFFF800;
    DWORD v5 = buffer[1] & 0x7F;
    DWORD v6 = buffer[1] >> 7;
    DWORD v7 = ((v5 << 25) | v4) >> 11;
    productID[1] = v7 & 0x000003FF;
    DWORD v8 = v7 & 0xFFFFFC00;
    DWORD v9 = (v6 >> 11) & 0x00001FFF;
    DWORD v10 = v9 & 0x00001C00;
    DWORD v11 = v9 & 0x000003FF;
    DWORD v12 = (((v6 << 21) & 0xFFFFFFFF) | v8) >> 10;
    DWORD v13 = (v11 << 22) & 0xFFFFFFFF;
    DWORD v14 = v13 | v12;
    productID[2] = v14 & 0x000FFFFF;
    productID[2] = calculateCheckDigit(productID[2]);
    productID[3] = (v14 & 0x3FF00000) >> 20;*/
    *version = buffer[0] & 7;
}

void ConfirmationID::Mix(unsigned char* buffer, size_t bufSize, const unsigned char* key, size_t keySize)
{
	unsigned char sha1_input[64];
	unsigned char sha1_result[20];
	size_t half = bufSize / 2;
	//assert(half <= sizeof(sha1_result) && half + keySize <= sizeof(sha1_input) - 9);
	int external_counter;
	for (external_counter = 0; external_counter < 4; external_counter++) {
		memset(sha1_input, 0, sizeof(sha1_input));
		switch (activationMode) {
			case 0:
			case 1:
			case 4:
				memcpy(sha1_input, buffer + half, half);
				memcpy(sha1_input + half, key, keySize);
				sha1_input[half + keySize] = 0x80;
				sha1_input[sizeof(sha1_input) - 1] = (half + keySize) * 8;
				sha1_input[sizeof(sha1_input) - 2] = (half + keySize) * 8 / 0x100;
				break;
			case 2:
			case 3:
				sha1_input[0] = 0x79;
				memcpy(sha1_input + 1, buffer + half, half);
				memcpy(sha1_input + 1 + half, key, keySize);
				sha1_input[1 + half + keySize] = 0x80;
				sha1_input[sizeof(sha1_input) - 1] = (1 + half + keySize) * 8;
				sha1_input[sizeof(sha1_input) - 2] = (1 + half + keySize) * 8 / 0x100;
		}
		sha1_single_block(sha1_input, sha1_result);
		size_t i;
		for (i = half & ~3; i < half; i++)
			sha1_result[i] = sha1_result[i + 4 - (half & 3)];
		for (i = 0; i < half; i++) {
			unsigned char tmp = buffer[i + half];
			buffer[i + half] = buffer[i] ^ sha1_result[i];
			buffer[i] = tmp;
		}
	}
}

void ConfirmationID::Unmix(unsigned char* buffer, size_t bufSize, const unsigned char* key, size_t keySize)
{
	unsigned char sha1_input[64];
	unsigned char sha1_result[20];
	size_t half = bufSize / 2;
	//assert(half <= sizeof(sha1_result) && half + keySize <= sizeof(sha1_input) - 9);
	int external_counter;
	for (external_counter = 0; external_counter < 4; external_counter++) {
		memset(sha1_input, 0, sizeof(sha1_input));
		switch (activationMode) {
			case 0:
			case 1:
			case 4:
				memcpy(sha1_input, buffer, half);
				memcpy(sha1_input + half, key, keySize);
				sha1_input[half + keySize] = 0x80;
				sha1_input[sizeof(sha1_input) - 1] = (half + keySize) * 8;
				sha1_input[sizeof(sha1_input) - 2] = (half + keySize) * 8 / 0x100;
				break;
			case 2:
			case 3:
				sha1_input[0] = 0x79;
				memcpy(sha1_input + 1, buffer, half);
				memcpy(sha1_input + 1 + half, key, keySize);
				sha1_input[1 + half + keySize] = 0x80;
				sha1_input[sizeof(sha1_input) - 1] = (1 + half + keySize) * 8;
				sha1_input[sizeof(sha1_input) - 2] = (1 + half + keySize) * 8 / 0x100;
		}
		sha1_single_block(sha1_input, sha1_result);
		size_t i;
		for (i = half & ~3; i < half; i++)
			sha1_result[i] = sha1_result[i + 4 - (half & 3)];
		for (i = 0; i < half; i++) {
			unsigned char tmp = buffer[i];
			buffer[i] = buffer[i + half] ^ sha1_result[i];
			buffer[i + half] = tmp;
		}
	}
}

int ConfirmationID::Generate(const char* installation_id_str, char confirmation_id[49], int mode, std::string productid)
{
	int version;
	unsigned char hardwareID[8];
	activationMode = mode;
	switch (activationMode) {
		case 0:
			MOD = 0x16A6B036D7F2A79;
			NON_RESIDUE = 43;
			f[0] = 0x0;
			f[1] = 0x21840136C85381;
			f[2] = 0x44197B83892AD0;
			f[3] = 0x1400606322B3B04;
			f[4] = 0x1400606322B3B04;
			f[5] = 0x1;
			break;
		case 1:
		case 2:
		case 3:
			MOD = 0x16E48DD18451FE9;
			NON_RESIDUE = 3;
			f[0] = 0x0;
			f[1] = 0xE5F5ECD95C8FD2;
			f[2] = 0xFF28276F11F61;
			f[3] = 0xFB2BD9132627E6;
			f[4] = 0xE5F5ECD95C8FD2;
			f[5] = 0x1;
			break;
		case 4:
			MOD = 0x16A5DABA0605983;
			NON_RESIDUE = 2;
			f[0] = 0x334F24F75CAA0E;
			f[1] = 0x1392FF62889BD7B;
			f[2] = 0x135131863BA2DB8;
			f[3] = 0x153208E78006010;
			f[4] = 0x163694F26056DB;
			f[5] = 0x1;
	}
	unsigned char installation_id[19]; // 10**45 < 256**19
	size_t installation_id_len = 0;
	const char* p = installation_id_str;
	size_t count = 0, totalCount = 0;
	unsigned check = 0;
	size_t i;
	for (; *p; p++) {
		if (*p == ' ' || *p == '-')
			continue;
		int d = *p - '0';
		if (d < 0 || d > 9)
			return ERR_INVALID_CHARACTER;
		if (count == 5 || p[1] == 0) {
			if (!count)
				return (totalCount == 45) ? ERR_TOO_LARGE : ERR_TOO_SHORT;
			if (d != check % 7)
				return (count < 5) ? ERR_TOO_SHORT : ERR_INVALID_CHECK_DIGIT;
			check = 0;
			count = 0;
			continue;
		}
		check += (count % 2 ? d * 2 : d);
		count++;
		totalCount++;
		if (totalCount > 45)
			return ERR_TOO_LARGE;
		unsigned char carry = d;
		for (i = 0; i < installation_id_len; i++) {
			unsigned x = installation_id[i] * 10 + carry;
			installation_id[i] = x & 0xFF;
			carry = x >> 8;
		}
		if (carry) {
			assert(installation_id_len < sizeof(installation_id));
			installation_id[installation_id_len++] = carry;
		}
	}
	if (totalCount != 41 && totalCount < 45)
		return ERR_TOO_SHORT;
	for (; installation_id_len < sizeof(installation_id); installation_id_len++)
		installation_id[installation_id_len] = 0;
	unsigned char iid_key[4] = { 0x0, 0x0, 0x0, 0x0 };
	switch (activationMode) {
		case 0:
		case 4:
			iid_key[0] = 0x6A;
			iid_key[1] = 0xC8;
			iid_key[2] = 0x5E;
			iid_key[3] = 0xD4;
			break;
		case 1:
		case 2:
		case 3:
			iid_key[0] = 0x5A;
			iid_key[1] = 0x30;
			iid_key[2] = 0xB9;
			iid_key[3] = 0xF3;
	}
	Unmix(installation_id, totalCount == 41 ? 17 : 19, iid_key, 4);
	if (installation_id[18] >= 0x10)
		return ERR_UNKNOWN_VERSION;

#pragma pack(push, 1)
	struct {
		QWORD HardwareID;
		QWORD ProductIDLow;
		unsigned char ProductIDHigh;
		unsigned short KeySHA1;
	} parsed;
#pragma pack(pop)
	switch (activationMode) {
		case 0:
		case 1:
		case 4:
			memcpy(&parsed, installation_id, sizeof(parsed));
			productID[0] = parsed.ProductIDLow & ((1 << 17) - 1);
			productID[1] = (parsed.ProductIDLow >> 17) & ((1 << 10) - 1);
			productID[2] = (parsed.ProductIDLow >> 27) & ((1 << 24) - 1);
			version    = (parsed.ProductIDLow >> 51) & 15;
			productID[3] = (parsed.ProductIDLow >> 55) | (parsed.ProductIDHigh << 9);
			switch (activationMode) {
				case 0:
					if (version != (totalCount == 41 ? 9 : 10))
						return ERR_UNKNOWN_VERSION;
					break;
				case 1:
					if (version != 1)
						return ERR_UNKNOWN_VERSION;
					break;
				case 3:
					if (version != 4)
						return ERR_UNKNOWN_VERSION;
			}
			break;
		case 2:
		case 3:
			decode_iid_new_version(installation_id, hardwareID, &version);
			switch (activationMode) {
				case 2:
					if (version != 3)
						return ERR_UNKNOWN_VERSION;
					break;
				case 3:
					if (version != 4)
						return ERR_UNKNOWN_VERSION;
			}
			memcpy(&parsed, hardwareID, 8);
			productID[0] = stoi(productid.substr(0,5));
			std::string channelid = productid.substr(6,3);
			char *p = &channelid[0];
			for (; *p; p++) {
				*p = toupper((unsigned char)*p);
			}
			if (strcmp(&channelid[0], "OEM") == 0) {
				productID[1] = stoi(productid.substr(12,3));
				productID[2] = (stoi(productid.substr(15,1)) * 100000) + stoi(productid.substr(18,5));
				productID[2] = calculateCheckDigit(productID[2]);
				productID[3] = ((stoi(productid.substr(10,2))) * 1000) + productID[3];
			} else {
				productID[1] = stoi(productid.substr(6,3));
				productID[2] = stoi(productid.substr(10,7));
				productID[3] = stoi(productid.substr(18,5));
			}
		fmt::print("ProductID: {}-{}-{}-{} \n", productID[0], productID[1], productID[2], productID[3]);
	}
	
	unsigned char keybuf[16];
	memcpy(keybuf, &parsed.HardwareID, 8);
	QWORD productIdMixed = (QWORD)productID[0] << 41 | (QWORD)productID[1] << 58 | (QWORD)productID[2] << 17 | productID[3];
	memcpy(keybuf + 8, &productIdMixed, 8);

	TDivisor d;
	unsigned char attempt;
	for (attempt = 0; attempt <= 0x80; attempt++) {
		union {
			unsigned char buffer[14];
			struct {
				QWORD lo;
				QWORD hi;
			};
		} u;
		u.lo = 0;
		u.hi = 0;
		switch (activationMode) {
			case 0:
			case 1:
			case 4:
				u.buffer[7] = attempt;
				break;
			case 2:
			case 3:
				u.buffer[6] = attempt;
		}
		Mix(u.buffer, 14, keybuf, 16);
		QWORD x2 = ui128_quotient_mod(u.lo, u.hi);
		QWORD x1 = u.lo - x2 * MOD;
		x2++;
		d.u[0] = residue_sub(residue_mul(x1, x1), residue_mul(NON_RESIDUE, residue_mul(x2, x2)));
		d.u[1] = residue_add(x1, x1);
		if (find_divisor_v(&d))
			break;
	}
	if (attempt > 0x80)
		return ERR_UNLUCKY;
	switch (activationMode) {
		case 0:
			divisor_mul128(&d, 0x04E21B9D10F127C1, 0x40DA7C36D44C, &d);
			break;
		case 1:
		case 2:
		case 3:
			divisor_mul128(&d, 0xEFE0302A1F7A5341, 0x01FB8CF48A70DF, &d);
			break;
		case 4:
			divisor_mul128(&d, 0x7C4254C43A5D1181, 0x01C61212ECE610, &d);
	}
	union {
		struct {
			QWORD encoded_lo, encoded_hi;
		};
		struct {
			uint32_t encoded[4];
		};
	} e;
	if (d.u[0] == BAD) {
		// we can not get the zero divisor, actually...
		e.encoded_lo = __umul128(MOD + 2, MOD, &e.encoded_hi);
	} else if (d.u[1] == BAD) {
		// O(1/MOD) chance
		//encoded = (unsigned __int128)(MOD + 1) * d.u[0] + MOD; // * MOD + d.u[0] is fine too
		e.encoded_lo = __umul128(MOD + 1, d.u[0], &e.encoded_hi);
		e.encoded_lo += MOD;
		e.encoded_hi += (e.encoded_lo < MOD);
	} else {
		QWORD x1 = (d.u[1] % 2 ? d.u[1] + MOD : d.u[1]) / 2;
		QWORD x2sqr = residue_sub(residue_mul(x1, x1), d.u[0]);
		QWORD x2 = residue_sqrt(x2sqr);
		if (x2 == BAD) {
			x2 = residue_sqrt(residue_mul(x2sqr, residue_inv(NON_RESIDUE)));
			assert(x2 != BAD);
			e.encoded_lo = __umul128(MOD + 1, MOD + x2, &e.encoded_hi);
			e.encoded_lo += x1;
			e.encoded_hi += (e.encoded_lo < x1);
		} else {
			// points (-x1+x2, v(-x1+x2)) and (-x1-x2, v(-x1-x2))
			QWORD x1a = residue_sub(x1, x2);
			QWORD y1 = residue_sub(d.v[0], residue_mul(d.v[1], x1a));
			QWORD x2a = residue_add(x1, x2);
			QWORD y2 = residue_sub(d.v[0], residue_mul(d.v[1], x2a));
			if (x1a > x2a) {
				QWORD tmp = x1a;
				x1a = x2a;
				x2a = tmp;
			}
			if ((y1 ^ y2) & 1) {
				QWORD tmp = x1a;
				x1a = x2a;
				x2a = tmp;
			}
			e.encoded_lo = __umul128(MOD + 1, x1a, &e.encoded_hi);
			e.encoded_lo += x2a;
			e.encoded_hi += (e.encoded_lo < x2a);
		}
	}
	unsigned char decimal[35];
	for (i = 0; i < 35; i++) {
		unsigned c = e.encoded[3] % 10;
		e.encoded[3] /= 10;
		unsigned c2 = ((QWORD)c << 32 | e.encoded[2]) % 10;
		e.encoded[2] = ((QWORD)c << 32 | e.encoded[2]) / 10;
		unsigned c3 = ((QWORD)c2 << 32 | e.encoded[1]) % 10;
		e.encoded[1] = ((QWORD)c2 << 32 | e.encoded[1]) / 10;
		unsigned c4 = ((QWORD)c3 << 32 | e.encoded[0]) % 10;
		e.encoded[0] = ((QWORD)c3 << 32 | e.encoded[0]) / 10;
		decimal[34 - i] = c4;
	}

	assert(e.encoded[0] == 0 && e.encoded[1] == 0 && e.encoded[2] == 0 && e.encoded[3] == 0);
	char* q = confirmation_id;
	for (i = 0; i < 7; i++) {
		if (i)
			*q++ = '-';
		unsigned char* p = decimal + i*5;
		q[0] = p[0] + '0';
		q[1] = p[1] + '0';
		q[2] = p[2] + '0';
		q[3] = p[3] + '0';
		q[4] = p[4] + '0';
		q[5] = ((p[0]+p[1]*2+p[2]+p[3]*2+p[4]) % 7) + '0';
		q += 6;
	}
	*q++ = 0;
	return 0;
}
