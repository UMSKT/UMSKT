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

#include <libumskt/libumskt.h>

std::FILE *UMSKT::debug;
std::FILE *UMSKT::verbose;

BOOL UMSKT::IS_CONSTRUCTED = UMSKT::CONSTRUCT();

/**
 * a static "constructor" that does some housekeeping for certain
 * platforms, in DJGPP for instance we need to setup the interval
 * timer for RNG.
 *
 * @return true
 */
BOOL UMSKT::CONSTRUCT()
{
#ifdef __DJGPP__
    // this should be set up as early as possible
    uclock();
#endif
    return true;
}

/**
 * sets the filestream used for debugging
 *
 * @param input std::FILE
 */
void UMSKT::setDebugOutput(std::FILE *input)
{
    debug = input;
}

/**
 * sets the filestream used for verbose messages
 *
 * @param input std::FILE
 */
void UMSKT::setVerboseOutput(std::FILE *input)
{
    verbose = input;
}
