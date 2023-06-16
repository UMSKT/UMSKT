/**
 * This file is a part of the WindowsXPKg Project
 *
 * Copyleft (C) 2019-2023 WindowsXPKg Contributors (et.al.)
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
 * @FileCreated by Neo on 6/6/2023
 * @Maintainer Neo
 */

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
