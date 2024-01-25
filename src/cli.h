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

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// fmt <-> json linkage
template <> struct fmt::formatter<json> : ostream_formatter
{
    auto format(const json &j, format_context &ctx) const
    {
        if (j.is_string())
        {
            return formatter<string_view>::format(j.get<std::string>(), ctx);
        }
        else
        {
            return basic_ostream_formatter<char>::format(j, ctx);
        }
    }
};

#include "help.h"
#include "libumskt/confid/confid.h"
#include "libumskt/libumskt.h"
#include "libumskt/pidgen2/PIDGEN2.h"
#include "libumskt/pidgen3/BINK1998.h"
#include "libumskt/pidgen3/BINK2002.h"
#include "libumskt/pidgen3/PIDGEN3.h"

#ifndef UMSKTCLI_VERSION_STRING
#define UMSKTCLI_VERSION_STRING "unknown version-dirty"
#endif

enum APPLICATION_STATE
{
    STATE_PIDGEN_GENERATE,
    STATE_PIDGEN_VALIDATE,
    STATE_CONFIRMATION_ID
};

enum PIDGEN_VERSION
{
    PIDGEN_2 = 2,
    PIDGEN_3 = 3,
};

struct Options
{
    int argc;
    char **argv;

    std::string binkID;
    std::string keysFilename;
    std::string installationID;
    std::string keyToCheck;
    std::string productID;
    std::string authInfo;
    std::string productCode;
    std::string productFlavour;

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

    struct Meta
    {
        std::string type;
        std::vector<std::string> tags;
        struct Activation
        {
            std::string flavour;
            int version;
        };
    };

    PIDGEN3::KeyInfo info;

    PIDGEN_VERSION pidgenversion;
    APPLICATION_STATE state;
};

class CLI
{
    std::string pKey;
    DWORD count, total, iBinkID;

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

    static BOOL loadJSON(const fs::path &filename);
    static BOOL loadEmbeddedJSON();

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
    static CLIHandlerFunc SetFlavourOption;

    static BOOL parseCommandLine();
    static BOOL processOptions();
    static void printID(DWORD *pid);
    static void printKey(std::string &pk);
    static BOOL stripKey(const std::string &in_key, std::string &out_key);
    static std::string validateInputKeyCharset(std::string &accumulator, char currentChar);

    BOOL InitPIDGEN3(PIDGEN3 &pidgen3);
    BOOL InitConfirmationID(ConfirmationID &confid);

    BOOL PIDGENGenerate();
    BOOL PIDGENValidate();

    BOOL PIDGEN2Generate(PIDGEN2 &pidgen2);
    BOOL PIDGEN2Validate(PIDGEN2 &pidgen2);
    BOOL BINK1998Generate(PIDGEN3 &pidgen3);
    BOOL BINK1998Validate(PIDGEN3 &pidgen3);
    BOOL BINK2002Generate(PIDGEN3 &pidgen3);
    BOOL BINK2002Validate(PIDGEN3 &pidgen3);
    BOOL ConfirmationIDGenerate();

    INLINE static std::string strtolower(std::string &in)
    {
        auto retval = std::string(in);
        std::transform(retval.begin(), retval.end(), retval.begin(), ::tolower);
        return retval;
    }

    INLINE static std::string strtoupper(std::string &in)
    {
        auto retval = std::string(in);
        std::transform(retval.begin(), retval.end(), retval.begin(), ::toupper);
        return retval;
    }

    int Run();
};

#endif // UMSKT_CLI_H
