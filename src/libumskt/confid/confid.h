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
 * @FileCreated by Neo on 06/06/2023
 * @Maintainer Neo
 */

#ifndef UMSKT_CONFID_H
#define UMSKT_CONFID_H

#include "../libumskt.h"

// Confirmation ID generator constants
enum CONFIRMATION_ID_STATUS
{
    SUCCESS = 0,
    ERR_TOO_SHORT = 1,
    ERR_TOO_LARGE = 2,
    ERR_INVALID_CHARACTER = 3,
    ERR_INVALID_CHECK_DIGIT = 4,
    ERR_UNKNOWN_VERSION = 5,
    ERR_UNLUCKY = 6
};

#define BAD 0xFFFFFFFFFFFFFFFFull

typedef struct
{
    QWORD u[2];
    QWORD v[2];
} TDivisor;

class EXPORT ConfirmationID
{
    QWORD MOD = 0, NON_RESIDUE = 0;
    QWORD curve[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    Q_OWORD privateKey;

    BYTE iid_key[4] = {0x0, 0x0, 0x0, 0x0};
    BOOL isOffice = false, isXPBrand = false;
    unsigned flagVersion = 0;

    DWORD calculateCheckDigit(DWORD pid);
    void decode_iid_new_version(BYTE *iid, BYTE *hwid, DWORD *version);
    void Mix(BYTE *buffer, BYTE bufSize, const BYTE *key, BYTE keySize);
    void Unmix(BYTE *buffer, BYTE bufSize, const BYTE *key, BYTE keySize);

    class Divisor
    {
        ConfirmationID *parent;

      public:
        explicit Divisor(ConfirmationID *p)
        {
            parent = p;
        }

        int find_divisor_v(TDivisor *d);
        void add(const TDivisor *src1, const TDivisor *src2, TDivisor *dst);
        void mul(const TDivisor *src, QWORD mult, TDivisor *dst);
        void mul128(const TDivisor *src, QWORD mult_lo, QWORD mult_hi, TDivisor *dst);
        int u2poly(const TDivisor *src, QWORD polyu[3], QWORD polyv[2]);
    };
    friend class Divisor;

    class Residue
    {
        ConfirmationID *parent;

      public:
        explicit Residue(ConfirmationID *p)
        {
            parent = p;
        }

        QWORD add(QWORD x, QWORD y);
        QWORD sub(QWORD x, QWORD y);
        QWORD __umul128(QWORD a, QWORD b, QWORD *hi);
        QWORD ui128_quotient_mod(QWORD lo, QWORD hi);
        QWORD mul(QWORD x, QWORD y);
        QWORD pow(QWORD x, QWORD y);
        QWORD inverse(QWORD u, QWORD v);
        QWORD inv(QWORD x);
        QWORD sqrt(QWORD what);
    };
    friend class Residue;

    class Polynomial
    {
        ConfirmationID *parent;

      public:
        explicit Polynomial(ConfirmationID *p)
        {
            parent = p;
        }

        int mul(int adeg, const QWORD a[], int bdeg, const QWORD b[], int resultprevdeg, QWORD result[]);
        int div_monic(int adeg, QWORD a[], int bdeg, const QWORD b[], QWORD *quotient);
        void xgcd(int adeg, const QWORD a[3], int bdeg, const QWORD b[3], int *pgcddeg, QWORD gcd[3], int *pmult1deg,
                  QWORD mult1[3], int *pmult2deg, QWORD mult2[3]);
    };
    friend class Polynomial;

    Residue *residue;
    Polynomial *polynomial;
    Divisor *divisor;

  public:
    ConfirmationID()
    {
        residue = new Residue(this);
        polynomial = new Polynomial(this);
        divisor = new Divisor(this);
        privateKey.qword[0] = privateKey.qword[1] = 0x00;
    }

    BOOL LoadHyperellipticCurve(const std::string &x0, const std::string &x1, const std::string &x2,
                                const std::string &x3, const std::string &x4, const std::string &x5,
                                const std::string &priv, const std::string &modulous, const std::string &nonresidue,
                                BOOL isOffice, BOOL isXPBrand, BYTE flagVersion);

    BOOL LoadHyperellipticCurve(QWORD x0, QWORD x1, QWORD x2, QWORD x3, QWORD x4, QWORD x5, Q_OWORD priv,
                                QWORD modulous, QWORD nonresidue, BOOL isOffice, BOOL isXPBrand, BYTE flagVersion);

    BOOL LoadHyperellipticCurve(QWORD *f, Q_OWORD priv, QWORD modulous, QWORD nonresidue, BOOL isOffice, BOOL isXPBrand,
                                BYTE flagVersion);

    CONFIRMATION_ID_STATUS Generate(const std::string &installation_id_str, std::string &confirmation_id,
                                    std::string &productid);

    ~ConfirmationID()
    {
        delete residue, polynomial, divisor;
    }
};

#endif // UMSKT_CONFID_H
