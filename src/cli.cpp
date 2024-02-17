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
CLI::Options CLI::options;
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
    options.argc = argcIn;
    options.argv = argvIn;
    options.binkID = "2E";
    options.productCode = "WINXP";
    options.productFlavour = "VLK";
    options.numKeys = 1;
    options.pidgenversion = Options::PIDGEN_VERSION::PIDGEN_3;
    options.state = Options::APPLICATION_STATE::STATE_PIDGEN_GENERATE;

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
        fmt::print("Loading keys file: {}\n", options.keysFilename);
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
        fmt::print("ERROR: Unable to parse keys from: {}\n", filename.string());
        return false;
    }

    if (options.verbose)
    {
        fmt::print("Loaded keys from \"{}\" successfully\n", options.keysFilename);
    }

    return true;
}

/**
 *
 * @param pidgen3
 * @return success
 */
BOOL CLI::InitPIDGEN3(PIDGEN3 *p3)
{
    auto bink = keys["BINK"][options.binkID];

    if (options.verbose)
    {
        fmt::print("{:->80}\n", "");
        fmt::print("Loaded the following elliptic curve parameters: BINK[{}]\n", options.binkID);
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

    p3->LoadEllipticCurve(options.binkID, bink["p"], bink["a"], bink["b"], bink["g"]["x"], bink["g"]["y"],
                          bink["pub"]["x"], bink["pub"]["y"], bink["n"], bink["priv"]);

    if (options.state != Options::APPLICATION_STATE::STATE_PIDGEN_GENERATE)
    {
        return true;
    }

    if (options.channelID.IsZero())
    {
        options.channelID.Randomize(UMSKT::rng, sizeof(DWORD32) * 8);
    }

    options.channelID %= 999;
    p3->info.ChannelID = options.channelID;
    if (options.verbose)
    {
        fmt::print("> Channel ID: {:d}\n", options.channelID);
    }

    if (options.serial.NotZero() && p3->checkFieldIsBink1998())
    {
        p3->info.Serial = options.serial;
        if (options.verbose)
        {
            fmt::print("> Serial {:d}\n", options.serial);
        }
    }
    else if (options.serial.NotZero() && !p3->checkFieldIsBink1998())
    {
        fmt::print("Warning: Discarding user-supplied serial for BINK2002\n");
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
        fmt::print("ERROR: product flavour \"{}\" does not have known activation values", options.productCode);
        return false;
    }

    auto meta = keys["products"][options.productCode]["meta"]["activation"];

    if (!keys["activation"].contains(meta["flavour"]))
    {
        fmt::print("ERROR: \"{}\" is an unknown activation flavour", meta["flavour"]);
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
    BOOL retval = false;

    if (options.pidgenversion == Options::PIDGEN_VERSION::PIDGEN_2)
    {
        auto p2 = PIDGEN2();
        retval = PIDGEN2Generate(p2);
        return retval;
    }
    else if (options.pidgenversion == Options::PIDGEN_VERSION::PIDGEN_3)
    {
        auto bink = keys["BINK"][options.binkID];

        auto p3 = PIDGEN3::Factory(bink["p"]);
        InitPIDGEN3(p3);
        retval = PIDGEN3Generate(p3);

        delete p3;
        return retval;
    }

    return retval;
}

/**
 *
 * @return isValid
 */
BOOL CLI::PIDValidate()
{
    BOOL retval = false;

    if (options.pidgenversion == Options::PIDGEN_VERSION::PIDGEN_2)
    {
        auto p2 = PIDGEN2();
        retval = PIDGEN2Validate(p2);
        return retval;
    }
    else if (options.pidgenversion == Options::PIDGEN_VERSION::PIDGEN_3)
    {
        auto bink = keys["BINK"][options.binkID];

        auto p3 = PIDGEN3::Factory(bink["p"]);
        InitPIDGEN3(p3);
        retval = PIDGEN3Validate(p3);

        delete p3;
        return retval;
    }

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
    case Options::APPLICATION_STATE::STATE_PIDGEN_GENERATE:
        return PIDGenerate();

    case Options::APPLICATION_STATE::STATE_PIDGEN_VALIDATE:
        return PIDValidate();

    case Options::APPLICATION_STATE::STATE_CONFIRMATION_ID:
        return ConfirmationIDGenerate();

    default:
        return 1;
    }
}