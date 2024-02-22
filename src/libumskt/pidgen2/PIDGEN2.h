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

#include <libumskt/pidgen.h>

class EXPORT PIDGEN2 : public PIDGEN
{
    static const std::vector<std::string> channelIDDisallowList;
    static const std::vector<std::string> validYears;

    enum KeySize
    {
        FPP = 10,
        Office = 11,
        OEM = 17
    };

  public:
    ~PIDGEN2() override = default;

    struct KeyInfo
    {
        BOOL isOEM, isOffice;
        Integer Day, Year, OEMID, ChannelID, Serial;
    } info;

    BOOL Generate(std::string &pKey) override;
    BOOL Validate(const std::string &pKey) override;
    std::string StringifyKey(const std::string &pKey) override;
    std::string StringifyProductID() override;
    BOOL ValidateKeyString(const std::string &in_key, std::string &out_key) override;

    BOOL isValidSerial();
    BOOL isValidOEMID();
    [[nodiscard]] BOOL isValidChannelID() const;
    [[nodiscard]] BOOL isValidOEMYear() const;
    [[nodiscard]] BOOL isValidOEMDay() const;
};

#endif // UMSKT_PIDGEN2_H
