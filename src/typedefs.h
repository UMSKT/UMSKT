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

// squelch the insane amount of warnings from cstdbool
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#endif // defined(_MSC_VER)

#include <algorithm>
#include <cctype>
#include <cstdbool>
#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <stdint128>
#include <string>
#include <vector>

#if defined(DEBUG) || 1
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

// Type definitions now with more windows compatability (unfortunately)
using BOOL = int32_t;
using BYTE = uint8_t;
using WORD = uint16_t;
using DWORD = unsigned long;
using DWORD32 = uint32_t;
using QWORD = uint64_t;
using OWORD = uint128_t;

typedef union {
    OWORD oword;
    QWORD qword[2];
    DWORD32 dword32[4];
    WORD word[8];
    BYTE byte[16];
} Q_OWORD;

#endif // UMSKT_TYPEDEFS_H
