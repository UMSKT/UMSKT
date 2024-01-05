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

class BINK2002 : public PIDGEN3
{

  public:
    BOOL Unpack(QWORD (&pRaw)[2]) override;
    BOOL Pack(QWORD (&pRaw)[2]) override;
    BOOL Verify(std::string &pKey) override;
    BOOL Generate(std::string &pKey) override;
};

#endif // UMSKT_BINK2002_H
