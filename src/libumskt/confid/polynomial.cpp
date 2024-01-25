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

/**
 * generic short slow code
 *
 * @param adeg
 * @param a
 * @param bdeg
 * @param b
 * @param resultprevdeg
 * @param result
 * @return
 */
int ConfirmationID::Polynomial::mul(int adeg, const QWORD a[], int bdeg, const QWORD b[], int resultprevdeg,
                                    QWORD result[])
{
    if (adeg < 0 || bdeg < 0)
    {
        return resultprevdeg;
    }

    int i, j;

    for (i = resultprevdeg + 1; i <= adeg + bdeg; i++)
    {
        result[i] = 0;
    }

    resultprevdeg = i - 1;

    for (i = 0; i <= adeg; i++)
    {
        for (j = 0; j <= bdeg; j++)
        {
            result[i + j] = parent->residue->add(result[i + j], parent->residue->mul(a[i], b[j]));
        }
    }

    while (resultprevdeg >= 0 && result[resultprevdeg] == 0)
    {
        --resultprevdeg;
    }

    return resultprevdeg;
}

/**
 *
 * @param adeg
 * @param a
 * @param bdeg
 * @param b
 * @param quotient
 * @return
 */
int ConfirmationID::Polynomial::div_monic(int adeg, QWORD a[], int bdeg, const QWORD b[], QWORD *quotient)
{
    assert(bdeg >= 0);
    assert(b[bdeg] == 1);
    int i, j;

    for (i = adeg - bdeg; i >= 0; i--)
    {
        QWORD q = a[i + bdeg];
        if (quotient)
        {
            quotient[i] = q;
        }
        for (j = 0; j < bdeg; j++)
        {
            a[i + j] = parent->residue->sub(a[i + j], parent->residue->mul(q, b[j]));
        }
        a[i + j] = 0;
    }

    i += bdeg;
    while (i >= 0 && a[i] == 0)
    {
        i--;
    }

    return i;
}

/**
 *
 * @param adeg
 * @param a
 * @param bdeg
 * @param b
 * @param pgcddeg
 * @param gcd
 * @param pmult1deg
 * @param mult1
 * @param pmult2deg
 * @param mult2
 */
void ConfirmationID::Polynomial::xgcd(int adeg, const QWORD a[3], int bdeg, const QWORD b[3], int *pgcddeg,
                                      QWORD gcd[3], int *pmult1deg, QWORD mult1[3], int *pmult2deg, QWORD mult2[3])
{
    int sdeg = -1;
    QWORD s[3] = {0, 0, 0};

    int mult1deg = 0;
    mult1[0] = 1;
    mult1[1] = 0;
    mult1[2] = 0;

    int tdeg = 0;
    QWORD t[3] = {1, 0, 0};

    int mult2deg = -1;
    mult2[0] = 0;
    mult2[1] = 0;
    mult2[2] = 0;

    int rdeg = bdeg;
    QWORD r[3] = {b[0], b[1], b[2]};

    int gcddeg = adeg;
    gcd[0] = a[0];
    gcd[1] = a[1];
    gcd[2] = a[2];
    // s*u1 + t*u2 = r
    // mult1*u1 + mult2*u2 = gcd

    while (rdeg >= 0)
    {
        if (rdeg > gcddeg)
        {
            unsigned tmp;
            int tmpi;

            tmp = rdeg;
            rdeg = gcddeg;
            gcddeg = tmp;

            tmpi = sdeg;
            sdeg = mult1deg;
            mult1deg = tmpi;

            tmpi = tdeg;
            tdeg = mult2deg;
            mult2deg = tmpi;

            QWORD tmp2;
            tmp2 = r[0];
            r[0] = gcd[0];
            gcd[0] = tmp2;

            tmp2 = r[1];
            r[1] = gcd[1];
            gcd[1] = tmp2;

            tmp2 = r[2];
            r[2] = gcd[2];
            gcd[2] = tmp2;

            tmp2 = s[0];
            s[0] = mult1[0];
            mult1[0] = tmp2;

            tmp2 = s[1];
            s[1] = mult1[1];
            mult1[1] = tmp2;

            tmp2 = s[2];
            s[2] = mult1[2];
            mult1[2] = tmp2;

            tmp2 = t[0];
            t[0] = mult2[0];
            mult2[0] = tmp2;

            tmp2 = t[1];
            t[1] = mult2[1];
            mult2[1] = tmp2;

            tmp2 = t[2];
            t[2] = mult2[2];
            mult2[2] = tmp2;

            continue;
        }

        int delta = gcddeg - rdeg;
        QWORD mult = parent->residue->mul(gcd[gcddeg], parent->residue->inv(r[rdeg]));
        // quotient = mult * x**delta

        assert(rdeg + delta < 3);
        for (int i = 0; i <= rdeg; i++)
        {
            gcd[i + delta] = parent->residue->sub(gcd[i + delta], parent->residue->mul(mult, r[i]));
        }

        while (gcddeg >= 0 && gcd[gcddeg] == 0)
        {
            gcddeg--;
        }
        assert(sdeg + delta < 3);

        for (int i = 0; i <= sdeg; i++)
        {
            mult1[i + delta] = parent->residue->sub(mult1[i + delta], parent->residue->mul(mult, s[i]));
        }

        if (mult1deg < sdeg + delta)
        {
            mult1deg = sdeg + delta;
        }

        while (mult1deg >= 0 && mult1[mult1deg] == 0)
        {
            mult1deg--;
        }

        assert(tdeg + delta < 3);

        for (int i = 0; i <= tdeg; i++)
        {
            mult2[i + delta] = parent->residue->sub(mult2[i + delta], parent->residue->mul(mult, t[i]));
        }

        if (mult2deg < tdeg + delta)
        {
            mult2deg = tdeg + delta;
        }

        while (mult2deg >= 0 && mult2[mult2deg] == 0)
        {
            mult2deg--;
        }
    }

    // d1 = gcd, e1 = mult1, e2 = mult2
    *pgcddeg = gcddeg;
    *pmult1deg = mult1deg;
    *pmult2deg = mult2deg;
}