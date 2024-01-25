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
 * @FileCreated by Neo on 06/24/2023
 * @Maintainer Neo
 */

#ifndef UMSKT_PIDGEN3_H
#define UMSKT_PIDGEN3_H

#include "../libumskt.h"

class BINK1998;
class BINK2002;

class EXPORT PIDGEN3
{
    friend class BINK1998;
    friend class BINK2002;

  protected:
    BIGNUM *privateKey, *genOrder;
    EC_POINT *genPoint, *pubPoint;
    EC_GROUP *eCurve;

  public:
    PIDGEN3()
    {
    }

    PIDGEN3(PIDGEN3 &p3)
    {
        privateKey = p3.privateKey;
        genOrder = p3.genOrder;
        genPoint = p3.genPoint;
        pubPoint = p3.pubPoint;
        eCurve = p3.eCurve;
    }

    virtual ~PIDGEN3()
    {
        EC_GROUP_free(eCurve);
        EC_POINT_free(genPoint);
        EC_POINT_free(pubPoint);
        BN_free(genOrder);
        BN_free(privateKey);
    }

    struct KeyInfo
    {
        DWORD Serial = 0, AuthInfo = 0, ChannelID = 0, Hash = 0;
        QWORD Signature = 0;
        BOOL isUpgrade = false;

        void setSerial(DWORD serialIn)
        {
            Serial = serialIn;
        }

        void setAuthInfo(DWORD AuthInfoIn)
        {
            AuthInfo = AuthInfoIn;
        }

        void setChannelID(DWORD ChannelIDIn)
        {
            ChannelID = ChannelIDIn;
        }
    } info;

    static constexpr char pKeyCharset[] = "BCDFGHJKMPQRTVWXY2346789";

    BOOL LoadEllipticCurve(std::string pSel, std::string aSel, std::string bSel, std::string generatorXSel,
                           std::string generatorYSel, std::string publicKeyXSel, std::string publicKeyYSel,
                           std::string genOrderSel, std::string privateKeySel);

    virtual BOOL Pack(QWORD *pRaw) = 0;
    virtual BOOL Unpack(QWORD *pRaw) = 0;
    virtual BOOL Generate(std::string &pKey);
    virtual BOOL Validate(std::string &pKey);

    // PIDGEN3.cpp
    void base24(std::string &cdKey, BYTE *byteSeq);
    void unbase24(BYTE *byteSeq, std::string cdKey);
    BOOL checkFieldIsBink1998();
    static BOOL checkFieldStrIsBink1998(std::string keyin);
};

#endif // UMSKT_PIDGEN3_H
