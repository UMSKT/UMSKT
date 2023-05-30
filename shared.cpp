//
// Created by neo on 5/26/2023.
//

#include "shared.h"

uint8_t cset[] = "BCDFGHJKMPQRTVWXY2346789";

// Reverse data
void endian(uint8_t *data, int len)
{
    int i;
    for (i = 0; i < len/2; i++) {
        uint8_t temp;
        temp = data[i];
        data[i] = data[len-i-1];
        data[len-i-1] = temp;
    }
}

void unbase24(uint32_t *x, uint8_t *c)
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
    BN_bn2bin(y, (uint8_t *)x);
    BN_free(y);

    endian((uint8_t *)x, n);
}

void base24(uint8_t *c, uint32_t *x)
{
    uint8_t y[16];
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
        uint8_t t = BN_div_word(z, 24);
        c[i] = cset[t];
    }

    BN_free(z);
}

void print_product_id(uint32_t *pid)
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

    printf("> %s\n", c);
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

void print_product_key(uint8_t *pk)
{
    int i;
    assert(strlen((const char *)pk) == 25);
    for (i = 0; i < 25; i++) {
        putchar(pk[i]);
        if (i != 24 && i % 5 == 4) putchar('-');
    }
}