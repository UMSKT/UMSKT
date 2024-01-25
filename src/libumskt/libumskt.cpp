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

std::map<UMSKT_TAG, UMSKT_Value> UMSKT::tags;

extern "C"
{

    /**
     * Sets debug output to a given C++ File stream
     * if the memory allocated at filestream is "STDOUT" or "STDERR"
     * simply use the global vars allocated by *this* C++ runtime.
     * otherwise, assume that the input pointer is an ABI equivalent std::FILE
     *
     * @param char* or std::FILE "filestream"
     */
    EXPORT BOOL UMSKT_SET_DEBUG_OUTPUT(void *filestream)
    {
        char buffer[7];
        memcpy(buffer, filestream, 6);
        buffer[6] = 0;
        auto buffstring = std::string(buffer);
        std::transform(buffstring.begin(), buffstring.end(), buffstring.begin(), ::tolower);

        if (buffstring == "stdout")
        {
            UMSKT::debug = stdout;
            return true;
        }
        else if (buffstring == "stderr")
        {
            UMSKT::debug = stderr;
            return true;
        }
        else
        {
            UMSKT::debug = (std::FILE *)filestream;
            return true;
        }

        return false;
    }

    // ---------------------------------------------

    /**
     *
     * @param tag
     * @param value
     * @param valueSize
     * @return success
     */
    EXPORT BOOL UMSKT_SET_TAG(UMSKT_TAG tag, char *value, size_t valueSize)
    {
        if (valueSize > sizeof(UMSKT_Value))
        {
            return false;
        }

        // wipe/set the tag
        memset(&UMSKT::tags[tag], 0, sizeof(UMSKT_Value));
        memcpy(&UMSKT::tags[tag], value, valueSize);

        return true;
    }

    EXPORT void UMSKT_RESET_TAGS()
    {
        UMSKT::tags.clear();
    }

    // ---------------------------------------------

    EXPORT void *CONFID_INIT()
    {
        auto cid = new ConfirmationID();

        // cid->LoadHyperellipticCurve(0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, 0);

        return cid;
    }

    EXPORT BYTE CONFID_GENERATE(void *cidIn, const char *installation_id_str, char *&confirmation_id, char *productid)
    {
        ConfirmationID *cid;
        try
        {
            cid = static_cast<ConfirmationID *>(cidIn);
        }
        catch (const std::bad_cast &e)
        {
            fmt::print(UMSKT::debug, "{}: input is not a {} - {}", __FUNCTION__, e.what());
            return -1;
        }

        for (auto const i : UMSKT::tags)
        {
            switch (i.first)
            {
            case UMSKT_tag_InstallationID:
                break;
            case UMSKT_tag_ProductID:
                break;
            default:
                break;
            }
        }

        std::string str, confid(confirmation_id), productids(productid);
        auto retval = cid->Generate(str, confid, productids);

        return retval;
    }

    EXPORT void CONFID_END(void *cidIn)
    {
        auto *cid((ConfirmationID *)cidIn);
        delete cid;
        cid = nullptr;
        cidIn = nullptr;
    }

    // ---------------------------------------------

    EXPORT void *PIDGEN3_INIT(const char *p, const char *a, const char *b, const char *generatorX,
                              const char *generatorY, const char *publicKeyX, const char *publicKeyY,
                              const char *genOrder, const char *privateKey)
    {
        PIDGEN3 *p3;

        if (PIDGEN3::checkFieldStrIsBink1998(p))
        {
            p3 = new BINK1998();
        }
        else
        {
            p3 = new BINK2002();
        }

        p3->LoadEllipticCurve(p, a, b, generatorX, generatorY, publicKeyX, publicKeyY, genOrder, privateKey);

        return p3;
    }

    EXPORT BOOL PIDGEN3_Generate(void *&ptrIn, char *&pKeyOut, int pKeySizeIn)
    {
        auto *p3((PIDGEN3 *)ptrIn);

        for (auto const i : UMSKT::tags)
        {
            switch (i.first)
            {
            case UMSKT_tag_isUpgrade:
                p3->info.isUpgrade = i.second.boolean;
                break;
            case UMSKT_tag_ChannelID:
                p3->info.setChannelID(i.second.dword);
                break;
            case UMSKT_tag_Serial:
                p3->info.setSerial(i.second.dword);
                break;
            case UMSKT_tag_AuthData:
                p3->info.setAuthInfo(i.second.dword);
            default:
                break;
            }
        }

        std::string str;
        BOOL retval = p3->Generate(str);

        assert(pKeySizeIn >= str.length() + 1);

        memcpy(pKeyOut, &str[0], str.length());
        pKeyOut[str.length()] = 0;

        return retval;
    }

    EXPORT BOOL PIDGEN3_Validate(void *&ptrIn, char *pKeyIn)
    {
        auto *p3((PIDGEN3 *)ptrIn);
        std::string str(pKeyIn);

        BOOL retval = p3->Validate(str);

        return retval;
    }

    EXPORT void PIDGEN3_END(void *ptrIn)
    {
        auto *p3((PIDGEN3 *)ptrIn);
        delete p3;
        ptrIn = nullptr;
        p3 = nullptr;
    }

    // ---------------------------------------------

    EXPORT void *PIDGEN2_INIT()
    {
        auto p2 = new PIDGEN2();
        return p2;
    }

    EXPORT BOOL PIDGEN2_GENERATE(void *ptrIn, char *&keyout)
    {
        auto p2 = (PIDGEN2 *)ptrIn;

        return true;
    }

    EXPORT void PIDGEN2_END(void *ptrIn)
    {
        auto p2 = (PIDGEN2 *)ptrIn;
        delete p2;
        p2 = nullptr;
        ptrIn = nullptr;
    }

} // extern "C"

/**
 * Convert data between endianness types.
 *
 * @param data   [in]
 * @param length [in]
 **/
void UMSKT::endian(BYTE *data, int length)
{
    for (int i = 0; i < length / 2; i++)
    {
        BYTE temp = data[i];
        data[i] = data[length - i - 1];
        data[length - i - 1] = temp;
    }
}

/**
 * Converts an OpenSSL BigNumber to it's Little Endian binary equivalent
 *
 * @param a     [in] BigNumber to convert
 * @param to    [out] char* binary representation
 * @param tolen [in] length of the char* array
 *
 * @return length of number in to
 **/
int UMSKT::BN_bn2lebin(const BIGNUM *a, unsigned char *to, int tolen)
{
    if (a == nullptr || to == nullptr)
    {
        return 0;
    }

    int len = BN_bn2bin(a, to);

    if (len > tolen)
    {
        return -1;
    }

    // Choke point inside BN_bn2lebinpad: OpenSSL uses len instead of tolen.
    endian(to, tolen);

    return len;
}