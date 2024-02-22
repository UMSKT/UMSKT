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