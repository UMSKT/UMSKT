//
// Created by Andrew on 01/06/2023.
//

#include "header.h"
#include <iostream>

char charset[] = "BCDFGHJKMPQRTVWXY2346789";

using json = nlohmann::json;

int main() {
    char* BINKID = "2E";

    std::ifstream f("keys.json");
    json keys = json::parse(f);

    rand();
    srand(time(nullptr));
    rand();

    // We cannot produce a valid key without knowing the private key k. The reason for this is that
    // we need the result of the function K(x; y) = kG(x; y).
    BIGNUM *privateKey = BN_new();

    // We can, however, validate any given key using the available public key: {p, a, b, G, K}.
    // genOrder the order of the generator G, a value we have to reverse -> Schoof's Algorithm.
    BIGNUM *genOrder = BN_new();

    /* Computed data */
    BN_dec2bn(&genOrder, keys["BINK"][BINKID]["n"].get<std::string>().c_str());
    BN_dec2bn(&privateKey, keys["BINK"][BINKID]["priv"].get<std::string>().c_str());

    std::cout << keys["BINK"][BINKID]["p"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["a"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["b"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["g"]["x"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["g"]["y"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["pub"]["x"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["pub"]["y"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["n"].get<std::string>().c_str() << std::endl;
    std::cout << keys["BINK"][BINKID]["priv"].get<std::string>().c_str() << std::endl;

    EC_POINT *genPoint, *pubPoint;
    EC_GROUP *eCurve = initializeEllipticCurve(
            keys["BINK"][BINKID]["p"].get<std::string>().c_str(),
            keys["BINK"][BINKID]["a"].get<std::string>().c_str(),
            keys["BINK"][BINKID]["b"].get<std::string>().c_str(),
            keys["BINK"][BINKID]["g"]["x"].get<std::string>().c_str(),
            keys["BINK"][BINKID]["g"]["y"].get<std::string>().c_str(),
            keys["BINK"][BINKID]["pub"]["x"].get<std::string>().c_str(),
            keys["BINK"][BINKID]["pub"]["y"].get<std::string>().c_str(),
            &genPoint,
            &pubPoint
    );

    /*BN_print_fp(stdout, p);
    std::cout << std::endl;
    BN_print_fp(stdout, a);
    std::cout << std::endl;
    BN_print_fp(stdout, b);
    std::cout << std::endl;
    BN_print_fp(stdout, gx);
    std::cout << std::endl;
    BN_print_fp(stdout, gy);
    std::cout << std::endl;
    BN_print_fp(stdout, pubx);
    std::cout << std::endl;
    BN_print_fp(stdout, puby);
    std::cout << std::endl;
    BN_print_fp(stdout, n);
    std::cout << std::endl;
    BN_print_fp(stdout, priv);
    std::cout << std::endl;*/
    // Calculation


    char pKey[25];

    ul32 nRaw = 640 * 1000000 ; /* <- change */
    //nRaw += rand() & 999999;

    printf("> PID: %u\n", nRaw);

    // generate a key
    BN_sub(privateKey, genOrder, privateKey);
    nRaw <<= 1;

    generateXPKey(pKey, eCurve, genPoint, genOrder, privateKey, &nRaw);
    print_product_key(pKey);
    printf("\n\n");

    // verify the key
    if (!verifyXPKey(eCurve, genPoint, pubPoint, pKey)) printf("Fail! Key is invalid.\n");

    return 0;
}