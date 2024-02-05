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
 * @FileCreated by Neo on 01/05/2024
 * @Maintainer Neo
 */

#include "cli.h"

/**
 *
 * @param pidgen2
 * @return success
 */
BOOL CLI::PIDGEN2Generate(PIDGEN2 &pidgen2)
{
    return true;
}

/**
 *
 * @param pidgen2
 * @return success
 */
BOOL CLI::PIDGEN2Validate(PIDGEN2 &pidgen2)
{
    return true;
}

/**
 *
 * @return success
 */
BOOL CLI::PIDGEN3Generate(PIDGEN3 *p3)
{
    // raw PID/serial value
    DWORD32 nRaw = options.channelID * 1'000'000;
    DWORD32 serialRnd;

    if (p3->checkFieldIsBink1998())
    {
        if (options.serialSet)
        {
            // using user-provided serial
            serialRnd = options.serial;
        }
        else
        {
            // generate a random number to use as a serial
            serialRnd = UMSKT::getRandom<DWORD32>();
        }

        // make sure it's less than 999999
        nRaw += (serialRnd % 999999);

        if (options.verbose)
        {
            // print the resulting Product ID
            // PID value is printed in BINK1998::Generate
            printID(&nRaw);
        }
    }

    for (DWORD32 i = 0; i < total; i++)
    {
        if (!p3->checkFieldIsBink1998())
        {
            auto authvalue = UMSKT::getRandom<DWORD32>() & BITMASK(10);
            p3->info.AuthInfo.Decode((BYTE *)&authvalue, sizeof(DWORD32));

            if (options.verbose)
            {
                fmt::print("> AuthInfo: {:#08x}\n", p3->info.AuthInfo);
            }
        }
        else
        {
            p3->info.setSerial(nRaw);
        }

        p3->Generate(pKey);

        bool isValid = p3->Validate(pKey);
        if (isValid)
        {
            printKey(pKey);
            if (i <= total - 1 || options.verbose)
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
                if (i <= total - 1)
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
BOOL CLI::PIDGEN3Validate(PIDGEN3 *p3)
{
    std::string product_key;

    if (!CLI::stripKey(options.keyToCheck, product_key))
    {
        fmt::print("ERROR: Product key is in an incorrect format!\n");
        return false;
    }

    CLI::printKey(product_key);
    fmt::print("\n");
    if (!p3->Validate(product_key))
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
    auto confid = ConfirmationID();
    std::string confirmation_id;

    if (!InitConfirmationID(confid))
    {
        return false;
    }

    DWORD32 err = confid.Generate(options.installationID, confirmation_id, options.productID);

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
