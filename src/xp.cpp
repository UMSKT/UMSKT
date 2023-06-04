/*
	Windows XP CD Key Verification/Generator v0.03
	by z22
	
	Compile with OpenSSL libs, modify to suit your needs.
	http://gnuwin32.sourceforge.net/packages/openssl.htm

	History:
	0.03	Stack corruptionerror on exit fixed (now pkey is large enough)
			More Comments added
	0.02	Changed name the *.cpp;
			Fixed minor bugs & Make it compilable on VC++
	0.01	First version compilable MingW


*/

#include "header.h"

/* Unpacks the Windows XP Product Key. */
void unpackXP(DWORD (&pRaw)[4], DWORD &pSerial, DWORD &pHash, DWORD (&pSignature)[2]) {

    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Serial = Bits [0..30] -> 31 bits
    pSerial = FIRSTNBITS(pRaw[0], 31);

    // Hash (e) = Bits [31..58] -> 28 bits
    pHash = FIRSTNBITS(pRaw[1] << 1 | pRaw[0] >> 31, 28);

    // Signature (s) = Bits [59..113] -> 55 bits
    pSignature[0] = pRaw[2] << 5 | pRaw[1] >> 27;
    pSignature[1] = pRaw[3] << 5 | pRaw[2] >> 27;
}

/* Packs the Windows XP Product Key. */
void packXP(DWORD (&pRaw)[4], DWORD pSerial, DWORD pHash, DWORD (&pSignature)[2]) {
    pRaw[0] = pSerial | FIRSTNBITS(pHash, 1) << 31;
    pRaw[1] = FIRSTNBITS(pSignature[0], 5) << 27 | pHash >> 1;
    pRaw[2] = pSignature[1] << 27 | pSignature[0] >> 5;
    pRaw[3] = pSignature[1] >> 5;
}

/* Verify Product Key */
bool verifyXPKey(EC_GROUP *eCurve, EC_POINT *generator, EC_POINT *publicKey, char (&cdKey)[25]) {
    BN_CTX *context = BN_CTX_new();

    // Convert Base24 CD-key to bytecode.
    DWORD bKey[4]{};
    DWORD pID, checkHash, sig[2];

    unbase24(bKey, cdKey);

    // Extract data, hash and signature from the bytecode.
    unpackXP(bKey, pID, checkHash, sig);

    // e = Hash
    // s = Signature
    BIGNUM *e, *s;

    // Put hash word into BigNum e.
    e = BN_new();
    BN_set_word(e, checkHash);

    // Reverse signature and create a new BigNum s.
    endian((BYTE *)sig, sizeof(sig));
    s = BN_bin2bn((BYTE *)sig, sizeof(sig), nullptr);

    // Create x and y.
    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();

    // Create 2 new points on the existing elliptic curve.
    EC_POINT *u = EC_POINT_new(eCurve);
    EC_POINT *v = EC_POINT_new(eCurve);

    // EC_POINT_mul calculates r = generator * n + q * m.
    // v = s * generator + e * (-publicKey)

    // u = generator * s
    EC_POINT_mul(eCurve, u, nullptr, generator, s, context);

    // v = publicKey * e
    EC_POINT_mul(eCurve, v, nullptr, publicKey, e, context);

    // v += u
    EC_POINT_add(eCurve, v, u, v, context);

    // EC_POINT_get_affine_coordinates() sets x and y, either of which may be nullptr, to the corresponding coordinates of p.
    // x = v.x; y = v.y;
    EC_POINT_get_affine_coordinates(eCurve, v, x, y, context);

    BYTE buf[FIELD_BYTES], md[SHA_DIGEST_LENGTH], t[4];
    DWORD newHash;

    SHA_CTX hContext;

    // h = First32(SHA-1(pID || v.x || v.y)) >> 4
    SHA1_Init(&hContext);

    // Chop Product ID into 4 bytes.
    t[0] = (pID & 0xff);                 // First 8 bits
    t[1] = (pID & 0xff00) >> 8;          // Second 8 bits
    t[2] = (pID & 0xff0000) >> 16;       // Third 8 bits
    t[3] = (pID & 0xff000000) >> 24;     // Fourth 8 bits

    // Hash chunk of data.
    SHA1_Update(&hContext, t, sizeof(t));

    // Empty buffer, place v.x in little-endian.
    memset(buf, 0, FIELD_BYTES);
    BN_bn2bin(x, buf);
    endian(buf, FIELD_BYTES);

    // Hash chunk of data.
    SHA1_Update(&hContext, buf, FIELD_BYTES);

    // Empty buffer, place v.y in little-endian.
    memset(buf, 0, FIELD_BYTES);
    BN_bn2bin(y, buf);
    endian(buf, FIELD_BYTES);

    // Hash chunk of data.
    SHA1_Update(&hContext, buf, FIELD_BYTES);

    // Store the final message from hContext in md.
    SHA1_Final(md, &hContext);

    // h = First32(SHA-1(pID || v.x || v.y)) >> 4
    newHash = (md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24)) >> 4;
    newHash &= 0xfffffff;

    BN_free(e);
    BN_free(s);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(context);

    EC_POINT_free(u);
    EC_POINT_free(v);

    // If we managed to generate a key with the same hash, the key is correct.
    return newHash == checkHash;
}

/* Generate a valid Product Key. */
void generateXPKey(EC_GROUP *eCurve, EC_POINT *generator, BIGNUM *order, BIGNUM *privateKey, DWORD pRaw, char (&pKey)[25]) {
    EC_POINT *r = EC_POINT_new(eCurve);
    BN_CTX *ctx = BN_CTX_new();

    BIGNUM *c = BN_new();
    BIGNUM *s = BN_new();
    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();

    DWORD bKey[4]{};

    do {
        DWORD hash = 0, sig[2]{};

        memset(bKey, 0, 4);

        // Generate a random number c consisting of 384 bits without any constraints.
        BN_rand(c, FIELD_BITS, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        // r = generator * c;
        EC_POINT_mul(eCurve, r, nullptr, generator, c, ctx);

        // x = r.x; y = r.y;
        EC_POINT_get_affine_coordinates(eCurve, r, x, y, ctx);

        SHA_CTX hContext;
        BYTE md[SHA_DIGEST_LENGTH]{}, buf[FIELD_BYTES]{}, t[4]{};

        // h = (First-32(SHA1(pRaw, r.x, r.y)) >> 4
        SHA1_Init(&hContext);

        // Chop Raw Product Key into 4 bytes.
        t[0] = (pRaw & 0xff);
        t[1] = (pRaw & 0xff00) >> 8;
        t[2] = (pRaw & 0xff0000) >> 16;
        t[3] = (pRaw & 0xff000000) >> 24;

        // Hash chunk of data.
        SHA1_Update(&hContext, t, sizeof(t));

        // Empty buffer, place r.x in little-endian
        memset(buf, 0, FIELD_BYTES);
        BN_bn2bin(x, buf);
        endian(buf, FIELD_BYTES);

        // Hash chunk of data.
        SHA1_Update(&hContext, buf, FIELD_BYTES);

        // Empty buffer, place r.y in little-endian.
        memset(buf, 0, FIELD_BYTES);
        BN_bn2bin(y, buf);
        endian(buf, FIELD_BYTES);

        // Hash chunk of data.
        SHA1_Update(&hContext, buf, FIELD_BYTES);

        // Store the final message from hContext in md.
        SHA1_Final(md, &hContext);

        // h = (First-32(SHA1(pRaw, r.x, r.y)) >> 4
        hash = (md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24)) >> 4;
        hash &= 0xfffffff;

        /* s = privateKey * hash + c; */
        // s = privateKey;
        BN_copy(s, privateKey);

        // s *= hash;
        BN_mul_word(s, hash);

        // BN_mod_add() adds a to b % m and places the non-negative result in r.
        // s = |s + c % order|;
        BN_mod_add(s, s, c, order, ctx);

        // Convert s from BigNum back to bytecode and reverse the endianness.
        BN_bn2bin(s, (BYTE *)sig);
        endian((BYTE *)sig, BN_num_bytes(s));

        // Pack product key.
        packXP(bKey, pRaw, hash, sig);

        //printf("PID: %.8X\nHash: %.8X\nSig: %.8X %.8X\n", pRaw[0], hash, sig[1], sig[0]);
        std::cout << " PID: " << std::hex << std::setw(8) << std::setfill('0') << pRaw   << std::endl
                  << "Hash: " << std::hex << std::setw(8) << std::setfill('0') << hash   << std::endl
                  << " Sig: " << std::hex << std::setw(8) << std::setfill('0') << sig[0] << " "
                              << std::hex << std::setw(8) << std::setfill('0') << sig[1] << std::endl
                              << std::endl;

    } while (bKey[3] >= 0x40000);
    // ↑ ↑ ↑
    // bKey[3] can't be longer than 18 bits, else the signature part will make
    // the CD-key longer than 25 characters.

    // Convert the key to Base24.
    base24(pKey, bKey);

    BN_free(c);
    BN_free(s);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(ctx);
    EC_POINT_free(r);
}