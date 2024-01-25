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
 * @FileCreated by Neo on 06/25/2023
 * @Maintainer Neo
 */

#include "libumskt.h"

#ifdef _WIN32
// this seems janky but it works, and doesn't use storage that would otherwise get clobbered
std::FILE *getFileStreamToNul()
{
    fopen_s(&UMSKT::debug, "nul", "w");
    return UMSKT::debug;
}
std::FILE *UMSKT::debug = getFileStreamToNul();
#else
std::FILE *UMSKT::debug = std::fopen("/dev/null", "w");
#endif

BOOL UMSKT::VERBOSE = false;
BOOL UMSKT::DEBUG = false;

/**
 * sets the filestream used for debugging
 *
 * @param input std::FILE
 */
void UMSKT::setDebugOutput(std::FILE *input)
{
    debug = input;
}
