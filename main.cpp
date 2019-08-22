/*
	Windows XP CD Key Verification/Generator v0.03
	by z22
	
	Compile with OpenSSL libs, modify to suit your needs.
	http://www.slproweb.com/download/Win32OpenSSL-v0.9.8b.exe

	History:
	0.03	Stack corruptionerror on exit fixed (now pkey is large enough)
			More Comments added
	0.02	Changed name the *.cpp;
			Fixed minor bugs & Make it compilable on VC++
	0.01	First version compilable MingW


*/

#include <stdio.h>
#include <string.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <assert.h>


#define FIELD_BITS 384
#define FIELD_BYTES 48
unsigned char cset[] = "BCDFGHJKMPQRTVWXY2346789";


static void unpack(unsigned long *pid, unsigned long *hash, unsigned long *sig, unsigned long *raw)
{
 // pid = Bit 0..30 
	pid[0] = raw[0] & 0x7fffffff;
 
 // hash(s) = Bit 31..58
	hash[0] = ((raw[0] >> 31) | (raw[1] << 1)) & 0xfffffff;
 
 // sig(e) = bit 58..113
	sig[0] = (raw[1] >> 27) | (raw[2] << 5);
	sig[1] = (raw[2] >> 27) | (raw[3] << 5);
}

static void pack(unsigned long *raw, unsigned long *pid, unsigned long *hash, unsigned long *sig)
{
	raw[0] = pid[0] | ((hash[0] & 1) << 31);
	raw[1] = (hash[0] >> 1) | ((sig[0] & 0x1f) << 27);
	raw[2] = (sig[0] >> 5) | (sig[1] << 27);
	raw[3] = sig[1] >> 5;
}

// Reverse data
static void endian(unsigned char *data, int len)
{
	int i;
	for (i = 0; i < len/2; i++) {
		unsigned char temp;
		temp = data[i];
		data[i] = data[len-i-1];
		data[len-i-1] = temp;
	}
}

void unbase24(unsigned long *x, unsigned char *c)
{

	memset(x, 0, 16);
	int i, n;

	BIGNUM *y = BN_new();
	BN_zero(y);
	
	for (i = 0; i < 25; i++)
	{
		BN_mul_word(y, 24);
		BN_add_word(y, c[i]);
	}
	n = BN_num_bytes(y);
	BN_bn2bin(y, (unsigned char *)x);
	BN_free(y);
	
	endian((unsigned char *)x, n);
}

void base24(unsigned char *c, unsigned long *x)
{
	unsigned char y[16];
	int i;
	BIGNUM *z;

 // Convert x to BigNum z
	memcpy(y, x, sizeof(y));				// Copy X to Y; Y=X
	for (i = 15; y[i] == 0; i--) {} i++;	// skip following nulls
	endian(y, i);							// Reverse y
 	z = BN_bin2bn(y, i, NULL);				// Convert y to BigNum z


 // Divide z by 24 and convert remainder with cset to Base24-CDKEY Char
	c[25] = 0;
	for (i = 24; i >= 0; i--) {
		unsigned char t = BN_div_word(z, 24);
		c[i] = cset[t];
	}

	BN_free(z);
}

void print_product_id(unsigned long *pid)
{
	char raw[12];
	char b[6], c[8];
	int i, digit = 0;
	
 //	Cut a away last bit of pid and convert it to an accii-number (=raw)
	sprintf(raw, "%d", pid[0] >> 1);
 
 // Make b-part {640-....}
	strncpy(b, raw, 3);
	b[3] = 0;

 // Make c-part {...-123456X...}
	strcpy(c, raw + 3);

 // Make checksum digit-part {...56X-}
	assert(strlen(c) == 6);
	for (i = 0; i < 6; i++) 
		digit -= c[i] - '0';	// Sum digits

	while (digit < 0) 
		digit += 7;
	c[6] = digit + '0';
	c[7] = 0;
	
	printf("Product ID: 55274-%s-%s-23xxx\n", b, c);
}

void print_product_key(unsigned char *pk)
{
	int i;
	assert(strlen((const char *)pk) == 25);
	for (i = 0; i < 25; i++) {
		putchar(pk[i]);
		if (i != 24 && i % 5 == 4) putchar('-');
	}
}

void verify(EC_GROUP *ec, EC_POINT *generator, EC_POINT *public_key, char *cdkey)
{
	unsigned char key[25];
	int i, j, k;

	BN_CTX *ctx = BN_CTX_new();
// remove Dashs from CDKEY
	for (i = 0, k = 0; i < strlen(cdkey); i++) {
		for (j = 0; j < 24; j++) {
			if (cdkey[i] != '-' && cdkey[i] == cset[j]) {
				key[k++] = j;
				break;
			}
			assert(j < 24);
		}
		if (k >= 25) break;
	}
	
 // Base24_CDKEY -> Bin_CDKEY
	unsigned long bkey[4] = {0};
	unsigned long pid[1], hash[1], sig[2];
	unbase24(bkey, key);
 
 // Output Bin_CDKEY
	printf("%.8x %.8x %.8x %.8x\n", bkey[3], bkey[2], bkey[1], bkey[0]);

 // Divide/Extract pid_data, hash, sig  from Bin_CDKEY
	unpack(pid, hash, sig, bkey);
	print_product_id(pid);
	
	printf("PID: %.8x\nHash: %.8x\nSig: %.8x %.8x\n", pid[0], hash[0], sig[1], sig[0]);

	
	BIGNUM *e, *s;
	
	/* e = hash, s = sig */
	e = BN_new();
	BN_set_word(e, hash[0]);
	endian((unsigned char *)sig, sizeof(sig));
	s = BN_bin2bn((unsigned char *)sig, sizeof(sig), NULL);
	
	BIGNUM *x = BN_new();
	BIGNUM *y = BN_new();
	EC_POINT *u = EC_POINT_new(ec);
	EC_POINT *v = EC_POINT_new(ec);
	
	/* v = s*generator + e*(-public_key) */
	EC_POINT_mul(ec, u, NULL, generator, s, ctx);
	EC_POINT_mul(ec, v, NULL, public_key, e, ctx);
	EC_POINT_add(ec, v, u, v, ctx);
	EC_POINT_get_affine_coordinates_GFp(ec, v, x, y, ctx);
	
	unsigned char buf[FIELD_BYTES], md[20];
	unsigned long h;
	unsigned char t[4];
	SHA_CTX h_ctx;
	
	/* h = (fist 32 bits of SHA1(pid || v.x, v.y)) >> 4 */
	SHA1_Init(&h_ctx);
	t[0] =  pid[0] & 0xff;
	t[1] = (pid[0] & 0xff00) >> 8;
	t[2] = (pid[0] & 0xff0000) >> 16;
	t[3] = (pid[0] & 0xff000000) >> 24;
	SHA1_Update(&h_ctx, t, sizeof(t));
	
	memset(buf, 0, sizeof(buf));
	BN_bn2bin(x, buf);
	endian((unsigned char *)buf, sizeof(buf));
	SHA1_Update(&h_ctx, buf, sizeof(buf));
	
	memset(buf, 0, sizeof(buf));
	BN_bn2bin(y, buf);
	endian((unsigned char *)buf, sizeof(buf));
	SHA1_Update(&h_ctx, buf, sizeof(buf));
	
	SHA1_Final(md, &h_ctx);
	h = (md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24)) >> 4;
	h &= 0xfffffff;
	
	printf("Calculated hash: %.8x\n", h);
	if (h == hash[0]) printf("Key valid\n");
	else printf("Key invalid\n");
	putchar('\n');
	
	BN_free(e);
	BN_free(s);
	BN_free(x);
	BN_free(y);
	EC_POINT_free(u);
	EC_POINT_free(v);

	BN_CTX_free(ctx);
}

void generate(unsigned char *pkey, EC_GROUP *ec, EC_POINT *generator, BIGNUM *order, BIGNUM *priv, unsigned long *pid)
{
	BN_CTX *ctx = BN_CTX_new();

	BIGNUM *k = BN_new();
	BIGNUM *s = BN_new();
	BIGNUM *x = BN_new();
	BIGNUM *y = BN_new();
	EC_POINT *r = EC_POINT_new(ec);
	unsigned long bkey[4];

 // Loop in case signaturepart will make cdkey(base-24 "digits") longer than 25 
	do {
		BN_pseudo_rand(k, FIELD_BITS, -1, 0);
		EC_POINT_mul(ec, r, NULL, generator, k, ctx);
		EC_POINT_get_affine_coordinates_GFp(ec, r, x, y, ctx);
		
		SHA_CTX h_ctx;
		unsigned char t[4], md[20], buf[FIELD_BYTES];
		unsigned long hash[1];
		/* h = (fist 32 bits of SHA1(pid || r.x, r.y)) >> 4 */
		SHA1_Init(&h_ctx);
		t[0] =  pid[0] & 0xff;
		t[1] = (pid[0] & 0xff00) >> 8;
		t[2] = (pid[0] & 0xff0000) >> 16;
		t[3] = (pid[0] & 0xff000000) >> 24;
		SHA1_Update(&h_ctx, t, sizeof(t));
		
		memset(buf, 0, sizeof(buf));
		BN_bn2bin(x, buf);
		endian((unsigned char *)buf, sizeof(buf));
		SHA1_Update(&h_ctx, buf, sizeof(buf));
		
		memset(buf, 0, sizeof(buf));
		BN_bn2bin(y, buf);
		endian((unsigned char *)buf, sizeof(buf));
		SHA1_Update(&h_ctx, buf, sizeof(buf));
		
		SHA1_Final(md, &h_ctx);
		hash[0] = (md[0] | (md[1] << 8) | (md[2] << 16) | (md[3] << 24)) >> 4;
		hash[0] &= 0xfffffff;
		
		/* s = priv*h + k */
		BN_copy(s, priv);
		BN_mul_word(s, hash[0]);
		BN_mod_add(s, s, k, order, ctx);
		
		unsigned long sig[2] = {0};
		BN_bn2bin(s, (unsigned char *)sig);
		endian((unsigned char *)sig, BN_num_bytes(s));
		pack(bkey, pid, hash, sig);
		printf("PID: %.8x\nHash: %.8x\nSig: %.8x %.8x\n", pid[0], hash[0], sig[1], sig[0]);
	} while (bkey[3] >= 0x62a32);

	base24(pkey, bkey);
	
	BN_free(k);
	BN_free(s);
	BN_free(x);
	BN_free(y);
	EC_POINT_free(r);

	BN_CTX_free(ctx);
}

int main()
{
 // Init
	BIGNUM *a, *b, *p, *gx, *gy, *pubx, *puby, *n, *priv;
	BN_CTX *ctx = BN_CTX_new();
	
	// make BigNumbers
	a = BN_new();
	b = BN_new();
	p = BN_new();
	gx = BN_new();
	gy = BN_new();
	pubx = BN_new();
	puby = BN_new();
	n = BN_new();
	priv = BN_new();

 // Data from pidgen-Bink-resources
	/* Elliptic curve parameters: y^2 = x^3 + ax + b mod p */
	BN_hex2bn(&p,    "92ddcf14cb9e71f4489a2e9ba350ae29454d98cb93bdbcc07d62b502ea12238ee904a8b20d017197aae0c103b32713a9");
	BN_set_word(a, 1);
	BN_set_word(b, 0);
	

	/* base point (generator) G */
	BN_hex2bn(&gx,   "46E3775ECE21B0898D39BEA57050D422A0AF989E497962BAEE2CB17E0A28D5360D5476B8DC966443E37A14F1AEF37742");
	BN_hex2bn(&gy,   "7C8E741D2C34F4478E325469CD491603D807222C9C4AC09DDB2B31B3CE3F7CC191B3580079932BC6BEF70BE27604F65E");

	/* inverse of public key */
	BN_hex2bn(&pubx, "5D8DBE75198015EC41C45AAB6143542EB098F6A5CC9CE4178A1B8A1E7ABBB5BC64DF64FAF6177DC1B0988AB00BA94BF8");
	BN_hex2bn(&puby, "23A2909A0B4803C89F910C7191758B48746CEA4D5FF07667444ACDB9512080DBCA55E6EBF30433672B894F44ACE92BFA");

 // Computed data
	/* order of G - computed in 18 hours using a P3-450 */
	BN_hex2bn(&n,    "DB6B4C58EFBAFD");

	/* THE private key  - computed in 10 hours using a P3-450 */
	BN_hex2bn(&priv, "565B0DFF8496C8");

 // Calculation
	EC_GROUP *ec = EC_GROUP_new_curve_GFp(p, a, b, ctx);
	EC_POINT *g = EC_POINT_new(ec);
	EC_POINT_set_affine_coordinates_GFp(ec, g, gx, gy, ctx);
	EC_POINT *pub = EC_POINT_new(ec);
	EC_POINT_set_affine_coordinates_GFp(ec, pub, pubx, puby, ctx);
	
	unsigned char pkey[26];
	unsigned long pid[1];
	pid[0] = 640000000 << 1; /* <- change */

 // generate a key
	generate(pkey, ec, g, n, priv, pid);
	print_product_key(pkey); printf("\n\n");

 // verify the key
	verify(ec, g, pub, (char*)pkey);
	
 // Cleanup
	BN_CTX_free(ctx);
	
	return 0;
}
