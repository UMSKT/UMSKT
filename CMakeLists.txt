# This file is a part of the UMSKT Project
#
# Copyleft (C) 2019-2024 UMSKT Contributors (et.al.)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# @FileCreated by Andrew on 05/30/2023
# @Maintainer Neo

CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(UMSKT)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_OSX_SYSROOT "macosx" CACHE PATH "macOS SDK path")
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

OPTION(BUILD_SHARED_LIBS "Build all libraries as shared" OFF)
OPTION(MUSL_STATIC "Enable fully static builds in a muslc environment (i.e. Alpine Linux)" OFF)
OPTION(DJGPP_WATT32 "Enable compilation and linking with DJGPP/WATT32" OFF)
OPTION(MSVC_MSDOS_STUB "Specify a custom MS-DOS stub for a 32-bit MSVC compilation" OFF)

SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS})
SET(UMSKT_LINK_DIRS ${UMSKT_LINK_DIRS})

IF (NOT MSVC)
    SET(CMAKE_CXX_FLAGS "-Os -fdata-sections -ffunction-sections -flto -Wl,--gc-sections")
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")
    SET(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra")
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -Wl,--gc-sections")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")
ENDIF ()

IF (DJGPP_WATT32)
    SET(CMAKE_SYSTEM_NAME MSDOS)
    SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
    SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS} ${DJGPP_WATT32})
    SET(UMSKT_LINK_DIRS ${UMSKT_LINK_DIRS} ${WATT_ROOT}/lib)
ENDIF ()

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(BUILD_SHARED_LIBS ON)
    MESSAGE(STATUS "[UMSKT] macOS has no static library - Shared library forced on")
ENDIF ()

# if we're compiling with MSVC, respect the DEBUG compile option
IF (MSVC)
    SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    SET(CMAKE_CXX_FLAGS "/O1 /GL /LTCG /Gy /OPT:REF /OPT:ICF /EHsc")
    SET(CMAKE_CXX_FLAGS_RELEASE "/MT")
    SET(CMAKE_CXX_FLAGS_DEBUG "/MTd /Z7")
    SET(CMAKE_EXE_LINKER_FLAGS "/INCREMENTAL:NO /NODEFAULTLIB:MSVCRT /LTCG")
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG")
    SET(CMAKE_ENABLE_EXPORTS ON)
    SET(UMSKT_EXE_WINDOWS_EXTRA src/windows/umskt.rc)
    SET(UMSKT_EXE_WINDOWS_DLL src/windows/dllmain.cpp)
ENDIF ()

IF (MUSL_STATIC)
    MESSAGE(STATUS "[UMSKT] Performing a fully static build using muslc")
    SET(BUILD_SHARED_LIBS OFF)

    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
ENDIF ()

# initialize cpm.CMake
FILE(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494
)
INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# fetch cpm.CMake dependencies
# Include JSON development library
CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.11.3
)

# Include fmt development library
CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 10.2.0
        VERSION 10.2.0
)

# Include cmrc resource compiler
CPMAddPackage(
        NAME cmrc
        GITHUB_REPOSITORY vector-of-bool/cmrc
        GIT_TAG 2.0.1
        VERSION 2.0.1
)

# Include Crypto++ development library
CPMAddPackage(
        NAME cryptopp-cmake
        GITHUB_REPOSITORY abdes/cryptopp-cmake
        GIT_TAG CRYPTOPP_8_9_0
        VERSION 8.9.0
        OPTIONS "CRYPTOPP_BUILD_TESTING OFF"
)

#include googletest unit testing library
#CPMAddPackage(
#        NAME googletest
#        GITHUB_REPOSITORY google/googletest
#        VERSION 1.13.0
#        OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
#)

### Resource compilation
CMRC_ADD_RESOURCE_LIBRARY(umskt-rc ALIAS umskt::rc NAMESPACE umskt keys.json)

SET(LIBUMSKT_PIDGEN2 src/libumskt/pidgen2/PIDGEN2.cpp)
SET(LIBUMSKT_PIDGEN3 src/libumskt/pidgen3/PIDGEN3.cpp src/libumskt/pidgen3/BINK1998.cpp src/libumskt/pidgen3/BINK2002.cpp)
SET(LIBUMSKT_CONFID src/libumskt/confid/confid.cpp src/libumskt/confid/polynomial.cpp src/libumskt/confid/residue.cpp src/libumskt/confid/divisor.cpp)
SET(LIBUMSKT_SRC src/libumskt/libumskt.cpp src/libumskt/debugoutput.cpp ${LIBUMSKT_PIDGEN2} ${LIBUMSKT_PIDGEN3} ${LIBUMSKT_CONFID})
SET(UMSKT_CLI_SRC src/main.cpp src/help.cpp src/cli.cpp src/generate.cpp)
SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS} fmt cryptopp)

IF (NOT MSVC)
    SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS} umskt::rc)
    SET(UMSKT_CLI_SRC ${UMSKT_CLI_SRC} src/rc.cpp)
ELSE()
    SET(UMSKT_CLI_SRC ${UMSKT_CLI_SRC} src/windows/platform.cpp)
ENDIF()

#### Separate Build Path for emscripten
IF (EMSCRIPTEN)
    ADD_EXECUTABLE(umskt ${UMSKT_CLI_SRC} ${LIBUMSKT_SRC})
    TARGET_INCLUDE_DIRECTORIES(umskt PUBLIC ${CMAKE_BINARY_DIR})
    TARGET_LINK_DIRECTORIES(umskt PUBLIC ${UMSKT_LINK_DIRS})
    TARGET_LINK_LIBRARIES(umskt PUBLIC ${UMSKT_LINK_LIBS})
    SET(CMAKE_EXECUTABLE_SUFFIX ".html")

    SET_TARGET_PROPERTIES(umskt PROPERTIES COMPILE_FLAGS "-Os -sEXPORTED_RUNTIME_METHODS=ccall,cwrap")
    SET_TARGET_PROPERTIES(umskt PROPERTIES LINK_FLAGS "-Os -sWASM=1 -sEXPORT_ALL=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap --no-entry")
ELSE ()
    ## umskt.so/.dll creation
    ADD_LIBRARY(libumskt SHARED ${LIBUMSKT_SRC} ${UMSKT_EXE_WINDOWS_EXTRA} ${UMSKT_EXE_WINDOWS_DLL})
    TARGET_INCLUDE_DIRECTORIES(libumskt PUBLIC ${CMAKE_BINARY_DIR})
    TARGET_LINK_DIRECTORIES(libumskt PUBLIC ${UMSKT_LINK_DIRS})
    TARGET_LINK_LIBRARIES(libumskt PUBLIC ${UMSKT_LINK_LIBS})
    IF (MSVC)
        SET_TARGET_PROPERTIES(libumskt PROPERTIES OUTPUT_NAME libumskt)
    ELSE ()
        SET_TARGET_PROPERTIES(libumskt PROPERTIES OUTPUT_NAME umskt)
    ENDIF ()

    ## umskt_static.a/.lib creation
    ADD_LIBRARY(libumskt_static ${LIBUMSKT_SRC} ${UMSKT_EXE_WINDOWS_EXTRA} ${UMSKT_EXE_WINDOWS_DLL})
    TARGET_INCLUDE_DIRECTORIES(libumskt_static PUBLIC ${CMAKE_BINARY_DIR})
    TARGET_LINK_DIRECTORIES(libumskt_static PUBLIC ${UMSKT_LINK_DIRS})
    TARGET_LINK_LIBRARIES(libumskt_static PUBLIC ${UMSKT_LINK_LIBS})
    IF (MSVC)
        SET_TARGET_PROPERTIES(libumskt_static PROPERTIES OUTPUT_NAME libumskt_static)
    ELSE ()
        SET_TARGET_PROPERTIES(libumskt_static PROPERTIES OUTPUT_NAME umskt_static)
    ENDIF ()

    ### UMSKT executable compilation
    IF (NOT BUILD_SHARED_LIBS)
        ## Link against the static lib
        ADD_EXECUTABLE(umskt ${UMSKT_CLI_SRC} ${UMSKT_EXE_WINDOWS_EXTRA})
        TARGET_LINK_LIBRARIES(umskt PUBLIC libumskt_static ${UMSKT_LINK_LIBS} nlohmann_json::nlohmann_json)
    ELSE ()
        ## Link against the dynamic lib
        ADD_EXECUTABLE(umskt ${UMSKT_CLI_SRC} ${UMSKT_EXE_WINDOWS_EXTRA})
        TARGET_LINK_LIBRARIES(umskt PUBLIC libumskt ${UMSKT_LINK_LIBS} nlohmann_json::nlohmann_json)
    ENDIF()
    TARGET_INCLUDE_DIRECTORIES(umskt PUBLIC ${CMAKE_BINARY_DIR})
    TARGET_LINK_DIRECTORIES(umskt PUBLIC ${UMSKT_LINK_DIRS})
    IF (MSVC)
        SET_PROPERTY(TARGET umskt APPEND PROPERTY COMPILE_FLAGS /DUMSKT_CLI_WINRC_EMBED_JSON)
        IF (MSVC_MSDOS_STUB)
            SET_PROPERTY(TARGET umskt APPEND PROPERTY LINK_FLAGS /STUB:${MSVC_MSDOS_STUB})
        ENDIF()
    ENDIF ()

    IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        INSTALL(TARGETS umskt DESTINATION bin)
    ENDIF ()
ENDIF ()
