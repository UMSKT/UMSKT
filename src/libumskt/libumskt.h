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

#ifndef UMSKT_LIBUMSKT_H
#define UMSKT_LIBUMSKT_H

#include "../typedefs.h"

#include <iostream>
#include <sstream>
#include <string>

#include <cryptopp/cryptlib.h>
#include <cryptopp/ecp.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/integer.h>
#include <cryptopp/nbtheory.h>
#include <cryptopp/osrng.h>
#include <cryptopp/randpool.h>
#include <cryptopp/rng.h>
#include <cryptopp/sha.h>

using Integer = CryptoPP::Integer;
using ECP = CryptoPP::ECP;
using SHA = CryptoPP::SHA1;

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

class HexInteger : public Integer
{
};

// fmt <-> CryptoPP linkage
template <> struct fmt::formatter<HexInteger> : fmt::formatter<std::string_view>
{
    auto format(const HexInteger &i, format_context &ctx) const
    {
        size_t size = i.MinEncodedSize();
        CryptoPP::SecByteBlock encoded;
        encoded.resize(size);
        i.Encode(encoded, size);

        std::string hexString;

        CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(hexString), false);
        encoder.Put(encoded, size);
        encoder.MessageEnd();

        return fmt::formatter<std::string_view>::format(hexString, ctx);
    }
};

template <> struct fmt::formatter<Integer> : ostream_formatter
{
    auto format(const Integer &i, format_context &ctx) const
    {
        return basic_ostream_formatter<char>::format(i, ctx);
    }
};

// Algorithm macros
#define PK_LENGTH 25
#define NULL_TERMINATOR 1

#define NEXTSNBITS(field, n, offset) (((QWORD)(field) >> (offset)) & ((1ULL << (n)) - 1))
#define FIRSTNBITS(field, n) NEXTSNBITS((field), (n), 0)

#define HIBYTES(field, bytes) NEXTSNBITS((QWORD)(field), ((bytes) * 8), ((bytes) * 8))
#define LOBYTES(field, bytes) FIRSTNBITS((QWORD)(field), ((bytes) * 8))

#define BYDWORD(n) (DWORD32)(*((n) + 0) | *((n) + 1) << 8 | *((n) + 2) << 16 | *((n) + 3) << 24)
#define BITMASK(n) ((1ULL << (n)) - 1)

#ifndef LIBUMSKT_VERSION_STRING
#define LIBUMSKT_VERSION_STRING "unknown version-dirty"
#endif

enum ValueType
{
    VALUE_BOOL,
    VALUE_WORD,
    VALUE_DWORD,
    VALUE_QWORD,
    VALUE_OWORD,
    VALUE_CHARPTR
};

struct UMSKT_Value
{
    ValueType type;
    union {
        BOOL boolean;
        WORD word;
        DWORD32 dword;
        QWORD qword;
        OWORD oword;
        char *chars;
    };
};

enum UMSKT_TAG
{
    UMSKT_tag_isOEM,
    UMSKT_tag_isUpgrade,
    UMSKT_tag_Year,
    UMSKT_tag_Day,
    UMSKT_tag_OEMID,
    UMSKT_tag_AuthData,
    UMSKT_tag_Serial,
    UMSKT_tag_ChannelID,
    UMSKT_tag_InstallationID,
    UMSKT_tag_ProductID
};

class EXPORT UMSKT
{
  public:
    static std::FILE *debug;
    static BOOL VERBOSE;
    static BOOL DEBUG;
    static std::map<UMSKT_TAG, UMSKT_Value> tags;
    static CryptoPP::DefaultAutoSeededRNG rng;

    static void DESTRUCT()
    {
        if (debug != nullptr)
        {
            std::fclose(debug);
        }
        debug = nullptr;
    }

    static void setDebugOutput(std::FILE *input);

    template <typename T> static T getRandom()
    {
        T retval;
        rng.GenerateBlock((BYTE *)&retval, sizeof(retval));
        return retval;
    }

    static const char *VERSION()
    {
        return fmt::format("LIBUMSKT {} compiled on {} {}", LIBUMSKT_VERSION_STRING, __DATE__, __TIME__).c_str();
    }
};

#endif // UMSKT_LIBUMSKT_H
