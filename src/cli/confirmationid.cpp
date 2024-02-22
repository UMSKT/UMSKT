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