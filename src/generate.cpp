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
BOOL CLI::PIDGEN2Generate(PIDGEN2 &p2)
{
    p2.info.ChannelID = options.channelID;
    p2.info.Serial = options.serial;
    p2.info.isOEM = options.oem;

    std::string serial;
    p2.Generate(serial);

    serial = p2.StringifyKey(serial);

    fmt::print("{}", serial);

    auto retval = p2.Validate(serial);

    if (!retval)
    {
        fmt::print(" [INVALID]");
    }

    fmt::print("\n");

    return retval;
}

/**
 *
 * @param pidgen2
 * @return success
 */
BOOL CLI::PIDGEN2Validate(PIDGEN2 &p2)
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
    Integer serialRnd;

    if (p3->checkFieldIsBink1998())
    {
        if (options.serial.NotZero())
        {
            // using user-provided serial
            serialRnd = options.serial;
        }
        else
        {
            // generate a random number to use as a serial
            serialRnd.Randomize(UMSKT::rng, sizeof(DWORD32) * 8);
        }

        // make sure it's less than 999999
        serialRnd %= 999999;
    }

    p3->info.isOEM = options.oem;

    for (DWORD32 i = 0; i < total; i++)
    {
        if (!p3->checkFieldIsBink1998())
        {
            if (options.authInfo.empty())
            {
                p3->info.AuthInfo.Randomize(UMSKT::rng, 10);
            }
            else
            {
                p3->info.AuthInfo = CryptoPP::Crop(UMSKT::IntegerS(options.authInfo), 10);
            }

            if (options.verbose)
            {
                fmt::print("> AuthInfo: {:d}\n", p3->info.AuthInfo);
            }
        }
        else
        {
            p3->info.Serial = serialRnd;
        }

        if (options.verbose)
        {
            fmt::print("\n");
        }

        p3->Generate(pKey);

        if (options.verbose)
        {
            fmt::print("> Product ID: {}\n\n", p3->StringifyProductID());
        }

        bool isValid = p3->Validate(pKey);
        if (isValid)
        {
            fmt::print(p3->StringifyKey(pKey));
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
                fmt::print("{} [Invalid]", p3->StringifyKey(pKey));
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

    if (!PIDGEN3::ValidateKeyString(options.keyToCheck, product_key))
    {
        fmt::print("ERROR: Product key is in an incorrect format!\n");
        return false;
    }

    fmt::print("{}\n", p3->StringifyKey(product_key));

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
