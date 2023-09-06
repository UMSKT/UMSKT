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
 * @FileCreated by Neo on 6/6/2023
 * @Maintainer Neo
 */

#ifndef UMSKT_CLI_H
#define UMSKT_CLI_H

#include "header.h"

#include <cmrc/cmrc.hpp>

#include "libumskt/libumskt.h"
#include "libumskt/pidgen2/PIDGEN2.h"
#include "libumskt/pidgen3/PIDGEN3.h"
#include "libumskt/pidgen3/BINK1998.h"
#include "libumskt/pidgen3/BINK2002.h"
#include "libumskt/confid/confid.h"

CMRC_DECLARE(umskt);

enum ACTIVATION_ALGORITHM {
    WINDOWS     = 0,
    OFFICE_XP   = 1,
    OFFICE_2K3  = 2,
    OFFICE_2K7  = 3,
    PLUS_DME    = 4,
};

enum MODE {
    MODE_BINK1998_GENERATE = 0,
    MODE_BINK2002_GENERATE = 1,
    MODE_CONFIRMATION_ID   = 2,
    MODE_BINK1998_VALIDATE = 3,
    MODE_BINK2002_VALIDATE = 4,
};

struct Options {
    std::string binkid;
    std::string keysFilename;
    std::string instid;
    std::string keyToCheck;
    std::string productid;
    int channelID;
    int serial;
    int numKeys;
    bool upgrade;
    bool serialSet;
    bool verbose;
    bool help;
    bool error;
    bool list;

    MODE applicationMode;
    ACTIVATION_ALGORITHM activationMode;
};

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
    static bool stripKey(const char *in_key, char out_key[PK_LENGTH]);

    int BINK1998Generate();
    int BINK2002Generate();
    int BINK1998Validate();
    int BINK2002Validate();
    int ConfirmationID();
};

#endif //UMSKT_CLI_H
