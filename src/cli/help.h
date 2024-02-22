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
 * @FileCreated by Neo on 01/02/2024
 * @Maintainer Neo
 */

#ifndef UMSKT_HELP_H
#define UMSKT_HELP_H

typedef BOOL CLIHandlerFunc(const std::string &);

/**
 * CLI Options List.
 *
 * Note: options are processed in the order found in the ENUM
 * order matters mostly for UX.
 *
 */
enum CLIHelpOptionIDs
{
    OPTION_HELP,
    OPTION_HELP2,
    OPTION_VERSION,
    OPTION_VERBOSE,
    OPTION_DEBUG,
    OPTION_FILE,
    OPTION_LIST,
    OPTION_PRODUCT,
    OPTION_FLAVOUR,
    OPTION_NUMBER,
    OPTION_OEM,
    OPTION_UPGRADE,
    OPTION_ACTIVATIONID,
    OPTION_ACTIVATIONPID,
    OPTION_BINK,
    OPTION_CHANNELID,
    OPTION_SERIAL,
    OPTION_AUTHDATA,
    OPTION_VALIDATE,

    CLIHelpOptionID_END
};

struct CLIHelpOptions
{
    std::string Short;
    std::string Long;
    std::string HelpText;
    BOOL hasArguments;
    std::string Default;
    CLIHandlerFunc *handler;
};

#endif // UMSKT_HELP_H
