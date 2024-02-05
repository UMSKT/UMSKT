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
 * @FileCreated by Neo on 06/06/2023
 * @Maintainer Neo
 */

#ifndef UMSKT_BINK2002_H
#define UMSKT_BINK2002_H

#include "PIDGEN3.h"

class EXPORT BINK2002 : public PIDGEN3
{
  public:
    using PIDGEN3::PIDGEN3;
    BINK2002() = default;
    explicit BINK2002(PIDGEN3 *p3)
    {
        privateKey = p3->privateKey;
        genOrder = p3->genOrder;
        genPoint = p3->genPoint;
        pubPoint = p3->pubPoint;
        eCurve = p3->eCurve;
    }

    static constexpr DWORD32 FieldBits = 512;
    static constexpr DWORD32 FieldBytes = FieldBits / 8;
    static constexpr DWORD32 SHAMessageLength = (3 + 2 * FieldBytes);

    using PIDGEN3::Pack;
    BOOL Pack(Q_OWORD *pRaw) override;

    using PIDGEN3::Unpack;
    BOOL Unpack(Q_OWORD *pRaw) override;

    using PIDGEN3::Generate;
    BOOL Generate(std::string &pKey) override;

    using PIDGEN3::Validate;
    BOOL Validate(std::string &pKey) override;
};

#endif // UMSKT_BINK2002_H
