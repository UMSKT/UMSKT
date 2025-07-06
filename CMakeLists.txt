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

CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(UMSKT)

# Force TDM-GCC on Windows if not using MSVC
if (WIN32 AND NOT MSVC)
    set(CMAKE_C_COMPILER "C:/TDM-GCC-64/bin/gcc.exe" CACHE FILEPATH "C Compiler" FORCE)
    set(CMAKE_CXX_COMPILER "C:/TDM-GCC-64/bin/g++.exe" CACHE FILEPATH "C++ Compiler" FORCE)
    message(STATUS "[UMSKT] Forcing use of TDM-GCC in C:/TDM-GCC-64")
    
    # Configure windres for resource compilation
    set(CMAKE_RC_COMPILER "C:/TDM-GCC-64/bin/windres.exe")
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_FLAGS "--use-temp-file -c65001")
    
    # Match resource architecture with target architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -F pe-i386")
    else()
        set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -F pe-x86-64")
    endif()
    
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff -I${CMAKE_CURRENT_SOURCE_DIR}/src/windows <DEFINES> -i <SOURCE> -o <OBJECT>")
    
    # Set the Windows resource file for GCC builds
    set(UMSKT_EXE_WINDOWS_EXTRA src/windows/umskt.rc)
endif()

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_OSX_SYSROOT "macosx" CACHE PATH "macOS SDK path")

OPTION(BUILD_SHARED_LIBS "Build internal libraries as dynamic" OFF)
OPTION(UMSKT_USE_SHARED_OPENSSL "Force linking against the system-wide OpenSSL library" OFF)
OPTION(MUSL_STATIC "Enable fully static builds in a muslc environment (i.e. Alpine Linux)" OFF)
OPTION(DJGPP_WATT32 "Enable compilation and linking with DJGPP/WATT32/OpenSSL" OFF)
OPTION(MSVC_MSDOS_STUB "Specify a custom MS-DOS stub for a 32-bit MSVC compilation" OFF)
OPTION(WINDOWS_ARM "Enable compilation for Windows on ARM (requires appropriate toolchain)" OFF)

SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS})
SET(UMSKT_LINK_DIRS ${UMSKT_LINK_DIRS})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dead_strip")
endif()

# neither does dos idk i'm trying random stuff
if (DJGPP_WATT32)
    SET(UMSKT_USE_SHARED_OPENSSL ON)
endif()

IF(UMSKT_USE_SHARED_OPENSSL)
    SET(OPENSSL_USE_STATIC_LIBS FALSE)
    SET(OPENSSL_MSVC_STATIC_RT FALSE)
    MESSAGE(STATUS "[UMSKT] Requesting dynamic version of OpenSSL")
ELSE()
    SET(OPENSSL_USE_STATIC_LIBS TRUE)
    SET(OPENSSL_MSVC_STATIC_RT TRUE)
    MESSAGE(STATUS "[UMSKT] Requesting static version of OpenSSL")
ENDIF()

# Configure ARM-specific settings if enabled
if (WINDOWS_ARM)
    if (MSVC)
        # MSVC ARM64 settings
        set(CMAKE_SYSTEM_PROCESSOR ARM64)
        set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE ARM64)
    else()
        # MinGW/GCC ARM settings
        set(CMAKE_SYSTEM_PROCESSOR arm)
        if (CMAKE_CROSSCOMPILING)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
        endif()
    endif()
    message(STATUS "[UMSKT] Configuring for Windows on ARM")
endif()

IF(DJGPP_WATT32)
    SET(ENV{STUBIFY} ":")
    SET(CMAKE_SYSTEM_NAME MSDOS)
    SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
    SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS} ${DJGPP_WATT32})
    SET(UMSKT_LINK_DIRS ${UMSKT_LINK_DIRS} ${WATT_ROOT}/lib)
    
    # Maximum compatibility flags for DOS/NTVDM
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=i386 -mtune=i386 -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -mno-ssse3 -mno-sse3 -mno-sse4 -mno-sse4.1 -mno-sse4.2 -mno-avx -mno-avx2 -mno-fma -mno-fma4")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i386 -mtune=i386 -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -mno-ssse3 -mno-sse3 -mno-sse4 -mno-sse4.1 -mno-sse4.2 -mno-avx -mno-avx2 -mno-fma -mno-fma4")
    
    MESSAGE(STATUS "[UMSKT] Using i386 target for DOS DGJPP with stub: ${CWSDSTUB_LOCATION}")
ENDIF()

##if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
##  SET(BUILD_SHARED_LIBS ON)
##  MESSAGE(STATUS "[UMSKT] macOS has no static library - Shared library forced on")
##endif()

# if we're compiling with MSVC, respect the DEBUG compile option
IF(MSVC)
    SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    IF(NOT BUILD_SHARED_LIBS)
        SET(CMAKE_CXX_FLAGS_RELEASE "/MT")
        SET(CMAKE_CXX_FLAGS_DEBUG "/MTd")
    ELSE()
        SET(CMAKE_CXX_FLAGS_RELEASE "/MD")
        SET(CMAKE_CXX_FLAGS_DEBUG "/MDd")
    ENDIF()
    SET(CMAKE_EXE_LINKER_FLAGS "/INCREMENTAL:NO /NODEFAULTLIB:MSVCRT")
    SET(CMAKE_ENABLE_EXPORTS ON)
    SET(UMSKT_EXE_WINDOWS_EXTRA src/windows/umskt.rc)
    SET(UMSKT_EXE_WINDOWS_DLL src/windows/dllmain.cpp)
ENDIF()

IF(MUSL_STATIC)
    MESSAGE(STATUS "[UMSKT] Performing a fully static build using muslc")
    SET(BUILD_SHARED_LIBS OFF)
    SET(OPENSSL_USE_STATIC_LIBS TRUE)

    SET(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
    SET(CMAKE_SHARED_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
ENDIF()

# find the system installed OpenSSL development library
FIND_PACKAGE(OpenSSL REQUIRED)
IF(NOT OPENSSL_FOUND)
    MESSAGE(SEND_ERROR "OpenSSL Development Libraries Not Found")
    MESSAGE(SEND_ERROR "Please consult your package manager of choice to install the prerequisite")
    MESSAGE(SEND_ERROR "The package name is commonly called libssl-dev or openssl-dev depending on distribution")
    MESSAGE(FATAL_ERROR "Can not continue without OpenSSL")
ENDIF()

IF(NOT MUSL_STATIC)
    # if we found shared libraries - do the following:
    IF (OPENSSL_USE_STATIC_LIBS)
	MESSAGE(STATUS "[UMSKT] requested static version of OpenSSL")
	if (NOT UMSKT_USE_SHARED_OPENSSL)
	    MESSAGE(STATUS "[UMSKT] not asked for shared version of OpenSSL")
	ENDIF()

	IF(MSVC)
	    SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS} "ws2_32.lib")
	    SET(UMSKT_LINK_LIBS ${UMSKT_LINK_LIBS} "crypt32.lib")
	    MESSAGE(STATUS "[UMSKT] msvc adding ws2_32.lib crypt32.lib")
	ENDIF()
    ENDIF()

    STRING(REGEX MATCH "(\\.so|\\.dll|\\.dylib)$" OPENSSL_CRYPTO_SHARED "${OPENSSL_CRYPTO_LIBRARY}")
    IF(OPENSSL_CRYPTO_SHARED)
	MESSAGE(STATUS "[UMSKT] Detected Shared library version of OpenSSL")
    ELSE()
	MESSAGE(STATUS "[UMSKT] Detected Static Library version of OpenSSL")

	# static libcrypto on Fedora needs -lz at link time
	IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	    FIND_PACKAGE(ZLIB REQUIRED)
	    IF (NOT ZLIB_FOUND)
		MESSAGE(FATAL_ERROR "[UMSKT] linux static OpenSSL requires zlib")
	    ENDIF()
	ENDIF()
    ENDIF()
ENDIF()

# initalize cpm.CMake
INCLUDE(cmake/CPM.cmake)

# fetch cpm.CMake dependencies
# Include JSON development library
CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.11.2
)

# Include fmt development library
CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 10.0.0
        VERSION 10.0.0
	#OPTIONS "FMT_INSTALL YES" "BUILD_SHARED_LIBS OFF"
)

# Include cmrc resource compiler
CPMAddPackage(
        NAME cmrc
        GITHUB_REPOSITORY vector-of-bool/cmrc
        GIT_TAG 2.0.1
        VERSION 2.0.1
)

# Include Crypto++ development library
#CPMAddPackage(
#        NAME cryptopp-cmake
#        GITHUB_REPOSITORY abdes/cryptopp-cmake
#        GIT_TAG CRYPTOPP_8_8_0
#        VERSION 8.8.0
#        OPTIONS "CRYPTOPP_BUILD_TESTING OFF"
#)

#include googletest unit testing library
#CPMAddPackage(
#        NAME googletest
#        GITHUB_REPOSITORY google/googletest
#        VERSION 1.13.0
#        OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
#)

### Resource compilation
CMRC_ADD_RESOURCE_LIBRARY(umskt-rc ALIAS umskt::rc NAMESPACE umskt keys.json)

SET(LIBUMSKT_SRC src/libumskt/libumskt.cpp src/libumskt/pidgen3/BINK1998.cpp src/libumskt/pidgen3/BINK2002.cpp src/libumskt/pidgen3/key.cpp src/libumskt/pidgen3/util.cpp src/libumskt/confid/confid.cpp src/libumskt/pidgen2/PIDGEN2.cpp src/libumskt/debugoutput.cpp)

#### Separate Build Path for emscripten
IF (EMSCRIPTEN)
    ADD_EXECUTABLE(umskt ${LIBUMSKT_SRC})
    TARGET_INCLUDE_DIRECTORIES(umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
    TARGET_LINK_LIBRARIES(umskt -static OpenSSL::Crypto cryptopp::cryptopp fmt)
    SET(CMAKE_EXECUTABLE_SUFFIX ".html")

    SET_TARGET_PROPERTIES(umskt PROPERTIES COMPILE_FLAGS "-Os -sEXPORTED_RUNTIME_METHODS=ccall,cwrap")
    SET_TARGET_PROPERTIES(umskt PROPERTIES LINK_FLAGS    "-Os -sWASM=1 -sEXPORT_ALL=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap --no-entry")
ELSE()
    ADD_LIBRARY(_umskt ${LIBUMSKT_SRC} ${UMSKT_EXE_WINDOWS_EXTRA} ${UMSKT_EXE_WINDOWS_DLL})
    TARGET_INCLUDE_DIRECTORIES(_umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
    TARGET_LINK_DIRECTORIES(_umskt PUBLIC ${UMSKT_LINK_DIRS})
    TARGET_LINK_LIBRARIES(_umskt ${OPENSSL_CRYPTO_LIBRARIES} fmt ${UMSKT_LINK_LIBS})

    ### UMSKT executable compilation
    ADD_EXECUTABLE(umskt src/main.cpp src/cli.cpp ${UMSKT_EXE_WINDOWS_EXTRA})
    TARGET_INCLUDE_DIRECTORIES(umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
    TARGET_LINK_LIBRARIES(umskt _umskt ${OPENSSL_CRYPTO_LIBRARIES} ${ZLIB_LIBRARIES} fmt nlohmann_json::nlohmann_json umskt::rc ${UMSKT_LINK_LIBS})
    TARGET_LINK_DIRECTORIES(umskt PUBLIC ${UMSKT_LINK_DIRS})

    # Link required Windows system libraries for OpenSSL
    if (WIN32)
        target_link_libraries(umskt crypt32 ws2_32)
    endif()

    IF(MSVC AND MSVC_MSDOS_STUB)
        SET_PROPERTY(TARGET umskt APPEND PROPERTY LINK_FLAGS /STUB:${MSVC_MSDOS_STUB})
    ENDIF()

    IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        install(TARGETS umskt DESTINATION bin)
    ENDIF()

    ### Copy Shared Libraries and dependency files
    IF (OPENSSL_CRYPTO_SHARED)
        GET_FILENAME_COMPONENT(OPENSSL_CRYPTO_LIBRARY_FILENAME ${OPENSSL_CRYPTO_LIBRARY} NAME)
        CONFIGURE_FILE(${OPENSSL_CRYPTO_LIBRARY} "${CMAKE_CURRENT_BINARY_DIR}/${OPENSSL_CRYPTO_LIBRARY_FILENAME}" COPYONLY)
    ENDIF()

    IF (DJGPP_WATT32)
        add_custom_command(TARGET umskt POST_BUILD 
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:umskt> $<TARGET_FILE_DIR:umskt>/umskt.tmp
            COMMAND ${STUBIFY_LOCATION} ${CWSDSTUB_LOCATION} $<TARGET_FILE_DIR:umskt>/umskt.tmp $<TARGET_FILE:umskt>
            COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE_DIR:umskt>/umskt.tmp
        )
    ENDIF()
ENDIF()
