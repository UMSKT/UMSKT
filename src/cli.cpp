//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

void showHelp(char *argv[]) {
    std::cout << "usage: " << argv[0] << std::endl                                             << std::endl
              << "\t-h --help\tshow this message"                                              << std::endl
              << "\t-v --verbose\tenable verbose output"                                       << std::endl
              << "\t-b --binkid\tspecify which BINK identifier to load (defaults to 2E)"       << std::endl
              << "\t-l --list\tshow which products/binks can be loaded"                        << std::endl
              << "\t-c --channelid\tspecify which Channel Identifier to use (defaults to 640)" << std::endl
              << std::endl << std::endl;
}

Options parseCommandLine(int argc, char* argv[]) {
    Options options = {
        "2E",
        640,
        false,
        false,
        false
    };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "-b" || arg == "--bink") {
            options.binkid = argv[i+1];
            i++;
        } else if (arg == "-l" || arg == "--list") {
            options.list = true;
        } else if (arg == "-c" || arg == "--channelid") {
            int siteID;
            if (!sscanf(argv[i+1], "%d", &siteID)) {
                options.error = true;
            } else {
                options.channelID = siteID;
            }
            i++;
        } else {
            options.error = true;
        }
    }

    return options;
}

void print_product_id(DWORD *pid)
{
    char raw[12];
    char b[6], c[8];
    int i, digit = 0;

    //	Cut a away last bit of pid and convert it to an accii-number (=raw)
    sprintf(raw, "%iu", pid[0] >> 1);

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