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
 * @FileCreated by Neo on 02/13/2024
 * @Maintainer Neo
 */

#include "libumskt.h"

#ifndef UMSKT_PIDGEN_H
#define UMSKT_PIDGEN_H

/**
 * PIDGEN Interface
 *
 * Defines three entry points:
 * Generate, Validate, StringifyKey
 */
class PIDGEN : public UMSKT
{
  public:
    static const Integer SEVEN;
    static const Integer TEN;
    static const Integer MaxChannelID;
    static const Integer MaxSerial;

    virtual BOOL Generate(std::string &pKey) = 0;
    virtual BOOL Validate(const std::string &pKey) = 0;
    virtual std::string StringifyKey(const std::string &pKey) = 0;
    virtual std::string StringifyProductID() = 0;

    Integer GenerateMod7(const Integer &in);
    BOOL isValidMod7(const Integer &in);
};

#endif // UMSKT_PIDGEN_H
