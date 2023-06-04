#include "header.h"

char charset[] = "BCDFGHJKMPQRTVWXY2346789";

void unpackServer(DWORD *osFamily, DWORD *hash, DWORD *sig, DWORD *prefix, DWORD *raw) {

    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // OS Family = Bits [0..10] -> 11 bits
    osFamily[0] = raw[0] & 0x7ff;

    // Hash = Bits [11..41] -> 31 bits
    hash[0] = ((raw[0] >> 11) | (raw[1] << 21)) & 0x7fffffff;

    // Signature = Bits [42..103] -> 62 bits
    sig[0] = (raw[1] >> 10) | (raw[2] << 22);
    sig[1] = ((raw[2] >> 10) | (raw[3] << 22)) & 0x3fffffff;

    // Prefix = Bits [104..113] -> 10 bits
    prefix[0] = (raw[3] >> 8) & 0x3ff;
}

void packServer(DWORD *raw, DWORD *osFamily, DWORD *hash, DWORD *sig, DWORD *prefix) {
    raw[0] = osFamily[0] | (hash[0] << 11);
    raw[1] = (hash[0] >> 21) | (sig[0] << 10);
    raw[2] = (sig[0] >> 22) | (sig[1] << 10);
    raw[3] = (sig[1] >> 22) | (prefix[0] << 8);
}


bool verifyServerKey(EC_GROUP *eCurve, EC_POINT *generator, EC_POINT *publicKey, char *cdKey) {
    BN_CTX *context = BN_CTX_new();

    // Convert Base24 CD-key to bytecode.
    DWORD osFamily, hash, sig[2], prefix;
    DWORD bKey[4]{};

    unbase24(bKey, cdKey);

    // Extract segments from the bytecode and reverse the signature.
    unpackServer(&osFamily, &hash, sig, &prefix, bKey);
    endian((BYTE *)sig, 8);

    BYTE t[FIELD_BYTES_2003]{}, md[SHA_DIGEST_LENGTH]{};
    DWORD checkHash, newHash[2]{};

    SHA_CTX hContext;

    // H = SHA-1(5D || OS Family || Hash || Prefix || 00 00)
    SHA1_Init(&hContext);

    t[0] = 0x5D;
    t[1] = (osFamily & 0xff);
    t[2] = (osFamily & 0xff00) >> 8;
    t[3] = (hash & 0xff);
    t[4] = (hash & 0xff00) >> 8;
    t[5] = (hash & 0xff0000) >> 16;
    t[6] = (hash & 0xff000000) >> 24;
    t[7] = (prefix & 0xff);
    t[8] = (prefix & 0xff00) >> 8;
    t[9] = 0x00;
    t[10] = 0x00;

    SHA1_Update(&hContext, t, 11);
    SHA1_Final(md, &hContext);

    // First word.
    newHash[0] = md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24);

    // Second word, right shift 2 bits.
    newHash[1] = (md[4] | (md[5] << 8) | (md[6] << 16) | (md[7] << 24)) >> 2;
    newHash[1] &= 0x3FFFFFFF;

    endian((BYTE *)newHash, 8);

    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();
    BIGNUM *s = BN_bin2bn((BYTE *)sig, 8, nullptr);
    BIGNUM *e = BN_bin2bn((BYTE *)newHash, 8, nullptr);

    EC_POINT *u = EC_POINT_new(eCurve);
    EC_POINT *v = EC_POINT_new(eCurve);

    // EC_POINT_mul calculates r = generator * n + q * m.
    // v = s * (s * generator + e * publicKey)

    // u = generator * s
    EC_POINT_mul(eCurve, u, nullptr, generator, s, context);

    // v = publicKey * e
    EC_POINT_mul(eCurve, v, nullptr, publicKey, e, context);

    // v += u
    EC_POINT_add(eCurve, v, u, v, context);

    // v *= s
    EC_POINT_mul(eCurve, v, nullptr, v, s, context);

    // EC_POINT_get_affine_coordinates() sets x and y, either of which may be nullptr, to the corresponding coordinates of p.
    // x = v.x; y = v.y;
    EC_POINT_get_affine_coordinates_GFp(eCurve, v, x, y, context);

    // Hash = First31(SHA-1(79 || OS Family || v.x || v.y))
    SHA1_Init(&hContext);

    t[0] = 0x79;
    t[1] = (osFamily & 0xff);
    t[2] = (osFamily & 0xff00) >> 8;

    // Hash chunk of data.
    SHA1_Update(&hContext, t, 3);

    // Empty buffer, place v.y in little-endian.
    memset(t, 0, FIELD_BYTES_2003);
    BN_bn2bin(x, t);
    endian(t, FIELD_BYTES_2003);

    // Hash chunk of data.
    SHA1_Update(&hContext, t, FIELD_BYTES_2003);

    // Empty buffer, place v.y in little-endian.
    memset(t, 0, FIELD_BYTES_2003);
    BN_bn2bin(y, t);
    endian(t, FIELD_BYTES_2003);

    // Hash chunk of data.
    SHA1_Update(&hContext, t, FIELD_BYTES_2003);

    // Store the final message from hContext in md.
    SHA1_Final(md, &hContext);

    // Hash = First31(SHA-1(79 || OS Family || v.x || v.y))
    checkHash = (md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24)) & 0x7fffffff;

    BN_free(s);
    BN_free(e);
    BN_free(x);
    BN_free(y);

    BN_CTX_free(context);

    EC_POINT_free(v);
    EC_POINT_free(u);

    // If we managed to generate a key with the same hash, the key is correct.
    return checkHash == hash;
}

void generateServerKey(char *pKey, EC_GROUP *eCurve, EC_POINT *generator, BIGNUM *order, BIGNUM *privateKey, DWORD *osFamily, DWORD *prefix) {
    EC_POINT *r = EC_POINT_new(eCurve);
    BN_CTX *ctx = BN_CTX_new();

    DWORD bKey[4]{},
            bSig[2]{};

    do {
        BIGNUM *c = BN_new();
        BIGNUM *s = BN_new();
        BIGNUM *x = BN_new();
        BIGNUM *y = BN_new();
        BIGNUM *b = BN_new();

        DWORD hash = 0, h[2]{};

        memset(bKey, 0, 4);
        memset(bSig, 0, 2);

        // Generate a random number c consisting of 512 bits without any constraints.
        BN_rand(c, FIELD_BITS_2003, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        // r = generator * c
        EC_POINT_mul(eCurve, r, nullptr, generator, c, ctx);

        // x = r.x; y = r.y;
        EC_POINT_get_affine_coordinates(eCurve, r, x, y, ctx);

        SHA_CTX hContext;
        BYTE md[SHA_DIGEST_LENGTH]{}, buf[FIELD_BYTES_2003]{};

        // Hash = SHA-1(79 || OS Family || r.x || r.y)
        SHA1_Init(&hContext);

        buf[0] = 0x79;

        buf[1] = (*osFamily & 0xff);
        buf[2] = (*osFamily & 0xff00) >> 8;

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

        buf[1] = (*osFamily & 0xff);
        buf[2] = (*osFamily & 0xff00) >> 8;

        buf[3] = (hash & 0xff);
        buf[4] = (hash & 0xff00) >> 8;
        buf[5] = (hash & 0xff0000) >> 16;
        buf[6] = (hash & 0xff000000) >> 24;

        buf[7] = prefix[0] & 0xff;
        buf[8] = (prefix[0] & 0xff00) >> 8;

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
         * ↓ G^(-1)(G (mod p)) = (mod n), n = order of G ↓
         *
         * Signature^2 + Hk * Signature = r (mod n)
         * Signature = -(b +- sqrt(D)) / 2a → Signature = (-Hk +- sqrt((Hk)^2 + 4r)) / 2
         *
         * S = (-Hk +- sqrt((Hk)^2 + 4r)) (mod n) / 2
         *
         * S = s
         * H = b
         * k = privateKey
         * n = order
         * r = c
         *
         * s = ( ( -b * privateKey +- sqrt( (b * privateKey)^2 + 4c ) ) / 2 ) % order
         */

        // b = (b * privateKey) % order
        BN_mod_mul(b, b, privateKey, order, ctx);

        // s = b
        BN_copy(s, b);

        // s = (s % order)^2
        BN_mod_sqr(s, s, order, ctx);

        // c <<= 2 (c = 4c)
        BN_lshift(c, c, 2);

        // s = s + c
        BN_add(s, s, c);

        // s^2 = s % order (order must be prime)
        BN_mod_sqrt(s, s, order, ctx);

        // s = s - b
        BN_mod_sub(s, s, b, order, ctx);

        // if s is odd, s = s + order
        if (BN_is_odd(s)) {
            BN_add(s, s, order);
        }

        // s >>= 1 (s = s / 2)
        BN_rshift1(s, s);

        // Convert s from BigNum back to bytecode and reverse the endianness.
        BN_bn2bin(s, (BYTE *)bSig);
        endian((BYTE *)bSig, BN_num_bytes(s));

        // Pack product key.
        packServer(bKey, osFamily, &hash, bSig, prefix);

        BN_free(c);
        BN_free(s);
        BN_free(x);
        BN_free(y);
        BN_free(b);
    } while (bSig[1] >= 0x40000000);

    base24(pKey, bKey);

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

    char pkey[25];
    DWORD osfamily[1], prefix[1];
	
	osfamily[0] = 1280;
	RAND_bytes((BYTE *)prefix, 4);
	prefix[0] &= 0x3ff;
	
	do {
		generateServerKey(pkey, ec, g, n, priv, osfamily, prefix);
	} while (!verifyServerKey(ec, g, pub, pkey));
	
	print_product_key(pkey);
    std::cout << std::endl << std::endl;

	BN_CTX_free(ctx);
	
	return 0;
}
