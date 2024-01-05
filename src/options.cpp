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
 * @FileCreated by Neo on 01/02/2024
 * @Maintainer Neo
 */

#include "cli.h"

// define static storage
std::array<CLIHelpOptions, CLIHelpOptionID_END> CLI::helpOptions;

void CLI::SetHelpText()
{
    helpOptions[OPTION_HELP] = {"h", "help", "show this help text", false, "", &DisplayHelp};

    helpOptions[OPTION_VERBOSE] = {"v", "verbose", "enable verbose output", false, "", &SetVerboseOption};

    helpOptions[OPTION_DEBUG] = {"d", "debug", "enable debug output", false, "", &SetDebugOption};

    helpOptions[OPTION_FILE] = {"f", "file", "specify which keys file to load", true, "", &SetFileOption};

    helpOptions[OPTION_LIST] = {"l", "list", "list supported products", false, "", &SetListOption};

    helpOptions[OPTION_NUMBER] = {"n",  "number", "(PIDGEN only) number of keys to generate",
                                  true, "1",      &SetNumberOption};

    helpOptions[OPTION_PRODUCT] = {"p",  "product",           "which product to generate keys for",
                                   true, options.productCode, nullptr};

    helpOptions[OPTION_ACTIVATIONID] = {
        "i",  "instid", "(activation only) installation ID used to generate confirmation ID",
        true, "",       &SetActivationIDOption};

    helpOptions[OPTION_ACTIVATIONPID] = {
        "P",  "productid", "(Office activation only) product ID to generate confirmation ID for",
        true, "",          &SetProductIDOption};

    helpOptions[OPTION_OEM] = {"o", "oem", "\t(PIDGEN) generate an OEM key", false, "", &SetOEMOption};

    helpOptions[OPTION_UPGRADE] = {"u",   "upgrade", "(PIDGEN 3 only) generate an upgrade key",
                                   false, "",        &SetUpgradeOption};

    helpOptions[OPTION_BINK] = {"b",  "binkid", "(advanced) override which BINK identifier to load",
                                true, "",       &SetBINKOption};

    helpOptions[OPTION_CHANNELID] = {"c",  "channelid", "(advanced) override which product channel to use",
                                     true, "",          &SetChannelIDOption};

    helpOptions[OPTION_SERIAL] = {
        "s",  "serial", "(advanced, PIDGEN 2/3 [BINK 1998] only) specify a serial to generate",
        true, "",       &SetSerialOption};

    helpOptions[OPTION_AUTHDATA] = {
        "a",  "authdata", "(advanced, PIDGEN 3 [BINK 2000] only) specify a value for the authentication data field",
        true, "",         nullptr};

    helpOptions[OPTION_VALIDATE] = {
        "V",  "validate", "validate a specified product ID against known BINKs and algorithms",
        true, "",         &SetValidateOption};
}

/**
 *
 * @return success
 */
BOOL CLI::parseCommandLine()
{
    for (DWORD i = 1; i < options.argc; i++)
    {
        std::string arg = options.argv[i];

        if (arg[0] == '-')
        {
            arg.erase(0, 1);
        }
        if (arg[0] == '-')
        {
            arg.erase(0, 1);
        }

        if (arg.empty())
        {
            continue;
        }

        for (BYTE j = 0; j < CLIHelpOptionID_END; j++)
        {
            if (arg != helpOptions[j].Short && arg != helpOptions[j].Long)
            {
                continue;
            }

            std::string nextarg;
            if (helpOptions[j].hasArguments)
            {
                if (i == options.argc - 1)
                {
                    options.error = true;
                    goto CommandLineParseEnd;
                }
                else
                {
                    nextarg = arg[i + 1];
                }
            }

            auto success = helpOptions[j].handler(1, &nextarg[0]);

            if (!success)
            {
                options.error = true;
                goto CommandLineParseEnd;
            }
            if (options.help)
            {
                goto CommandLineParseEnd;
            }
            goto ParseNextCommandLineOption;
        }

        fmt::print("unknown option: {}\n", arg);
        options.error = true;
        goto CommandLineParseEnd;

    ParseNextCommandLineOption:
        continue;
    }

CommandLineParseEnd:
    if (options.error)
    {
        DisplayErrorMessage(0, nullptr);
    }
    return !options.error;
}

/**
 *
 * @return success
 */
BOOL CLI::DisplayHelp(int, char *)
{
    options.help = true;
    fmt::print("usage: {} \n", options.argv[0]);

    for (BYTE i = 0; i < CLIHelpOptionID_END; i++)
    {
        CLIHelpOptions o = helpOptions[i];
        fmt::print("    -{}    --{}\t{}", o.Short, o.Long, o.HelpText);
        if (!o.Default.empty())
        {
            fmt::print(" (defaults to {})", o.Default);
        }
        fmt::print("\n");
    }

    fmt::print("\n");

    return true;
}

BOOL CLI::DisplayErrorMessage(int, char *)
{
    fmt::print("error parsing command line options\n");
    DisplayHelp(0, nullptr);
    options.error = true;
    return false;
}

BOOL CLI::SetVerboseOption(int, char *)
{
    fmt::print("enabling verbose option\n");
    options.verbose = true;
    UMSKT::VERBOSE = true;
    UMSKT::setDebugOutput(stderr);
    return true;
}

BOOL CLI::SetDebugOption(int, char *)
{
    fmt::print("enabling debug option\n");
    options.verbose = true;
    UMSKT::DEBUG = true;
    UMSKT::setDebugOutput(stderr);
    return true;
}

BOOL CLI::SetListOption(int, char *)
{
    options.list = true;
    return true;
}

BOOL CLI::SetOEMOption(int, char *)
{
    options.oem = true;
    return true;
}

BOOL CLI::SetUpgradeOption(int, char *)
{
    options.upgrade = true;
    return true;
}

BOOL CLI::SetFileOption(int count, char *file)
{
    options.keysFilename = file;
    return true;
}

BOOL CLI::SetNumberOption(int count, char *num)
{
    int nKeys;
    if (!sscanf(num, "%d", &nKeys))
    {
        return false;
    }

    options.numKeys = nKeys;
    return true;
}

/**
 *
 * @param count
 * @param channum
 * @return
 */
BOOL CLI::SetChannelIDOption(int count, char *channum)
{
    int siteID;
    if (!sscanf(channum, "%d", &siteID))
    {
        return false;
    }

    // channel ids must be between 000 and 999
    if (siteID > 999)
    {
        fmt::print("ERROR: refusing to create a key with a Channel ID greater than 999\n");
        return false;
    }

    options.channelID = siteID;
    return true;
}

BOOL CLI::SetBINKOption(int count, char *bink)
{
    options.binkid = bink;
    return true;
}

/**
 *
 * @param count
 * @param arg
 * @return
 */
BOOL CLI::SetSerialOption(int count, char *arg)
{
    int serial_val;
    if (!sscanf(arg, "%d", &serial_val))
    {
        return false;
    }

    // serials must be between 000000 and 999999
    if (serial_val > 999999)
    {
        fmt::print("ERROR: refusing to create a key with a Serial not between 000000 and 999999\n");
        return false;
    }

    options.serialSet = true;
    options.serial = serial_val;
    return true;
}

BOOL CLI::SetActivationIDOption(int count, char *aid)
{
    options.instid = aid;
    options.state = STATE_CONFIRMATION_ID;
    return true;
}

BOOL CLI::SetProductIDOption(int count, char *product)
{
    if (options.verbose)
    {
        fmt::print("Setting product ID to {}", product);
    }
    options.productid = product;
    return true;
}

BOOL CLI::SetValidateOption(int count, char *product)
{
    options.keyToCheck = product;
    return true;
}
