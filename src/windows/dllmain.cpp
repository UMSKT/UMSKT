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
 * @FileCreated by Neo on 06/17/2023
 * @Maintainer Neo
 */

#define WIN32_LEAN_AND_MEAN
#include "resource.h"
#include <windows.h>

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD nReason, IN LPVOID Reserved)
{

    BOOLEAN bSuccess = TRUE;
    //  Perform global initialization.
    switch (nReason)
    {
    case DLL_PROCESS_ATTACH:
        //  For optimization.
        DisableThreadLibraryCalls(hDllHandle);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return bSuccess;
}

//  end DllMain
