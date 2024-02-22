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

const std::vector<std::string> PIDGEN2::channelIDDisallowList = {"333", "444", "555", "666", "777", "888", "999"};
const std::vector<std::string> PIDGEN2::validYears = {"95", "96", "97", "98", "99", "00", "01", "02"};

/**
 * Generates a PID 2.0 key, output is placed in pKey
 *
 * @param pKey
 * @return true
 */
BOOL PIDGEN2::Generate(std::string &pKey)
{
    Integer random;
    random.Randomize(rng, sizeof(DWORD32) * 8);

    info.ChannelID = info.ChannelID % MaxChannelID;
    info.Serial = info.Serial % MaxSerial;

    if (info.isOEM)
    {
        info.Day = info.Day % Integer(366);
        // info.Year = info.Year;

        info.OEMID = (info.ChannelID * TEN) + (info.Serial / (MaxSerial / TEN));
        info.Serial %= (MaxSerial / TEN);

        info.OEMID = (info.OEMID * TEN) + GenerateMod7(info.OEMID);

        DWORD32 day = EncodeN(info.Day), year = EncodeN(info.Year), serial = EncodeN(info.Serial),
                oemid = EncodeN(info.OEMID);

        if (debug)
        {
            fmt::print("\n{:03d}{:02d}-OEM-{:07d}-{:05d}\n", day, year, oemid, serial);
        }

        pKey = fmt::format("{:03d}{:02d}{:07d}{:05d}", day, year, oemid, serial);
    }
    else if (info.isOffice)
    {
        info.ChannelID = (info.ChannelID * TEN) + ((info.ChannelID % TEN) + 1);
        info.Serial = (info.Serial * TEN) + GenerateMod7(info.Serial);

        DWORD32 channelid = EncodeN(info.ChannelID), serial = EncodeN(info.Serial);

        if (debug)
        {
            fmt::print("\n{:04d}-{:07d}\n", channelid, serial);
        }

        pKey = fmt::format("{:04d}{:07d}", channelid, serial);
    }
    else
    {
        info.Serial = (info.Serial * TEN) + GenerateMod7(info.Serial);

        DWORD32 channelid = EncodeN(info.ChannelID), serial = EncodeN(info.Serial);

        if (debug)
        {
            fmt::print("\n{:03d}-{:07d}\n", channelid, serial);
        }

        pKey = fmt::format("{:03d}{:07d}", channelid, serial);
    }

    return true;
}

/**
 * Valid serial types are:
 *
 * C = Channel/Site ID (001 - 998)
 * E = Office Channel ID (+1) Check Digit
 * N = Serial
 * K = Mod7 Check Digit
 *
 * -- OEM Specific
 * D = 3 Digit day (001 - 366)
 * Y = 2 Digit year
 * O = OEM ID - typically seen as a channel ID + the first digit of the serial + mod7 check digit
 *
 * note that the N segment for OEM serials do not have a Mod7 check
 *
 * CCC-NNNNNNK
 * CCCE-NNNNNNK
 * DDDYY-ZZOOONK-NNNNN
 * DDDYY-OEM-ZZOOONK-NNNNN
 *
 * we can determine what type of key we have
 * simply by counting the numeric characters
 *
 * @param pKey
 * @return
 */
BOOL PIDGEN2::Validate(const std::string &pKey)
{
    std::string filtered;
    ValidateKeyString(pKey, filtered);

    bool bIsValidChannelID, bIsValidSerial, bIsValidOEMDay, bIsValidOEMYear, bIsValidOEMID;

    switch (filtered.length())
    {
    case KeySize::FPP:
        // standard FPP/CCP has 10 digits
        info.ChannelID = IntegerS(filtered.substr(0, 3));
        info.Serial = IntegerS(filtered.substr(3, 7));

        bIsValidChannelID = isValidChannelID();
        bIsValidSerial = isValidSerial();

        if (debug)
        {
            fmt::print("\n\nisValidChannelID: {} isValidSerial: {}\n", bIsValidChannelID, bIsValidSerial);
        }

        return bIsValidChannelID && bIsValidSerial;

    case KeySize::Office:
        // so far only office 97 has been documented using this
        info.isOffice = true;
        info.ChannelID = IntegerS(filtered.substr(0, 4));
        info.Serial = IntegerS(filtered.substr(4, 7));

        bIsValidChannelID = isValidChannelID();
        bIsValidSerial = isValidSerial();

        if (debug)
        {
            fmt::print("\n\nisValidChannelID: {} isValidSerial: {}\n", bIsValidChannelID, bIsValidSerial);
        }

        return bIsValidChannelID && bIsValidSerial;

    case KeySize::OEM:
        // all OEM keys follow this pattern
        info.isOEM = true;
        info.Day = IntegerS(filtered.substr(0, 3));
        info.Year = IntegerS(filtered.substr(3, 2));
        info.OEMID = IntegerS(filtered.substr(5, 7)); // 6 + check digit
        info.Serial = IntegerS(filtered.substr(12, 5));

        bIsValidOEMDay = isValidOEMDay();
        bIsValidOEMYear = isValidOEMYear();
        bIsValidOEMID = isValidOEMID();

        if (debug)
        {
            fmt::print("\n\nisValidOEMDay: {} isValidOEMYear: {} isValidOEMID: {}\n", bIsValidOEMDay, bIsValidOEMYear,
                       bIsValidOEMID);
        }

        return bIsValidOEMDay && bIsValidOEMYear && bIsValidOEMID;

    default:
        return false;
    }
}

/**
 *
 * @param pKey
 * @return
 */
std::string PIDGEN2::StringifyKey(const std::string &pKey)
{
    switch (pKey.length())
    {
    case KeySize::FPP:
        return fmt::format("{}-{}", pKey.substr(0, 3), pKey.substr(3, 7));

    case KeySize::Office:
        return fmt::format("{}-{}", pKey.substr(0, 4), pKey.substr(4, 7));

    case KeySize::OEM:
        return fmt::format("{}-OEM-{}-{}", pKey.substr(0, 5), pKey.substr(5, 7), pKey.substr(12, 5));

    default:
        return "";
    }
}

/**
 *
 * @return
 */
std::string PIDGEN2::StringifyProductID()
{
    if (info.isOEM)
    {
        return fmt::format("{:d}{:d}-OEM-{:d}-{:d}", info.Year, info.Day, info.OEMID, info.Serial);
    }

    return fmt::format("{}-{}", info.ChannelID, info.Serial);
}

/**
 *
 * @param in_key
 * @param out_key
 * @return
 */
BOOL INLINE PIDGEN2::ValidateKeyString(const std::string &in_key, std::string &out_key)
{
    std::copy_if(in_key.begin(), in_key.end(), std::back_inserter(out_key), [](char c) { return std::isdigit(c); });

    return out_key.length() == KeySize::FPP || out_key.length() == KeySize::Office || out_key.length() == KeySize::OEM;
}

/**
 * Is the Serial with check digit a valid serial?
 *
 * standard Mod7 Check
 *
 * @return validity
 */
BOOL PIDGEN2::isValidSerial()
{
    return isValidMod7(info.Serial);
}

/**
 * Is the OEMID a valid?
 *
 * @return validity
 */
BOOL PIDGEN2::isValidOEMID()
{
    if (info.OEMID.IsZero())
    {
        return false;
    }

    return isValidMod7(info.OEMID);
}

/**
 * Is the Channel ID a valid Channel ID?
 * also validates Channel ID check digit if applicable
 *
 * Known invalid Channel IDs are:
 * 333, 444, 555, 666, 777, 888, 999
 *
 * @return validity
 */
BOOL PIDGEN2::isValidChannelID() const
{
    // if we're office, do the last digit +1 checksum
    if (info.isOffice)
    {
        Integer CheckDigit = (info.ChannelID % TEN), ChannelID = (info.ChannelID / TEN);

        if (std::find(channelIDDisallowList.begin(), channelIDDisallowList.end(), IntToString(ChannelID)) !=
            channelIDDisallowList.end())
        {
            return false;
        }

        return (ChannelID % TEN) + 1 == CheckDigit;
    }

    // otherwise just make sure we're not in the disallow list
    return std::find(channelIDDisallowList.begin(), channelIDDisallowList.end(), IntToString(info.ChannelID)) ==
           channelIDDisallowList.end();
}

/**
 * Is the OEM year in the allow list?
 *
 * Known allowed years are:
 * 95, 96, 97, 98, 99, 00, 01, 02
 *
 * @return validity
 */
BOOL PIDGEN2::isValidOEMYear() const
{
    auto year = fmt::format("{:02d}", info.Year.ConvertToLong());
    return std::find(validYears.begin(), validYears.end(), year) != validYears.end();
}

/**
 * Is the OEM Day an allowed day?
 *
 * Allowed days are 1 - 366 inclusive
 *
 * @return validity
 */
BOOL PIDGEN2::isValidOEMDay() const
{
    return info.Day >= 0 && info.Day <= 366;
}
