//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

/* Unpacks the Windows XP-like Product Key. */
void unpackXP(
        QWORD (&pRaw)[2],
        DWORD &pSerial,
        DWORD &pHash,
        QWORD &pSignature
) {
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Serial = Bits [0..30] -> 31 bits
    pSerial = FIRSTNBITS(pRaw[0], 31);

    // Hash = Bits [31..58] -> 28 bits
    pHash = NEXTSNBITS(pRaw[0], 28, 31);

    // Signature = Bits [59..113] -> 56 bits
    pSignature = FIRSTNBITS(pRaw[1], 51) << 5 | NEXTSNBITS(pRaw[0], 5, 59);
}

/* Packs the Windows XP-like Product Key. */
void packXP(
        QWORD (&pRaw)[2],
        DWORD pSerial,
        DWORD pHash,
        QWORD pSignature
) {
    // The quantity of information the key provides is 114 bits.
    // We're storing it in 2 64-bit quad-words with 14 trailing bits.
    // 64 * 2 = 128

    // Signature [114..59] <- Hash [58..31] <- Serial [30..1] <- Upgrade [0]
    pRaw[0] = FIRSTNBITS(pSignature, 5) << 59 | FIRSTNBITS(pHash, 28) << 31 | pSerial;
    pRaw[1] = NEXTSNBITS(pSignature, 51, 5);
}

/* Verifies the Windows XP-like Product Key. */
bool verifyXPKey(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        EC_POINT *publicKey,
        char (&cdKey)[25]
) {
    BN_CTX *numContext = BN_CTX_new();

    QWORD pRaw[2]{},
          pSignature = 0;

    DWORD pSerial = 0,
          pHash = 0;

    // Convert Base24 CD-key to bytecode.
    unbase24((BYTE *)pRaw, cdKey);

    // Extract RPK, hash and signature from bytecode.
    unpackXP(pRaw, pSerial, pHash, pSignature);

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

    // p = eK
    EC_POINT_mul(eCurve, p, nullptr, publicKey, e, numContext);

    // p += t
    EC_POINT_add(eCurve, p, t, p, numContext);

    // x = p.x; y = p.y;
    EC_POINT_get_affine_coordinates(eCurve, p, x, y, numContext);

    BYTE    msgDigest[SHA_DIGEST_LENGTH]{},
            msgBuffer[SHA_MSG_LENGTH_XP]{},
            xBin[FIELD_BYTES]{},
            yBin[FIELD_BYTES]{};

    // Convert resulting point coordinates to bytes.
    BN_bn2lebin(x, xBin, FIELD_BYTES);
    BN_bn2lebin(y, yBin, FIELD_BYTES);

    // Assemble the SHA message.
    memcpy((void *)&msgBuffer[0], (void *)&pSerial, 4);
    memcpy((void *)&msgBuffer[4], (void *)xBin, FIELD_BYTES);
    memcpy((void *)&msgBuffer[4 + FIELD_BYTES], (void *)yBin, FIELD_BYTES);

    // Retrieve the message digest.
    SHA1(msgBuffer, SHA_MSG_LENGTH_XP, msgDigest);

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

/* Generate a valid Product Key. */
void generateXPKey(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        BIGNUM   *genOrder,
        BIGNUM   *privateKey,
        DWORD    pSerial,
        char     (&pKey)[25]
) {
    BN_CTX *numContext = BN_CTX_new();

    BIGNUM *c = BN_new();
    BIGNUM *s = BN_new();
    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();

    QWORD pRaw[2]{};

    do {
        EC_POINT *r = EC_POINT_new(eCurve);

        QWORD pSignature = 0;
        DWORD pHash;

        // Generate a random number c consisting of 384 bits without any constraints.
        BN_rand(c, FIELD_BITS, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

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
        memcpy((void *)&msgBuffer[0], (void *)&pSerial, 4);
        memcpy((void *)&msgBuffer[4], (void *)xBin, FIELD_BYTES);
        memcpy((void *)&msgBuffer[4 + FIELD_BYTES], (void *)yBin, FIELD_BYTES);

        // Retrieve the message digest.
        SHA1(msgBuffer, SHA_MSG_LENGTH_XP, msgDigest);

        // Translate the byte digest into a 32-bit integer - this is our computed pHash.
        // Truncate the pHash to 28 bits.
        pHash  = BYDWORD(msgDigest) >> 4;
        pHash &= BITMASK(28);

        /*
         *
         * Scalars:
         *  c = Random multiplier
         *  e = Hash
         *  s = Signature
         *  n = Order of G
         *  k = Private Key
         *  K = Public Key
         *
         * Points:
         *  G(x, y) = Generator (Base Point)
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
        packXP(pRaw, pSerial, pHash, pSignature);

        std::cout << "    Serial: 0x" << std::hex << std::setw(8) << std::setfill('0') << pSerial << std::endl
                  << "      Hash: 0x" << std::hex << std::setw(8) << std::setfill('0') << pHash << std::endl
                  << " Signature: 0x" << std::hex << std::setw(8) << std::setfill('0') << pSignature << std::endl
                                      << std::endl;

        EC_POINT_free(r);
    } while (pRaw[1] > BITMASK(50));
    // ↑ ↑ ↑
    // pRaw[1] can't be longer than 50 bits, else the signature part
    // will make the CD-key longer than 25 characters.

    // Convert bytecode to Base24 CD-key.
    base24(pKey, (BYTE *)pRaw);

    BN_free(c);
    BN_free(s);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(numContext);
}