//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

char pCharset[] = "BCDFGHJKMPQRTVWXY2346789";

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
    pChannelID = pRaw[0] & 0x7ff;

    // Hash = Bits [11..41] -> 31 bits
    pHash = ((pRaw[0] >> 11) | (pRaw[1] << 21)) & 0x7fffffff;

    // Signature = Bits [42..103] -> 62 bits
    pSignature = (((QWORD)pRaw[2] >> 10 | (QWORD)pRaw[3] << 22) & 0x3fffffff) << 32 | (pRaw[1] >> 10) | (pRaw[2] << 22);

    // Prefix = Bits [104..113] -> 10 bits
    pAuthInfo = (pRaw[3] >> 8) & 0x3ff;
}

void packServer(
        DWORD (&pRaw)[4],
        DWORD pChannelID,
        DWORD pHash,
        QWORD &pSignature,
        DWORD pAuthInfo
) {
    pRaw[0] = pChannelID | (pHash << 11);
    pRaw[1] = (pHash >> 21) | pSignature << 10;
    pRaw[2] = (DWORD)(pSignature >> 22);
    pRaw[3] = pSignature >> 54 | (pAuthInfo << 8);
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
    EC_POINT *r = EC_POINT_new(eCurve);
    BN_CTX *ctx = BN_CTX_new();

    DWORD bKey[4]{};

    QWORD pSignature = 0;

    do {
        BIGNUM *c = BN_new();
        BIGNUM *s = BN_new();
        BIGNUM *x = BN_new();
        BIGNUM *y = BN_new();
        BIGNUM *b = BN_new();

        DWORD hash = 0, h[2]{};

        memset(bKey, 0, 4);

        // Generate a random number c consisting of 512 bits without any constraints.
        BN_rand(c, FIELD_BITS_2003, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        // r = basePoint * c
        EC_POINT_mul(eCurve, r, nullptr, basePoint, c, ctx);

        // x = r.x; y = r.y;
        EC_POINT_get_affine_coordinates(eCurve, r, x, y, ctx);

        SHA_CTX hContext;
        BYTE md[SHA_DIGEST_LENGTH]{}, buf[FIELD_BYTES_2003]{};

        // Hash = SHA-1(79 || OS Family || r.x || r.y)
        SHA1_Init(&hContext);

        buf[0] = 0x79;

        buf[1] = (pChannelID & 0xff);
        buf[2] = (pChannelID & 0xff00) >> 8;

        SHA1_Update(&hContext, buf, 3);

        memset(buf, 0, FIELD_BYTES_2003);

        BN_bn2bin(x, buf);
        endian((BYTE *)buf, FIELD_BYTES_2003);
        SHA1_Update(&hContext, buf, FIELD_BYTES_2003);

        memset(buf, 0, FIELD_BYTES_2003);

        BN_bn2bin(y, buf);
        endian((BYTE *)buf, FIELD_BYTES_2003);

        SHA1_Update(&hContext, buf, FIELD_BYTES_2003);
        SHA1_Final(md, &hContext);

        hash = (md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24)) & 0x7fffffff;

        // H = SHA-1(5D || OS Family || Hash || Prefix || 00 00)
        SHA1_Init(&hContext);
        buf[0] = 0x5D;

        buf[1] = (pChannelID & 0xff);
        buf[2] = (pChannelID & 0xff00) >> 8;

        buf[3] = (hash & 0xff);
        buf[4] = (hash & 0xff00) >> 8;
        buf[5] = (hash & 0xff0000) >> 16;
        buf[6] = (hash & 0xff000000) >> 24;

        buf[7] = (pAuthInfo & 0xff);
        buf[8] = (pAuthInfo & 0xff00) >> 8;

        buf[9] = 0x00;
        buf[10] = 0x00;

        // Input length is 11 BYTEs.
        SHA1_Update(&hContext, buf, 11);
        SHA1_Final(md, &hContext);

        // First word.
        h[0] = md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24);

        // Second word, right shift 2 bits.
        h[1] = (md[4] | (md[5] << 8) | (md[6] << 16) | (md[7] << 24)) >> 2;
        h[1] &= 0x3FFFFFFF;

        endian((BYTE *)h, 8);
        BN_bin2bn((BYTE *)h, 8, b);

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
         * Signature = -(b +- sqrt(D)) / 2a → Signature = (-Hk +- sqrt((Hk)^2 + 4r)) / 2
         *
         * S = (-Hk +- sqrt((Hk)^2 + 4r)) (mod n) / 2
         *
         * S = s
         * H = b
         * k = privateKey
         * n = genOrder
         * r = c
         *
         * s = ( ( -b * privateKey +- sqrt( (b * privateKey)^2 + 4c ) ) / 2 ) % genOrder
         */

        // b = (b * privateKey) % genOrder
        BN_mod_mul(b, b, privateKey, genOrder, ctx);

        // s = b
        BN_copy(s, b);

        // s = (s % genOrder)^2
        BN_mod_sqr(s, s, genOrder, ctx);

        // c <<= 2 (c = 4c)
        BN_lshift(c, c, 2);

        // s = s + c
        BN_add(s, s, c);

        // s^2 = s % genOrder (genOrder must be prime)
        BN_mod_sqrt(s, s, genOrder, ctx);

        // s = s - b
        BN_mod_sub(s, s, b, genOrder, ctx);

        // if s is odd, s = s + genOrder
        if (BN_is_odd(s)) {
            BN_add(s, s, genOrder);
        }

        // s >>= 1 (s = s / 2)
        BN_rshift1(s, s);

        // Convert s from BigNum back to bytecode and reverse the endianness.
        BN_bn2bin(s, (BYTE *)&pSignature);
        endian((BYTE *)&pSignature, BN_num_bytes(s));

        // Pack product key.
        packServer(bKey, pChannelID, hash, pSignature, pAuthInfo);

        BN_free(c);
        BN_free(s);
        BN_free(x);
        BN_free(y);
        BN_free(b);
    } while ((pSignature >> 32) >= 0x40000000);

    base24(pKey, (BYTE *)bKey);

    BN_CTX_free(ctx);
    EC_POINT_free(r);
}

int main()
{
	BIGNUM *a, *b, *p, *gx, *gy, *pubx, *puby, *n, *priv;
	BN_CTX *ctx = BN_CTX_new();
	
	a = BN_new();
	b = BN_new();
	p = BN_new();
	gx = BN_new();
	gy = BN_new();
	pubx = BN_new();
	puby = BN_new();
	n = BN_new();
	priv = BN_new();

	/* Windows Sever 2003 VLK */
	BN_set_word(a, 1);
	BN_set_word(b, 0);
	BN_hex2bn(&p,    "C9AE7AED19F6A7E100AADE98134111AD8118E59B8264734327940064BC675A0C682E19C89695FBFA3A4653E47D47FD7592258C7E3C3C61BBEA07FE5A7E842379");
	BN_hex2bn(&gx,   "85ACEC9F9F9B456A78E43C3637DC88D21F977A9EC15E5225BD5060CE5B892F24FEDEE574BF5801F06BC232EEF2161074496613698D88FAC4B397CE3B475406A7");
	BN_hex2bn(&gy,   "66B7D1983F5D4FE43E8B4F1E28685DE0E22BBE6576A1A6B86C67533BF72FD3D082DBA281A556A16E593DB522942C8DD7120BA50C9413DF944E7258BDDF30B3C4");
	BN_hex2bn(&pubx, "90BF6BD980C536A8DB93B52AA9AEBA640BABF1D31BEC7AA345BB7510194A9B07379F552DA7B4A3EF81A9B87E0B85B5118E1E20A098641EE4CCF2045558C98C0E");
	BN_hex2bn(&puby, "6B87D1E658D03868362945CDD582E2CF33EE4BA06369E0EFE9E4851F6DCBEC7F15081E250D171EA0CC4CB06435BCFCFEA8F438C9766743A06CBD06E7EFB4C3AE");
	BN_hex2bn(&n,    "4CC5C56529F0237D"); // from mskey 4in1
	BN_hex2bn(&priv, "2606120F59C05118");
	
	
	EC_GROUP *ec = EC_GROUP_new_curve_GFp(p, a, b, ctx);
	EC_POINT *g = EC_POINT_new(ec);
	EC_POINT_set_affine_coordinates_GFp(ec, g, gx, gy, ctx);
	EC_POINT *pub = EC_POINT_new(ec);
	EC_POINT_set_affine_coordinates_GFp(ec, pub, pubx, puby, ctx);
	
	assert(EC_POINT_is_on_curve(ec, g, ctx) == 1);
	assert(EC_POINT_is_on_curve(ec, pub, ctx) == 1);

    char pKey[25];
    DWORD pChannelID = 640 << 1, pAuthInfo;

	RAND_bytes((BYTE *)&pAuthInfo, 4);
    pAuthInfo &= 0x3ff;
	
	do {
		generateServerKey(ec, g, n, priv, pChannelID, pAuthInfo, pKey);
	} while (!verifyServerKey(ec, g, pub, pKey));
	
	print_product_key(pKey);
    std::cout << std::endl << std::endl;

	BN_CTX_free(ctx);
	
	return 0;
}
