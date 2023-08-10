# This file is a part of the UMSKT Project
#
# Copyleft (C) 2019-2023 UMSKT Contributors (et.al.)
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

cmake_minimum_required(VERSION 3.12)

project(UMSKT)
set(CMAKE_OSX_SYSROOT "macosx" CACHE PATH "macOS SDK path")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(UMSKT_USE_SHARED_OPENSSL "Force linking against the system-wide OpenSSL library" OFF)
option(MUSL_STATIC "Enable fully static builds in a muslc environment (i.e. Alpine Linux)" OFF)
option(DJGPP_WATT32 "Enable compilation and linking with DJGPP/WATT32/OpenSSL" OFF)
option(MSVC_MSDOS_STUB "Specify a custom MS-DOS stub for a 32-bit MSVC compilation" OFF)

find_package(OpenSSL REQUIRED)

if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL Development Libraries Not Found. Please install the required OpenSSL development package.")
endif()

if(UMSKT_USE_SHARED_OPENSSL)
    set(OPENSSL_USE_STATIC_LIBS FALSE)
    set(OPENSSL_MSVC_STATIC_RT FALSE)
else()
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(OPENSSL_MSVC_STATIC_RT TRUE)
endif()

# if we found shared libraries - do the following:
string(REGEX MATCH "(\\.so|\\.dll|\\.dylib)$" OPENSSL_CRYPTO_SHARED "${OPENSSL_CRYPTO_LIBRARY}")
if(OPENSSL_CRYPTO_SHARED)
    message(STATUS "[UMSKT] Detected Shared library version of OpenSSL")
    set(BUILD_SHARED_LIBS ON)
else()
    message(STATUS "[UMSKT] Detected Static Library version of OpenSSL")
endif()

# if we're compiling with MSVC, respect the DEBUG compile option
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    if(NOT BUILD_SHARED_LIBS)
        set(CMAKE_CXX_FLAGS_RELEASE "/MT")
        set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "/MD")
        set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "/INCREMENTAL:NO /NODEFAULTLIB:MSVCRT")
    set(CMAKE_ENABLE_EXPORTS ON)
    set(UMSKT_EXE_WINDOWS_EXTRA src/windows/umskt.rc)
    set(UMSKT_EXE_WINDOWS_DLL src/windows/dllmain.cpp)
endif()

if(MUSL_STATIC AND NOT UMSKT_USE_SHARED_OPENSSL)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    VERSION 3.11.2
)

CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY fmtlib/fmt
    GIT_TAG 10.0.0
    VERSION 10.0.0
)

CPMAddPackage(
    NAME cmrc
    GITHUB_REPOSITORY vector-of-bool/cmrc
    GIT_TAG 2.0.1
    VERSION 2.0.1
)

# For Emscripten builds, set CMAKE_TOOLCHAIN_FILE to the appropriate file
set(EMSCRIPTEN_BUILD OFF CACHE BOOL "Build for Emscripten" FORCE)
if(EMSCRIPTEN_BUILD)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/Emscripten.cmake" CACHE string "Emscripten toolchain file")
endif()

cmrc_add_resource_library(umskt-rc ALIAS umskt::rc NAMESPACE umskt keys.json)

set(LIBUMSKT_SRC
    src/libumskt/libumskt.cpp
    src/libumskt/pidgen3/BINK1998.cpp
    src/libumskt/pidgen3/BINK2002.cpp
    src/libumskt/pidgen3/key.cpp
    src/libumskt/pidgen3/util.cpp
    src/libumskt/confid/confid.cpp
    src/libumskt/pidgen2/PIDGEN2.cpp
    src/libumskt/debugoutput.cpp
)

#### Separate Build Path for emscripten
if (EMSCRIPTEN)
    add_executable(umskt ${LIBUMSKT_SRC})
    target_include_directories(umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(umskt -static OpenSSL::Crypto cryptopp::cryptopp fmt)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set_target_properties(umskt PROPERTIES COMPILE_FLAGS "-Os -sEXPORTED_RUNTIME_METHODS=ccall,cwrap")
    set_target_properties(umskt PROPERTIES LINK_FLAGS    "-Os -sWASM=1 -sEXPORT_ALL=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap --no-entry")
else()
    if(NOT UMSKT_USE_SHARED_OPENSSL)
        ### Static library compilation
        add_library(_umskt STATIC ${LIBUMSKT_SRC})
        target_include_directories(_umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(_umskt -static OpenSSL::Crypto fmt ${UMSKT_LINK_LIBS})
    else()
        ### Shared library compilation
        add_library(_umskt SHARED ${LIBUMSKT_SRC} ${UMSKT_EXE_WINDOWS_EXTRA} ${UMSKT_EXE_WINDOWS_DLL})
        target_include_directories(_umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(_umskt OpenSSL::Crypto fmt ${UMSKT_LINK_LIBS})
        target_link_directories(_umskt PUBLIC ${UMSKT_LINK_DIRS})
    endif()

    ### UMSKT executable compilation
    add_executable(umskt src/main.cpp src/cli.cpp ${UMSKT_EXE_WINDOWS_EXTRA})
    target_include_directories(umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(umskt _umskt OpenSSL::Crypto fmt nlohmann_json::nlohmann_json umskt::rc ${UMSKT_LINK_LIBS})
    target_link_directories(umskt PUBLIC ${UMSKT_LINK_DIRS})
    if(MSVC AND MSVC_MSDOS_STUB)
        set_property(TARGET umskt APPEND PROPERTY LINK_FLAGS /STUB:${MSVC_MSDOS_STUB})
    endif()

    ### Copy Shared Libraries and dependency files
    if (OPENSSL_CRYPTO_SHARED)
        get_filename_component(OPENSSL_CRYPTO_LIBRARY_FILENAME ${OPENSSL_CRYPTO_LIBRARY} NAME)
        configure_file(${OPENSSL_CRYPTO_LIBRARY} "${CMAKE_CURRENT_BINARY_DIR}/${OPENSSL_CRYPTO_LIBRARY_FILENAME}" COPYONLY)
    endif()
endif()
