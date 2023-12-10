/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2023 UMSKT Contributors (et.al.)
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

EXPORT class PIDGEN2
{
  public:
    static bool isNumericString(char *input);
    static bool isValidChannelID(char *channelID);
    static bool isValidOEMID(char *OEMID);
    static bool isValidYear(char *year);
    static bool isValidDay(char *day);
    static bool isValidRetailProductID(char *productID);
    static int addDigits(char *input);
    static int GenerateRetail(char *channelID, char *&keyout);
    static int GenerateOEM(char *year, char *day, char *oem, char *&keyout);
};

#endif // UMSKT_PIDGEN2_H
