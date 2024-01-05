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

class PIDGEN3
{
  protected:
    EC_GROUP *eCurve;
    EC_POINT *basePoint, *publicKey, *genPoint, *pubPoint;
    BIGNUM *genOrder, *privateKey;
    DWORD Serial, AuthInfo, ChannelID, Hash;
    QWORD Signature;
    BOOL isUpgrade;

  public:
    PIDGEN3()
    {
    }

    ~PIDGEN3()
    {
        EC_GROUP_free(eCurve);
        EC_POINT_free(genPoint);
        EC_POINT_free(pubPoint);
        EC_POINT_free(basePoint);
        EC_POINT_free(publicKey);
        BN_free(genOrder);
        BN_free(privateKey);
        BN_free(privateKey);
        BN_free(genOrder);
    }

    static constexpr char pKeyCharset[] = "BCDFGHJKMPQRTVWXY2346789";

    BOOL LoadEllipticCurve(std::string pSel, std::string aSel, std::string bSel, std::string generatorXSel,
                           std::string generatorYSel, std::string publicKeyXSel, std::string publicKeyYSel,
                           std::string genOrderSel, std::string privateKeySel);

    virtual BOOL Unpack(QWORD (&pRaw)[2]) = 0;
    virtual BOOL Pack(QWORD (&pRaw)[2]) = 0;
    virtual BOOL Verify(std::string &pKey) = 0;
    virtual BOOL Generate(std::string &pKey) = 0;

    // PIDGEN3.cpp
    // Hello OpenSSL developers, please tell me, where is this function at?
    int BN_bn2lebin(const BIGNUM *a, unsigned char *to, int tolen);
    void endian(BYTE *data, int length);

    void base24(std::string &cdKey, BYTE *byteSeq);
    void unbase24(BYTE *byteSeq, std::string cdKey);

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
};

#endif // UMSKT_PIDGEN3_H
