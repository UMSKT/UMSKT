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

/* Unpacks a Windows XP-like Product Key. */
void PIDGEN3::BINK1998::Unpack(
        QWORD (&pRaw)[2],
         BOOL &pUpgrade,
        DWORD &pSerial,
        DWORD &pHash,
        QWORD &pSignature
) {
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Upgrade = Bit 0
    pUpgrade = FIRSTNBITS(pRaw[0], 1);

    // Serial = Bits [1..30] -> 30 bits
    pSerial = NEXTSNBITS(pRaw[0], 30, 1);

    // Hash = Bits [31..58] -> 28 bits
    pHash = NEXTSNBITS(pRaw[0], 28, 31);

    // Signature = Bits [59..113] -> 56 bits
    pSignature = FIRSTNBITS(pRaw[1], 51) << 5 | NEXTSNBITS(pRaw[0], 5, 59);
}

/* Packs a Windows XP-like Product Key. */
void PIDGEN3::BINK1998::Pack(
        QWORD (&pRaw)[2],
         BOOL pUpgrade,
        DWORD pSerial,
        DWORD pHash,
        QWORD pSignature
) {
    // The quantity of information the key provides is 114 bits.
    // We're storing it in 2 64-bit quad-words with 14 trailing bits.
    // 64 * 2 = 128

    // Signature [114..59] <- Hash [58..31] <- Serial [30..1] <- Upgrade [0]
    pRaw[0] = FIRSTNBITS(pSignature, 5) << 59 | FIRSTNBITS(pHash, 28) << 31 | pSerial << 1 | pUpgrade;
    pRaw[1] = NEXTSNBITS(pSignature, 51, 5);
}

/* Verifies a Windows XP-like Product Key. */
bool PIDGEN3::BINK1998::Verify(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        EC_POINT *publicKey,
            char (&pKey)[25]
) {
    BN_CTX *numContext = BN_CTX_new();

    QWORD pRaw[2]{},
          pSignature;

    DWORD pData,
          pSerial,
          pHash;

    BOOL  pUpgrade;

    // Convert Base24 CD-key to bytecode.
    PIDGEN3::unbase24((BYTE *)pRaw, pKey);

    // Extract RPK, hash and signature from bytecode.
    Unpack(pRaw, pUpgrade, pSerial, pHash, pSignature);

    fmt::print(UMSKT::debug, "Validation results:\n");
    fmt::print(UMSKT::debug, "   Upgrade: 0x{:08x}\n", pUpgrade);
    fmt::print(UMSKT::debug, "    Serial: 0x{:08x}\n", pSerial);
    fmt::print(UMSKT::debug, "      Hash: 0x{:08x}\n", pHash);
    fmt::print(UMSKT::debug, " Signature: 0x{:08x}\n", pSignature);
    fmt::print(UMSKT::debug, "\n");

    pData = pSerial << 1 | pUpgrade;

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

    BIGNUM *e = BN_lebin2bn((BYTE *)&pHash, sizeof(pHash), nullptr),
           *s = BN_lebin2bn((BYTE *)&pSignature, sizeof(pSignature), nullptr),
           *x = BN_new(),
           *y = BN_new();

    // Create 2 points on the elliptic curve.
    EC_POINT *t = EC_POINT_new(eCurve);
    EC_POINT *p = EC_POINT_new(eCurve);

    // t = sG
    EC_POINT_mul(eCurve, t, nullptr, basePoint, s, numContext);

    // P = eK
    EC_POINT_mul(eCurve, p, nullptr, publicKey, e, numContext);

    // P += t
    EC_POINT_add(eCurve, p, t, p, numContext);

    // x = P.x; y = P.y;
    EC_POINT_get_affine_coordinates(eCurve, p, x, y, numContext);

    BYTE    msgDigest[SHA_DIGEST_LENGTH]{},
            msgBuffer[SHA_MSG_LENGTH_XP]{},
            xBin[FIELD_BYTES]{},
            yBin[FIELD_BYTES]{};

    // Convert resulting point coordinates to bytes.
    BN_bn2lebin(x, xBin, FIELD_BYTES);
    BN_bn2lebin(y, yBin, FIELD_BYTES);

    // Assemble the SHA message.
    memcpy((void *)&msgBuffer[0], (void *)&pData, 4);
    memcpy((void *)&msgBuffer[4], (void *)xBin, FIELD_BYTES);
    memcpy((void *)&msgBuffer[4 + FIELD_BYTES], (void *)yBin, FIELD_BYTES);

    // compHash = SHA1(pSerial || P.x || P.y)
    SHA1_DIGEST(msgBuffer, SHA_MSG_LENGTH_XP, msgDigest);

    // Translate the byte digest into a 32-bit integer - this is our computed hash.
    // Truncate the hash to 28 bits.
    DWORD compHash = BYDWORD(msgDigest) >> 4 & BITMASK(28);

    BN_free(e);
    BN_free(s);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(numContext);

    EC_POINT_free(t);
    EC_POINT_free(p);

    // If the computed hash checks out, the key is valid.
    return compHash == pHash;
}

/* Generates a Windows XP-like Product Key. */
void PIDGEN3::BINK1998::Generate(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
          BIGNUM *genOrder,
          BIGNUM *privateKey,
           DWORD pSerial,
            BOOL pUpgrade,
            char (&pKey)[25]
) {
    BN_CTX *numContext = BN_CTX_new();

    BIGNUM *c = BN_new(),
           *s = BN_new(),
           *x = BN_new(),
           *y = BN_new();

    QWORD pRaw[2]{},
          pSignature = 0;

    // Data segment of the RPK.
    DWORD pData = pSerial << 1 | pUpgrade;

    do {
        EC_POINT *r = EC_POINT_new(eCurve);

        // Generate a random number c consisting of 384 bits without any constraints.
        UMSKT::umskt_bn_rand(c, FIELD_BITS, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        // Pick a random derivative of the base point on the elliptic curve.
        // R = cG;
        EC_POINT_mul(eCurve, r, nullptr, basePoint, c, numContext);

        // Acquire its coordinates.
        // x = R.x; y = R.y;
        EC_POINT_get_affine_coordinates(eCurve, r, x, y, numContext);

        BYTE    msgDigest[SHA_DIGEST_LENGTH]{},
                msgBuffer[SHA_MSG_LENGTH_XP]{},
                xBin[FIELD_BYTES]{},
                yBin[FIELD_BYTES]{};

        // Convert coordinates to bytes.
        BN_bn2lebin(x, xBin, FIELD_BYTES);
        BN_bn2lebin(y, yBin, FIELD_BYTES);

        // Assemble the SHA message.
        memcpy((void *)&msgBuffer[0], (void *)&pData, 4);
        memcpy((void *)&msgBuffer[4], (void *)xBin, FIELD_BYTES);
        memcpy((void *)&msgBuffer[4 + FIELD_BYTES], (void *)yBin, FIELD_BYTES);

        // pHash = SHA1(pSerial || R.x || R.y)
        SHA1_DIGEST(msgBuffer, SHA_MSG_LENGTH_XP, msgDigest);

        // Translate the byte digest into a 32-bit integer - this is our computed pHash.
        // Truncate the pHash to 28 bits.
        DWORD pHash = BYDWORD(msgDigest) >> 4 & BITMASK(28);

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
        BN_mul_word(s, pHash);

        // s += c (mod n)
        BN_mod_add(s, s, c, genOrder, numContext);

        // Translate resulting scalar into a 64-bit integer (the byte order is little-endian).
        BN_bn2lebinpad(s, (BYTE *)&pSignature, BN_num_bytes(s));

        // Pack product key.
        Pack(pRaw, pUpgrade, pSerial, pHash, pSignature);

        fmt::print(UMSKT::debug, "Generation results:\n");
        fmt::print(UMSKT::debug, "   Upgrade: 0x{:08x}\n", pUpgrade);
        fmt::print(UMSKT::debug, "    Serial: 0x{:08x}\n", pSerial);
        fmt::print(UMSKT::debug, "      Hash: 0x{:08x}\n", pHash);
        fmt::print(UMSKT::debug, " Signature: 0x{:08x}\n", pSignature);
        fmt::print(UMSKT::debug, "\n");

        EC_POINT_free(r);
    } while (pSignature > BITMASK(55));
    // ↑ ↑ ↑
    // The signature can't be longer than 55 bits, else it will
    // make the CD-key longer than 25 characters.

    // Convert bytecode to Base24 CD-key.
    base24(pKey, (BYTE *)pRaw);

    BN_free(c);
    BN_free(s);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(numContext);
}
