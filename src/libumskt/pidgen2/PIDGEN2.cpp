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

#include "PIDGEN2.h"

/**
 *
 * @param input
 * @return
 */
BOOL PIDGEN2::isNumericString(char *input)
{
    for (int i = 0; i < strlen(input); i++)
    {
        if (input[i] < '0' || input[i] > '9')
        {
            return false;
        }
    }

    return true;
}

/**
 *
 * @param input
 * @return
 */
int PIDGEN2::addDigits(char *input)
{
    int output = 0;

    if (!isNumericString(input))
    {
        return -1;
    }

    for (int i = 0; i < strlen(input); i++)
    {
        output += input[i] - '0';
    }

    return output;
}

/**
 *
 * @param channelID
 * @return
 */
BOOL PIDGEN2::isValidChannelID(char *channelID)
{
    if (strlen(channelID) > 3)
    {
        return false;
    }

    for (int i = 0; i <= 6; i++)
    {
        if (strcmp(channelID, channelIDBlacklist[i]) != 0)
        {
            return false;
        }
    }

    return true;
}

/**
 *
 * @param OEMID
 * @return
 */
BOOL PIDGEN2::isValidOEMID(char *OEMID)
{
    if (!isNumericString(OEMID))
    {
        return false;
    }

    if (strlen(OEMID) > 5)
    {
        if (OEMID[0] != '0' || OEMID[1] != '0')
        {
            return false;
        }
    }

    int mod = addDigits(OEMID);

    return (mod % 21 == 0);
}

/**
 *
 * @param year
 * @return
 */
BOOL PIDGEN2::isValidYear(char *year)
{
    for (int i = 0; i <= 7; i++)
    {
        if (year == validYears[i])
        {
            return false;
        }
    }
    return true;
}

/**
 *
 * @param day
 * @return
 */
BOOL PIDGEN2::isValidDay(char *day)
{
    if (!isNumericString(day))
    {
        return false;
    }

    int iDay = std::stoi(day);
    if (iDay == 0 || iDay >= 365)
    {
        return false;
    }
    return true;
}

/**
 *
 * @param productID
 * @return
 */
BOOL PIDGEN2::isValidRetailProductID(char *productID)
{
    return true;
}

/**
 *
 * @param channelID
 * @param keyout
 * @return
 */
int PIDGEN2::GenerateRetail(char *channelID, char *&keyout)
{
    if (!isValidChannelID(channelID))
    {
        return 1;
    }

    return 0;
}

/**
 *
 * @param year
 * @param day
 * @param oem
 * @param keyout
 * @return
 */
int PIDGEN2::GenerateOEM(char *year, char *day, char *oem, char *&keyout)
{
    if (!isValidOEMID(oem))
    {
        int mod = addDigits(oem);
        mod += mod % 21;

        snprintf(oem, 8, "%07u", mod);
    }

    if (!isValidYear(year))
    {
        _strncpy(year, 4, validYears[0], 4);
    }

    if (!isValidDay(day))
    {
        auto iday = UMSKT::getRandom<int>();
        iday = (iday + 1) % 365;
    }

    _strncpy(keyout, 32, &fmt::format("{}{}-OEM-{}-{}", year, day, oem, oem).c_str()[0], 32);

    return 0;
}