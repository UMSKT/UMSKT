//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

/* Unpacks the Windows Server 2003-like Product Key. */
void unpackServer(
        QWORD (&pRaw)[2],
        DWORD &pChannelID,
        DWORD &pHash,
        QWORD &pSignature,
        DWORD &pAuthInfo
) {
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Channel ID = Bits [0..10] -> 11 bits
    pChannelID = FIRSTNBITS(pRaw[0], 11);

    // Hash = Bits [11..41] -> 31 bits
    pHash = NEXTSNBITS(pRaw[0], 31, 11);

    // Signature = Bits [42..103] -> 62 bits
    // The quad-word signature overlaps AuthInfo in bits 104 and 105,
    // hence Microsoft employs a secret technique called: Signature = HIDWORD(Signature) >> 2 | LODWORD(Signature)
    pSignature = NEXTSNBITS(pRaw[1], 30, 10) << 32 | FIRSTNBITS(pRaw[1], 10) << 22 | NEXTSNBITS(pRaw[0], 22, 42);

    // AuthInfo = Bits [104..113] -> 10 bits
    pAuthInfo = NEXTSNBITS(pRaw[1], 10, 40);
}

/* Packs the Windows Server 2003-like Product Key. */
void packServer(
        QWORD (&pRaw)[2],
        DWORD pChannelID,
        DWORD pHash,
        QWORD &pSignature,
        DWORD pAuthInfo
) {
    // AuthInfo [113..104] <- Signature [103..42] <- Hash [41..11] <- Channel ID [10..1] <- Upgrade [0]
    pRaw[0] = FIRSTNBITS(pSignature, 22) << 42 | (QWORD)pHash << 11 | pChannelID;
    pRaw[1] = FIRSTNBITS(pAuthInfo, 10) << 40 | NEXTSNBITS(pSignature, 40, 22);
}

/* Verifies the Windows Server 2003-like Product Key. */
bool verifyServerKey(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        EC_POINT *publicKey,
        char (&cdKey)[25]
) {
    BN_CTX *context = BN_CTX_new();

    QWORD bKey[2]{},
          pSignature = 0;

    DWORD pChannelID,
          pHash,
          pAuthInfo;

    // Convert Base24 CD-key to bytecode.
    unbase24((BYTE *)bKey, cdKey);

    // Extract product key segments from bytecode.
    unpackServer(bKey, pChannelID, pHash, pSignature, pAuthInfo);

    if (options.verbose) {
        fmt::print("Validation results:\n");
        fmt::print("    Serial: 0x{:08x}\n", pChannelID);
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
    msgBuffer[0x01] = (pChannelID & 0x00FF);
    msgBuffer[0x02] = (pChannelID & 0xFF00) >> 8;
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
    msgBuffer[0x01] = (pChannelID & 0x00FF);
    msgBuffer[0x02] = (pChannelID & 0xFF00) >> 8;

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

void generateServerKey(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        BIGNUM   *genOrder,
        BIGNUM   *privateKey,
        DWORD    pChannelID,
        DWORD    pAuthInfo,
        char     (&pKey)[25]
) {
    BN_CTX *numContext = BN_CTX_new();

    BIGNUM *c = BN_new(),
           *e = BN_new(),
           *s = BN_new(),
           *x = BN_new(),
           *y = BN_new();

    QWORD pRaw[2]{},
          pSignature = 0;

    BOOL wrong = false;

    do {
        EC_POINT *r = EC_POINT_new(eCurve);

        wrong = false;

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
        msgBuffer[0x01] = (pChannelID & 0x00FF);
        msgBuffer[0x02] = (pChannelID & 0xFF00) >> 8;

        memcpy((void *)&msgBuffer[3], (void *)xBin, FIELD_BYTES_2003);
        memcpy((void *)&msgBuffer[3 + FIELD_BYTES_2003], (void *)yBin, FIELD_BYTES_2003);

        // pHash = SHA1(79 || Channel ID || R.x || R.y)
        SHA1(msgBuffer, SHA_MSG_LENGTH_2003, msgDigest);

        // Translate the byte digest into a 32-bit integer - this is our computed hash.
        // Truncate the hash to 31 bits.
        DWORD pHash = BYDWORD(msgDigest) & BITMASK(31);

        // Assemble the second SHA message.
        msgBuffer[0x00] = 0x5D;
        msgBuffer[0x01] = (pChannelID & 0x00FF);
        msgBuffer[0x02] = (pChannelID & 0xFF00) >> 8;
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
         * Signature * (Signature * G + H * K) = rG (mod p)
         * ↓ K = kG ↓
         *
         * Signature * (Signature * G + H * k * G) = rG (mod p)
         * Signature^2 * G + Signature * HkG = rG (mod p)
         * G(Signature^2 + Signature * HkG) = G (mod p) * r
         * ↓ G^(-1)(G (mod p)) = (mod n), n = genOrder of G ↓
         *
         * Signature^2 + Hk * Signature = r (mod n)
         * Signature = -(e +- sqrt(D)) / 2a → Signature = (-Hk +- sqrt((Hk)^2 + 4r)) / 2
         *
         * S = (-Hk +- sqrt((Hk)^2 + 4r)) (mod n) / 2
         *
         * S = s
         * H = e
         * k = privateKey
         * n = genOrder
         * r = c
         *
         * s = ( ( -e * privateKey +- sqrt( (e * privateKey)^2 + 4c ) ) / 2 ) % genOrder
         */

        // e = ek (mod n)
        BN_mod_mul(e, e, privateKey, genOrder, numContext);

        // s = e
        BN_copy(s, e);

        // s = (s (mod n))^2
        BN_mod_sqr(s, s, genOrder, numContext);

        // c <<= 2 (c *= 4)
        BN_lshift(c, c, 2);

        // s += c
        BN_add(s, s, c);

        // Around half of numbers modulo a prime are not squares -> BN_sqrt_mod fails about half of the times,
        // hence if BN_sqrt_mod returns NULL, we need to restart with a different seed.
        // s = sqrt(s (mod n))
        if (BN_mod_sqrt(s, s, genOrder, numContext) == nullptr) wrong = true;

        // s = s (mod n) - e
        BN_mod_sub(s, s, e, genOrder, numContext);

        // If s is odd, add order to it.
        // s += n
        if (BN_is_odd(s))
            BN_add(s, s, genOrder);

        // s >>= 1 (s /= 2)
        BN_rshift1(s, s);

        // Translate resulting scalar into a 64-bit integer (the byte order is little-endian).
        BN_bn2lebinpad(s, (BYTE *)&pSignature, BN_num_bytes(s));

        // Pack product key.
        packServer(pRaw, pChannelID, pHash, pSignature, pAuthInfo);

        if (options.verbose) {
            fmt::print("Generation results:\n");
            fmt::print("    Serial: 0x{:08x}\n", pChannelID);
            fmt::print("      Hash: 0x{:08x}\n", pHash);
            fmt::print(" Signature: 0x{:08x}\n", pSignature);
            fmt::print("  AuthInfo: 0x{:08x}\n", pAuthInfo);
            fmt::print("\n");
        }

        EC_POINT_free(r);
    } while (pSignature > BITMASK(62) || wrong);
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