//
// Created by neo on 6/6/2023.
//

#ifndef WINDOWSXPKG_CLI_H
#define WINDOWSXPKG_CLI_H

#include "header.h"

class CLI {
    Options options;
    json keys;
    const char* BINKID;
    BIGNUM *privateKey, *genOrder;
    EC_POINT *genPoint, *pubPoint;
    EC_GROUP *eCurve;
    char pKey[25];
    int count, total;

public:
    CLI(Options options, json keys);

    static bool loadJSON(const fs::path& filename, json *output);
    static void showHelp(char *argv[]);
    static int parseCommandLine(int argc, char* argv[], Options *options);
    static int validateCommandLine(Options* options, char *argv[], json *keys);
    static void printID(DWORD *pid);
    static void printKey(char *pk);

    int BINK1998();
    int BINK2002();
    int ConfirmationID();
};

#endif //WINDOWSXPKG_CLI_H
