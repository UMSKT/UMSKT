CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
PROJECT(WindowsXPKg)
SET(CMAKE_CXX_STANDARD 17)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

if (!OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL Development Libraries Not Found")
endif()

include(cmake/CPM.cmake)

# include json runtime library
CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.11.2)

configure_file(keys.json keys.json COPYONLY)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")
ADD_EXECUTABLE(xpkey main.cpp xp.cpp key.cpp util.cpp cli.cpp)
TARGET_INCLUDE_DIRECTORIES(xpkey PUBLIC crypto)
TARGET_LINK_LIBRARIES(xpkey PUBLIC crypto nlohmann_json::nlohmann_json)

ADD_EXECUTABLE(srv2003key server.cpp key.cpp util.cpp cli.cpp)
TARGET_INCLUDE_DIRECTORIES(srv2003key PUBLIC crypto)
TARGET_LINK_LIBRARIES(srv2003key PUBLIC crypto nlohmann_json::nlohmann_json)