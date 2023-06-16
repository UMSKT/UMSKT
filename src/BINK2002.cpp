/**
 * This file is a part of the WindowsXPKg Project
 *
 * Copyleft (C) 2019-2023 WindowsXPKg Contributors (et.al.)
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

#include "BINK2002.h"

/* Unpacks a Windows Server 2003-like Product Key. */
void BINK2002::Unpack(
        QWORD (&pRaw)[2],
         BOOL &pUpgrade,
        DWORD &pChannelID,
        DWORD &pHash,
        QWORD &pSignature,
        DWORD &pAuthInfo
) {
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Upgrade = Bit 0
    pUpgrade = FIRSTNBITS(pRaw[0], 1);

    // Channel ID = Bits [1..10] -> 10 bits
    pChannelID = NEXTSNBITS(pRaw[0], 10, 1);

    // Hash = Bits [11..41] -> 31 bits
    pHash = NEXTSNBITS(pRaw[0], 31, 11);

    // Signature = Bits [42..103] -> 62 bits
    // The quad-word signature overlaps AuthInfo in bits 104 and 105,
    // hence Microsoft employs a secret technique called: Signature = HIDWORD(Signature) >> 2 | LODWORD(Signature)
    pSignature = NEXTSNBITS(pRaw[1], 30, 10) << 32 | FIRSTNBITS(pRaw[1], 10) << 22 | NEXTSNBITS(pRaw[0], 22, 42);

    // AuthInfo = Bits [104..113] -> 10 bits
    pAuthInfo = NEXTSNBITS(pRaw[1], 10, 40);
}

/* Packs a Windows Server 2003-like Product Key. */
void BINK2002::Pack(
        QWORD (&pRaw)[2],
         BOOL pUpgrade,
        DWORD pChannelID,
        DWORD pHash,
        QWORD pSignature,
        DWORD pAuthInfo
) {
    // AuthInfo [113..104] <- Signature [103..42] <- Hash [41..11] <- Channel ID [10..1] <- Upgrade [0]
    pRaw[0] = FIRSTNBITS(pSignature, 22) << 42 | (QWORD)pHash << 11 | pChannelID << 1 | pUpgrade;
    pRaw[1] = FIRSTNBITS(pAuthInfo, 10) << 40 | NEXTSNBITS(pSignature, 40, 22);
}

/* Verifies a Windows Server 2003-like Product Key. */
bool BINK2002::Verify(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        EC_POINT *publicKey,
            char (&cdKey)[25]
) {
    BN_CTX *context = BN_CTX_new();

    QWORD bKey[2]{},
          pSignature = 0;

    DWORD pData,
          pChannelID,
          pHash,
          pAuthInfo;

    BOOL  pUpgrade;

    // Convert Base24 CD-key to bytecode.
    unbase24((BYTE *)bKey, cdKey);

    // Extract product key segments from bytecode.
    Unpack(bKey, pUpgrade, pChannelID, pHash, pSignature, pAuthInfo);

    pData = pChannelID << 1 | pUpgrade;

    if (options.verbose) {
        fmt::print("Validation results:\n");
        fmt::print("   Upgrade: 0x{:08x}\n", pUpgrade);
        fmt::print("Channel ID: 0x{:08x}\n", pChannelID);
        fmt::print("      Hash: 0x{:08x}\n", pHash);
        fmt::print(" Signature: 0x{:08x}\n", pSignature);
        fmt::print("  AuthInfo: 0x{:08x}\n", pAuthInfo);
        fmt::print("\n");
    }

    BYTE    msgDigest[SHA_DIGEST_LENGTH]{},
            msgBuffer[SHA_MSG_LENGTH_2003]{},
            xBin[FIELD_BYTES_2003]{},
            yBin[FIELD_BYTES_2003]{};

    // Assemble the first SHA message.
    msgBuffer[0x00] = 0x5D;
    msgBuffer[0x01] = (pData & 0x00FF);
    msgBuffer[0x02] = (pData & 0xFF00) >> 8;
    msgBuffer[0x03] = (pHash & 0x000000FF);
    msgBuffer[0x04] = (pHash & 0x0000FF00) >> 8;
    msgBuffer[0x05] = (pHash & 0x00FF0000) >> 16;
    msgBuffer[0x06] = (pHash & 0xFF000000) >> 24;
    msgBuffer[0x07] = (pAuthInfo & 0x00FF);
    msgBuffer[0x08] = (pAuthInfo & 0xFF00) >> 8;
    msgBuffer[0x09] = 0x00;
    msgBuffer[0x0A] = 0x00;

    // newSignature = SHA1(5D || Channel ID || Hash || AuthInfo || 00 00)
    SHA1(msgBuffer, 11, msgDigest);

    // Translate the byte digest into a 64-bit integer - this is our computed intermediate signature.
    // As the signature is only 62 bits long at most, we have to truncate it by shifting the high DWORD right 2 bits (per spec).
    QWORD iSignature = NEXTSNBITS(BYDWORD(&msgDigest[4]), 30, 2) << 32 | BYDWORD(msgDigest);

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
     *  P = s(sG + eK)
     *
     */

    BIGNUM *e = BN_lebin2bn((BYTE *)&iSignature, sizeof(iSignature), nullptr),
           *s = BN_lebin2bn((BYTE *)&pSignature, sizeof(pSignature), nullptr),
           *x = BN_new(),
           *y = BN_new();

    // Create 2 points on the elliptic curve.
    EC_POINT *p = EC_POINT_new(eCurve);
    EC_POINT *t = EC_POINT_new(eCurve);

    // t = sG
    EC_POINT_mul(eCurve, t, nullptr, basePoint, s, context);

    // p = eK
    EC_POINT_mul(eCurve, p, nullptr, publicKey, e, context);

    // p += t
    EC_POINT_add(eCurve, p, t, p, context);

    // p *= s
    EC_POINT_mul(eCurve, p, nullptr, p, s, context);

    // x = p.x; y = p.y;
    EC_POINT_get_affine_coordinates(eCurve, p, x, y, context);

    // Convert resulting point coordinates to bytes.
    BN_bn2lebin(x, xBin, FIELD_BYTES_2003);
    BN_bn2lebin(y, yBin, FIELD_BYTES_2003);

    // Assemble the second SHA message.
    msgBuffer[0x00] = 0x79;
    msgBuffer[0x01] = (pData & 0x00FF);
    msgBuffer[0x02] = (pData & 0xFF00) >> 8;

    memcpy((void *)&msgBuffer[3], (void *)xBin, FIELD_BYTES_2003);
    memcpy((void *)&msgBuffer[3 + FIELD_BYTES_2003], (void *)yBin, FIELD_BYTES_2003);

    // compHash = SHA1(79 || Channel ID || p.x || p.y)
    SHA1(msgBuffer, SHA_MSG_LENGTH_2003, msgDigest);

    // Translate the byte digest into a 32-bit integer - this is our computed hash.
    // Truncate the hash to 31 bits.
    DWORD compHash = BYDWORD(msgDigest) & BITMASK(31);

    BN_free(s);
    BN_free(e);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(context);

    EC_POINT_free(p);
    EC_POINT_free(t);

    // If the computed hash checks out, the key is valid.
    return compHash == pHash;
}

/* Generates a Windows Server 2003-like Product Key. */
void BINK2002::Generate(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
          BIGNUM *genOrder,
          BIGNUM *privateKey,
           DWORD pChannelID,
           DWORD pAuthInfo,
            BOOL pUpgrade,
            char (&pKey)[25]
) {
    BN_CTX *numContext = BN_CTX_new();

    BIGNUM *c = BN_new(),
           *e = BN_new(),
           *s = BN_new(),
           *x = BN_new(),
           *y = BN_new();

    QWORD pRaw[2]{},
          pSignature = 0;

    // Data segment of the RPK.
    DWORD pData = pChannelID << 1 | pUpgrade;

    BOOL noSquare;

    do {
        EC_POINT *r = EC_POINT_new(eCurve);

        // Generate a random number c consisting of 512 bits without any constraints.
        BN_rand(c, FIELD_BITS_2003, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        // R = cG
        EC_POINT_mul(eCurve, r, nullptr, basePoint, c, numContext);

        // Acquire its coordinates.
        // x = R.x; y = R.y;
        EC_POINT_get_affine_coordinates(eCurve, r, x, y, numContext);

        BYTE    msgDigest[SHA_DIGEST_LENGTH]{},
                msgBuffer[SHA_MSG_LENGTH_2003]{},
                xBin[FIELD_BYTES_2003]{},
                yBin[FIELD_BYTES_2003]{};

        // Convert resulting point coordinates to bytes.
        BN_bn2lebin(x, xBin, FIELD_BYTES_2003);
        BN_bn2lebin(y, yBin, FIELD_BYTES_2003);

        // Assemble the first SHA message.
        msgBuffer[0x00] = 0x79;
        msgBuffer[0x01] = (pData & 0x00FF);
        msgBuffer[0x02] = (pData & 0xFF00) >> 8;

        memcpy((void *)&msgBuffer[3], (void *)xBin, FIELD_BYTES_2003);
        memcpy((void *)&msgBuffer[3 + FIELD_BYTES_2003], (void *)yBin, FIELD_BYTES_2003);

        // pHash = SHA1(79 || Channel ID || R.x || R.y)
        SHA1(msgBuffer, SHA_MSG_LENGTH_2003, msgDigest);

        // Translate the byte digest into a 32-bit integer - this is our computed hash.
        // Truncate the hash to 31 bits.
        DWORD pHash = BYDWORD(msgDigest) & BITMASK(31);

        // Assemble the second SHA message.
        msgBuffer[0x00] = 0x5D;
        msgBuffer[0x01] = (pData & 0x00FF);
        msgBuffer[0x02] = (pData & 0xFF00) >> 8;
        msgBuffer[0x03] = (pHash & 0x000000FF);
        msgBuffer[0x04] = (pHash & 0x0000FF00) >> 8;
        msgBuffer[0x05] = (pHash & 0x00FF0000) >> 16;
        msgBuffer[0x06] = (pHash & 0xFF000000) >> 24;
        msgBuffer[0x07] = (pAuthInfo & 0x00FF);
        msgBuffer[0x08] = (pAuthInfo & 0xFF00) >> 8;
        msgBuffer[0x09] = 0x00;
        msgBuffer[0x0A] = 0x00;

        // newSignature = SHA1(5D || Channel ID || Hash || AuthInfo || 00 00)
        SHA1(msgBuffer, 11, msgDigest);

        // Translate the byte digest into a 64-bit integer - this is our computed intermediate signature.
        // As the signature is only 62 bits long at most, we have to truncate it by shifting the high DWORD right 2 bits (per spec).
        QWORD iSignature = NEXTSNBITS(BYDWORD(&msgDigest[4]), 30, 2) << 32 | BYDWORD(msgDigest);

        BN_lebin2bn((BYTE *)&iSignature, sizeof(iSignature), e);

        /*
         *
         * Scalars:
         *  c = Random multiplier
         *  e = Intermediate Signature
         *  s = Signature
         *  n = Order of G
         *  k = Private Key
         *
         * Points:
         *  G(x, y) = Generator (Base Point)
         *  R(x, y) = Random derivative of the generator
         *  K(x, y) = Public Key
         *
         * Equation:
         *  s(sG + eK) = R (mod p)
         *  ↓ K = kG; R = cG ↓
         *
         *  s(sG + ekG) = cG (mod p)
         *  s(s + ek)G = cG (mod p)
         *  ↓ G cancels out, the scalar arithmetic shrinks to order n ↓
         *
         *  s(s + ek) = c (mod n)
         *  s² + (ek)s - c = 0 (mod n)
         *  ↓ This is a quadratic equation in respect to the signature ↓
         *
         *  s = (-ek ± √((ek)² + 4c)) / 2 (mod n)
         */

        // e = ek (mod n)
        BN_mod_mul(e, e, privateKey, genOrder, numContext);

        // s = e
        BN_copy(s, e);

        // s = (ek (mod n))²
        BN_mod_sqr(s, s, genOrder, numContext);

        // c *= 4 (c <<= 2)
        BN_lshift(c, c, 2);

        // s += c
        BN_add(s, s, c);

        // Around half of numbers modulo a prime are not squares -> BN_sqrt_mod fails about half of the times,
        // hence if BN_sqrt_mod returns NULL, we need to restart with a different seed.
        // s = √((ek)² + 4c (mod n))
        noSquare = BN_mod_sqrt(s, s, genOrder, numContext) == nullptr;

        // s = -ek + √((ek)² + 4c) (mod n)
        BN_mod_sub(s, s, e, genOrder, numContext);

        // If s is odd, add order to it.
        // The order is a prime, so it can't be even.
        if (BN_is_odd(s))

            // s = -ek + √((ek)² + 4c) + n
            BN_add(s, s, genOrder);

        // s /= 2 (s >>= 1)
        BN_rshift1(s, s);

        // Translate resulting scalar into a 64-bit integer (the byte order is little-endian).
        BN_bn2lebinpad(s, (BYTE *)&pSignature, BN_num_bytes(s));

        // Pack product key.
        Pack(pRaw, pUpgrade, pChannelID, pHash, pSignature, pAuthInfo);

        if (options.verbose) {
            fmt::print("Generation results:\n");
            fmt::print("   Upgrade: 0x{:08x}\n", pUpgrade);
            fmt::print("Channel ID: 0x{:08x}\n", pChannelID);
            fmt::print("      Hash: 0x{:08x}\n", pHash);
            fmt::print(" Signature: 0x{:08x}\n", pSignature);
            fmt::print("  AuthInfo: 0x{:08x}\n", pAuthInfo);
            fmt::print("\n");
        }

        EC_POINT_free(r);
    } while (pSignature > BITMASK(62) || noSquare);
    // ↑ ↑ ↑
    // The signature can't be longer than 62 bits, else it will
    // overlap with the AuthInfo segment next to it.

    // Convert bytecode to Base24 CD-key.
    base24(pKey, (BYTE *)pRaw);

    BN_free(c);
    BN_free(s);
    BN_free(x);
    BN_free(y);
    BN_free(e);

    BN_CTX_free(numContext);
}