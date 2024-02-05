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

/**
 *
 * @param argcIn
 * @param argvIn
 * @return exit status (where success is 0)
 */
BYTE CLI::Init(int argcIn, char **argvIn)
{
    // set default options
    options = {argcIn, argvIn, "2E",  "",    "",    "",    "",    "",    "WINXP", "PROVLK", 0,
               0,      1,      false, false, false, false, false, false, false,   PIDGEN_3, STATE_PIDGEN_GENERATE};

    SetHelpText();

    BOOL success = parseCommandLine();
    if (!success)
    {
        return options.error;
    }

    // if we displayed help, without an error
    // return success
    if (options.help)
    {
        return -1;
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
    if (filename.empty())
    {
        if (options.verbose)
        {
            fmt::print("Loading internal keys file\n");
        }

        auto retval = loadEmbeddedJSON();

        if (retval && options.verbose)
        {
            fmt::print("Loaded internal keys file successfully\n\n");
        }
        else if (!retval)
        {
            fmt::print("Error loading internal keys file...\n\n");
        }
        return retval;
    }

    if (!fs::exists(filename))
    {
        fmt::print("ERROR: File {} does not exist\n", filename.string());
        return false;
    }

    if (options.verbose)
    {
        fmt::print("Loading keys file {}\n", options.keysFilename);
    }

    std::ifstream f(filename);
    try
    {
        keys = json::parse(f, nullptr, false, false);
    }
    catch (const json::exception &e)
    {
        fmt::print("ERROR: Exception thrown while parsing {}: {}\n", filename.string(), e.what());
        return false;
    }

    if (keys.is_discarded())
    {
        fmt::print("ERROR: Unable to parse keys from {}\n", filename.string());
        return false;
    }

    if (options.verbose)
    {
        fmt::print("Loaded keys from {} successfully\n", options.keysFilename);
    }

    return true;
}

/**
 *
 * @param pid
 */
void CLI::printID(DWORD32 *pid)
{
    char raw[12], b[6], c[8];
    char i, digit = 0;

    // Convert PID to ascii-number (=raw)
    snprintf(raw, sizeof(raw), "%09u", pid[0]);

    // Make b-part {640-....}
    _strncpy(b, 6, &raw[0], 3);
    b[3] = 0;

    // Make c-part {...-123456X...}
    _strcpy(c, &raw[3]);

    // Make checksum digit-part {...56X-}
    assert(strlen(c) == 6);
    for (i = 0; i < 6; i++)
    {
        digit += c[i] - '0'; // Sum digits
    }

    digit %= 7;
    if (digit > 0)
    {
        digit = 7 - digit;
    }

    c[6] = digit + '0';
    c[7] = 0;

    DWORD32 binkid;
    _sscanf(&options.binkID[0], "%x", &binkid);
    binkid /= 2;

    fmt::print("> Product ID: PPPPP-{}-{}-{}xxx\n", b, c, binkid);
}

/**
 *
 * @param pidgen3
 * @return success
 */
BOOL CLI::InitPIDGEN3(PIDGEN3 *p3)
{
    const char *BINKID = &options.binkID[0];
    auto bink = keys["BINK"][BINKID];

    if (options.verbose)
    {
        fmt::print("{:->80}\n", "");
        fmt::print("Loaded the following elliptic curve parameters: BINK[{}]\n", BINKID);
        fmt::print("{:->80}\n", "");
        fmt::print("{:>6}: {}\n", "P", bink["p"]);
        fmt::print("{:>6}: {}\n", "a", bink["a"]);
        fmt::print("{:>6}: {}\n", "b", bink["b"]);
        fmt::print("{:>6}: [{},\n{:>9}{}]\n", "G[x,y]", bink["g"]["x"], "", bink["g"]["y"]);
        fmt::print("{:>6}: [{},\n{:>9}{}]\n", "K[x,y]", bink["pub"]["x"], "", bink["pub"]["y"]);
        fmt::print("{:>6}: {}\n", "n", bink["n"]);
        fmt::print("{:>6}: {}\n", "k", bink["priv"]);
        fmt::print("\n");
    }

    p3->LoadEllipticCurve(bink["p"], bink["a"], bink["b"], bink["g"]["x"], bink["g"]["y"], bink["pub"]["x"],
                          bink["pub"]["y"], bink["n"], bink["priv"]);

    if (options.state != STATE_PIDGEN_GENERATE)
    {
        return true;
    }

    p3->info.setChannelID(options.channelID);
    if (options.verbose)
    {
        fmt::print("> Channel ID: {:#03d}\n", options.channelID);
    }

    if (options.serialSet)
    {
        p3->info.setSerial(options.serial);
        if (options.verbose)
        {
            fmt::print("> Serial {:#06d}\n", options.serial);
        }
    }

    return true;
}

/**
 *
 * @param confid
 * @return success
 */
BOOL CLI::InitConfirmationID(ConfirmationID &confid)
{
    if (!keys["products"][options.productCode].contains("meta") ||
        !keys["products"][options.productCode]["meta"].contains("activation"))
    {
        fmt::print("ERROR: product flavour {} does not have known activation values", options.productCode);
        return false;
    }

    auto meta = keys["products"][options.productCode]["meta"]["activation"];

    if (!keys["activation"].contains(meta["flavour"]))
    {
        fmt::print("ERROR: {} is an unknown activation flavour", meta["flavour"]);
        return false;
    }

    auto flavour = keys["activation"][meta["flavour"]];

    if (options.verbose)
    {
        fmt::print("{:->80}\n", "");
        fmt::print("Loaded the following hyperelliptic curve parameters: activation[{}]\n", meta["flavour"]);
        fmt::print("{:->80}\n", "");
        fmt::print("{:>7}: {}\n", "name", flavour["name"]);
        fmt::print("{:>7}: {}\n", "version", meta["version"]);
        fmt::print("{:>7}: {}\n", "Fp", flavour["p"]);
        fmt::print("{:>7}: [{}, {}, {},\n{:>10}{}, {}, {}]\n", "F[]", flavour["x"]["0"], flavour["x"]["1"],
                   flavour["x"]["2"], "", flavour["x"]["3"], flavour["x"]["4"], flavour["x"]["5"]);
        fmt::print("{:>7}: {}\n", "INV", flavour["quotient"]);
        fmt::print("{:>7}: {}\n", "mqnr", flavour["non_residue"]);
        fmt::print("{:>7}: {}\n", "k", flavour["priv"]);
        fmt::print("{:>7}: {}\n", "IID", flavour["iid_key"]);
        fmt::print("\n");
    }

    confid.LoadHyperellipticCurve(flavour["x"]["0"], flavour["x"]["1"], flavour["x"]["2"], flavour["x"]["3"],
                                  flavour["x"]["4"], flavour["x"]["5"], flavour["priv"], flavour["quotient"],
                                  flavour["non_residue"], flavour["iid_key"], meta["tags"].contains("xpbrand"),
                                  meta["tags"].contains("office"), meta["activation"]["version"]);
    return false;
}

/**
 *
 * @return success
 */
BOOL CLI::PIDGenerate()
{
    // TODO:
    // if options.pidgen2generate
    // return pidgen2generate
    // otherwise...

    const char *BINKID = &options.binkID[0];
    auto bink = keys["BINK"][BINKID];

    std::string key;
    bink["p"].get_to(key);

    auto p3 = PIDGEN3::Factory(key);
    InitPIDGEN3(p3);

    auto retval = PIDGEN3Generate(p3);

    delete p3;
    return retval;
}

/**
 *
 * @return isValid
 */
BOOL CLI::PIDValidate()
{
    // TODO:
    // if options.pidgen2validate
    // return pidgen2validate
    // otherwise...

    const char *BINKID = &options.binkID[0];
    auto bink = keys["BINK"][BINKID];

    std::string key;
    bink["p"].get_to(key);

    auto p3 = PIDGEN3::Factory(key);
    InitPIDGEN3(p3);
    auto retval = PIDGEN3Validate(p3);

    delete p3;
    return retval;
}

/**
 * Process application state
 *
 * @return application status code
 */
int CLI::Run()
{
    /**
     * TODO: we should be checking if the system's pseudorandom facilities work
     * before attempting generation, validation does not require entropy
     */
    switch (options.state)
    {
    case STATE_PIDGEN_GENERATE:
        return PIDGenerate();

    case STATE_PIDGEN_VALIDATE:
        return PIDValidate();

    case STATE_CONFIRMATION_ID:
        return ConfirmationIDGenerate();

    default:
        return 1;
    }
}

/**
 * Prints a product key to stdout
 *
 * @param pk std::string to print
 */
void CLI::printKey(std::string &pk)
{
    assert(pk.length() >= PK_LENGTH);

    fmt::print("{}-{}-{}-{}-{}", pk.substr(0, 5), pk.substr(5, 5), pk.substr(10, 5), pk.substr(15, 5),
               pk.substr(20, 5));
}

/**
 * std::BinaryOperation compatible accumulator for validating/stripping an input string against the PIDGEN3 charset
 * this can be moved to the PIDGEN3 at a later date
 *
 * @param accumulator
 * @param currentChar
 * @return
 */
std::string CLI::validateInputKeyCharset(std::string &accumulator, char currentChar)
{
    char cchar = ::toupper(currentChar);
    if (std::find(std::begin(PIDGEN3::pKeyCharset), std::end(PIDGEN3::pKeyCharset), cchar) !=
        std::end(PIDGEN3::pKeyCharset))
    {
        accumulator.push_back(cchar);
    }
    return accumulator;
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
    out_key = std::accumulate(in_key.begin(), in_key.end(), std::string(), validateInputKeyCharset);

    // only return true if we've handled exactly PK_LENGTH chars
    return (out_key.length() == PK_LENGTH);
}
