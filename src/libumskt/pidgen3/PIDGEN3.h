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
 * @FileCreated by Neo on 6/24/2023
 * @Maintainer Neo
 */

#ifndef UMSKT_PIDGEN3_H
#define UMSKT_PIDGEN3_H

#include "../libumskt.h"

class PIDGEN3
{
  public:
    class BINK1998;
    class BINK2002;

    // util.cpp
    static int BN_bn2lebin(const BIGNUM *a, unsigned char *to,
                           int tolen); // Hello OpenSSL developers, please tell me, where is this function at?
    static void endian(BYTE *data, int length);
    static EC_GROUP *initializeEllipticCurve(std::string pSel, std::string aSel, std::string bSel,
                                             std::string generatorXSel, std::string generatorYSel,
                                             std::string publicKeyXSel, std::string publicKeyYSel, EC_POINT *&genPoint,
                                             EC_POINT *&pubPoint);

    // key.cpp
    static constexpr char pKeyCharset[] = "BCDFGHJKMPQRTVWXY2346789";
    static void unbase24(BYTE *byteSeq, const char *cdKey);
    static void base24(char *cdKey, BYTE *byteSeq);
};

#endif // UMSKT_PIDGEN3_H
