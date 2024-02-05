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
    Integer privateKey, genOrder;
    ECP::Point genPoint, pubPoint;
    ECP eCurve;

  public:
    PIDGEN3() = default;

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
    }

    struct KeyInfo
    {
        Integer Serial = 0, AuthInfo = 0, ChannelID = 0, Hash = 0, Signature = 0;
        BOOL isUpgrade = false;

        void setSerial(DWORD32 serialIn)
        {
            Serial.Decode((BYTE *)&serialIn, sizeof(serialIn));
        }

        void setAuthInfo(DWORD32 AuthInfoIn)
        {
            AuthInfo.Decode((BYTE *)&AuthInfoIn, sizeof(AuthInfoIn));
        }

        void setChannelID(DWORD32 ChannelIDIn)
        {
            ChannelID.Decode((BYTE *)&ChannelIDIn, sizeof(ChannelIDIn));
        }
    } info;

    static constexpr char pKeyCharset[] = "BCDFGHJKMPQRTVWXY2346789";
    static const Integer MaxSizeBINK1998;

    BOOL LoadEllipticCurve(std::string pSel, std::string aSel, std::string bSel, std::string generatorXSel,
                           std::string generatorYSel, std::string publicKeyXSel, std::string publicKeyYSel,
                           std::string genOrderSel, std::string privateKeySel);

    virtual BOOL Pack(Q_OWORD *pRaw) = 0;
    virtual BOOL Unpack(Q_OWORD *pRaw) = 0;
    virtual BOOL Generate(std::string &pKey);
    virtual BOOL Validate(std::string &pKey);

    // PIDGEN3.cpp
    void base24(std::string &cdKey, BYTE *byteSeq);
    void unbase24(BYTE *byteSeq, std::string cdKey);
    BOOL checkFieldIsBink1998();
    static BOOL checkFieldStrIsBink1998(std::string keyin);
    static PIDGEN3 *Factory(const std::string &field);
};

#endif // UMSKT_PIDGEN3_H
