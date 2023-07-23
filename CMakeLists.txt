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

CMRC_ADD_RESOURCE_LIBRARY(umskt-rc ALIAS umskt::rc NAMESPACE umskt keys.json)

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

if(UMSKT_USE_SHARED_OPENSSL)
    add_library(_umskt SHARED ${LIBUMSKT_SRC})
else()
    add_library(_umskt STATIC ${LIBUMSKT_SRC})
endif()

target_include_directories(_umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(_umskt PRIVATE OpenSSL::Crypto fmt)

add_executable(umskt src/main.cpp src/cli.cpp ${UMSKT_EXE_WINDOWS_EXTRA})

target_include_directories(umskt PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(umskt PRIVATE _umskt OpenSSL::Crypto fmt nlohmann_json::nlohmann_json umskt::rc)
