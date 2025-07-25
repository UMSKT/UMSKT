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

#ifndef UMSKT_LIBUMSKT_H
#define UMSKT_LIBUMSKT_H

#include "../typedefs.h"

#include <string>
#include <iostream>
#include <sstream>
#include <random>
#include <chrono>
#include <algorithm>
#include <array>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>

#include "sha1/sha1.h"
#include <fmt/core.h>
#include <fmt/format.h>

#ifdef __DJGPP__
#include <sys/time.h>
#endif

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

// RNG utility functions
#ifdef __DJGPP__
#define UMSKT_RNG_DJGPP 1
extern "C" {
    long int random(void);
    int srandom(int seed);
}
#else
#define UMSKT_RNG_DJGPP 0
#endif

class UMSKT {
private:
    static std::mt19937_64& get_rng();

public:
    static std::FILE* debug;
    class PIDGEN2;
    class PIDGEN3;
    class ConfigurationID;

    static void setDebugOutput(std::FILE* input);

    // RNG utility functions
    static int umskt_rand_bytes(unsigned char *buf, int num);
    static int umskt_bn_rand(BIGNUM *rnd, int bits, int top, int bottom);
};

#endif //UMSKT_LIBUMSKT_H
