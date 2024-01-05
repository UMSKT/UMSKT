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

#include <cstdbool>
#include <cstdint>

#ifdef DEBUG
#include <cassert>
#else
#define assert(x) /* do nothing */
#endif

#ifdef _MSC_VER
#define EXPORT extern "C" __declspec(dllexport)
#define INLINE __forceinline
#elif defined(__GNUC__)
#define EXPORT extern "C" __attribute__((visibility("default")))
#define INLINE __attribute__((always_inline))
#else
#define EXPORT extern "C"
#define INLINE
#warning "function inlining not handled"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define FNEXPORT EMSCRIPTEN_KEEPALIVE EXPORT
#else
#define FNEXPORT EXPORT
#endif

#ifdef _MSC_VER
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strcmp
#define strcmp strcmp_s
#endif
#ifndef sscanf
#define sscanf sscanf_s
#endif
#endif

// Type definitions
typedef bool BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;

#ifdef __SIZEOF_INT128__
typedef unsigned __int128 OWORD;
#endif

typedef union {
    // OWORD oword;
    QWORD qword[2];
    DWORD dword[4];
    WORD word[8];
    BYTE byte[16];
} Q_OWORD;

#endif // UMSKT_TYPEDEFS_H
