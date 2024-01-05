/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2024 UMSKT Contributors (et.al.)
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
 * @FileCreated by Neo on 06/06/2023
 * @Maintainer Neo
 */

#ifndef UMSKT_CLI_H
#define UMSKT_CLI_H

#include "typedefs.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <cmrc/cmrc.hpp>
#include <fmt/core.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

#include "libumskt/confid/confid.h"
#include "libumskt/libumskt.h"
#include "libumskt/pidgen2/PIDGEN2.h"
#include "libumskt/pidgen3/BINK1998.h"
#include "libumskt/pidgen3/BINK2002.h"
#include "libumskt/pidgen3/PIDGEN3.h"
#include "options.h"

CMRC_DECLARE(umskt);

enum APPLICATION_STATE
{
    STATE_BINK1998_GENERATE,
    STATE_BINK2002_GENERATE,
    STATE_CONFIRMATION_ID,
    STATE_BINK1998_VALIDATE,
    STATE_BINK2002_VALIDATE
};

struct Options
{
    int argc;
    char **argv;

    std::string binkid;
    std::string keysFilename;
    std::string instid;
    std::string keyToCheck;
    std::string productid;
    std::string authInfo;
    std::string productCode;

    DWORD channelID;
    DWORD serial;
    DWORD numKeys;
    BOOL oem;
    BOOL upgrade;
    BOOL serialSet;
    BOOL verbose;
    BOOL help;
    BOOL error;
    BOOL list;

    APPLICATION_STATE state;
};

class CLI
{
    std::string pKey;
    DWORD count, total;

    static Options options;

  public:
    CLI()
    {
        count = 0;
        total = options.numKeys;
    }

    static std::array<CLIHelpOptions, CLIHelpOptionID_END> helpOptions;
    static json keys;

    static BYTE Init(int argv, char *argc[]);
    static void SetHelpText();

    static CLIHandlerFunc loadJSON;
    static CLIHandlerFunc DisplayHelp;
    static CLIHandlerFunc DisplayErrorMessage;
    static CLIHandlerFunc SetVerboseOption;
    static CLIHandlerFunc SetDebugOption;
    static CLIHandlerFunc SetListOption;
    static CLIHandlerFunc SetOEMOption;
    static CLIHandlerFunc SetUpgradeOption;
    static CLIHandlerFunc SetFileOption;
    static CLIHandlerFunc SetNumberOption;
    static CLIHandlerFunc SetChannelIDOption;
    static CLIHandlerFunc SetBINKOption;
    static CLIHandlerFunc SetSerialOption;
    static CLIHandlerFunc SetActivationIDOption;
    static CLIHandlerFunc SetProductIDOption;
    static CLIHandlerFunc SetValidateOption;
    static CLIHandlerFunc SetProductCodeOption;

    static BOOL parseCommandLine();
    static BOOL processOptions();
    static void printID(DWORD *pid);
    static void printKey(std::string pk);
    static BOOL stripKey(const std::string &in_key, std::string &out_key);

    BOOL InitPIDGEN3(PIDGEN3 *pidgen3);
    BOOL InitConfirmationID(ConfirmationID *confid);

    BOOL BINK1998Generate();
    BOOL BINK2002Generate();
    BOOL BINK1998Validate();
    BOOL BINK2002Validate();
    BOOL ConfirmationIDGenerate();

    int Run();
};

#endif // UMSKT_CLI_H
