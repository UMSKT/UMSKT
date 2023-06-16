/**
 * This file is a part of the WindowsXPKg Project
 *
 * Copyleft (C) 2019-2023 WindowsXPKg Contributors (et.al.)
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

#include "header.h"
#include "cli.h"

Options options;

int main(int argc, char *argv[]) {
    if (!CLI::parseCommandLine(argc, argv, &options)) {
        fmt::print("error parsing command line options\n");
        CLI::showHelp(argv);
        return !options.error ? 0 : 1;
    }

    json keys;

    int status = CLI::validateCommandLine(&options, argv, &keys);
    if (status > 0) {
        return status;
    }

    CLI* run = new CLI(options, keys);

    switch(options.applicationMode) {
        case MODE_BINK1998:
            return run->BINK1998();

        case MODE_BINK2002:
            return run->BINK2002();

        case MODE_CONFIRMATION_ID:
            return run->ConfirmationID();

        default:
            return 1;
    }
}
