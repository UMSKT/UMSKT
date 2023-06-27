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

#ifndef UMSKT_TYPEDEFS_H
#define UMSKT_TYPEDEFS_H

#include <cstdint>
#include <cstdbool>

#ifdef DEBUG
#include <cassert>
#else
#define assert(x) /* nothing */
#endif

#ifdef _MSC_VER
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define FNEXPORT EMSCRIPTEN_KEEPALIVE EXPORT
#else
#define FNEXPORT EXPORT
#endif

// Type definitions
typedef bool     BOOL;
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;

#ifdef __SIZEOF_INT128__
typedef unsigned __int128 OWORD;
#endif

#endif //UMSKT_TYPEDEFS_H
