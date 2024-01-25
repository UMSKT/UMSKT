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

#ifndef UMSKT_TYPEDEFS_H
#define UMSKT_TYPEDEFS_H

#if defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#include <intrin.h>
#include <windows.h>

#endif // defined(WIN32)

#include <algorithm>
#include <cstdbool>
#include <cstdint>
#include <map>
#include <vector>

#ifdef DEBUG
#include <cassert>
#else
#define assert(x) /* do nothing */
#endif

#if defined(_MSC_VER)
#define FNEXPORT __declspec(dllexport)
#define FNIMPORT __declspec(dllimport)
#define FNINLINE __forceinline
#elif defined(__GNUC__)
#define FNEXPORT __attribute__((visibility("default")))
#define FNIMPORT __attribute__((visibility("default")))
#define FNINLINE inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
#define forceinline inline __attribute__((__always_inline__))
#else
#define forceinline inline
#endif // __has_attribute(__always_inline__)
#else
#define FNEXPORT
#define FNINLINE
#warning "function inlining not handled"
#endif // defined(_MSC_VER)

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE FNEXPORT
#define INLINE FNINLINE
#else
#ifdef UMSKT_IMPORT_LIB
#define EXPORT FNIMPORT
#else
#define EXPORT FNEXPORT
#endif // ifdef UMSKT_IMPORT_LIB
#define INLINE FNINLINE
#endif // ifdef  __EMSCRIPTEN__

// POSIX <-> Windows compatability layer, because MS just *had* to be different
#ifdef _MSC_VER
#ifndef _sscanf
#define _sscanf sscanf_s
#endif
#ifndef _strncpy
#define _strncpy strncpy_s
#endif
#ifndef _strcpy
#define _strcpy strcpy_s
#endif
#else
#define _sscanf sscanf
#define _strncpy(x, y, z, w) strncpy(x, z, w)
#define _strcpy strcpy
#endif // ifdef _MSC_VER

// Type definitions now with more windows compatability (unfortunately)
using BOOL = int32_t;
using BYTE = uint8_t;
using WORD = uint16_t;
using DWORD = unsigned long;
using QWORD = uint64_t;

#if defined(_M_ARM) // for Windows on ARM ??
using __m128 = __n128;
#endif

#if defined(__SIZEOF_INT128__) || defined(__int128)
using uint128_t = unsigned __int128;
#else // use the intel-supplied __m128 intrisic
using uint128_t = __m128;
#endif
using OWORD = uint128_t;

typedef union {
    OWORD oword;
    QWORD qword[2];
    DWORD dword[4];
    WORD word[8];
    BYTE byte[16];
} Q_OWORD;

#endif // UMSKT_TYPEDEFS_H
