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
 * @FileCreated by Andrew on 01/06/2023
 * @Maintainer Neo
 */

#include "cli.h"

// define static storage
Options CLI::options;
json CLI::keys;

BYTE CLI::Init(int argcIn, char **argvIn)
{
    // set default options
    options = {argcIn, argvIn, "",    "",    "",    "",    "",    "",    "WINXPPVLK", 0,
               0,      1,      false, false, false, false, false, false, false,       STATE_BINK1998_GENERATE};

    SetHelpText();

    BOOL success = parseCommandLine();
    if (!success)
    {
        return options.error;
    }

    success = processOptions();
    if (!success)
    {
        return 2;
    }

    return 0;
}

/**
 *
 * @param filename
 * @return success
 */
BOOL CLI::loadJSON(const fs::path &filename)
{
    if (!filename.empty() && !fs::exists(filename))
    {
        fmt::print("ERROR: File {} does not exist\n", filename.string());
        return false;
    }
    else if (fs::exists(filename))
    {
        std::ifstream f(filename);
        try
        {
            keys = json::parse(f, nullptr, false, false);
        }
        catch (const json::exception &e)
        {
            fmt::print("ERROR: Exception thrown while parsing {}: {}\n", filename.string(), e.what());
        }
    }
    else if (filename.empty())
    {
        cmrc::embedded_filesystem fs = cmrc::umskt::get_filesystem();
        cmrc::file jsonFile = fs.open("keys.json");
        keys = json::parse(jsonFile, nullptr, false, false);
    }

    if (keys.is_discarded())
    {
        fmt::print("ERROR: Unable to parse keys from {}\n", filename.string());
        return false;
    }

    return true;
}

/**
 *
 * @return success
 */
BOOL CLI::processOptions()
{
    if (options.verbose)
    {
        if (options.keysFilename.empty())
        {
            fmt::print("Loading internal keys file\n");
        }
        else
        {
            fmt::print("Loading keys file {}\n", options.keysFilename);
        }
    }

    if (!loadJSON(options.keysFilename))
    {
        options.error = true;
        return false;
    }

    if (options.verbose)
    {
        if (options.keysFilename.empty())
        {
            fmt::print("Loaded internal keys file successfully\n");
        }
        else
        {
            fmt::print("Loaded keys from {} successfully\n", options.keysFilename);
        }
    }

    if (options.list)
    {
        for (auto const &i : keys["Products"].items())
        {
            auto el = i.value();
            int id;
            sscanf(&(el["BINK"][0]).get<std::string>()[0], "%x", &id);
            fmt::print("{}\n\tName: {}\n\tBINKs: ", i.key(), el["Name"].get<std::string>());
            std::cout << el["BINK"] << std::endl << std::endl;
        }

        fmt::print("\n");
        return false;
    }

    if (!options.productCode.empty())
    {
        const char *productCode = &options.productCode[0];
        auto product = keys["Products"][productCode];

        if (options.verbose)
        {
            fmt::print("Selecting product: {}\n", productCode);
        }

        if (options.oem)
        {
            options.binkid = product["BINK"][1].get<std::string>();
        }
        else
        {
            options.binkid = product["BINK"][0].get<std::string>();
        }

        if (options.verbose)
        {
            fmt::print("Selected BINK: {}\n", options.binkid);
        }

        std::vector<json> filtered;

        if (product.contains("DPC") && options.channelID == 0)
        {
            for (auto const &i : product["DPC"][options.binkid].items())
            {
                auto el = i.value();
                if (!el["IsEvaluation"].get<bool>())
                {
                    filtered.push_back(el);
                }
            }

            // roll a die to choose which DPC entry to pick
            auto rand = UMSKT::getRandom<BYTE>();
            auto dpc = filtered[rand % filtered.size()];
            auto min = dpc["Min"].get<WORD>(), max = dpc["Max"].get<WORD>();
            options.channelID = min + (rand % (max - min));

            if (options.verbose)
            {
                fmt::print("Selected channel ID: {} (DPC entry {})\n", options.channelID, rand % filtered.size());
            }
        }

        if (options.channelID == 0)
        {
            options.channelID = UMSKT::getRandom<WORD>() % 999;
        }
    }

    DWORD intBinkID;
    sscanf(&options.binkid[0], "%x", &intBinkID);

    // FE and FF are BINK 1998, but do not generate valid keys, so we throw an error
    if (intBinkID >= 0xFE)
    {
        fmt::print("ERROR: Terminal Services BINKs (FE and FF) are unsupported at this time\n");
        return false;
    }

    if (intBinkID >= 0x40)
    {
        // set bink2002 validate mode if in bink1998 validate mode, else set bink2002 generate mode
        options.state = (options.state == STATE_BINK1998_VALIDATE) ? STATE_BINK2002_VALIDATE : STATE_BINK2002_GENERATE;
    }

    return true;
}

void CLI::printID(DWORD *pid)
{
    char raw[12], b[6], c[8];
    char i, digit = 0;

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
    {
        digit -= c[i] - '0'; // Sum digits
    }

    while (digit < 0)
    {
        digit += 7;
    }
    c[6] = digit + '0';
    c[7] = 0;

    fmt::print("> Product ID: PPPPP-{}-{}-BBxxx\n", b, c);
}

/**
 *
 * @param pk
 */
void CLI::printKey(std::string &pk)
{
    assert(pk.length() >= PK_LENGTH);

    fmt::print("{}-{}-{}-{}-{}", pk.substr(0, 5), pk.substr(5, 5), pk.substr(10, 5), pk.substr(15, 5),
               pk.substr(20, 5));
}

/**
 *
 * @param in_key
 * @param out_key
 * @return
 */
BOOL CLI::stripKey(const std::string &in_key, std::string &out_key)
{
    // copy out the product key stripping out extraneous characters
    const char *p = &in_key[0];
    BYTE i = 0;

    for (; *p; p++)
    {
        // strip out space or dash
        if (*p == ' ' || *p == '-')
        {
            continue;
        }
        // check if we've passed the product key length to avoid overflow
        if (i >= PK_LENGTH)
        {
            return false;
        }
        // convert to uppercase - if character allowed, copy into array
        for (int j = 0; j < strlen(PIDGEN3::pKeyCharset); j++)
        {
            if (toupper(*p) == PIDGEN3::pKeyCharset[j])
            {
                out_key[i++] = toupper(*p);
                continue;
            }
        }
    }

    // only return true if we've handled exactly PK_LENGTH chars
    return (i == PK_LENGTH);
}

/**
 *
 * @param pidgen3
 * @return success
 */
BOOL CLI::InitPIDGEN3(PIDGEN3 *pidgen3)
{
    const char *BINKID = &options.binkid[0];
    auto bink = keys["BINK"][BINKID];

    if (options.verbose)
    {
        fmt::print("----------------------------------------------------------- \n");
        fmt::print("Loaded the following elliptic curve parameters: BINK[{}]\n", BINKID);
        fmt::print("----------------------------------------------------------- \n");
        fmt::print(" P: {}\n", bink["p"].get<std::string>());
        fmt::print(" a: {}\n", bink["a"].get<std::string>());
        fmt::print(" b: {}\n", bink["b"].get<std::string>());
        fmt::print("Gx: {}\n", bink["g"]["x"].get<std::string>());
        fmt::print("Gy: {}\n", bink["g"]["y"].get<std::string>());
        fmt::print("Kx: {}\n", bink["pub"]["x"].get<std::string>());
        fmt::print("Ky: {}\n", bink["pub"]["y"].get<std::string>());
        fmt::print(" n: {}\n", bink["n"].get<std::string>());
        fmt::print(" k: {}\n", bink["priv"].get<std::string>());
        fmt::print("\n");
    }

    pidgen3->LoadEllipticCurve(bink["p"], bink["a"], bink["b"], bink["g"]["x"], bink["g"]["y"], bink["pub"]["x"],
                               bink["pub"]["y"], bink["n"], bink["priv"]);

    options.info.setChannelID(options.channelID);
    if (options.verbose)
    {
        fmt::print("> Channel ID: {:03d}\n", options.channelID);
    }

    if (options.serialSet)
    {
        options.info.setSerial(options.serial);
        if (options.verbose)
        {
            fmt::print("> Serial {:#09d}\n", options.serial);
        }
    }

    return true;
}

/**
 *
 * @param confid
 * @return success
 */
BOOL CLI::InitConfirmationID(ConfirmationID *confid)
{
    return true;
}

/**
 *
 * @return success
 */
BOOL CLI::BINK1998Generate()
{
    auto bink1998 = BINK1998();
    BOOL retval = InitPIDGEN3(&bink1998);
    if (!retval)
    {
        return retval;
    }

    // raw PID/serial value
    DWORD nRaw = options.channelID * 1'000'000;
    DWORD serialRnd;

    if (options.serialSet)
    {
        // using user-provided serial
        serialRnd = options.serial;
    }
    else
    {
        // generate a random number to use as a serial
        serialRnd = UMSKT::getRandom<DWORD>();
    }

    // make sure it's less than 999999
    nRaw += (serialRnd % 999999);

    if (options.verbose)
    {
        // print the resulting Product ID
        // PID value is printed in BINK1998::Generate
        printID(&nRaw);
    }

    for (int i = 0; i < total; i++)
    {
        options.info.setSerial(nRaw);
        bink1998.Generate(options.info, pKey);

        bool isValid = bink1998.Verify(pKey);
        if (isValid)
        {
            printKey(pKey);
            if (i < total - 1 || options.verbose)
            {
                fmt::print("\n");
            }
            count += isValid;
        }
        else
        {
            if (options.verbose)
            {
                printKey(pKey);
                fmt::print(" [Invalid]");
                if (i < total - 1)
                {
                    fmt::print("\n");
                }
            }
            total++; // queue a redo, basically
        }
    }

    if (options.verbose)
    {
        fmt::print("\nSuccess count: {}/{}\n", count, total);
    }

    return true;
}

/**
 *
 * @return success
 */
BOOL CLI::BINK2002Generate()
{
    auto bink2002 = BINK2002();
    InitPIDGEN3(&bink2002);

    // generate a key
    for (int i = 0; i < total; i++)
    {
        DWORD pAuthInfo;
        RAND_bytes((BYTE *)&pAuthInfo, 4);
        pAuthInfo &= BITMASK(10);

        if (options.verbose)
        {
            fmt::print("> AuthInfo: {}\n", pAuthInfo);
        }

        bink2002.Generate(options.info, pKey);

        bool isValid = bink2002.Verify(pKey);
        if (isValid)
        {
            CLI::printKey(pKey);
            if (i < total - 1 || options.verbose)
            { // check if end of list or verbose
                fmt::print("\n");
            }
            count += isValid; // add to count
        }
        else
        {
            if (options.verbose)
            {
                CLI::printKey(pKey);      // print the key
                fmt::print(" [Invalid]"); // and add " [Invalid]" to the key
                if (i < this->total - 1)
                { // check if end of list
                    fmt::print("\n");
                }
            }
            total++; // queue a redo, basically
        }
    }

    if (options.verbose)
    {
        fmt::print("\nSuccess count: {}/{}\n", count, total);
    }

    return true;
}

/**
 *
 * @return success
 */
BOOL CLI::BINK1998Validate()
{
    auto bink1998 = BINK1998();
    InitPIDGEN3(&bink1998);

    std::string product_key;

    if (!CLI::stripKey(options.keyToCheck, product_key))
    {
        fmt::print("ERROR: Product key is in an incorrect format!\n");
        return false;
    }

    CLI::printKey(product_key);
    fmt::print("\n");
    if (!bink1998.Verify(product_key))
    {
        fmt::print("ERROR: Product key is invalid! Wrong BINK ID?\n");
        return false;
    }

    fmt::print("Key validated successfully!\n");
    return true;
}

/**
 *
 * @return success
 */
BOOL CLI::BINK2002Validate()
{
    auto bink2002 = BINK2002();
    InitPIDGEN3(&bink2002);

    std::string product_key;

    if (!CLI::stripKey(options.keyToCheck, product_key))
    {
        fmt::print("ERROR: Product key is in an incorrect format!\n");
        return false;
    }

    CLI::printKey(product_key);
    fmt::print("\n");
    if (!bink2002.Verify(product_key))
    {
        fmt::print("ERROR: Product key is invalid! Wrong BINK ID?\n");
        return false;
    }

    fmt::print("Key validated successfully!\n");
    return true;
}

/**
 *
 * @return success
 */
BOOL CLI::ConfirmationIDGenerate()
{
    std::string confirmation_id;

    auto confid = ConfirmationID();
    int err = confid.Generate(options.instid, confirmation_id, options.productid);

    if (err == SUCCESS)
    {
        fmt::print("{}\n", confirmation_id);
        return true;
    }

    switch (err)
    {
    case ERR_TOO_SHORT:
        fmt::print("ERROR: Installation ID is too short.\n");
        break;

    case ERR_TOO_LARGE:
        fmt::print("ERROR: Installation ID is too long.\n");
        break;

    case ERR_INVALID_CHARACTER:
        fmt::print("ERROR: Invalid character in installation ID.\n");
        break;

    case ERR_INVALID_CHECK_DIGIT:
        fmt::print("ERROR: Installation ID checksum failed. Please check that it is typed correctly.\n");
        break;

    case ERR_UNKNOWN_VERSION:
        fmt::print("ERROR: Unknown installation ID version.\n");
        break;

    case ERR_UNLUCKY:
        fmt::print("ERROR: Unable to generate valid confirmation ID.\n");
        break;

    default:
        fmt::print("Unknown error occurred during Confirmation ID generation: {}\n", err);
        break;
    }

    return false;
}
/**
 *
 * @return application status code
 */
int CLI::Run()
{
    BINK1998Generate();
    /*
    switch (state)
    {
    case STATE_BINK1998_GENERATE:
        return BINK1998Generate();

    case STATE_BINK2002_GENERATE:
        return BINK2002Generate();

    case STATE_BINK1998_VALIDATE:
        return BINK1998Validate();

    case STATE_BINK2002_VALIDATE:
        return BINK2002Validate();

    case STATE_CONFIRMATION_ID:
        return ConfirmationIDGenerate();

    default:
        return 1;
    }
     */
    return 0;
}
