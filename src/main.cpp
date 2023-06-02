//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

char charset[] = "BCDFGHJKMPQRTVWXY2346789";
const std::string filename = "keys.json";

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    Options options = parseCommandLine(argc, argv);

    if (options.help || options.error) {
        if (options.error) {
            std::cout << "error parsing command line options" << std::endl;
        }
        showHelp(argv);
        return 0;
    }

    if (options.verbose) {
        std::cout << "loading " << filename << std::endl;
    }

    std::ifstream f(filename);
    json keys = json::parse(f);

    if (options.verbose) {
        std::cout << "loaded " << filename << " successfully" << std::endl;
    }

    if (options.list) {
        for (auto el : keys["Products"].items()) {
            int id;
            sscanf((el.value()[0]).get<std::string>().c_str(), "%x", &id);
            if (id >= 0x50) {
                continue;
            }
            std::cout << el.key() << ": " << el.value() << std::endl;
        }

        std::cout << std::endl << std::endl
                  << "** Please note: any BINK ID other than 2E is considered experimental at this time **"
                  << std::endl;
        return 0;
    }

    int intBinkID;
    sscanf(options.binkid.c_str(), "%x", &intBinkID);

    if (intBinkID >= 0x50) {
        std::cout << "ERROR: BINK2002 and beyond is not supported in this application at this time" << std::endl;
        return 1;
    }

    if (options.channelID > 999) {
        std::cout << "ERROR: refusing to create a key with a siteID greater than 999" << std::endl;
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
        std::cout << "-----------------------------------------------------------"    << std::endl
                  << "Loaded the following curve constraints: BINK[" << BINKID << "]" << std::endl
                  << "-----------------------------------------------------------"    << std::endl
                  << " P: " << keys["BINK"][BINKID]["p"].get<std::string>()           << std::endl
                  << " a: " << keys["BINK"][BINKID]["a"].get<std::string>()           << std::endl
                  << " b: " << keys["BINK"][BINKID]["b"].get<std::string>()           << std::endl
                  << "Gx: " << keys["BINK"][BINKID]["g"]["x"].get<std::string>()      << std::endl
                  << "Gy: " << keys["BINK"][BINKID]["g"]["y"].get<std::string>()      << std::endl
                  << "Kx: " << keys["BINK"][BINKID]["pub"]["x"].get<std::string>()    << std::endl
                  << "Ky: " << keys["BINK"][BINKID]["pub"]["y"].get<std::string>()    << std::endl
                  << " n: " << keys["BINK"][BINKID]["n"].get<std::string>()           << std::endl
                  << " k: " << keys["BINK"][BINKID]["priv"].get<std::string>()        << std::endl
                  << std::endl << std::endl;
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
            &genPoint,
            &pubPoint
    );

    // Calculation
    char pKey[25];

    uint32_t nRaw = options.channelID * 1000000 ; /* <- change */

    BIGNUM *bnrand = BN_new();
    BN_rand(bnrand, 19, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

    int oRaw;
    char *cRaw = BN_bn2dec(bnrand);

    sscanf(cRaw, "%d", &oRaw);
    nRaw += (oRaw &= 0xF423F); // ensure our serial is less than 999999

    if (options.verbose) {
        std::cout << "> PID: " << std::setw(9) << std::setfill('0') << nRaw << std::endl;
    }

    // generate a key
    BN_sub(privateKey, genOrder, privateKey);
    nRaw <<= 1;

    generateXPKey(pKey, eCurve, genPoint, genOrder, privateKey, &nRaw);
    print_product_key(pKey);
    std::cout << std::endl << std::endl;

    // verify the key
    if (!verifyXPKey(eCurve, genPoint, pubPoint, pKey)) {
        std::cout << "Fail! Key is invalid." << std::endl;
    }

    return 0;
}