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
 * @FileCreated by Neo on 5/26/2023
 * @Maintainer Neo
 */

#ifndef UMSKT_HEADER_H
#define UMSKT_HEADER_H

#ifdef DEBUG
#include <cassert>
#else
#define assert(x) /* nothing */
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <random>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

// Algorithm macros
#define PK_LENGTH               25
#define NULL_TERMINATOR         1

#define FIELD_BITS              384
#define FIELD_BYTES             48
#define FIELD_BITS_2003         512
#define FIELD_BYTES_2003        64

#define SHA_MSG_LENGTH_XP       (4 + 2 * FIELD_BYTES)
#define SHA_MSG_LENGTH_2003     (3 + 2 * FIELD_BYTES_2003)

#define NEXTSNBITS(field, n, offset)   (((QWORD)(field) >> (offset)) & ((1ULL << (n)) - 1))
#define FIRSTNBITS(field, n)           NEXTSNBITS((field), (n), 0)

#define HIBYTES(field, bytes)          NEXTSNBITS((QWORD)(field), ((bytes) * 8), ((bytes) * 8))
#define LOBYTES(field, bytes)          FIRSTNBITS((QWORD)(field), ((bytes) * 8))

#define BYDWORD(n)                     (DWORD)(*((n) + 0) | *((n) + 1) << 8 | *((n) + 2) << 16 | *((n) + 3) << 24)
#define BITMASK(n)                     ((1ULL << (n)) - 1)

using json = nlohmann::json;
namespace fs = std::filesystem;

enum MODE {
    MODE_BINK1998 = 0,
    MODE_BINK2002 = 1,
    MODE_CONFIRMATION_ID = 2,
};

struct Options {
    std::string binkid;
    std::string keysFilename;
    std::string instid;
    int channelID;
    int numKeys;
    bool verbose;
    bool help;
    bool error;
    bool list;

    MODE applicationMode;
};

// Type definitions
typedef bool     BOOL;
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;

#ifdef __SIZEOF_INT128__
typedef unsigned __int128 OWORD;
#endif

// Global variables
extern Options options;

// util.cpp
int  BN_bn2lebin(const BIGNUM *a, unsigned char *to, int tolen); // Hello OpenSSL developers, please tell me, where is this function at?
void endian(BYTE *data, int length);

EC_GROUP *initializeEllipticCurve(
        std::string pSel,
        std::string aSel,
        std::string bSel,
        std::string generatorXSel,
        std::string generatorYSel,
        std::string publicKeyXSel,
        std::string publicKeyYSel,
           EC_POINT *&genPoint,
           EC_POINT *&pubPoint
);

// key.cpp
void unbase24(BYTE *byteSeq, const char *cdKey);
void base24(char *cdKey, BYTE *byteSeq);


#endif //UMSKT_HEADER_H
