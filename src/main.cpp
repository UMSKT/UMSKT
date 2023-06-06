//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

char pCharset[] = "BCDFGHJKMPQRTVWXY2346789";

int main(int argc, char *argv[]) {
    Options options;

    if (!parseCommandLine(argc, argv, &options)) {
        fmt::print("error parsing command line\n");
        showHelp(argv);
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

    if (options.instid != "") {
        char confirmation_id[49];
        int err = generateConfId(options.instid.c_str(), confirmation_id);
        
        switch (err) {
            case ERR_TOO_SHORT:
                std::cout << "ERROR: Installation ID is too short." << std::endl;
                return 1;
            case ERR_TOO_LARGE:
                std::cout << "ERROR: Installation ID is too long." << std::endl;
                return 1;
            case ERR_INVALID_CHARACTER:
                std::cout << "ERROR: Invalid character in installation ID."  << std::endl;
                return 1;
            case ERR_INVALID_CHECK_DIGIT:
                std::cout << "ERROR: Installation ID checksum failed. Please check that it is typed correctly." << std::endl;
                return 1;
            case ERR_UNKNOWN_VERSION:
                std::cout << "ERROR: Unknown installation ID version." << std::endl;
                return 1;
            case ERR_UNLUCKY:
                std::cout << "ERROR: Unable to generate valid confirmation ID." << std::endl;
                return 1;
            case SUCCESS:
                std::cout << "Confirmation ID: " << confirmation_id << std::endl;
                return 0;
        }
    }

    // Calculation
    char pKey[25];
    int count = 0, total = options.numKeys;

    if (options.genServer) {
        DWORD pChannelID = options.channelID << 1;

        if (options.verbose) {
            fmt::print("> Channel ID: {:03d}\n", options.channelID);
        }

        // generate a key
        for (int i = 0; i < total; i++) {
            DWORD pAuthInfo;
            RAND_bytes((BYTE *)&pAuthInfo, 4);
            pAuthInfo &= 0x3ff;

            if (options.verbose) {
                fmt::print("> AuthInfo: {}\n", pAuthInfo);
            }

            generateServerKey(eCurve, genPoint, genOrder, privateKey, pChannelID, pAuthInfo, pKey, options.verbose);
            print_product_key(pKey);
            fmt::print("\n\n");

            // verify a key
            count += verifyServerKey(eCurve, genPoint, pubPoint, pKey, options.verbose);
        }
    } else {
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

        for (int i = 0; i < total; i++) {
            generateXPKey(eCurve, genPoint, genOrder, privateKey, nRaw, pKey, options.verbose);
            print_product_key(pKey);
            fmt::print("\n\n");

            // verify the key
            count += verifyXPKey(eCurve, genPoint, pubPoint, pKey, options.verbose);
        }
    }

    fmt::print("Success count: {}/{}\n", count, total);
    return 0;
}
