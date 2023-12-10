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
 * @FileCreated by Neo on 12/05/2023
 * @Maintainer Neo
 */

#include "confid.h"

int Divisor::find_divisor_v(TDivisor *d)
{
    // u | v^2 - f
    // u = u0 + u1*x + x^2
    // f%u = f0 + f1*x
    QWORD v1, f2[6];

    for (int i = 0; i < 6; i++)
    {
        f2[i] = parent->f[i];
    }

    const QWORD u0 = d->u[0];
    const QWORD u1 = d->u[1];
    for (int j = 4; j--;)
    {
        f2[j] = parent->residue->sub(f2[j], parent->residue->mul(u0, f2[j + 2]));
        f2[j + 1] = parent->residue->sub(f2[j + 1], parent->residue->mul(u1, f2[j + 2]));
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
    const QWORD u0double = parent->residue->add(u0, u0);
    const QWORD coeff2 = parent->residue->sub(parent->residue->mul(u1, u1), parent->residue->add(u0double, u0double));
    const QWORD coeff1 = parent->residue->sub(parent->residue->add(f0, f0), parent->residue->mul(f1, u1));

    if (coeff2 == 0)
    {
        if (coeff1 == 0)
        {
            if (f1 == 0)
            {
                // impossible
                // printf("bad f(), double root detected\n");
            }
            return 0;
        }
        QWORD sqr = parent->residue->mul(parent->residue->mul(f1, f1),
                                         parent->residue->inv(parent->residue->add(coeff1, coeff1)));
        v1 = parent->residue->sqrt(sqr);
        if (v1 == BAD)
        {
            return 0;
        }
    }
    else
    {
        QWORD d = parent->residue->add(
            parent->residue->mul(f0, f0),
            parent->residue->mul(f1, parent->residue->sub(parent->residue->mul(f1, u0), parent->residue->mul(f0, u1))));
        d = parent->residue->sqrt(d);
        if (d == BAD)
        {
            return 0;
        }

        d = parent->residue->add(d, d);
        QWORD inv = parent->residue->inv(coeff2);
        QWORD root = parent->residue->mul(parent->residue->add(coeff1, d), inv);
        v1 = parent->residue->sqrt(root);
        if (v1 == BAD)
        {
            root = parent->residue->mul(parent->residue->sub(coeff1, d), inv);
            v1 = parent->residue->sqrt(root);
            if (v1 == BAD)
            {
                return 0;
            }
        }
    }

    QWORD v0 = parent->residue->mul(parent->residue->add(f1, parent->residue->mul(u1, parent->residue->mul(v1, v1))),
                                    parent->residue->inv(parent->residue->add(v1, v1)));
    d->v[0] = v0;
    d->v[1] = v1;

    return 1;
}

int Divisor::u2poly(const TDivisor *src, QWORD polyu[3], QWORD polyv[2])
{
    if (src->u[1] != BAD)
    {
        polyu[0] = src->u[0];
        polyu[1] = src->u[1];
        polyu[2] = 1;
        polyv[0] = src->v[0];
        polyv[1] = src->v[1];
        return 2;
    }

    if (src->u[0] != BAD)
    {
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

void Divisor::add(const TDivisor *src1, const TDivisor *src2, TDivisor *dst)
{
    QWORD u1[3], u2[3], v1[2], v2[2];
    int u1deg = u2poly(src1, u1, v1);
    int u2deg = u2poly(src2, u2, v2);

    // extended gcd: d1 = gcd(u1, u2) = e1*u1 + e2*u2
    int d1deg, e1deg, e2deg;
    QWORD d1[3], e1[3], e2[3];
    parent->polynomial->xgcd(u1deg, u1, u2deg, u2, &d1deg, d1, &e1deg, e1, &e2deg, e2);
    assert(e1deg <= 1);
    assert(e2deg <= 1);

    // extended gcd again: d = gcd(d1, v1+v2) = c1*d1 + c2*(v1+v2)
    QWORD b[3] = {parent->residue->add(v1[0], v2[0]), parent->residue->add(v1[1], v2[1]), 0};
    int bdeg = (b[1] == 0 ? (b[0] == 0 ? -1 : 0) : 1);
    int ddeg, c1deg, c2deg;
    QWORD d[3], c1[3], c2[3];
    parent->polynomial->xgcd(d1deg, d1, bdeg, b, &ddeg, d, &c1deg, c1, &c2deg, c2);
    assert(c1deg <= 0);
    assert(c2deg <= 1);
    assert(ddeg >= 0);
    QWORD dmult = parent->residue->inv(d[ddeg]);
    int i;
    for (i = 0; i < ddeg; i++)
    {
        d[i] = parent->residue->mul(d[i], dmult);
    }

    d[i] = 1;
    for (i = 0; i <= c1deg; i++)
    {
        c1[i] = parent->residue->mul(c1[i], dmult);
    }

    for (i = 0; i <= c2deg; i++)
    {
        c2[i] = parent->residue->mul(c2[i], dmult);
    }

    QWORD u[5];
    int udeg = parent->polynomial->mul(u1deg, u1, u2deg, u2, -1, u);
    // u is monic

    QWORD v[7], tmp[7];
    int vdeg, tmpdeg;
    // c1*(e1*u1*v2 + e2*u2*v1) + c2*(v1*v2 + f)
    // c1*(e1*u1*(v2-v1) + d1*v1) + c2*(v1*v2 + f)
    v[0] = parent->residue->sub(v2[0], v1[0]);
    v[1] = parent->residue->sub(v2[1], v1[1]);
    tmpdeg = parent->polynomial->mul(e1deg, e1, 1, v, -1, tmp);
    vdeg = parent->polynomial->mul(u1deg, u1, tmpdeg, tmp, -1, v);
    vdeg = parent->polynomial->mul(d1deg, d1, 1, v1, vdeg, v);

    for (i = 0; i <= vdeg; i++)
    {
        v[i] = parent->residue->mul(v[i], c1[0]);
    }

    memcpy(tmp, parent->f, 6 * sizeof(parent->f[0]));
    tmpdeg = 5;
    tmpdeg = parent->polynomial->mul(1, v1, 1, v2, tmpdeg, tmp);
    vdeg = parent->polynomial->mul(c2deg, c2, tmpdeg, tmp, vdeg, v);

    if (ddeg > 0)
    {
        assert(udeg >= 2 * ddeg);
        QWORD udiv[5];
        parent->polynomial->div_monic(udeg, u, ddeg, d, udiv);
        udeg -= ddeg;
        parent->polynomial->div_monic(udeg, udiv, ddeg, d, u);
        udeg -= ddeg;
        if (vdeg >= 0)
        {
            assert(vdeg >= ddeg);
            parent->polynomial->div_monic(vdeg, v, ddeg, d, udiv);
            vdeg -= ddeg;
            memcpy(v, udiv, (vdeg + 1) * sizeof(v[0]));
        }
    }

    vdeg = parent->polynomial->div_monic(vdeg, v, udeg, u, NULL);

    while (udeg > 2)
    {
        assert(udeg <= 4);
        assert(vdeg <= 3);
        // u' = monic((f-v^2)/u), v'=-v mod u'
        tmpdeg = parent->polynomial->mul(vdeg, v, vdeg, v, -1, tmp);
        for (i = 0; i <= tmpdeg && i <= 5; i++)
        {
            tmp[i] = parent->residue->sub(parent->f[i], tmp[i]);
        }

        for (; i <= tmpdeg; i++)
        {
            tmp[i] = parent->residue->sub(0, tmp[i]);
        }

        for (; i <= 5; i++)
        {
            tmp[i] = parent->f[i];
        }

        tmpdeg = i - 1;
        QWORD udiv[5];
        parent->polynomial->div_monic(tmpdeg, tmp, udeg, u, udiv);

        udeg = tmpdeg - udeg;
        QWORD mult = parent->residue->inv(udiv[udeg]);

        for (i = 0; i < udeg; i++)
        {
            u[i] = parent->residue->mul(udiv[i], mult);
        }

        u[i] = 1;

        for (i = 0; i <= vdeg; i++)
        {
            v[i] = parent->residue->sub(0, v[i]);
        }

        vdeg = parent->polynomial->div_monic(vdeg, v, udeg, u, NULL);
    }

    if (udeg == 2)
    {
        dst->u[0] = u[0];
        dst->u[1] = u[1];
        dst->v[0] = (vdeg >= 0 ? v[0] : 0);
        dst->v[1] = (vdeg >= 1 ? v[1] : 0);
    }
    else if (udeg == 1)
    {
        dst->u[0] = u[0];
        dst->u[1] = BAD;
        dst->v[0] = (vdeg >= 0 ? v[0] : 0);
        dst->v[1] = BAD;
    }
    else
    {
        assert(udeg == 0);
        dst->u[0] = BAD;
        dst->u[1] = BAD;
        dst->v[0] = BAD;
        dst->v[1] = BAD;
    }
}

#define divisor_double(src, dst) add(src, src, dst)

void Divisor::mul(const TDivisor *src, QWORD mult, TDivisor *dst)
{
    if (mult == 0)
    {
        dst->u[0] = BAD;
        dst->u[1] = BAD;
        dst->v[0] = BAD;
        dst->v[1] = BAD;
        return;
    }

    TDivisor cur = *src;
    while (!(mult & 1))
    {
        divisor_double(&cur, &cur);
        mult >>= 1;
    }

    *dst = cur;
    while ((mult >>= 1) != 0)
    {
        divisor_double(&cur, &cur);
        if (mult & 1)
        {
            add(dst, &cur, dst);
        }
    }
}

void Divisor::mul128(const TDivisor *src, QWORD mult_lo, QWORD mult_hi, TDivisor *dst)
{
    if (mult_lo == 0 && mult_hi == 0)
    {
        dst->u[0] = BAD;
        dst->u[1] = BAD;
        dst->v[0] = BAD;
        dst->v[1] = BAD;
        return;
    }

    TDivisor cur = *src;
    while (!(mult_lo & 1))
    {
        divisor_double(&cur, &cur);
        mult_lo >>= 1;
        if (mult_hi & 1)
        {
            mult_lo |= (1ULL << 63);
        }
        mult_hi >>= 1;
    }

    *dst = cur;
    for (;;)
    {
        mult_lo >>= 1;
        if (mult_hi & 1)
        {
            mult_lo |= (1ULL << 63);
        }

        mult_hi >>= 1;
        if (mult_lo == 0 && mult_hi == 0)
        {
            break;
        }

        divisor_double(&cur, &cur);
        if (mult_lo & 1)
        {
            add(dst, &cur, dst);
        }
    }
}
