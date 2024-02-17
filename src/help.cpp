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
    helpOptions[OPTION_HELP2] = {"?", "", "show this help text", false, "", &DisplayHelp};

    helpOptions[OPTION_VERSION] = {"", "version", "show UMSKT CLI / libumskt versions", false, "", nullptr};

    helpOptions[OPTION_VERBOSE] = {"v", "verbose", "enable verbose output", false, "", &SetVerboseOption};

    helpOptions[OPTION_DEBUG] = {"d", "debug", "enable debug output", false, "", &SetDebugOption};

    helpOptions[OPTION_FILE] = {
        "F", "file", "(advanced) specify which keys JSON file to load", true, "[embedded file]", &SetFileOption};

    helpOptions[OPTION_LIST] = {"l", "list", "list supported products", false, "", &SetListOption};

    helpOptions[OPTION_PRODUCT] = {"p",  "product",           "[REQUIRED] which product to generate keys for",
                                   true, options.productCode, &SetProductCodeOption};

    helpOptions[OPTION_FLAVOUR] = {
        "f",  "flavour", "which product flavour to generate keys for (required in some instances)",
        true, "",        &SetFlavourOption};

    helpOptions[OPTION_NUMBER] = {"n",  "number", "(PIDGEN only) number of keys to generate",
                                  true, "1",      &SetNumberOption};

    helpOptions[OPTION_ACTIVATIONID] = {
        "i", "installationID",      "(activation only) installation ID used to generate confirmation ID", true,
        "",  &SetActivationIDOption};

    helpOptions[OPTION_ACTIVATIONPID] = {
        "P",  "productID", "(Office activation only) product ID to generate confirmation ID for",
        true, "",          &SetProductIDOption};

    helpOptions[OPTION_OEM] = {"O", "oem", "(PIDGEN) generate an OEM key", false, "", &SetOEMOption};

    helpOptions[OPTION_UPGRADE] = {"U",   "upgrade", "(PIDGEN 3 only) generate an upgrade key",
                                   false, "",        &SetUpgradeOption};

    helpOptions[OPTION_BINK] = {"b",  "binkID", "(advanced) override which BINK identifier to load",
                                true, "",       &SetBINKOption};

    helpOptions[OPTION_CHANNELID] = {"c",  "channelid", "(advanced) override which product channel to use",
                                     true, "",          &SetChannelIDOption};

    helpOptions[OPTION_SERIAL] = {
        "s",  "serial", "(advanced, PIDGEN 2/3 [BINK 1998] only) specify a serial to generate",
        true, "",       &SetSerialOption};

    helpOptions[OPTION_AUTHDATA] = {
        "a",  "authdata", "(advanced, PIDGEN 3 [BINK 2002] only) specify a value for the authentication data field",
        true, "",         &SetAuthDataOption};

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
    for (DWORD32 i = 1; i < options.argc; i++)
    {
        std::string arg = options.argv[i];

        if (arg[0] == '-' || arg[0] == '/')
        {
            arg.erase(0, 1 + (arg[1] == '-' ? 1 : 0));
        }

        if (arg.empty())
        {
            continue;
        }

        for (BYTE j = 0; j < CLIHelpOptionID_END; j++)
        {
            auto thisOption = helpOptions[j];
            if (arg != thisOption.Short && arg != thisOption.Long)
            {
                continue;
            }

            std::string nextarg;
            if (thisOption.hasArguments)
            {
                if (i == options.argc - 1)
                {
                    options.error = true;
                    goto CommandLineParseEnd;
                }
                else
                {
                    i++;
                    nextarg = std::string(options.argv[i]);
                }
            }

            if (thisOption.handler == nullptr)
            {
                // prevent accidental segmentation faults
                continue;
            }

            auto success = thisOption.handler(nextarg);

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
    if (options.verbose)
    {
        fmt::print("\n");
    }
    if (options.error)
    {
        DisplayErrorMessage("");
    }
    return !options.error;
}

/**
 *
 * @return success
 */
BOOL CLI::processOptions()
{
    if (!loadJSON(options.keysFilename))
    {
        options.error = true;
        return false;
    }

    if (options.list)
    {
        return processListCommand();
    }

    if (options.productCode.empty())
    {
        fmt::print("ERROR: product code is required. Exiting...\n");
        DisplayHelp("");
        return false;
    }

    if (!keys["products"].contains(options.productCode))
    {
        fmt::print("ERROR: Product \"{}\" is unknown\n", options.productCode);
        return false;
    }

    auto product = keys["products"][options.productCode];
    if (options.verbose)
    {
        fmt::print("Selecting product: {}\n", options.productCode);
    }

    json flavour;
    if (product.contains("flavours"))
    {
        // no default flavour, no flavour specified
        if (!product["meta"].contains("default") && options.productFlavour.empty())
        {
            fmt::print("ERROR: Product \"{}\n does not have a default flavour. Please specify a flavour.",
                       options.productCode);
            return false;
        }
        // yes flavour specified, but not found
        else if (!product["flavours"].contains(options.productFlavour) && !options.productFlavour.empty())
        {
            fmt::print("ERROR: Product \"{}\" does not have a flavour named \"{}\"\n", options.productCode,
                       options.productFlavour);
            return false;
        }
        // yes default flavour, no flavour specified
        else if (product["meta"].contains("default") && options.productFlavour.empty())
        {
            flavour = product["flavours"][product["meta"]["default"]];
            if (options.verbose)
            {
                fmt::print("Selecting default flavour: {}\n", product["meta"]["default"]);
            }
        }
        // yes flavour specified, and is found
        else
        {
            flavour = product["flavours"][options.productFlavour];
            if (options.verbose)
            {
                fmt::print("Selecting flavour: {}\n", options.productFlavour);
            }
        }
    }
    else
    {
        // no variants, just go with what we have
        flavour = product;
    }

    if (options.state != Options::STATE_PIDGEN_GENERATE && options.state != Options::STATE_PIDGEN_VALIDATE)
    {
        // exit early if we're not doing PIDGEN
        goto processOptionsExitEarly;
    }

    if (flavour["meta"]["type"] == "PIDGEN3")
    {
        options.pidgenversion = Options::PIDGEN_VERSION::PIDGEN_3;
        if (options.verbose)
        {
            fmt::print("Setting PIDGEN type to \"PIDGEN3\"\n");
        }

        if (options.oem)
        {
            flavour["BINK"][1].get_to(options.binkID);
        }
        else
        {
            flavour["BINK"][0].get_to(options.binkID);
        }

        if (options.verbose)
        {
            fmt::print("Selected BINK: {}\n", options.binkID);
        }
    }
    else if (flavour["meta"]["type"] == "PIDGEN2")
    {
        options.pidgenversion = Options::PIDGEN_VERSION::PIDGEN_2;
        if (options.verbose)
        {
            fmt::print("Setting PIDGEN type to \"PIDGEN2\"\n");
        }
    }

    if (options.state != Options::STATE_PIDGEN_GENERATE)
    {
        // exit early if we're only validating
        goto processOptionsExitEarly;
    }

    if (flavour.contains("DPC") && flavour["DPC"].contains(options.binkID) && options.channelID == 0)
    {
        std::vector<json> filtered;
        for (auto const &i : flavour["DPC"][options.binkID].items())
        {
            auto el = i.value();
            if (!el["isEvaluation"].get<bool>())
            {
                filtered.emplace_back(el);
            }
        }

        // roll a die to choose which DPC entry to pick
        auto rand = UMSKT::getRandom<BYTE>();
        auto dpc = filtered[rand % filtered.size()];
        auto min = dpc["min"].get<WORD>(), max = dpc["max"].get<WORD>();

        if (min == max)
        {
            options.channelID = min;
        }
        else
        {
            options.channelID = min + (rand % (max - min));
        }

        if (options.verbose)
        {
            fmt::print("Selected channel ID: {} (DPC entry {}/{})\n", options.channelID, (rand % filtered.size()) + 1,
                       filtered.size());
        }
    }

    if (options.channelID.IsZero())
    {
        options.channelID.Randomize(UMSKT::rng, sizeof(DWORD32) * 8);
        options.channelID %= PIDGEN::MaxChannelID;
        if (options.verbose)
        {
            fmt::print("Generated channel ID: {}\n", options.channelID);
        }
    }

    // FE and FF are BINK 1998, but use a different, currently unhandled encoding scheme, we return an error here
    if (options.binkID == "FE" || options.binkID == "FF")
    {
        fmt::print("ERROR: Terminal Services BINKs (FE and FF) are unsupported at this time\n");
        return false;
    }

processOptionsExitEarly:
    if (options.verbose)
    {
        fmt::print("\n");
    }

    return true;
}

/**
 * Displays the contents of the input JSON file in an
 * intuitive and attractive pattern
 *
 * @return false
 */
BOOL CLI::processListCommand()
{
    // the following code is absolutely unhinged
    // I'm so sorry

#if defined(__UNICODE__) || defined(__GNUC__)
    auto *leaf = "\u251C", *last = "\u2514", *line = "\u2500";
#else
    auto *leaf = "\xC3", *last = "\xC0", *line = "\xC4";
#endif

    fmt::print("Listing known products and flavours: \n\n");

    fmt::print("* The following product list uses this style of formatting:\n");
    fmt::print("{}: {} \n", fmt::styled("PRODUCT", fmt::emphasis::bold), "Product name");
    fmt::print("{}{}{} {}: {} \n", last, line, line, "FLAVOUR", "Flavour name");
    fmt::print("* Products that require a flavour are noted with {}\n\n",
               fmt::styled("(no default)", fmt::emphasis::bold));

    for (auto const &i : keys["products"].items())
    {
        auto el = i.value();
        auto containsFlavours = el.contains("flavours");

        fmt::print("{:<9} {} ", fmt::styled(fmt::format("{}:", i.key()), fmt::emphasis::bold), el["name"]);
        if (el.contains("BINK"))
        {
            fmt::print("{}\n", el["BINK"]);
        }
        else if (el["meta"].contains("default"))
        {
            fmt::print("(default: {} {})\n", fmt::styled(el["meta"]["default"], fmt::emphasis::bold),
                       el["flavours"][el["meta"]["default"]]["BINK"]);
        }
        else if (el["meta"]["type"] == "PIDGEN3")
        {
            fmt::print("[{}]\n", el["meta"]["type"]);
        }
        else
        {
            fmt::print("{}\n", fmt::styled("(no default)", fmt::emphasis::bold));
        }

        if (containsFlavours)
        {
            auto flavours = el["flavours"];
            for (auto j = flavours.begin(); j != flavours.end(); j++)
            {
                auto el2 = j.value();
                BOOL isLast = j == --flavours.end();

                fmt::print("{}{}{} {:<9} {} ", !isLast ? leaf : last, line, line, fmt::format("{}:", j.key()),
                           fmt::format("{} {}", el["name"], el2["name"]));

                if (el2.contains("meta") && el2["meta"].contains("type"))
                {
                    fmt::print("[{}]\n", el2["meta"]["type"]);
                }
                else
                {
                    fmt::print("{}\n", el2["BINK"]);
                }
            }
        }

        fmt::print("\n");
    }

    return false;
}

/**
 *
 * @return success
 */
BOOL CLI::DisplayHelp(const std::string &)
{
    options.help = true;
    fmt::print("usage: {} \n", options.argv[0]);

    for (BYTE i = 0; i < CLIHelpOptionID_END; i++)
    {
        CLIHelpOptions o = helpOptions[i];

        if (o.Short.empty())
        {
            fmt::print("\t{:>2} --{:<15} {}", "", o.Long, o.HelpText);
        }
        else if (o.Long.empty())
        {
            fmt::print("\t-{}   {:<15} {}", o.Short, "", o.HelpText);
        }
        else
        {
            fmt::print("\t-{} --{:<15} {}", o.Short, o.Long, o.HelpText);
        }

        if (!o.Default.empty())
        {
            fmt::print(" (defaults to {})", o.Default);
        }
        fmt::print("\n");
    }

    fmt::print("\n");

    return true;
}

BOOL CLI::DisplayErrorMessage(const std::string &)
{
    fmt::print("Error parsing command line options\n");
    DisplayHelp("");
    options.error = true;
    return false;
}

BOOL CLI::SetVerboseOption(const std::string &)
{
    options.verbose = true;
    UMSKT::setVerboseOutput(stdout);
    fmt::print(UMSKT::verbose, "Enabling verbose option\n");
    return true;
}

BOOL CLI::SetDebugOption(const std::string &)
{
    options.verbose = true;
    UMSKT::setDebugOutput(stdout);
    fmt::print(UMSKT::debug, "Enabling debug option\n");
    return true;
}

BOOL CLI::SetListOption(const std::string &)
{
    if (options.verbose)
    {
        fmt::print("Setting list option\n");
    }
    options.list = true;
    return true;
}

BOOL CLI::SetOEMOption(const std::string &)
{
    if (options.verbose)
    {
        fmt::print("Setting OEM option\n");
    }
    options.oem = true;
    return true;
}

BOOL CLI::SetUpgradeOption(const std::string &)
{
    if (options.verbose)
    {
        fmt::print("Setting upgrade option\n");
    }
    options.upgrade = true;
    return true;
}

BOOL CLI::SetFileOption(const std::string &file)
{
    if (options.verbose)
    {
        fmt::print("Setting file option to: {}\n", file);
    }
    options.keysFilename = file;
    return true;
}

BOOL CLI::SetNumberOption(const std::string &num)
{
    auto nKeys = UMSKT::IntegerS(num);

    if (options.verbose)
    {
        fmt::print("Setting generation number option to: {}\n", num);
    }

    options.numKeys = nKeys.ConvertToLong();
    return true;
}

BOOL CLI::SetChannelIDOption(const std::string &channum)
{
    Integer channelID = UMSKT::IntegerS(channum);

    // channel ids must be between 000 and 999
    if (channelID > PIDGEN::MaxChannelID)
    {
        fmt::print("ERROR: refusing to create a key with a Channel ID greater than 999\n");
        return false;
    }

    if (options.verbose)
    {
        fmt::print("Setting Channel ID option to: {}\n", channelID);
    }

    options.channelID = channelID;
    return true;
}

BOOL CLI::SetBINKOption(const std::string &bink)
{
    auto strbinkid = std::string(bink);
    options.binkID = UMSKT::strtoupper(strbinkid);

    if (options.verbose)
    {
        fmt::print("Setting BINK option to: {}\n", strbinkid);
    }
    return true;
}

BOOL CLI::SetFlavourOption(const std::string &flavour)
{
    auto strFlavour = UMSKT::strtoupper(flavour);
    options.productFlavour = strFlavour;

    if (options.verbose)
    {
        fmt::print("Setting flavour option to: {}\n", strFlavour);
    }
    return true;
}

BOOL CLI::SetSerialOption(const std::string &arg)
{
    Integer Serial = UMSKT::IntegerS(arg);

    // serials must be between 000000 and 999999
    if (Serial > PIDGEN::MaxSerial)
    {
        fmt::print("ERROR: refusing to create a key with a Serial not between 000000 and 999999\n");
        return false;
    }

    options.serial = Serial;

    if (options.verbose)
    {
        fmt::print("Setting serial number option to: {}\n", Serial);
    }
    return true;
}

BOOL CLI::SetActivationIDOption(const std::string &aid)
{
    options.installationID = aid;
    options.state = Options::STATE_CONFIRMATION_ID;

    if (options.verbose)
    {
        fmt::print("Setting program state to Confirmation ID Generation\n");
    }

    return true;
}

BOOL CLI::SetProductIDOption(const std::string &product)
{
    options.productID = product;

    if (options.verbose)
    {
        fmt::print("Setting product ID option to: {}\n", product);
    }

    return true;
}

BOOL CLI::SetValidateOption(const std::string &productID)
{
    options.keyToCheck = productID;
    options.state = Options::STATE_PIDGEN_VALIDATE;

    if (options.verbose)
    {
        fmt::print("Setting program state to PIDGEN Validation\n");
    }

    return true;
}

BOOL CLI::SetProductCodeOption(const std::string &product)
{
    auto strProduct = std::string(product);
    options.productCode = UMSKT::strtoupper(strProduct);
    options.productFlavour = "";

    if (options.verbose)
    {
        fmt::print("Setting product code to: {}\n", strProduct);
    }
    return true;
}

BOOL CLI::SetAuthDataOption(const std::string &authData)
{
    auto strAuthData = std::string(authData);
    options.authInfo = strAuthData;

    if (options.verbose)
    {
        fmt::print("Setting authdata option to: {}\n", strAuthData);
    }

    return true;
}
