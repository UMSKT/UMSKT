/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2023 UMSKT Contributors (et.al.)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @FileCreated by Andrew on 01/06/2023
 * @Maintainer Neo
 */

#include "cli.h"

bool CLI::loadJSON(const fs::path& filename, json *output) {
    if (!filename.empty() && !fs::exists(filename)) {
        fmt::print("ERROR: File {} does not exist\n", filename.string());
        return false;
    }
    else if (fs::exists(filename)) {
        std::ifstream f(filename);
        *output = json::parse(f, nullptr, false, false);
    }
    else if (filename.empty()) {
        cmrc::embedded_filesystem fs = cmrc::umskt::get_filesystem();
        cmrc::file keys = fs.open("keys.json");
        *output = json::parse(keys, nullptr, false, false);
    }

    if (output->is_discarded()) {
        fmt::print("ERROR: Unable to parse keys from {}\n", filename.string());
        return false;
    }

    return true;
}


void CLI::showHelp(char *argv[]) {
    fmt::print("usage: {} \n", argv[0]);
    fmt::print("\t-h --help\tshow this message\n");
    fmt::print("\t-v --verbose\tenable verbose output\n");
    fmt::print("\t-n --number\tnumber of keys to generate (defaults to 1)\n");
    fmt::print("\t-f --file\tspecify which keys file to load\n");
    fmt::print("\t-i --instid\tinstallation ID used to generate confirmation ID\n");
    fmt::print("\t-m --mode\tproduct family to activate. valid options are \"Windows\", \"OfficeXP\", \"Office2K3+\", \"PlusDME\"\n");
    fmt::print("\t-p --productid\tthe product ID of the Program to activate. only required for Office 2K3 or Office 2K7 programs\n");
    fmt::print("\t-b --binkid\tspecify which BINK identifier to load (defaults to 2E)\n");
    fmt::print("\t-l --list\tshow which products/binks can be loaded\n");
    fmt::print("\t-c --channelid\tspecify which Channel Identifier to use (defaults to 640)\n");
    fmt::print("\t-s --serial\tspecifies a serial to use in the product ID (defaults to random, BINK1998 only)\n");
    fmt::print("\t-u --upgrade\tspecifies the Product Key will be an \"Upgrade\" version\n");
    fmt::print("\t-V --validate\tproduct key to validate signature\n");
    fmt::print("\n");
}

int CLI::parseCommandLine(int argc, char* argv[], Options* options) {
    // set default options
    *options = Options {
            "2E",
            "",
            "",
            "",
            "",
            640,
            0,
            1,
	    false,
            false,
            false,
            false,
            false,
            false,
            MODE_BINK1998_GENERATE,
	    WINDOWS
    };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-v" || arg == "--verbose") {
            options->verbose = true;
            UMSKT::setDebugOutput(stderr);
        } else if (arg == "-h" || arg == "--help") {
            options->help = true;
        } else if (arg == "-n" || arg == "--number") {
            if (i == argc - 1) {
                options->error = true;
                break;
            }

            int nKeys;
            if (!sscanf(argv[i+1], "%d", &nKeys)) {
                options->error = true;
            } else {
                options->numKeys = nKeys;
            }
            i++;
        } else if (arg == "-b" || arg == "--bink") {
            if (i == argc - 1) {
                options->error = true;
                break;
            }

            options->binkid = argv[i+1];
            i++;
        } else if (arg == "-l" || arg == "--list") {
            options->list = true;
        } else if (arg == "-c" || arg == "--channelid") {
            if (i == argc - 1) {
                options->error = true;
                break;
            }

            int siteID;
            if (!sscanf(argv[i+1], "%d", &siteID)) {
                options->error = true;
            } else {
                options->channelID = siteID;
            }
            i++;
        } else if (arg == "-s" || arg == "--serial") {
            if (i == argc - 1) {
                options->error = true;
                break;
            }

            int serial_val;
            if (!sscanf(argv[i+1], "%d", &serial_val)) {
                options->error = true;
            } else {
                options->serialSet = true;
                options->serial = serial_val;
            }
            i++;
	} else if (arg == "-u" || arg == "--upgrade") {
	    options->upgrade = true;
            break;
        } else if (arg == "-f" || arg == "--file") {
            if (i == argc - 1) {
                options->error = true;
                break;
            }

            options->keysFilename = argv[i+1];
            i++;
        } else if (arg == "-i" || arg == "--instid") {
            if (i == argc - 1) {
                options->error = true;
                break;
            }

            options->instid = argv[i+1];
            options->applicationMode = MODE_ACTIVATION;
            i++;
            if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mode") == 0) {
                if (strcmp(argv[i+1], "windows") == 0 || strcmp(argv[i+1], "Windows") == 0 || strcmp(argv[i+1], "WINDOWS") == 0) {
                    options->activationMode = WINDOWS;
		} else if (strcmp(argv[i+1], "officexp") == 0 || strcmp(argv[i+1], "OffieXP") == 0 || strcmp(argv[i+1], "OFFICEXP") == 0) {
                    options->activationMode = OFFICE_XP;
		} else if (strcmp(argv[i+1], "office2k3+") == 0 || strcmp(argv[i+1], "Offie2K3+") == 0 || strcmp(argv[i+1], "OFFICE2K3+") == 0) {
                    options->activationMode = OFFICE_2K3+;
		} else if (strcmp(argv[i+1], "plusdme") == 0 || strcmp(argv[i+1], "PlusDME") == 0 || strcmp(argv[i+1], "PLUSDME") == 0) {
                    options->activationMode = PLUS_DME;
		}
            }
            if (options.activationMode == OFFICE_2K3+) {
                options->productid = argv[i+1];
	    }
            break;
        } else if (arg == "-V" || arg == "--validate") {
            if (i == argc - 1) {
                options->error = true;
                break;
            }

            options->keyToCheck = argv[i+1];
            options->applicationMode = MODE_BINK1998_VALIDATE;
            i++;
        } else {
            options->error = true;
        }
    }

    return !options->error;
}

int CLI::validateCommandLine(Options* options, char *argv[], json *keys) {
    if (options->help || options->error) {
        if (options->error) {
            fmt::print("error parsing command line options\n");
        }
        showHelp(argv);
        return 1;
    }

    if (options->verbose) {
        if(options->keysFilename.empty()) {
            fmt::print("Loading internal keys file\n");
        } else {
            fmt::print("Loading keys file {}\n", options->keysFilename);
        }
    }

    if (!loadJSON(options->keysFilename, keys)) {
        return 2;
    }

    if (options->verbose) {
        if(options->keysFilename.empty()) {
            fmt::print("Loaded internal keys file successfully\n");
        } else {
            fmt::print("Loaded keys from {} successfully\n",options->keysFilename);
        }
    }

    if (options->list) {
        for (auto el : (*keys)["Products"].items()) {
            int id;
            sscanf((el.value()["BINK"][0]).get<std::string>().c_str(), "%x", &id);
            std::cout << el.key() << ": " << el.value()["BINK"] << std::endl;
        }

        fmt::print("\n\n");
        fmt::print("** Please note: any BINK ID other than 2E is considered experimental at this time **\n");
        fmt::print("\n");
        return 1;
    }

    int intBinkID;
    sscanf(options->binkid.c_str(), "%x", &intBinkID);

    if (intBinkID >= 0x40) {
        // set bink2002 validate mode if in bink1998 validate mode, else set bink2002 generate mode
        options->applicationMode = (options->applicationMode == MODE_BINK1998_VALIDATE) ? MODE_BINK2002_VALIDATE : MODE_BINK2002_GENERATE;
    }

    if (options->channelID > 999) {
        fmt::print("ERROR: refusing to create a key with a Channel ID greater than 999\n");
        return 1;
    }

    // don't allow any serial not between 0 and 999999
    if (options->serial > 999999 || options->serial < 0) {
        fmt::print("ERROR: refusing to create a key with a Serial not between 000000 and 999999\n");
        return 1;
    }

    return 0;
}

void CLI::printID(DWORD *pid) {
    char raw[12];
    char b[6], c[8];
    int i, digit = 0;

    // Convert PID to ascii-number (=raw)
    snprintf(raw, sizeof(raw), "%09u", pid[0]);

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

    fmt::print("> Product ID: PPPPP-{}-{}-23xxx\n", b, c);
}

void CLI::printKey(char *pk) {
    assert(strlen(pk) >= PK_LENGTH);

    std::string spk = pk;
    fmt::print("{}-{}-{}-{}-{}",
               spk.substr(0,5),
               spk.substr(5,5),
               spk.substr(10,5),
               spk.substr(15,5),
               spk.substr(20,5));
}

bool CLI::stripKey(const char *in_key, char out_key[PK_LENGTH]) {
    // copy out the product key stripping out extraneous characters
    const char *p = in_key;
    size_t i = 0;
    for (; *p; p++) {
        // strip out space or dash
		if (*p == ' ' || *p == '-')
			continue;
        // check if we've passed the product key length to avoid overflow
        if (i >= PK_LENGTH)
            return false;
        // convert to uppercase - if character allowed, copy into array
        for (int j = 0; j < strlen(PIDGEN3::pKeyCharset); j++) {
            if (toupper(*p) == PIDGEN3::pKeyCharset[j]) {
                out_key[i++] = toupper(*p);
                continue;
            }
        }
    }
    // only return true if we've handled exactly PK_LENGTH chars
    return (i == PK_LENGTH);
}

CLI::CLI(Options options, json keys) {
    this->options = options;
    this->keys = keys;

    this->BINKID = options.binkid.c_str();

    // We cannot produce a valid key without knowing the private key k. The reason for this is that
    // we need the result of the function K(x; y) = kG(x; y).
    this->privateKey = BN_new();

    // We can, however, validate any given key using the available public key: {p, a, b, G, K}.
    // genOrder the order of the generator G, a value we have to reverse -> Schoof's Algorithm.
    this->genOrder = BN_new();

    /* Computed data */
    BN_dec2bn(&this->genOrder,   this->keys["BINK"][this->BINKID]["n"].   get<std::string>().c_str());
    BN_dec2bn(&this->privateKey, this->keys["BINK"][this->BINKID]["priv"].get<std::string>().c_str());

    if (options.verbose) {
        fmt::print("----------------------------------------------------------- \n");
        fmt::print("Loaded the following elliptic curve parameters: BINK[{}]\n", this->BINKID);
        fmt::print("----------------------------------------------------------- \n");
        fmt::print(" P: {}\n", this->keys["BINK"][this->BINKID]["p"].get<std::string>());
        fmt::print(" a: {}\n", this->keys["BINK"][this->BINKID]["a"].get<std::string>());
        fmt::print(" b: {}\n", this->keys["BINK"][this->BINKID]["b"].get<std::string>());
        fmt::print("Gx: {}\n", this->keys["BINK"][this->BINKID]["g"]["x"].get<std::string>());
        fmt::print("Gy: {}\n", this->keys["BINK"][this->BINKID]["g"]["y"].get<std::string>());
        fmt::print("Kx: {}\n", this->keys["BINK"][this->BINKID]["pub"]["x"].get<std::string>());
        fmt::print("Ky: {}\n", this->keys["BINK"][this->BINKID]["pub"]["y"].get<std::string>());
        fmt::print(" n: {}\n", this->keys["BINK"][this->BINKID]["n"].get<std::string>());
        fmt::print(" k: {}\n", this->keys["BINK"][this->BINKID]["priv"].get<std::string>());
        fmt::print("\n");
    }


    eCurve = PIDGEN3::initializeEllipticCurve(
            this->keys["BINK"][this->BINKID]["p"].get<std::string>(),
            this->keys["BINK"][this->BINKID]["a"].get<std::string>(),
            this->keys["BINK"][this->BINKID]["b"].get<std::string>(),
            this->keys["BINK"][this->BINKID]["g"]["x"].get<std::string>(),
            this->keys["BINK"][this->BINKID]["g"]["y"].get<std::string>(),
            this->keys["BINK"][this->BINKID]["pub"]["x"].get<std::string>(),
            this->keys["BINK"][this->BINKID]["pub"]["y"].get<std::string>(),
            this->genPoint,
            this->pubPoint
    );

    this->count = 0;
    this->total = this->options.numKeys;
}

int CLI::BINK1998Generate() {
    // raw PID/serial value
    DWORD nRaw = this->options.channelID * 1'000'000 ; /* <- change */

    // using user-provided serial
    if (this->options.serialSet) {
        // just in case, make sure it's less than 999999
        int serialRnd = (this->options.serial % 999999);
        nRaw += serialRnd;
    } else {
        // generate a random number to use as a serial
        BIGNUM *bnrand = BN_new();
        BN_rand(bnrand, 19, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

        int oRaw;
        char *cRaw = BN_bn2dec(bnrand);

        sscanf(cRaw, "%d", &oRaw);
        nRaw += (oRaw % 999999); // ensure our serial is less than 999999
    }

    if (this->options.verbose) {
        // print the resulting Product ID
        // PID value is printed in BINK1998::Generate
        printID(&nRaw);
    }

    // generate a key
    BN_sub(this->privateKey, this->genOrder, this->privateKey);

    // Specify whether an upgrade version or not
    bool bUpgrade = false;
    if (options.upgrade == true)
	    bUpgrade = true;

    for (int i = 0; i < this->total; i++) {
        PIDGEN3::BINK1998::Generate(this->eCurve, this->genPoint, this->genOrder, this->privateKey, nRaw, bUpgrade, this->pKey);

        bool isValid = PIDGEN3::BINK1998::Verify(this->eCurve, this->genPoint, this->pubPoint, this->pKey);
        if (isValid) {
            CLI::printKey(this->pKey);
            if (i < this->total - 1 || this->options.verbose) {
                fmt::print("\n");
            }
            this->count += isValid;
        }
        else {
            if (this->options.verbose) {
                CLI::printKey(this->pKey);
                fmt::print(" [Invalid]");
                if (i < this->total - 1) {
                    fmt::print("\n");
                }
            }
            this->total++; // queue a redo, basically
        }
    }

    if (this->options.verbose) {
        fmt::print("\nSuccess count: {}/{}", this->count, this->total);
    }
#ifndef _WIN32
    fmt::print("\n");
#endif

    return 0;
}

int CLI::BINK2002Generate() {
    DWORD pChannelID = this->options.channelID;

    if (this->options.verbose) {
        fmt::print("> Channel ID: {:03d}\n", this->options.channelID);
    }

    // generate a key
    for (int i = 0; i < this->total; i++) {
        DWORD pAuthInfo;
        RAND_bytes((BYTE *)&pAuthInfo, 4);
        pAuthInfo &= BITMASK(10);

        if (this->options.verbose) {
            fmt::print("> AuthInfo: {}\n", pAuthInfo);
        }

        // Specify whether an upgrade version or not
        bool bUpgrade = false;
        if (options.upgrade == true)
            bUpgrade = true;

        PIDGEN3::BINK2002::Generate(this->eCurve, this->genPoint, this->genOrder, this->privateKey, pChannelID, pAuthInfo, bUpgrade, this->pKey);

        bool isValid = PIDGEN3::BINK2002::Verify(this->eCurve, this->genPoint, this->pubPoint, this->pKey);
        if (isValid) {
            CLI::printKey(this->pKey);
            if (i < this->total - 1 || this->options.verbose) { // check if end of list or verbose
                fmt::print("\n");
            }
            this->count += isValid; // add to count
        }
        else {
            if (this->options.verbose) {
                CLI::printKey(this->pKey); // print the key
                fmt::print(" [Invalid]"); // and add " [Invalid]" to the key
                if (i < this->total - 1) { // check if end of list
                    fmt::print("\n");
                }
            }
            this->total++; // queue a redo, basically
        }
    }

    if (this->options.verbose) {
        fmt::print("\nSuccess count: {}/{}", this->count, this->total);
    }
#ifndef _WIN32
    fmt::print("\n");
#endif

    return 0;
}

int CLI::BINK1998Validate() {
    char product_key[PK_LENGTH]{};

    if (!CLI::stripKey(this->options.keyToCheck.c_str(), product_key)) {
        fmt::print("ERROR: Product key is in an incorrect format!\n");
        return 1;
    }

    CLI::printKey(product_key);
    fmt::print("\n");
    if (!PIDGEN3::BINK1998::Verify(this->eCurve, this->genPoint, this->pubPoint, product_key)) {
        fmt::print("ERROR: Product key is invalid! Wrong BINK ID?\n");
        return 1;
    }

    fmt::print("Key validated successfully!\n");
    return 0;
}

int CLI::BINK2002Validate() {
    char product_key[PK_LENGTH]{};

    if (!CLI::stripKey(this->options.keyToCheck.c_str(), product_key)) {
        fmt::print("ERROR: Product key is in an incorrect format!\n");
        return 1;
    }

    CLI::printKey(product_key);
    fmt::print("\n");
    if (!PIDGEN3::BINK2002::Verify(this->eCurve, this->genPoint, this->pubPoint, product_key)) {
        fmt::print("ERROR: Product key is invalid! Wrong BINK ID?\n");
        return 1;
    }

    fmt::print("Key validated successfully!\n");
    return 0;
}

int CLI::ConfirmationID() {
    char confirmation_id[49];
    int err = ConfirmationID::Generate(this->options.instid.c_str(), confirmation_id);

    switch (err) {
        case ERR_TOO_SHORT:
            fmt::print("ERROR: Installation ID is too short.\n");
            return 1;

        case ERR_TOO_LARGE:
            fmt::print("ERROR: Installation ID is too long.\n");
            return 1;

        case ERR_INVALID_CHARACTER:
            fmt::print("ERROR: Invalid character in installation ID.\n");
            return 1;

        case ERR_INVALID_CHECK_DIGIT:
            fmt::print("ERROR: Installation ID checksum failed. Please check that it is typed correctly.\n");
            return 1;

        case ERR_UNKNOWN_VERSION:
            fmt::print("ERROR: Unknown installation ID version.\n");
            return 1;

        case ERR_UNLUCKY:
            fmt::print("ERROR: Unable to generate valid confirmation ID.\n");
            return 1;

        case SUCCESS:
            fmt::print(confirmation_id);
#ifndef _WIN32
            fmt::print("\n");
#endif
            return 0;

        default:
            fmt::print("Unknown error occurred during Confirmation ID generation: {}\n", err);
    }
    return 1;
}


