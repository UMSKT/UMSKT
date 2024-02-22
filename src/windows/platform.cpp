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
 * @FileCreated by Neo on 01/07/2024
 * @Maintainer Neo
 */

#include "cli/cli.h"
#include "resource.h"

/**
 *
 * @return success
 */
BOOL CLI::loadEmbeddedJSON()
{
    HMODULE hModule = GetModuleHandle(nullptr);

    // Find
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_JSON1), "JSON");
    if (hResource == nullptr)
    {
        fmt::print("ERROR: Could not find internal JSON resource?");
        return false;
    }

    // Load
    HGLOBAL hResourceData = LoadResource(hModule, hResource);
    if (hResourceData == nullptr)
    {
        fmt::print("ERROR: Could not load internal JSON resource?");
        return false;
    }

    // Lock
    LPVOID pData = LockResource(hResourceData);
    if (pData == nullptr)
    {
        fmt::print("ERROR: Could not lock internal JSON resource?");
        return false;
    }

    try
    {
        keys = json::parse((char *)pData, nullptr, false, false);
    }
    catch (const json::exception &e)
    {
        fmt::print("ERROR: Exception occurred while parsing internal JSON file: {}\n", e.what());
    }

    FreeResource(hResourceData);

    return true;
}
