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
#include "BINK1998.h"
#include "BINK2002.h"

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
 * Creates an Integer that populates PIDGEN3::MaxSizeBINK1998
 * Invoked during Runtime startup
 *
 * @return
 */
Integer MakeMaxSizeBINK1998()
{
    Integer max;

    // 1 << 385 (or max size of BINK1998 field in bits + 1)
    max.SetBit((12 * 4 * 8) + 1);

    return max;
}

const Integer PIDGEN3::MaxSizeBINK1998 = MakeMaxSizeBINK1998();

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

    // We're presented with an elliptic curve, a multivariable function y(x; p; a; b), where
    // y^2 % p = x^3 + ax + b % p.
    auto p = Integer(&pSel[0]), a = Integer(&aSel[0]), b = Integer(&bSel[0]),

         // Public key will consist of the resulting (x; y) values.
        generatorX = Integer(&generatorXSel[0]), generatorY = Integer(&generatorYSel[0]),

         // G(x; y) is a generator function, its return value represents a point on the elliptic curve.
        publicKeyX = Integer(&publicKeyXSel[0]), publicKeyY = Integer(&publicKeyYSel[0]);

    /* Computed Data */
    genOrder = Integer(&genOrderSel[0]);
    privateKey = Integer(&privateKeySel[0]);

    /* Elliptic Curve calculations. */
    // The group is defined via Fp = all integers [0; p - 1], where p is prime.
    // The function EC_POINT_set_affine_coordinates() sets the x and y coordinates for the point p defined over the
    // curve given in group.
    eCurve = ECP(p, a, b);

    // Create new point N for the generator on the elliptic curve and set its coordinates to (genX; genY).
    genPoint = ECP::Point(generatorX, generatorY);

    // Create new point Q for the public key on the elliptic curve and set its coordinates to (pubX; pubY).
    pubPoint = ECP::Point(publicKeyX, publicKeyY);

    // If generator and public key points are not on the elliptic curve, either the generator or the public key values
    // are incorrect.
    assert(eCurve.VerifyPoint(genPoint) == true);
    assert(eCurve.VerifyPoint(pubPoint) == true);

    return true;
}

PIDGEN3 *PIDGEN3::Factory(const std::string &field)
{
    if (checkFieldStrIsBink1998(field))
    {
        return new BINK1998();
    }
    return new BINK2002();
}

BOOL PIDGEN3::Generate(std::string &pKey)
{
    BOOL retval;

    if (checkFieldIsBink1998())
    {
        auto p3 = BINK1998();
        retval = p3.Generate(pKey);
    }
    else
    {
        auto p3 = BINK2002();
        retval = p3.Generate(pKey);
    }

    return retval;
}

BOOL PIDGEN3::Validate(std::string &pKey)
{
    BOOL retval;

    if (checkFieldIsBink1998())
    {
        auto p3 = BINK1998(this);
        retval = p3.Validate(pKey);
    }
    else
    {
        auto p3 = BINK2002(this);
        retval = p3.Validate(pKey);
    }

    return retval;
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

    // Copy byte sequence to the reversed byte sequence.
    memcpy(rbyteSeq, byteSeq, sizeof(rbyteSeq));

    // Skip trailing zeroes and reverse y.
    int length;

    for (length = 15; rbyteSeq[length] <= 0; length--)
    {
        ; // do nothing, just counting
    }

    // Convert reversed byte sequence to BigNum z.
    auto z = Integer((BYTE *)&rbyteSeq, sizeof(rbyteSeq));

    // Divide z by 24 and convert the remainder to a CD-key char.
    for (int i = 24; i >= 0; i--)
    {
        output[i] = pKeyCharset[z.Modulo(24)];
    }

    output[25] = 0;

    cdKey = (char *)output;
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
    Integer y;

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
        y *= PK_LENGTH - 1;
        y += pDecodedKey[i];
    }

    // Acquire length.
    auto n = y.ByteCount();

    // Place the generated code into the byte sequence.
    y.Encode(byteSeq, 16);
}

/**
 * Checks to see if the currently instantiated PIDGEN3 object has a
 * field size greater than the maximum known BINK1998 size.
 *
 * @return
 */
BOOL PIDGEN3::checkFieldIsBink1998()
{
    // is max > privateKey?
    return (MaxSizeBINK1998 > privateKey);
}

/**
 * Checks if a given field, in a std::string, is greater than
 * the maximum known BINK1998 size
 *
 * @param keyin
 * @return
 */
BOOL PIDGEN3::checkFieldStrIsBink1998(std::string keyin)
{
    Integer check(&keyin[0]);

    // is max > check?
    return (MaxSizeBINK1998 > check);
}
