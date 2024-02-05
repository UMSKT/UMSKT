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

#ifndef UMSKT_PIDGEN2_H
#define UMSKT_PIDGEN2_H

#include "../libumskt.h"

class EXPORT PIDGEN2
{
    DWORD32 year;
    DWORD32 day;
    BOOL isOEM;
    BOOL isOffice;

    static constexpr char channelIDBlacklist[7][4] = {"333", "444", "555", "666", "777", "888", "999"};
    static constexpr char validYears[8][3] = {"95", "96", "97", "98", "99", "00", "01", "02"};

  public:
    BOOL isNumericString(char *input);
    BOOL isValidChannelID(char *channelID);
    BOOL isValidOEMID(char *OEMID);
    BOOL isValidYear(char *year);
    BOOL isValidDay(char *day);
    BOOL isValidRetailProductID(char *productID);
    int addDigits(char *input);
    int GenerateRetail(char *channelID, char *&keyout);
    int GenerateOEM(char *year, char *day, char *oem, char *&keyout);
};

#endif // UMSKT_PIDGEN2_H
