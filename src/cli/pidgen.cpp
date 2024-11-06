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
 * @FileCreated by Neo on 02/18/2024
 * @Maintainer Neo
 */

#include "cli.h"

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

    options.channelID %= 1000;
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
    std::string product_key;

    if (!p2.ValidateKeyString(options.keyToCheck, product_key))
    {
        fmt::print("ERROR: Product key is in an incorrect format!\n");
        return false;
    }

    fmt::print("{}\n", p2.StringifyKey(product_key));

    if (!p2.Validate(product_key))
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
        serialRnd %= 1000000;
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

    if (!p3->ValidateKeyString(options.keyToCheck, product_key))
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
