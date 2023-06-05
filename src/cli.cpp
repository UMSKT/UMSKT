//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

bool loadJSON(const fs::path& filename, json *output) {
    if (!fs::exists(filename)) {
        fmt::print("{} does not exist", filename.string());
        return false;
    }

    std::ifstream f(filename);
    *output = json::parse(f);

    return true;
}


void showHelp(char *argv[]) {
    fmt::print("usage: {} \n", argv[0]);
    fmt::print("\t-h --help\tshow this message\n");
    fmt::print("\t-v --verbose\tenable verbose output\n");
    fmt::print("\t-f --file\tspecify which keys file to load (defaults to keys.json)\n");
    fmt::print("\t-b --binkid\tspecify which BINK identifier to load (defaults to 2E)\n");
    fmt::print("\t-l --list\tshow which products/binks can be loaded\n");
    fmt::print("\t-c --channelid\tspecify which Channel Identifier to use (defaults to 640)\n");
    fmt::print("\n\n");
}

int parseCommandLine(int argc, char* argv[], Options* options) {
    // set default options
    *options = Options {
        "2E",
        640,
        "keys.json",
        false,
        false,
        false
    };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-v" || arg == "--verbose") {
            options->verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            options->help = true;
        } else if (arg == "-b" || arg == "--bink") {
            options->binkid = argv[i+1];
            i++;
        } else if (arg == "-l" || arg == "--list") {
            options->list = true;
        } else if (arg == "-c" || arg == "--channelid") {
            int siteID;
            if (!sscanf(argv[i+1], "%d", &siteID)) {
                options->error = true;
            } else {
                options->channelID = siteID;
            }
            i++;
        } else if (arg == "-f" || arg == "--file") {
            options->keysFilename = argv[i+1];
            i++;
        } else {
            options->error = true;
        }
    }

    return !options->error;
}

int validateCommandLine(Options* options, char *argv[], json *keys) {
    if (options->help || options->error) {
        if (options->error) {
            fmt::print("error parsing command line options\n");
        }
        showHelp(argv);
        return 1;
    }

    if (options->verbose) {
        fmt::print("loading {}\n", options->keysFilename);
    }

    if (!loadJSON(options->keysFilename, keys)) {
        return 2;
    }

    if (options->verbose) {
        fmt::print("loaded {} successfully\n",options->keysFilename);
    }

    if (options->list) {
        for (auto el : (*keys)["Products"].items()) {
            int id;
            sscanf((el.value()["BINK"][0]).get<std::string>().c_str(), "%x", &id);
            if (id >= 0x50) {
                continue;
            }
            std::cout << el.key() << ": " << el.value()["BINK"] << std::endl;
        }

        fmt::print("\n\n");
        fmt::print("** Please note: any BINK ID other than 2E is considered experimental at this time **\n");
        fmt::print("\n");
        return 1;
    }

    int intBinkID;
    sscanf(options->binkid.c_str(), "%x", &intBinkID);

    if (intBinkID >= 0x50) {
        std::cout << "ERROR: BINK2002 and beyond is not supported in this application at this time" << std::endl;
        return 1;
    }

    if (options->channelID > 999) {
        std::cout << "ERROR: refusing to create a key with a siteID greater than 999" << std::endl;
        return 1;
    }

    return 0;
}

void print_product_id(DWORD *pid)
{
    char raw[12];
    char b[6], c[8];
    int i, digit = 0;

    //	Cut away last bit of pid and convert it to an accii-number (=raw)
    sprintf(raw, "%iu", pid[0] >> 1);

    // Make b-part {640-....}
    strncpy(b, raw, 3);
    b[3] = 0;

    // Make c-part {...-123456X...}
    strcpy(c, raw + 3);
    fmt::print("> {}\n", c);

    // Make checksum digit-part {...56X-}
    assert(strlen(c) == 6);
    for (i = 0; i < 6; i++)
        digit -= c[i] - '0';	// Sum digits

    while (digit < 0)
        digit += 7;
    c[6] = digit + '0';
    c[7] = 0;

    fmt::print("Product ID: PPPPP-{}-{}-23xxx\n", b, c);
}

void print_product_key(char *pk) {
    assert(strlen(pk) == 25);

    std::string spk = pk;
    fmt::print("{}-{}-{}-{}-{}",
               spk.substr(0,5),
               spk.substr(5,5),
               spk.substr(10,5),
               spk.substr(15,5),
               spk.substr(20,5));
}