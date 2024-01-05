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
 * @FileCreated by Andrew on 01/06/2023
 * @Maintainer Andrew
 */

#include "PIDGEN3.h"

/**
 * https://xkcd.com/221/
 *
 * @return 4
 */
int getRandomNumber()
{
    return 4; // chosen by fair dice roll
              // guaranteed to be random
}

/**
 * Initializes the elliptic curve
 *
 * @param pSel          [in] prime
 * @param aSel          [in] a
 * @param bSel          [in] b
 * @param generatorXSel [in] G[x]
 * @param generatorYSel [in] G[y]
 * @param publicKeyXSel [in] pub[x]
 * @param publicKeyYSel [in] pub[y]
 * @param genOrderSel   [in] computed order of G
 * @param privateKeySel [in] computed private key
 *
 * @return true on success, false on fail
 */
BOOL PIDGEN3::LoadEllipticCurve(const std::string pSel, const std::string aSel, const std::string bSel,
                                const std::string generatorXSel, const std::string generatorYSel,
                                const std::string publicKeyXSel, const std::string publicKeyYSel,
                                const std::string genOrderSel, const std::string privateKeySel)
{
    // We cannot produce a valid key without knowing the private key k. The reason for this is that
    // we need the result of the function K(x; y) = kG(x; y).

    // We can, however, validate any given key using the available public key: {p, a, b, G, K}.
    // genOrder the order of the generator G, a value we have to reverse -> Schoof's Algorithm.

    // Initialize BIGNUM and BIGNUMCTX structures.
    // BIGNUM - Large numbers
    // BIGNUMCTX - Context large numbers (temporary)

    // Context variable
    BN_CTX *context = BN_CTX_new();

    // We're presented with an elliptic curve, a multivariable function y(x; p; a; b), where
    // y^2 % p = x^3 + ax + b % p.
    BIGNUM *a = BN_CTX_get(context), *b = BN_CTX_get(context), *p = BN_CTX_get(context);

    // Public key will consist of the resulting (x; y) values.
    BIGNUM *publicKeyX = BN_CTX_get(context), *publicKeyY = BN_CTX_get(context);

    // G(x; y) is a generator function, its return value represents a point on the elliptic curve.
    BIGNUM *generatorX = BN_CTX_get(context), *generatorY = BN_CTX_get(context);

    genOrder = BN_new();
    privateKey = BN_new();

    /* Public data */
    BN_dec2bn(&p, &pSel[0]);
    BN_dec2bn(&a, &aSel[0]);
    BN_dec2bn(&b, &bSel[0]);
    BN_dec2bn(&generatorX, &generatorXSel[0]);
    BN_dec2bn(&generatorY, &generatorYSel[0]);

    BN_dec2bn(&publicKeyX, &publicKeyXSel[0]);
    BN_dec2bn(&publicKeyY, &publicKeyYSel[0]);

    /* Computed Data */
    BN_dec2bn(&genOrder, &genOrderSel[0]);
    BN_dec2bn(&privateKey, &privateKeySel[0]);
    BN_sub(privateKey, genOrder, privateKey);

    /* Elliptic Curve calculations. */
    // The group is defined via Fp = all integers [0; p - 1], where p is prime.
    // The function EC_POINT_set_affine_coordinates() sets the x and y coordinates for the point p defined over the
    // curve given in group.
    eCurve = EC_GROUP_new_curve_GFp(p, a, b, context);

    // Create new point for the generator on the elliptic curve and set its coordinates to (genX; genY).
    genPoint = EC_POINT_new(eCurve);
    EC_POINT_set_affine_coordinates(eCurve, genPoint, generatorX, generatorY, context);

    // Create new point for the public key on the elliptic curve and set its coordinates to (pubX; pubY).
    pubPoint = EC_POINT_new(eCurve);
    EC_POINT_set_affine_coordinates(eCurve, pubPoint, publicKeyX, publicKeyY, context);

    // If generator and public key points are not on the elliptic curve, either the generator or the public key values
    // are incorrect.
    assert(EC_POINT_is_on_curve(eCurve, genPoint, context) == true);
    assert(EC_POINT_is_on_curve(eCurve, pubPoint, context) == true);

    // Cleanup
    BN_CTX_free(context);

    return true;
}

/**
 * Convert data between endianness types.
 *
 * @param data   [in]
 * @param length [in]
 **/
inline void PIDGEN3::endian(BYTE *data, int length)
{
    for (int i = 0; i < length / 2; i++)
    {
        BYTE temp = data[i];
        data[i] = data[length - i - 1];
        data[length - i - 1] = temp;
    }
}

/**
 * Converts an OpenSSL BigNumber to it's Little Endian binary equivalent
 *
 * @param a     [in] BigNumber to convert
 * @param to    [out] char* binary representation
 * @param tolen [in] length of the char* array
 *
 * @return length of number in to
 **/
int PIDGEN3::BN_bn2lebin(const BIGNUM *a, unsigned char *to, int tolen)
{
    if (a == nullptr || to == nullptr)
    {
        return 0;
    }

    int len = BN_bn2bin(a, to);

    if (len > tolen)
    {
        return -1;
    }

    // Choke point inside BN_bn2lebinpad: OpenSSL uses len instead of tolen.
    endian(to, tolen);

    return len;
}

/**
 * Converts from byte sequence to the CD-key.
 *
 * @param cdKey   [out] std::string CDKey input
 * @param byteSeq [in] BYTE*
 **/
void PIDGEN3::base24(std::string &cdKey, BYTE *byteSeq)
{
    BYTE rbyteSeq[16], output[26];
    BIGNUM *z;

    // Copy byte sequence to the reversed byte sequence.
    memcpy(rbyteSeq, byteSeq, sizeof(rbyteSeq));

    // Skip trailing zeroes and reverse y.
    int length;

    for (length = 15; rbyteSeq[length] <= 0; length--)
    {
        ; // do nothing, just counting
    }

    endian(rbyteSeq, ++length);

    // Convert reversed byte sequence to BigNum z.
    z = BN_bin2bn(rbyteSeq, length, nullptr);

    // Divide z by 24 and convert the remainder to a CD-key char.
    for (int i = 24; i >= 0; i--)
    {
        output[i] = pKeyCharset[BN_div_word(z, 24)];
    }

    output[25] = 0;

    cdKey = (char *)output;

    BN_free(z);
}

/**
 * Converts from CD-key to a byte sequence.
 *
 * @param byteSeq [out] *BYTE representation of the CDKey
 * @param cdKey   [in] std::string CDKey to convert
 **/
void PIDGEN3::unbase24(BYTE *byteSeq, std::string cdKey)
{
    BYTE pDecodedKey[PK_LENGTH + NULL_TERMINATOR]{};
    BIGNUM *y = BN_new();

    BN_zero(y);

    // Remove dashes from the CD-key and put it into a Base24 byte array.
    for (int i = 0, k = 0; i < cdKey.length() && k < PK_LENGTH; i++)
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
