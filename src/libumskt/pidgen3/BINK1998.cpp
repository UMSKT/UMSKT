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
 *
  * @History {
 *   Algorithm was initially written and open sourced by z22
 *   and uploaded to GitHub by TheMCHK in August of 2019
 *
 *   Endermanch (Andrew) rewrote the algorithm in May of 2023
 * }
 */

#include "BINK1998.h"

/**
 * Unpacks a Windows XP-like Product Key.
 *
 * @param pRaw [out] *QWORD[2] raw product key output
 **/
BOOL BINK1998::Unpack(KeyInfo &info, QWORD *pRaw)
{
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Upgrade = Bit 0
    info.isUpgrade = FIRSTNBITS(pRaw[0], 1);

    // Serial = Bits [1..30] -> 30 bits
    info.Serial = NEXTSNBITS(pRaw[0], 30, 1);

    // Hash = Bits [31..58] -> 28 bits
    info.Hash = NEXTSNBITS(pRaw[0], 28, 31);

    // Signature = Bits [59..113] -> 56 bits
    info.Signature = FIRSTNBITS(pRaw[1], 51) << 5 | NEXTSNBITS(pRaw[0], 5, 59);

    return true;
}

/**
 * Packs a Windows XP-like Product Key.
 *
 * @param pRaw [in] *QWORD[2] raw product key input
 **/
BOOL BINK1998::Pack(const KeyInfo &info, QWORD *pRaw)
{
    // The quantity of information the key provides is 114 bits.
    // We're storing it in 2 64-bit quad-words with 14 trailing bits.
    // 64 * 2 = 128

    // Signature [114..59] <- Hash [58..31] <- Serial [30..1] <- Upgrade [0]
    pRaw[0] = FIRSTNBITS(info.Signature, 5) << 59 | FIRSTNBITS(info.Hash, 28) << 31 | info.Serial << 1 | info.isUpgrade;
    pRaw[1] = NEXTSNBITS(info.Signature, 51, 5);

    return true;
}

/**
 * Verifies a Windows XP-like Product Key.
 *
 * @param pKey [in]
 *
 * @return true if provided key validates against loaded curve
 */
BOOL BINK1998::Verify(std::string &pKey)
{
    if (pKey.length() != 25)
    {
        return false;
    }

    BN_CTX *numContext = BN_CTX_new();
    KeyInfo info;

    QWORD pRaw[2];

    // Convert Base24 CD-key to bytecode.
    unbase24((BYTE *)pRaw, pKey);

    // Extract RPK, hash and signature from bytecode.
    Unpack(info, pRaw);

    fmt::print(UMSKT::debug, "Validation results:\n");
    fmt::print(UMSKT::debug, "   Upgrade: {:#08x}\n", info.isUpgrade);
    fmt::print(UMSKT::debug, "    Serial: {:#08x}\n", info.Serial);
    fmt::print(UMSKT::debug, "      Hash: {:#08x}\n", info.Hash);
    fmt::print(UMSKT::debug, " Signature: {:#08x}\n", info.Signature);
    fmt::print(UMSKT::debug, "\n");

    DWORD pData = info.Serial << 1 | info.isUpgrade;

    /*
     *
     * Scalars:
     *  e = Hash
     *  s = Schnorr Signature
     *
     * Points:
     *  G(x, y) = Generator (Base Point)
     *  K(x, y) = Public Key
     *
     * Equation:
     *  P = sG + eK
     *
     */

    BIGNUM *e = BN_lebin2bn((BYTE *)&info.Hash, sizeof(info.Hash), nullptr),
           *s = BN_lebin2bn((BYTE *)&info.Signature, sizeof(info.Signature), nullptr);

    BIGNUM *x = BN_CTX_get(numContext), *y = BN_CTX_get(numContext);

    // Create 2 points on the elliptic curve.
    EC_POINT *t = EC_POINT_new(eCurve), *p = EC_POINT_new(eCurve);

    // t = sG
    EC_POINT_mul(eCurve, t, nullptr, genPoint, s, numContext);

    // P = eK
    EC_POINT_mul(eCurve, p, nullptr, pubPoint, e, numContext);

    // P += t
    EC_POINT_add(eCurve, p, t, p, numContext);

    // x = P.x; y = P.y;
    EC_POINT_get_affine_coordinates(eCurve, p, x, y, numContext);

    BYTE msgDigest[SHA_DIGEST_LENGTH], msgBuffer[SHA_MSG_LENGTH_XP], xBin[FIELD_BYTES], yBin[FIELD_BYTES];

    // Convert resulting point coordinates to bytes.
    BN_bn2lebin(x, xBin, FIELD_BYTES);
    BN_bn2lebin(y, yBin, FIELD_BYTES);

    // Assemble the SHA message.
    memcpy(&msgBuffer[0], &pData, 4);
    memcpy(&msgBuffer[4], xBin, FIELD_BYTES);
    memcpy(&msgBuffer[4 + FIELD_BYTES], yBin, FIELD_BYTES);

    // compHash = SHA1(pSerial || P.x || P.y)
    SHA1(msgBuffer, SHA_MSG_LENGTH_XP, msgDigest);

    // Translate the byte digest into a 32-bit integer - this is our computed hash.
    // Truncate the hash to 28 bits.
    DWORD compHash = BYDWORD(msgDigest) >> 4 & BITMASK(28);

    BN_free(e);
    BN_free(s);

    BN_CTX_free(numContext);

    EC_POINT_free(t);
    EC_POINT_free(p);

    // If the computed hash checks out, the key is valid.
    return compHash == info.Hash;
}

/**
 * Generates a Windows XP-like Product Key.
 *
 * @param pKey [out]
 *
 * @return true on success, false on fail
 */
BOOL BINK1998::Generate(KeyInfo &info, std::string &pKey)
{
    BN_CTX *numContext = BN_CTX_new();

    BIGNUM *c = BN_CTX_get(numContext), *s = BN_CTX_get(numContext), *x = BN_CTX_get(numContext),
           *y = BN_CTX_get(numContext);

    QWORD pRaw[2];

    // Data segment of the RPK.
    DWORD pData = info.Serial << 1 | info.isUpgrade;

    // prepare the private key for generation
    BN_sub(privateKey, genOrder, privateKey);

    do
    {
        EC_POINT *r = EC_POINT_new(eCurve);

        // Generate a random number c consisting of 384 bits without any constraints.
        BN_rand(c, FIELD_BITS, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        // Pick a random derivative of the base point on the elliptic curve.
        // R = cG;
        EC_POINT_mul(eCurve, r, nullptr, genPoint, c, numContext);

        // Acquire its coordinates.
        // x = R.x; y = R.y;
        EC_POINT_get_affine_coordinates(eCurve, r, x, y, numContext);

        BYTE msgDigest[SHA_DIGEST_LENGTH], msgBuffer[SHA_MSG_LENGTH_XP];
        BYTE xBin[FIELD_BYTES], yBin[FIELD_BYTES];

        // Convert coordinates to bytes.
        BN_bn2lebin(x, xBin, FIELD_BYTES);
        BN_bn2lebin(y, yBin, FIELD_BYTES);

        // Assemble the SHA message.
        memcpy(&msgBuffer[0], &pData, 4);
        memcpy(&msgBuffer[4], xBin, FIELD_BYTES);
        memcpy(&msgBuffer[4 + FIELD_BYTES], yBin, FIELD_BYTES);

        // pHash = SHA1(pSerial || R.x || R.y)
        SHA1(msgBuffer, SHA_MSG_LENGTH_XP, msgDigest);

        // Translate the byte digest into a 32-bit integer - this is our computed pHash.
        // Truncate the pHash to 28 bits.
        info.Hash = BYDWORD(msgDigest) >> 4 & BITMASK(28);

        /*
         *
         * Scalars:
         *  c = Random multiplier
         *  e = Hash
         *  s = Signature
         *  n = Order of G
         *  k = Private Key
         *
         * Points:
         *  G(x, y) = Generator (Base Point)
         *  R(x, y) = Random derivative of the generator
         *  K(x, y) = Public Key
         *
         * We need to find the signature s that satisfies the equation with a given hash:
         *  P = sG + eK
         *  s = ek + c (mod n) <- computation optimization
         */

        // s = ek;
        BN_copy(s, privateKey);
        BN_mul_word(s, info.Hash);

        // s += c (mod n)
        BN_mod_add(s, s, c, genOrder, numContext);

        // Translate resulting scalar into a 64-bit integer (the byte order is little-endian).
        BN_bn2lebinpad(s, (BYTE *)&info.Signature, BN_num_bytes(s));

        // Pack product key.
        Pack(info, pRaw);

        fmt::print(UMSKT::debug, "Generation results:\n");
        fmt::print(UMSKT::debug, "   Upgrade: {:#08x}\n", info.isUpgrade);
        fmt::print(UMSKT::debug, "    Serial: {:#08x}\n", info.Serial);
        fmt::print(UMSKT::debug, "      Hash: {:#08x}\n", info.Hash);
        fmt::print(UMSKT::debug, " Signature: {:#08x}\n", info.Signature);
        fmt::print(UMSKT::debug, "\n");

        EC_POINT_free(r);
    } while (info.Signature > BITMASK(55));
    // ↑ ↑ ↑
    // The signature can't be longer than 55 bits, else it will
    // make the CD-key longer than 25 characters.

    // Convert bytecode to Base24 CD-key.
    base24(pKey, (BYTE *)pRaw);

    BN_CTX_free(numContext);

    return true;
}
