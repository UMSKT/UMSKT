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

#include <libumskt/pidgen.h>

class BINK1998;
class BINK2002;

class EXPORT PIDGEN3 : public PIDGEN
{
    friend class BINK1998;
    friend class BINK2002;

  protected:
    Integer BINKID;
    Integer privateKey, genOrder;
    ECP::Point pubPoint, genPoint;
    ECP eCurve;

  public:
    PIDGEN3() = default;

    PIDGEN3(PIDGEN3 &p3)
    {
        privateKey = p3.privateKey;
        genOrder = p3.genOrder;
        pubPoint = p3.pubPoint;
        genPoint = p3.genPoint;
        eCurve = p3.eCurve;
    }

    ~PIDGEN3() override = default;

    struct KeyInfo
    {
        BOOL isUpgrade = false, isOEM = false;
        Integer ChannelID = 0, Serial = 0, AuthInfo = 0, Rand = 0, Hash = 0, Signature = 0;

        void setSerial(DWORD32 SerialIn)
        {
            Serial = IntegerN(SerialIn);
        }

        void setAuthInfo(DWORD32 AuthInfoIn)
        {
            AuthInfo = IntegerN(AuthInfoIn);
        }

        void setChannelID(DWORD32 ChannelIDIn)
        {
            ChannelID = IntegerN(ChannelIDIn);
        }
    } info;

    static const std::string pKeyCharset;
    static const DWORD32 MaxSizeBINK1998;

    BOOL LoadEllipticCurve(const std::string &BinkIDSel, const std::string &pSel, const std::string &aSel,
                           const std::string &bSel, const std::string &generatorXSel, const std::string &generatorYSel,
                           const std::string &publicKeyXSel, const std::string &publicKeyYSel,
                           const std::string &genOrderSel, const std::string &privateKeySel);

    BOOL Generate(std::string &pKey) override;
    BOOL Validate(const std::string &pKey) override;
    std::string StringifyKey(const std::string &pKey) override;
    std::string StringifyProductID() override;
    BOOL ValidateKeyString(const std::string &in_key, std::string &out_key) override;

    virtual Integer Pack(const KeyInfo &ki)
    {
        throw std::runtime_error("PIDGEN3::Pack() pure virtual function call");
    }

    virtual KeyInfo Unpack(const Integer &raw)
    {
        throw std::runtime_error("PIDGEN3::Unpack() pure virtual function call");
    }

    // PIDGEN3.cpp
    static PIDGEN3 *Factory(const std::string &field);
    static BOOL checkFieldStrIsBink1998(std::string keyin);
    static std::string ValidateStringKeyInputCharset(std::string &accumulator, char currentChar);
    static std::string base24(Integer &seq);
    static Integer unbase24(const std::string &cdKey);

    BOOL checkFieldIsBink1998();
};

#endif // UMSKT_PIDGEN3_H
