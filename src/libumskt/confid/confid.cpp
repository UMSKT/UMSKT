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
 * @FileCreated by WitherOrNot on 06/02/2023
 * @Maintainer WitherOrNot
 *
 * @History {
 *  This algorithm was provided to the UMSKT project by diamondggg
 *  the history provided by diamondggg is that they are the originator of the code
 *  and was created in tandem with an acquaintance who knows number theory.
 *  The file dates suggest this code was written sometime in 2017/2018
 *
 *  The algorithm was refactored by Neo in 2023
 * }
 */

#include "confid.h"

/**
 *
 * @param x0
 * @param x1
 * @param x2
 * @param x3
 * @param x4
 * @param x5
 * @param priv
 * @param modulus
 * @param nonresidue
 * @param isOffice
 * @param isXPBrand
 * @param flagVersion
 * @return
 */
BOOL ConfirmationID::LoadHyperellipticCurve(QWORD x0, QWORD x1, QWORD x2, QWORD x3, QWORD x4, QWORD x5, Q_OWORD priv,
                                            QWORD modulus, QWORD nonresidue, DWORD32 iidkey, BOOL isOffice,
                                            BOOL isXPBrand, BYTE flagVersion)
{
    QWORD fvals[6] = {x0, x1, x2, x3, x4, x5};

    return LoadHyperellipticCurve(fvals, priv, modulus, nonresidue, iidkey, isOffice, isXPBrand, flagVersion);
}

/**
 *
 * @param f
 * @param priv
 * @param modulus
 * @param nonresidue
 * @param isOffice
 * @param isXPBrand
 * @param flagVersion
 * @return
 */
BOOL ConfirmationID::LoadHyperellipticCurve(QWORD *f, Q_OWORD priv, QWORD modulus, QWORD nonresidue, DWORD32 iidkey,
                                            BOOL isOffice, BOOL isXPBrand, BYTE flagVersion)
{
    memcpy(&curve, f, sizeof(curve));
    memcpy(&privateKey, &priv, sizeof(Q_OWORD));

    MOD = modulus;
    NON_RESIDUE = nonresidue;
    this->isOffice = isOffice;
    this->isXPBrand = isXPBrand;
    this->flagVersion = flagVersion;
    return true;
}

BOOL ConfirmationID::LoadHyperellipticCurve(const std::string &x0, const std::string &x1, const std::string &x2,
                                            const std::string &x3, const std::string &x4, const std::string &x5,
                                            const std::string &priv, const std::string &modulus,
                                            const std::string &nonresidue, const std::string &iidkey, BOOL isOffice,
                                            BOOL isXPBrand, BYTE flagVersion)
{
    std::string f[6];
    f[0] = x0;
    f[1] = x1;
    f[2] = x2;
    f[3] = x3;
    f[4] = x4;
    f[5] = x5;

    return LoadHyperellipticCurve(f, priv, modulus, nonresidue, iidkey, isOffice, isXPBrand, flagVersion);
}

BOOL ConfirmationID::LoadHyperellipticCurve(const std::string *f, const std::string &priv, const std::string &modulus,
                                            const std::string &nonresidue, const std::string &iidkey, BOOL isOffice,
                                            BOOL isXPBrand, BYTE flagVersion)
{
    for (int i = 0; i < 6; i++)
    {
        EncodeN(IntegerS(f[i]), curve[i]);
    }

    EncodeN(IntegerS(priv), privateKey);

    EncodeN(IntegerS(modulus), MOD);

    EncodeN(IntegerS(nonresidue), NON_RESIDUE);

    this->isOffice = isOffice;
    this->isXPBrand = isXPBrand;
    this->flagVersion = flagVersion;

    return true;
}

/**
 *
 * @param pid
 * @return
 */
DWORD32 ConfirmationID::calculateCheckDigit(DWORD32 pid)
{
    DWORD32 i = 0, j = 0, k = 0;
    for (j = pid; j; i += k)
    {
        k = j % 10;
        j /= 10;
    }

    return ((10 * pid) - (i % 7)) + 7;
}

/**
 *
 * @param iid
 * @param hwid
 * @param version
 */
void ConfirmationID::decode_iid_new_version(BYTE *iid, BYTE *hwid, DWORD32 *version)
{
    QWORD buffer[5];
    for (BYTE i = 0; i < 5; i++)
    {
        memcpy(&buffer[i], (iid + (4 * i)), 4);
    }

    DWORD32 v1 = (buffer[3] & 0xFFFFFFF8) | 2;
    DWORD32 v2 = ((buffer[3] & 7) << 29) | (buffer[2] >> 3);
    QWORD hardwareIDVal = ((QWORD)v1 << 32) | v2;
    for (BYTE i = 0; i < 8; ++i)
    {
        hwid[i] = (hardwareIDVal >> (8 * i)) & 0xFF;
    }

    *version = buffer[0] & 7;
}

/**
 *
 * @param buffer
 * @param bufSize
 * @param key
 * @param keySize
 */
void ConfirmationID::Mix(BYTE *buffer, BYTE bufSize, const BYTE *key, BYTE keySize)
{
    BYTE sha1_input[64], sha1_result[SHA1::DIGESTSIZE];
    BYTE half = bufSize / 2;
    auto digest = SHA1();

    // assert(half <= sizeof(sha1_result) && half + keySize <= sizeof(sha1_input) - 9);
    for (BYTE external_counter = 0; external_counter < 4; external_counter++)
    {
        memset(sha1_input, 0, sizeof(sha1_input));

        if (isXPBrand)
        {
            memcpy(sha1_input, buffer + half, half);
            memcpy(sha1_input + half, key, keySize);

            sha1_input[half + keySize] = 0x80;
            sha1_input[sizeof(sha1_input) - 1] = (half + keySize) * 8;
            sha1_input[sizeof(sha1_input) - 2] = (half + keySize) * 8 / 0x100;
        }
        else if (isOffice)
        {
            sha1_input[0] = 0x79;
            memcpy(sha1_input + 1, buffer + half, half);
            memcpy(sha1_input + 1 + half, key, keySize);

            sha1_input[1 + half + keySize] = 0x80;
            sha1_input[sizeof(sha1_input) - 1] = (1 + half + keySize) * 8;
            sha1_input[sizeof(sha1_input) - 2] = (1 + half + keySize) * 8 / 0x100;
        }

        digest.Update(sha1_input, sizeof(sha1_input));
        digest.Final(sha1_result);

        for (BYTE i = half & ~3; i < half; i++)
        {
            sha1_result[i] = sha1_result[i + 4 - (half & 3)];
        }

        for (BYTE i = 0; i < half; i++)
        {
            unsigned char tmp = buffer[i + half];
            buffer[i + half] = buffer[i] ^ sha1_result[i];
            buffer[i] = tmp;
        }
    }
}

/**
 *
 * @param buffer
 * @param bufSize
 * @param key
 * @param keySize
 */
void ConfirmationID::Unmix(BYTE *buffer, BYTE bufSize, const BYTE key[4], BYTE keySize)
{
    BYTE sha1_input[64], sha1_result[SHA1::DIGESTSIZE];
    BYTE half = bufSize / 2;
    auto digest = SHA1();
    // assert(half <= sizeof(sha1_result) && half + keySize <= sizeof(sha1_input) - 9);

    for (BYTE external_counter = 0; external_counter < 4; external_counter++)
    {
        memset(sha1_input, 0, sizeof(sha1_input));

        if (isXPBrand)
        {
            memcpy(sha1_input, buffer, half);
            memcpy(sha1_input + half, key, keySize);
            sha1_input[half + keySize] = 0x80;
            sha1_input[sizeof(sha1_input) - 1] = (half + keySize) * 8;
            sha1_input[sizeof(sha1_input) - 2] = (half + keySize) * 8 / 0x100;
        }
        else if (isOffice)
        {
            sha1_input[0] = 0x79;
            memcpy(sha1_input + 1, buffer, half);
            memcpy(sha1_input + 1 + half, key, keySize);
            sha1_input[1 + half + keySize] = 0x80;
            sha1_input[sizeof(sha1_input) - 1] = (1 + half + keySize) * 8;
            sha1_input[sizeof(sha1_input) - 2] = (1 + half + keySize) * 8 / 0x100;
        }

        digest.Update(sha1_input, sizeof(sha1_input));
        digest.Final(sha1_result);

        for (BYTE i = half & ~3; i < half; i++)
        {
            sha1_result[i] = sha1_result[i + 4 - (half & 3)];
        }

        for (BYTE i = 0; i < half; i++)
        {
            unsigned char tmp = buffer[i];
            buffer[i] = buffer[i + half] ^ sha1_result[i];
            buffer[i + half] = tmp;
        }
    }
}

/**
 *
 * @param installationIDIn
 * @param confirmationIDOut
 * @param productIDIn
 * @return
 */
CONFIRMATION_ID_STATUS ConfirmationID::Generate(const std::string &installationIDIn, std::string &confirmationIDOut,
                                                std::string &productIDIn)
{
    DWORD32 version;
    BYTE hardwareID[8];
    BYTE installation_id[19]; // 10**45 < 256**19
    BYTE productID[4];

    BYTE installation_id_len = 0;
    auto pid = &installationIDIn[0];

    BYTE count = 0, totalCount = 0;
    unsigned check = 0;

    BYTE i;

    Q_OWORD mod_inv;
    memcpy(&mod_inv, &privateKey, sizeof(mod_inv));

    for (; *pid; pid++)
    {
        if (*pid == ' ' || *pid == '-')
        {
            continue;
        }

        int d = *pid - '0';

        if (d < 0 || d > 9)
        {
            return ERR_INVALID_CHARACTER;
        }

        if (count == 5 || mod_inv.qword[0] == 0)
        {
            if (!count)
            {
                return (totalCount == 45) ? ERR_TOO_LARGE : ERR_TOO_SHORT;
            }

            if (d != check % 7)
            {
                return (count < 5) ? ERR_TOO_SHORT : ERR_INVALID_CHECK_DIGIT;
            }

            check = 0;
            count = 0;
            continue;
        }

        check += (count % 2 ? d * 2 : d);
        count++;

        totalCount++;
        if (totalCount > 45)
        {
            return ERR_TOO_LARGE;
        }

        unsigned char carry = d;
        for (i = 0; i < installation_id_len; i++)
        {
            unsigned x = installation_id[i] * 10 + carry;
            installation_id[i] = x & 0xFF;
            carry = x >> 8;
        }

        if (carry)
        {
            assert(installation_id_len < sizeof(installation_id));
            installation_id[installation_id_len++] = carry;
        }
    }

    if (totalCount != 41 && totalCount < 45)
    {
        return ERR_TOO_SHORT;
    }

    for (; installation_id_len < sizeof(installation_id); installation_id_len++)
    {
        installation_id[installation_id_len] = 0;
    }

    Unmix(installation_id, totalCount == 41 ? 17 : 19, iid_key, 4);

    if (installation_id[18] >= 0x10)
    {
        return ERR_UNKNOWN_VERSION;
    }

#pragma pack(push, 1)
    struct
    {
        QWORD HardwareID;
        QWORD ProductIDLow;
        BYTE ProductIDHigh;
        WORD KeySHA1;
    } parsed;
#pragma pack(pop)

    if (isXPBrand)
    {
        memcpy(&parsed, installation_id, sizeof(parsed));
        productID[0] = parsed.ProductIDLow & ((1 << 17) - 1);
        productID[1] = (parsed.ProductIDLow >> 17) & ((1 << 10) - 1);
        productID[2] = (parsed.ProductIDLow >> 27) & ((1 << 24) - 1);
        version = (parsed.ProductIDLow >> 51) & 15;
        productID[3] = (parsed.ProductIDLow >> 55) | (parsed.ProductIDHigh << 9);

        if (flagVersion == 0)
        {
            if (version != (totalCount == 41 ? 9 : 10))
            {
                return ERR_UNKNOWN_VERSION;
            }
        }
        else if (flagVersion != version)
        {
            return ERR_UNKNOWN_VERSION;
        }
    }
    else if (isOffice)
    {
        decode_iid_new_version(installation_id, hardwareID, &version);

        if (flagVersion != version)
        {
            return ERR_UNKNOWN_VERSION;
        }

        memcpy(&parsed, hardwareID, 8);
        productID[0] = stoi(productIDIn.substr(0, 5));

        auto channelid = productIDIn.substr(6, 3);
        char *p = &channelid[0];
        for (; *p; p++)
        {
            *p = toupper((unsigned char)*p);
        }

        if (strcmp(&channelid[0], "OEM") == 0)
        {
            productID[1] = stoi(productIDIn.substr(12, 3));
            productID[2] = (stoi(productIDIn.substr(15, 1)) * 100000) + stoi(productIDIn.substr(18, 5));
            productID[2] = calculateCheckDigit(productID[2]);
            productID[3] = ((stoi(productIDIn.substr(10, 2))) * 1000) + productID[3];
        }
        else
        {
            productID[1] = stoi(productIDIn.substr(6, 3));
            productID[2] = stoi(productIDIn.substr(10, 7));
            productID[3] = stoi(productIDIn.substr(18, 5));
        }
        // fmt::print("ProductID: {}-{}-{}-{} \n", productID[0], productID[1], productID[2], productID[3]);
    }

    BYTE keybuf[16];
    memcpy(keybuf, &parsed.HardwareID, 8);

    QWORD productIdMixed =
        (QWORD)productID[0] << 41 | (QWORD)productID[1] << 58 | (QWORD)productID[2] << 17 | productID[3];
    memcpy(keybuf + 8, &productIdMixed, 8);

    TDivisor d;
    BYTE attempt;

    Q_OWORD ulowhi;
    memset(&ulowhi, 0, sizeof(ulowhi));

    for (attempt = 0; attempt <= 0x80; attempt++)
    {
        if (isXPBrand)
        {
            ulowhi.byte[7] = attempt;
        }
        else if (isOffice)
        {
            ulowhi.byte[6] = attempt;
        }

        Mix(ulowhi.byte, 14, keybuf, 16);
        QWORD x2 = residue->ui128_quotient_mod(ulowhi.qword[0], ulowhi.qword[1]);
        QWORD x1 = ulowhi.qword[0] - x2 * MOD;
        x2++;

        d.u.qword[0] = residue->sub(residue->mul(x1, x1), residue->mul(NON_RESIDUE, residue->mul(x2, x2)));
        d.u.qword[1] = residue->add(x1, x1);
        if (divisor->find_divisor_v(&d))
        {
            break;
        }
    }

    if (attempt > 0x80)
    {
        return ERR_UNLUCKY;
    }

    Q_OWORD priv;
    memcpy(&priv, &privateKey, sizeof(priv));

    divisor->mul128(&d, priv.qword[0], priv.qword[1], &d);

    Q_OWORD e;
    memset(&e, 0, sizeof(e));

    if (d.u.qword[0] == BAD)
    {
        // we can not get the zero divisor, actually...
        e.qword[0] = residue->__umul128(MOD + 2, MOD, &e.qword[1]);
    }
    else if (d.u.qword[1] == BAD)
    {
        // O(1/MOD) chance
        // encoded = (unsigned __int128)(MOD + 1) * d.u[0] + MOD; // * MOD + d.u[0] is fine too
        e.qword[0] = residue->__umul128(MOD + 1, d.u.qword[0], &e.qword[1]);
        e.qword[0] += MOD;
        e.qword[1] += (e.qword[0] < MOD);
    }
    else
    {
        QWORD x1 = (d.u.qword[1] % 2 ? d.u.qword[1] + MOD : d.u.qword[1]) / 2;
        QWORD x2sqr = residue->sub(residue->mul(x1, x1), d.u.qword[0]);
        QWORD x2 = residue->sqrt(x2sqr);

        if (x2 == BAD)
        {
            x2 = residue->sqrt(residue->mul(x2sqr, residue->inv(NON_RESIDUE)));
            assert(x2 != BAD);
            e.qword[0] = residue->__umul128(MOD + 1, MOD + x2, &e.qword[1]);
            e.qword[0] += x1;
            e.qword[1] += (e.qword[0] < x1);
        }
        else
        {
            // points (-x1+x2, v(-x1+x2)) and (-x1-x2, v(-x1-x2))
            QWORD x1a = residue->sub(x1, x2);
            QWORD y1 = residue->sub(d.v.qword[0], residue->mul(d.v.qword[1], x1a));
            QWORD x2a = residue->add(x1, x2);
            QWORD y2 = residue->sub(d.v.qword[0], residue->mul(d.v.qword[1], x2a));
            if (x1a > x2a)
            {
                QWORD tmp = x1a;
                x1a = x2a;
                x2a = tmp;
            }

            if ((y1 ^ y2) & 1)
            {
                QWORD tmp = x1a;
                x1a = x2a;
                x2a = tmp;
            }

            e.qword[0] = residue->__umul128(MOD + 1, x1a, &e.qword[1]);
            e.qword[0] += x2a;
            e.qword[1] += (e.qword[0] < x2a);
        }
    }

    BYTE decimal[35];
    for (i = 0; i < 35; i++)
    {
        unsigned c = e.byte[3] % 10;
        e.byte[3] /= 10;

        unsigned c2 = ((QWORD)c << 32 | e.byte[2]) % 10;
        e.byte[2] = ((QWORD)c << 32 | e.byte[2]) / 10;

        unsigned c3 = ((QWORD)c2 << 32 | e.byte[1]) % 10;
        e.byte[1] = ((QWORD)c2 << 32 | e.byte[1]) / 10;

        unsigned c4 = ((QWORD)c3 << 32 | e.byte[0]) % 10;
        e.byte[0] = ((QWORD)c3 << 32 | e.byte[0]) / 10;

        decimal[34 - i] = c4;
    }

    assert(e.byte[0] == 0 && e.byte[1] == 0 && e.byte[2] == 0 && e.byte[3] == 0);

    char *q = &confirmationIDOut[0];

    for (i = 0; i < 7; i++)
    {
        if (i)
        {
            *q++ = '-';
        }

        unsigned char *p = decimal + i * 5;
        q[0] = p[0] + '0';
        q[1] = p[1] + '0';
        q[2] = p[2] + '0';
        q[3] = p[3] + '0';
        q[4] = p[4] + '0';
        q[5] = ((p[0] + p[1] * 2 + p[2] + p[3] * 2 + p[4]) % 7) + '0';
        q += 6;
    }

    return SUCCESS;
}
