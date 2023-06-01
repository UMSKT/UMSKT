//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

void print_product_id(ul32 *pid)
{
    char raw[12];
    char b[6], c[8];
    int i, digit = 0;

    //	Cut a away last bit of pid and convert it to an accii-number (=raw)
    sprintf(raw, "%lu", pid[0] >> 1);

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

    printf("Product ID: PPPPP-%s-%s-23xxx\n", b, c);
}

void print_product_key(char *pk) {
    int i;
    assert(strlen(pk) == 25);
    for (i = 0; i < 25; i++) {
        putchar(pk[i]);
        if (i != 24 && i % 5 == 4) putchar('-');
    }
}