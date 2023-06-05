//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

char pCharset[] = "BCDFGHJKMPQRTVWXY2346789";
const std::string filename = "keys.json";

using json = nlohmann::json;


void unpackServer(
        DWORD (&pRaw)[4],
        DWORD &pChannelID,
        DWORD &pHash,
        QWORD &pSignature,
        DWORD &pAuthInfo
) {
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // OS Family = Bits [0..10] -> 11 bits
    pChannelID = pRaw[0] & BITMASK(11);

    // Hash = Bits [11..41] -> 31 bits
    pHash = (pRaw[1] << 21 | pRaw[0] >> 11) & BITMASK(31);

    // Signature = Bits [42..103] -> 62 bits
    pSignature = (((QWORD)pRaw[3] << 22 | (QWORD)pRaw[2] >> 10) & BITMASK(30)) << 32 | pRaw[2] << 22 | pRaw[1] >> 10;

    // Prefix = Bits [104..113] -> 10 bits
    pAuthInfo = pRaw[3] >> 8 & BITMASK(10);
}

void packServer(
        DWORD (&pRaw)[4],
        DWORD pChannelID,
        DWORD pHash,
        QWORD &pSignature,
        DWORD pAuthInfo
) {
    pRaw[0] = pHash << 11 | pChannelID;
    pRaw[1] = pSignature << 10 | pHash >> 21;
    pRaw[2] = (DWORD)(pSignature >> 22);
    pRaw[3] = pAuthInfo << 8 | pSignature >> 54;
}


bool verifyServerKey(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        EC_POINT *publicKey,
        char (&cdKey)[25]
) {
    BN_CTX *context = BN_CTX_new();

    // Convert Base24 CD-key to bytecode.
    DWORD pChannelID, pHash, pAuthInfo;
    DWORD bKey[4]{};

    QWORD pSignature = 0;

    unbase24((BYTE *)bKey, cdKey);

    // Extract segments from the bytecode and reverse the signature.
    unpackServer(bKey, pChannelID, pHash, pSignature, pAuthInfo);

    BYTE    msgDigest[SHA_DIGEST_LENGTH]{},
            msgBuffer[SHA_MSG_LENGTH_2003]{},
            xBin[FIELD_BYTES_2003]{},
            yBin[FIELD_BYTES_2003]{};

    // H = SHA-1(5D || OS Family || Hash || Prefix || 00 00)
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

    SHA1(msgBuffer, 11, msgDigest);

    QWORD newHash = (BYDWORD(&msgDigest[4]) >> 2 & BITMASK(30)) << 32 | BYDWORD(msgDigest);

    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();
    BIGNUM *s = BN_lebin2bn((BYTE *)&pSignature, sizeof(pSignature), nullptr);
    BIGNUM *e = BN_lebin2bn((BYTE *)&newHash, sizeof(newHash), nullptr);

    EC_POINT *u = EC_POINT_new(eCurve);
    EC_POINT *v = EC_POINT_new(eCurve);

    // EC_POINT_mul calculates r = basePoint * n + q * m.
    // v = s * (s * basePoint + e * publicKey)

    // u = basePoint * s
    EC_POINT_mul(eCurve, u, nullptr, basePoint, s, context);

    // v = publicKey * e
    EC_POINT_mul(eCurve, v, nullptr, publicKey, e, context);

    // v += u
    EC_POINT_add(eCurve, v, u, v, context);

    // v *= s
    EC_POINT_mul(eCurve, v, nullptr, v, s, context);

    // EC_POINT_get_affine_coordinates() sets x and y, either of which may be nullptr, to the corresponding coordinates of p.
    // x = v.x; y = v.y;
    EC_POINT_get_affine_coordinates(eCurve, v, x, y, context);

    // Convert resulting point coordinates to bytes.
    BN_bn2lebin(x, xBin, FIELD_BYTES_2003);
    BN_bn2lebin(y, yBin, FIELD_BYTES_2003);

    // Assemble the SHA message.
    msgBuffer[0x00] = 0x79;
    msgBuffer[0x01] = (pChannelID & 0x00FF);
    msgBuffer[0x02] = (pChannelID & 0xFF00) >> 8;

    memcpy((void *)&msgBuffer[3], (void *)xBin, FIELD_BYTES_2003);
    memcpy((void *)&msgBuffer[3 + FIELD_BYTES_2003], (void *)yBin, FIELD_BYTES_2003);

    // Retrieve the message digest.
    SHA1(msgBuffer, SHA_MSG_LENGTH_2003, msgDigest);

    // Translate the byte digest into a 32-bit integer - this is our computed pHash.
    // Truncate the pHash to 28 bits.
    // Hash = First31(SHA-1(79 || OS Family || v.x || v.y))
    DWORD compHash = BYDWORD(msgDigest) & BITMASK(31);

    BN_free(s);
    BN_free(e);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(context);

    EC_POINT_free(v);
    EC_POINT_free(u);

    // If we managed to generate a key with the same pHash, the key is correct.
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

    BIGNUM *c = BN_new();
    BIGNUM *s = BN_new();
    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();
    BIGNUM *e = BN_new();

    DWORD pRaw[4]{};
    BOOL wrong = false;
    QWORD pSignature = 0;

    do {
        EC_POINT *r = EC_POINT_new(eCurve);

        wrong = false;

        DWORD hash = 0;
        QWORD h = 0;

        // Generate a random number c consisting of 512 bits without any constraints.
        BN_rand(c, FIELD_BITS_2003, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        // r = basePoint * c
        EC_POINT_mul(eCurve, r, nullptr, basePoint, c, numContext);

        // x = r.x; y = r.y;
        EC_POINT_get_affine_coordinates(eCurve, r, x, y, numContext);

        BYTE    msgDigest[SHA_DIGEST_LENGTH]{},
                msgBuffer[SHA_MSG_LENGTH_2003]{},
                xBin[FIELD_BYTES_2003]{},
                yBin[FIELD_BYTES_2003]{};

        // Convert resulting point coordinates to bytes.
        BN_bn2lebin(x, xBin, FIELD_BYTES_2003);
        BN_bn2lebin(y, yBin, FIELD_BYTES_2003);

        // Assemble the SHA message.
        // Hash = SHA-1(79 || OS Family || r.x || r.y)
        msgBuffer[0x00] = 0x79;
        msgBuffer[0x01] = (pChannelID & 0x00FF);
        msgBuffer[0x02] = (pChannelID & 0xFF00) >> 8;

        memcpy((void *)&msgBuffer[3], (void *)xBin, FIELD_BYTES_2003);
        memcpy((void *)&msgBuffer[3 + FIELD_BYTES_2003], (void *)yBin, FIELD_BYTES_2003);

        // Retrieve the message digest.
        SHA1(msgBuffer, SHA_MSG_LENGTH_2003, msgDigest);

        hash = BYDWORD(msgDigest) & BITMASK(31);

        // H = SHA-1(5D || OS Family || Hash || Prefix || 00 00)
        msgBuffer[0x00] = 0x5D;
        msgBuffer[0x01] = (pChannelID & 0x00FF);
        msgBuffer[0x02] = (pChannelID & 0xFF00) >> 8;
        msgBuffer[0x03] = (hash & 0x000000FF);
        msgBuffer[0x04] = (hash & 0x0000FF00) >> 8;
        msgBuffer[0x05] = (hash & 0x00FF0000) >> 16;
        msgBuffer[0x06] = (hash & 0xFF000000) >> 24;
        msgBuffer[0x07] = (pAuthInfo & 0x00FF);
        msgBuffer[0x08] = (pAuthInfo & 0xFF00) >> 8;
        msgBuffer[0x09] = 0x00;
        msgBuffer[0x0A] = 0x00;

        SHA1(msgBuffer, 11, msgDigest);

        // First word.
        h = (BYDWORD(&msgDigest[4]) >> 2 & BITMASK(30)) << 32 | BYDWORD(msgDigest);

        BN_lebin2bn((BYTE *)&h, sizeof(h), e);

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

        // e = (e * privateKey) % genOrder
        BN_mod_mul(e, e, privateKey, genOrder, numContext);

        // s = e
        BN_copy(s, e);

        // s = (s % genOrder)^2
        BN_mod_sqr(s, s, genOrder, numContext);

        // c <<= 2 (c = 4c)
        BN_lshift(c, c, 2);

        // s = s + c
        BN_add(s, s, c);

        // s^2 = s % genOrder (genOrder must be prime)
        if (BN_mod_sqrt(s, s, genOrder, numContext) == nullptr) wrong = true;

        // s = s - e
        BN_mod_sub(s, s, e, genOrder, numContext);

        // if s is odd, s = s + genOrder
        if (BN_is_odd(s)) {
            BN_add(s, s, genOrder);
        }

        // s >>= 1 (s = s / 2)
        BN_rshift1(s, s);

        // Convert s from BigNum back to bytecode and reverse the endianness.
        BN_bn2lebinpad(s, (BYTE *)&pSignature, BN_num_bytes(s));

        // Pack product key.
        packServer(pRaw, pChannelID, hash, pSignature, pAuthInfo);

        EC_POINT_free(r);
    } while (HIBYTES(pSignature, sizeof(DWORD)) >= 0x40000000);

    base24(pKey, (BYTE *)pRaw);

    std::cout << "attempt pass " << pKey << " key is " << (wrong ? "INVALID" : "VALID") << std::endl;

    BN_free(c);
    BN_free(s);
    BN_free(x);
    BN_free(y);
    BN_free(e);

    BN_CTX_free(numContext);
}

int main()
{
    const char* BINKID = "5A";

    // We cannot produce a valid key without knowing the private key k. The reason for this is that
    // we need the result of the function K(x; y) = kG(x; y).
    BIGNUM *privateKey = BN_new();

    // We can, however, validate any given key using the available public key: {p, a, b, G, K}.
    // genOrder the order of the generator G, a value we have to reverse -> Schoof's Algorithm.
    BIGNUM *genOrder = BN_new();

    std::ifstream f(filename);
    json keys = json::parse(f);

    EC_POINT *genPoint, *pubPoint;
    EC_GROUP *eCurve = initializeEllipticCurve(
            keys["BINK"][BINKID]["p"].get<std::string>(),
            keys["BINK"][BINKID]["a"].get<std::string>(),
            keys["BINK"][BINKID]["b"].get<std::string>(),
            keys["BINK"][BINKID]["g"]["x"].get<std::string>(),
            keys["BINK"][BINKID]["g"]["y"].get<std::string>(),
            keys["BINK"][BINKID]["pub"]["x"].get<std::string>(),
            keys["BINK"][BINKID]["pub"]["y"].get<std::string>(),
            genPoint,
            pubPoint
    );

    BN_dec2bn(&genOrder, keys["BINK"][BINKID]["n"].get<std::string>().c_str());
    BN_dec2bn(&privateKey, keys["BINK"][BINKID]["priv"].get<std::string>().c_str());

    char pKey[25]{};
    DWORD pChannelID = 640 << 1, pAuthInfo;

	RAND_bytes((BYTE *)&pAuthInfo, 4);
    pAuthInfo &= 0x3ff;
	
	do {
		generateServerKey(eCurve, genPoint, genOrder, privateKey, pChannelID, pAuthInfo, pKey);
	} while (!verifyServerKey(eCurve, genPoint, pubPoint, pKey));
	
	print_product_key(pKey);
    std::cout << std::endl << std::endl;

	return 0;
}
