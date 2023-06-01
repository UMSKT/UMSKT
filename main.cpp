//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

char charset[] = "BCDFGHJKMPQRTVWXY2346789";

int main()
{
    initBink();

    rand();
    srand(time(nullptr));
    rand();

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

    char* BINKID = "2E";

    // Data from pidgen-Bink-resources
    /* Elliptic curve parameters: y^2 = x^3 + ax + b mod p */
    BN_dec2bn(&p,    std::get<0>(BINKData[BINKID].E).c_str());
    BN_dec2bn(&a,    std::get<1>(BINKData[BINKID].E).c_str());
    BN_dec2bn(&b,    std::get<2>(BINKData[BINKID].E).c_str());


    /* base point (generator) G */
    BN_dec2bn(&gx,   std::get<0>(BINKData[BINKID].G).c_str());
    BN_dec2bn(&gy,   std::get<1>(BINKData[BINKID].G).c_str());

    /* inverse of public key */
    BN_dec2bn(&pubx, std::get<0>(BINKData[BINKID].K).c_str());
    BN_dec2bn(&puby, std::get<1>(BINKData[BINKID].K).c_str());

    // Computed data
    /* order of G - computed in 18 hours using a P3-450 */
    BN_dec2bn(&n,    BINKData[BINKID].n.c_str());

    /* THE private key  - computed in 10 hours using a P3-450 */
    BN_dec2bn(&priv, BINKData[BINKID].k.c_str());

    // Calculation
    EC_GROUP *ec = EC_GROUP_new_curve_GFp(p, a, b, ctx);
    EC_POINT *g = EC_POINT_new(ec);
    EC_POINT_set_affine_coordinates_GFp(ec, g, gx, gy, ctx);
    EC_POINT *pub = EC_POINT_new(ec);
    EC_POINT_set_affine_coordinates_GFp(ec, pub, pubx, puby, ctx);

    char pkey[26];
    ul32 pid[1];
    pid[0] = 640 * 1000000 ; /* <- change */
    pid[0] += rand() & 999999;

    printf("> PID: %lu\n", pid[0]);

    // generate a key
    BN_sub(priv, n, priv);
    generateXPKey(pkey, ec, g, n, priv, pid);
    print_product_key(pkey);
    printf("\n\n");

    // verify the key
    verifyXPKey(ec, g, pub, (char*)pkey);

    // Cleanup
    BN_CTX_free(ctx);

    return 0;
}