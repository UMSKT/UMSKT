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
 * @FileCreated by Neo on 5/26/2023
 * @Maintainer Andrew
 */

#include "PIDGEN3.h"

/* Converts from CD-key to a byte sequence. */
void PIDGEN3::unbase24(BYTE *byteSeq, const char *cdKey)
{
    BYTE pDecodedKey[PK_LENGTH + NULL_TERMINATOR]{};
    BIGNUM *y = BN_new();

    BN_zero(y);

    // Remove dashes from the CD-key and put it into a Base24 byte array.
    for (int i = 0, k = 0; i < strlen(cdKey) && k < PK_LENGTH; i++)
    {
        for (int j = 0; j < 24; j++)
        {
            if (cdKey[i] != '-' && cdKey[i] == pKeyCharset[j])
            {
                pDecodedKey[k++] = j;
                break;
            }
        }
    }

    // Empty byte sequence.
    memset(byteSeq, 0, 16);

    // Calculate the weighed sum of byte array elements.
    for (int i = 0; i < PK_LENGTH; i++)
    {
        BN_mul_word(y, PK_LENGTH - 1);
        BN_add_word(y, pDecodedKey[i]);
    }

    // Acquire length.
    int n = BN_num_bytes(y);

    // Place the generated code into the byte sequence.
    BN_bn2bin(y, byteSeq);
    BN_free(y);

    // Reverse the byte sequence.
    endian(byteSeq, n);
}

/* Converts from byte sequence to the CD-key. */
void PIDGEN3::base24(char *cdKey, BYTE *byteSeq)
{
    BYTE rbyteSeq[16];
    BIGNUM *z;

    // Copy byte sequence to the reversed byte sequence.
    memcpy(rbyteSeq, byteSeq, sizeof(rbyteSeq));

    // Skip trailing zeroes and reverse y.
    int length;

    for (length = 15; rbyteSeq[length] == 0; length--)
    {
        ;
    }
    endian(rbyteSeq, ++length);

    // Convert reversed byte sequence to BigNum z.
    z = BN_bin2bn(rbyteSeq, length, nullptr);

    // Divide z by 24 and convert the remainder to a CD-key char.
    cdKey[25] = 0;

    for (int i = 24; i >= 0; i--)
    {
        cdKey[i] = pKeyCharset[BN_div_word(z, 24)];
    }

    BN_free(z);
}
