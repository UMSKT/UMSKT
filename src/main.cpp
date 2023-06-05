//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

char pCharset[] = "BCDFGHJKMPQRTVWXY2346789";

int main(int argc, char *argv[]) {
    Options options;

    if (!parseCommandLine(argc, argv, &options)) {
        fmt::print("error parsing command line\n");
        return !options.error ? 0 : 1;
    }

    json keys;
    if (validateCommandLine(&options, argv, &keys) < 0) {
        return 1;
    }

    const char* BINKID = options.binkid.c_str();

    // We cannot produce a valid key without knowing the private key k. The reason for this is that
    // we need the result of the function K(x; y) = kG(x; y).
    BIGNUM *privateKey = BN_new();

    // We can, however, validate any given key using the available public key: {p, a, b, G, K}.
    // genOrder the order of the generator G, a value we have to reverse -> Schoof's Algorithm.
    BIGNUM *genOrder = BN_new();

    /* Computed data */
    BN_dec2bn(&genOrder, keys["BINK"][BINKID]["n"].get<std::string>().c_str());
    BN_dec2bn(&privateKey, keys["BINK"][BINKID]["priv"].get<std::string>().c_str());

    if (options.verbose) {
        fmt::print("----------------------------------------------------------- \n");
        fmt::print("Loaded the following curve constraints: BINK[{}]\n", BINKID);
        fmt::print("----------------------------------------------------------- \n");
        fmt::print(" P: {}\n", keys["BINK"][BINKID]["p"].get<std::string>());
        fmt::print(" a: {}\n", keys["BINK"][BINKID]["a"].get<std::string>());
        fmt::print(" b: {}\n", keys["BINK"][BINKID]["b"].get<std::string>());
        fmt::print("Gx: {}\n", keys["BINK"][BINKID]["g"]["x"].get<std::string>());
        fmt::print("Gy: {}\n", keys["BINK"][BINKID]["g"]["y"].get<std::string>());
        fmt::print("Kx: {}\n", keys["BINK"][BINKID]["pub"]["x"].get<std::string>());
        fmt::print("Ky: {}\n", keys["BINK"][BINKID]["pub"]["y"].get<std::string>());
        fmt::print(" n: {}\n", keys["BINK"][BINKID]["n"].get<std::string>());
        fmt::print(" k: {}\n", keys["BINK"][BINKID]["priv"].get<std::string>());
    }

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

    // Calculation
    char pKey[25];

    DWORD nRaw = options.channelID * 1000000 ; /* <- change */

    BIGNUM *bnrand = BN_new();
    BN_rand(bnrand, 19, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

    int oRaw;
    char *cRaw = BN_bn2dec(bnrand);

    sscanf(cRaw, "%d", &oRaw);
    nRaw += (oRaw &= 0xF423F); // ensure our serial is less than 999999

    if (options.verbose) {
        fmt::print("> PID: {:09d}\n", nRaw);
    }

    // generate a key
    BN_sub(privateKey, genOrder, privateKey);
    nRaw <<= 1;
    int count = 0, total = 1000;

    for (int i = 0; i < total; i++) {
        generateXPKey(eCurve, genPoint, genOrder, privateKey, nRaw, pKey);
        print_product_key(pKey);
        fmt::print("\n\n");

        // verify the key
        count += verifyXPKey(eCurve, genPoint, pubPoint, pKey);
    }

    fmt::print("Success count: {}/{}\n", count, total);

    return 0;
}
