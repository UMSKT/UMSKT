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

EXPORT class PIDGEN2 {
public:
    bool isNumericString(std::string input);
    bool isValidChannelID(std::string channelID);
    bool isValidOEMID(std::string OEMID);
    bool isValidYear(std::string year);
    bool isValidDay(std::string day);
    bool isValidRetailProductID(std::string productID);
    int addDigits(std::string input);
    int GenerateRetail(std::string channelID, std::string *keyout);
    int GenerateOEM(std::string year, std::string day, std::string oem, std::string *keyout);
};

#endif //UMSKT_PIDGEN2_H
