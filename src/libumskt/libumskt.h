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

#ifdef __DJGPP__
#include <time.h>
#endif

#include <cryptopp/cryptlib.h>
#include <cryptopp/ecp.h>
#include <cryptopp/integer.h>
#include <cryptopp/misc.h>
#include <cryptopp/nbtheory.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>

using ECP = CryptoPP::ECP;
using SHA1 = CryptoPP::SHA1;
using Integer = CryptoPP::Integer;

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

// fmt <-> CryptoPP linkage
template <> class fmt::formatter<Integer>
{
    char type_ = 'd';

  public:
    constexpr auto parse(format_parse_context &ctx)
    {
        auto i = ctx.begin(), end = ctx.end();

        if (i != end)
        {
            switch (*i)
            {
            case 'B':
            case 'b':
            case 'o':
            case 'X':
            case 'x':
            case 'd':
                type_ = *i++;
            }
        }

        if (i != end && *i != '}')
        {
            throw format_error("invalid format");
        }
        return i;
    }

    template <typename FmtContext> constexpr auto format(const Integer &i, FmtContext &ctx) const
    {
        switch (type_)
        {
        case 'B':
        case 'b':
            return format_to(ctx.out(), "{}", IntToString(i, 2));

        case 'o':
            return format_to(ctx.out(), "{}", IntToString(i, 8));

        case 'X':
        case 'x':
            return format_to(ctx.out(), "{}", IntToString(i, 16));

        case 'd':
        default:
            return format_to(ctx.out(), "{}", IntToString(i, 10));
        }
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
    /**
     * Convert a std::string to an Integer
     *
     * @param in
     * @return
     */
    INLINE static Integer IntegerS(const std::string &in)
    {
        return Integer(&in[0]);
    }

    /**
     * Convert a std::string to an Integer
     *
     * @param in
     * @return
     */
    INLINE static Integer IntegerHexS(const std::string &in)
    {
        return IntegerS("0x" + in);
    }

    /**
     * Convert Native byte buffer to Integer
     *
     * @param buf
     * @param size
     * @return
     */
    INLINE static Integer IntegerN(BYTE *buf, size_t size)
    {
        return {buf, size, Integer::UNSIGNED, CryptoPP::LITTLE_ENDIAN_ORDER};
    }

    /**
     * Convert Native Type T to Integer, where T is a concrete type
     *
     * @tparam T
     * @param in
     * @return
     */
    template <typename T> INLINE static Integer IntegerN(const T &in)
    {
        return IntegerN((BYTE *)&in, sizeof(T));
    }

    /**
     * Encode Integer to a Native byte buffer
     *
     * @param in
     * @param buf
     * @param buflen
     * @return
     */
    INLINE static BYTE *EncodeN(const Integer &in, BYTE *buf, size_t buflen)
    {
        in.Encode(buf, buflen);
        std::reverse(buf, buf + buflen);
        return buf + buflen;
    }

    /**
     * Encode Integer to Native type T where T is a concrete type
     *
     * @tparam T
     * @param in
     * @param buf
     * @return
     */
    template <typename T> INLINE static BYTE *EncodeN(const Integer &in, T &buf)
    {
        return EncodeN(in, (BYTE *)&buf, sizeof(T));
    }

    /**
     * Encode a random number into a Native concrete type
     *
     * @tparam T
     * @return
     */
    template <typename T> INLINE static T getRandom()
    {
        T retval;
        rng.GenerateBlock((BYTE *)&retval, sizeof(retval));
        return retval;
    }

    INLINE static std::string strtolower(std::string &in)
    {
        auto retval = std::string(in);
        std::transform(retval.begin(), retval.end(), retval.begin(), ::tolower);
        return retval;
    }

    INLINE static std::string strtoupper(const std::string &in)
    {
        auto retval = std::string(in);
        std::transform(retval.begin(), retval.end(), retval.begin(), ::toupper);
        return retval;
    }

    /**
     * Gets the compiled-in version information
     *
     * @return Null-Terminated C-Style string pointer
     */
    INLINE static const std::string VERSION()
    {
        return fmt::format("LIBUMSKT {} compiled on {} {}", LIBUMSKT_VERSION_STRING, __DATE__, __TIME__);
    }

    static std::FILE *debug;
    static std::FILE *verbose;
    static BOOL IS_CONSTRUCTED;
    static std::map<UMSKT_TAG, UMSKT_Value> tags;
    static CryptoPP::DefaultAutoSeededRNG rng;

    static BOOL CONSTRUCT();

    static void DESTRUCT()
    {
        if (debug != nullptr && debug != stdout && debug != stderr)
        {
            std::fclose(debug);
            debug = nullptr;
        }
        if (verbose != nullptr && verbose != stdout && debug != stderr)
        {
            std::fclose(verbose);
            verbose = nullptr;
        }
    }

    static void setDebugOutput(std::FILE *input);
    static void setVerboseOutput(std::FILE *input);
};

#endif // UMSKT_LIBUMSKT_H
