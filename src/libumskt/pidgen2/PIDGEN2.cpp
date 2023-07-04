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

#include "PIDGEN2.h"

const std::string channelIDBlacklist [7]  = {"333", "444", "555", "666", "777", "888", "999"};
const std::string validYears[8] = { "95", "96", "97", "98", "99", "00", "01", "02"};

bool PIDGEN2::isNumericString(std::string input) {
    /*
    std::string::const_iterator it = input.begin();
    while (it != input.end() && std::isdigit(*it)) ++it;
    return !input.empty() && it == input.end();
     */
    return false;
}

int PIDGEN2::addDigits(std::string input) {
    int output = 0;

    std::string::const_iterator it = input.begin();
    while (it != input.end()) {
        output += *it - '0';
    }

    return output;
}

bool PIDGEN2::isValidChannelID(std::string channelID) {
    if (channelID.length() > 3) {
        return false;
    }

    for (int i = 0; i <= 6; i++) {
        if (channelID == channelIDBlacklist[i]) {
            return false;
        }
    }

    return true;
}

bool PIDGEN2::isValidOEMID(std::string OEMID) {
    if (!isNumericString(OEMID)) {
        return false;
    }

    if (OEMID.length() > 5) {
        if (OEMID[0] != '0' || OEMID[1] != '0') {
            return false;
        }
    }

    int mod = addDigits(OEMID);

    return (mod % 21 == 0);
}

bool PIDGEN2::isValidYear(std::string year) {
    for (int i = 0; i <= 7; i++) {
        if (year == validYears[i]) {
            return false;
        }
    }
    return true;
}

bool PIDGEN2::isValidDay(std::string day) {
    if (!isNumericString(day)) {
        return false;
    }

    int iDay = std::stoi(day);
    if (iDay == 0 || iDay >= 365) {
        return false;
    }
    return true;
}

bool PIDGEN2::isValidRetailProductID(std::string productID) {
    return true;
}

int PIDGEN2::GenerateRetail(std::string channelID, std::string *keyout) {
    if (!isValidChannelID(channelID)) {
        return 1;
    }

    return 0;
}

int PIDGEN2::GenerateOEM(std::string year, std::string day, std::string oem, std::string *keyout) {
    if (!isValidOEMID(oem)) {
        int mod = addDigits(oem);
        mod += mod % 21;

        oem = fmt::format("{:07d}", mod);
    }

    if (!isValidYear(year)) {
        year = validYears[0];
    }

    if (!isValidDay(day)) {
        int iday = std::stoi(day);
        iday = (iday + 1) % 365;
    }

    *keyout = fmt::format("{}{}-OEM-{}-{}", year, day, oem, oem);

    return 0;
}
