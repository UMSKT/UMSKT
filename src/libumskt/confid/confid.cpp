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

void ConfirmationID::setMod(QWORD mod)
{
    MOD = mod;
}

void ConfirmationID::setNonResidue(QWORD nonResidue)
{
    NON_RESIDUE = nonResidue;
}

void ConfirmationID::setPValues(QWORD p0, QWORD p1, QWORD p2, QWORD p3)
{
    p[0] = p0;
    p[1] = p1;
    p[2] = p2;
    p[3] = p3;
}

void ConfirmationID::setPValues(QWORD pValues[4])
{
    memcpy(p, pValues, sizeof(*pValues * 4));
}

void ConfirmationID::setFValues(QWORD f0, QWORD f1, QWORD f2, QWORD f3, QWORD f4, QWORD f5)
{
    f[0] = f0;
    f[1] = f1;
    f[2] = f2;
    f[3] = f3;
    f[4] = f4;
    f[5] = f5;
}

void ConfirmationID::setFValues(QWORD fValues[6])
{
    memcpy(p, fValues, sizeof(*fValues * 6));
}

void ConfirmationID::setIsOffice(BOOL isOffice)
{
    this->isOffice = isOffice;
}

void ConfirmationID::setIsXPBrand(BOOL isXpBrand)
{
    this->isXPBrand = isXpBrand;
}

void ConfirmationID::setFlagVersion(unsigned int flagVersion)
{
    ConfirmationID::flagVersion = flagVersion;
}


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

void ConfirmationID::decode_iid_new_version(unsigned char* iid, unsigned char* hwid, int* version)
{
    QWORD buffer[5];
    for (int i = 0; i < 5; i++)
    {
        memcpy(&buffer[i], (iid + (4 * i)), 4);
    }

    DWORD v1 = (buffer[3] & 0xFFFFFFF8) | 2;
    DWORD v2 = ((buffer[3] & 7) << 29) | (buffer[2] >> 3);
    QWORD hardwareIDVal = ((QWORD)v1 << 32) | v2;
    for (int i = 0; i < 8; ++i)
    {
        hwid[i] = (hardwareIDVal >> (8 * i)) & 0xFF;
    }

    *version = buffer[0] & 7;
}

void ConfirmationID::Mix(unsigned char* buffer, size_t bufSize, const unsigned char* key, size_t keySize)
{
	unsigned char sha1_input[64], sha1_result[20];
	size_t half = bufSize / 2;

	//assert(half <= sizeof(sha1_result) && half + keySize <= sizeof(sha1_input) - 9);
	for (int external_counter = 0; external_counter < 4; external_counter++)
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

        SHA1(sha1_input, sizeof(sha1_input), sha1_result);

		for (size_t i = half & ~3; i < half; i++)
        {
            sha1_result[i] = sha1_result[i + 4 - (half & 3)];
        }

		for (size_t i = 0; i < half; i++)
        {
			unsigned char tmp = buffer[i + half];
			buffer[i + half] = buffer[i] ^ sha1_result[i];
			buffer[i] = tmp;
		}
	}
}

void ConfirmationID::Unmix(unsigned char* buffer, size_t bufSize, const unsigned char key[4], size_t keySize)
{
	unsigned char sha1_input[64];
	unsigned char sha1_result[20];
	size_t half = bufSize / 2;
	//assert(half <= sizeof(sha1_result) && half + keySize <= sizeof(sha1_input) - 9);

	for (int external_counter = 0; external_counter < 4; external_counter++)
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

		SHA1(sha1_input, sizeof(sha1_input), sha1_result);

		for (size_t i = half & ~3; i < half; i++)
        {
            sha1_result[i] = sha1_result[i + 4 - (half & 3)];
        }

		for (size_t i = 0; i < half; i++)
        {
			unsigned char tmp = buffer[i];
			buffer[i] = buffer[i + half] ^ sha1_result[i];
			buffer[i + half] = tmp;
		}
	}
}

int ConfirmationID::Generate(const char* installation_id_str, char confirmation_id[49], std::string productid)
{
	int version;
	unsigned char hardwareID[8];
	unsigned char installation_id[19]; // 10**45 < 256**19
    unsigned char productID[4];

	size_t installation_id_len = 0;
	const char* pid = installation_id_str;

	size_t count = 0, totalCount = 0;
	unsigned check = 0;

	size_t i;

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

		if (count == 5 || p[1] == 0)
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
	struct {
		QWORD HardwareID;
		QWORD ProductIDLow;
		unsigned char ProductIDHigh;
		unsigned short KeySHA1;
	} parsed;
#pragma pack(pop)

    if (isXPBrand)
    {
        memcpy(&parsed, installation_id, sizeof(parsed));
        productID[0] = parsed.ProductIDLow & ((1 << 17) - 1);
        productID[1] = (parsed.ProductIDLow >> 17) & ((1 << 10) - 1);
        productID[2] = (parsed.ProductIDLow >> 27) & ((1 << 24) - 1);
        version      = (parsed.ProductIDLow >> 51) & 15;
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
        productID[0] = stoi(productid.substr(0,5));

        std::string channelid = productid.substr(6,3);
        char *p = &channelid[0];
        for (; *p; p++)
        {
            *p = toupper((unsigned char)*p);
        }

        if (strcmp(&channelid[0], "OEM") == 0)
        {
            productID[1] = stoi(productid.substr(12,3));
            productID[2] = (stoi(productid.substr(15,1)) * 100000) + stoi(productid.substr(18,5));
            productID[2] = calculateCheckDigit(productID[2]);
            productID[3] = ((stoi(productid.substr(10,2))) * 1000) + productID[3];
        }
        else
        {
            productID[1] = stoi(productid.substr(6,3));
            productID[2] = stoi(productid.substr(10,7));
            productID[3] = stoi(productid.substr(18,5));
        }
        //fmt::print("ProductID: {}-{}-{}-{} \n", productID[0], productID[1], productID[2], productID[3]);
    }

	unsigned char keybuf[16];
	memcpy(keybuf, &parsed.HardwareID, 8);

	QWORD productIdMixed = (QWORD)productID[0] << 41 | (QWORD)productID[1] << 58 | (QWORD)productID[2] << 17 | productID[3];
	memcpy(keybuf + 8, &productIdMixed, 8);

	TDivisor d;
	unsigned char attempt;

    union {
        unsigned char buffer[14];
        struct {
            QWORD lo;
            QWORD hi;
        };
    } ulowhi;

	for (attempt = 0; attempt <= 0x80; attempt++)
    {
        ulowhi.lo = this->u[0];
        ulowhi.hi = this->u[1];

        if (isXPBrand)
        {
            ulowhi.buffer[7] = attempt;
        }
        else if (isOffice)
        {
            ulowhi.buffer[6] = attempt;
        }

		Mix(ulowhi.buffer, 14, keybuf, 16);
		QWORD x2 = residue->ui128_quotient_mod(ulowhi.lo, ulowhi.hi);
		QWORD x1 = ulowhi.lo - x2 * MOD;
		x2++;

		d.u[0] = residue->sub(residue->mul(x1, x1), residue->mul(NON_RESIDUE, residue->mul(x2, x2)));
		d.u[1] = residue->add(x1, x1);
		if (divisor->find_divisor_v(&d))
        {
            break;
        }
	}

	if (attempt > 0x80)
    {
        return ERR_UNLUCKY;
    }

    divisor->mul128(&d, u[0], u[1], &d);

	union {
		struct {
			QWORD encoded_lo, encoded_hi;
		};
		struct {
			uint32_t encoded[4];
		};
	} e;

	if (d.u[0] == BAD)
    {
		// we can not get the zero divisor, actually...
		e.encoded_lo = residue->__umul128(MOD + 2, MOD, &e.encoded_hi);
	}
    else if (d.u[1] == BAD)
    {
		// O(1/MOD) chance
		//encoded = (unsigned __int128)(MOD + 1) * d.u[0] + MOD; // * MOD + d.u[0] is fine too
		e.encoded_lo = residue->__umul128(MOD + 1, d.u[0], &e.encoded_hi);
		e.encoded_lo += MOD;
		e.encoded_hi += (e.encoded_lo < MOD);
	}
    else
    {
		QWORD x1 = (d.u[1] % 2 ? d.u[1] + MOD : d.u[1]) / 2;
		QWORD x2sqr = residue->sub(residue->mul(x1, x1), d.u[0]);
		QWORD x2 = residue->sqrt(x2sqr);

        if (x2 == BAD)
        {
			x2 = residue->sqrt(residue->mul(x2sqr, residue->inv(NON_RESIDUE)));
			assert(x2 != BAD);
			e.encoded_lo = residue->__umul128(MOD + 1, MOD + x2, &e.encoded_hi);
			e.encoded_lo += x1;
			e.encoded_hi += (e.encoded_lo < x1);
		}
        else
        {
			// points (-x1+x2, v(-x1+x2)) and (-x1-x2, v(-x1-x2))
			QWORD x1a = residue->sub(x1, x2);
			QWORD y1  = residue->sub(d.v[0], residue->mul(d.v[1], x1a));
			QWORD x2a = residue->add(x1, x2);
			QWORD y2  = residue->sub(d.v[0], residue->mul(d.v[1], x2a));
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

			e.encoded_lo = residue->__umul128(MOD + 1, x1a, &e.encoded_hi);
			e.encoded_lo += x2a;
			e.encoded_hi += (e.encoded_lo < x2a);

		}
	}

	unsigned char decimal[35];
	for (i = 0; i < 35; i++)
    {
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
	for (i = 0; i < 7; i++)
    {
		if (i)
        {
            *q++ = '-';
        }

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
