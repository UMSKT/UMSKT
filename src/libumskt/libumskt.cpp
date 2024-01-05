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
 * @FileCreated by Neo on 06/25/2023
 * @Maintainer Neo
 */

#include "libumskt.h"
#include "confid/confid.h"
#include "pidgen2/PIDGEN2.h"
#include "pidgen3/BINK1998.h"
#include "pidgen3/BINK2002.h"
#include "pidgen3/PIDGEN3.h"

FNEXPORT BOOL LIBUMSKT_SET_DEBUG_OUTPUT(void *filestream)
{
    char buffer[7];
    memcpy(buffer, filestream, 6);
    buffer[6] = 0;
    auto buff_string = std::string(buffer);

    if (strcasecmp("STDOUT", &buffer[0]) != 0)
    {
        UMSKT::debug = stdout;
        return true;
    }
    else if (strcasecmp("STDERR", &buffer[0]) != 0)
    {
        UMSKT::debug = stderr;
        return true;
    }

    return false;
}

// ---------------------------------------------

FNEXPORT void *CONFID_INIT()
{
    auto cid = new ConfirmationID();

    // cid->LoadHyperellipticCurve(0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, 0);

    return cid;
}

FNEXPORT BYTE CONFID_GENERATE(void *cidIn, const char *installation_id_str, char *&confirmation_id, char *productid)
{
    auto *cid((ConfirmationID *)cidIn);

    std::string str, confid(confirmation_id), productids(productid);
    auto retval = cid->Generate(str, confid, productids);

    return retval;
}

FNEXPORT BYTE CONFID_END(void *cidIn)
{
    auto *cid((ConfirmationID *)cidIn);
    delete cid;

    return true;
}

// ---------------------------------------------

FNEXPORT void *PIDGEN3_BINK1998_INIT(const char *p, const char *a, const char *b, const char *generatorX,
                                     const char *generatorY, const char *publicKeyX, const char *publicKeyY,
                                     const char *genOrder, const char *privateKey)
{

    auto *bink1998 = new BINK1998();

    bink1998->LoadEllipticCurve(p, a, b, generatorX, generatorY, publicKeyX, publicKeyY, genOrder, privateKey);

    return bink1998;
}

FNEXPORT void *PIDGEN3_BINK2002_INIT(const char *p, const char *a, const char *b, const char *generatorX,
                                     const char *generatorY, const char *publicKeyX, const char *publicKeyY,
                                     const char *genOrder, const char *privateKey, const char *authinfo)
{

    auto bink2002 = new BINK2002();

    bink2002->LoadEllipticCurve(p, a, b, generatorX, generatorY, publicKeyX, publicKeyY, genOrder, privateKey);

    return bink2002;
}

FNEXPORT BOOL PIDGEN3_Generate(void *&ptrIn, char *&pKeyOut, int pKeySizeIn)
{
    auto *p3((PIDGEN3 *)ptrIn);

    std::string str;
    BOOL retval = p3->Generate(str);

    if (pKeySizeIn > str.length() + 1)
    {
        return false;
    }

    memcpy(pKeyOut, &str[0], str.length());
    pKeyOut[str.length()] = 0;

    return retval;
}

FNEXPORT BOOL PIDGEN3_Verify(void *&ptrIn, char *pKeyIn)
{
    auto *p3((PIDGEN3 *)ptrIn);
    std::string str(pKeyIn);

    BOOL retval = p3->Verify(str);

    return retval;
}

FNEXPORT void PIDGEN3_END(void *ptrIn)
{
    auto *p3((PIDGEN3 *)ptrIn);
    delete p3;
}

// ---------------------------------------------

FNEXPORT BOOL PIDGEN2_INIT()
{
    return true;
}

FNEXPORT BOOL PIDGEN2_GENERATE()
{
    return true;
}

FNEXPORT BOOL PIDGEN2_END()
{
    return true;
}

FNEXPORT BOOL PIDGEN2_GenerateRetail(char *channelID, char *&keyout)
{
    auto P2 = new PIDGEN2();
    BOOL retval = P2->GenerateRetail(channelID, keyout);
    delete P2;
    return retval;
}

FNEXPORT BOOL PIDGEN2_GenerateOEM(char *year, char *day, char *oem, char *&keyout)
{
    auto P2 = new PIDGEN2();
    BOOL retval = P2->GenerateOEM(year, day, oem, keyout);
    delete P2;
    return retval;
}