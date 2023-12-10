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
 * @FileCreated by Neo on 6/6/2023
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

enum ACTIVATION_ALGORITHM
{
    WINDOWS = 0,
    OFFICE_XP = 1,
    OFFICE_2K3 = 2,
    OFFICE_2K7 = 3,
    PLUS_DME = 4,
};

EXPORT class ConfirmationID
{
    QWORD MOD = 0, NON_RESIDUE = 0;
    QWORD f[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    QWORD p[4] = {0x0, 0x0, 0x0, 0x0};
    QWORD u[2] = {0x0, 0x0};

    unsigned char iid_key[4] = {0x0, 0x0, 0x0, 0x0};
    BOOL isOffice = false, isXPBrand = false;
    unsigned flagVersion = 0;

  public:
    void setFlagVersion(unsigned int flagVersion);

  private:
    int calculateCheckDigit(int pid);
    void decode_iid_new_version(unsigned char *iid, unsigned char *hwid, int *version);
    void Mix(unsigned char *buffer, size_t bufSize, const unsigned char *key, size_t keySize);
    void Unmix(unsigned char *buffer, size_t bufSize, const unsigned char *key, size_t keySize);

    friend class Residue;
    Residue *residue;

    friend class Polynomial;
    Polynomial *polynomial;

    friend class Divisor;
    Divisor *divisor;

  public:
    int Generate(const char *installation_id_str, char confirmation_id[49], std::string productid);

    void setMod(QWORD mod);
    void setNonResidue(QWORD nonResidue);
    void setPValues(QWORD p0, QWORD p1, QWORD p2, QWORD p3);
    void setPValues(QWORD pValues[4]);
    void setFValues(QWORD f0, QWORD f1, QWORD f2, QWORD f3, QWORD f4, QWORD f5);
    void setFValues(QWORD fValues[6]);
    void setIsOffice(BOOL isOffice);
    void setIsXPBrand(BOOL isXpBrand);

    ConfirmationID()
    {
        residue = new Residue(this);
        polynomial = new Polynomial(this);
        divisor = new Divisor(this);
    }

    ~ConfirmationID()
    {
        delete residue, polynomial, divisor;
    }
};

class Residue
{
    ConfirmationID *parent;

  public:
    explicit Residue(ConfirmationID *in)
    {
        parent = in;
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

class Polynomial
{
    ConfirmationID *parent;

  public:
    explicit Polynomial(ConfirmationID *in)
    {
        parent = in;
    }

    int mul(int adeg, const QWORD a[], int bdeg, const QWORD b[], int resultprevdeg, QWORD result[]);
    int div_monic(int adeg, QWORD a[], int bdeg, const QWORD b[], QWORD *quotient);
    void xgcd(int adeg, const QWORD a[3], int bdeg, const QWORD b[3], int *pgcddeg, QWORD gcd[3], int *pmult1deg,
              QWORD mult1[3], int *pmult2deg, QWORD mult2[3]);
    int u2poly(const TDivisor *src, QWORD polyu[3], QWORD polyv[2]);
};

class Divisor
{
    ConfirmationID *parent;

  public:
    explicit Divisor(ConfirmationID *in)
    {
        parent = in;
    }

    int find_divisor_v(TDivisor *d);
    void add(const TDivisor *src1, const TDivisor *src2, TDivisor *dst);
    void mul(const TDivisor *src, QWORD mult, TDivisor *dst);
    void mul128(const TDivisor *src, QWORD mult_lo, QWORD mult_hi, TDivisor *dst);
};

#endif // UMSKT_CONFID_H
