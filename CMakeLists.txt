CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
PROJECT(WindowsXPKg)
SET(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

if (!OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL Not Found")
endif()

ADD_EXECUTABLE(xpkey xp_algorithm.cpp shared.cpp)
TARGET_INCLUDE_DIRECTORIES(xpkey PUBLIC crypto)
TARGET_LINK_LIBRARIES(xpkey PUBLIC crypto)

ADD_EXECUTABLE(srv2003key server_algorithm.cpp shared.cpp)
TARGET_INCLUDE_DIRECTORIES(srv2003key PUBLIC crypto)
TARGET_LINK_LIBRARIES(srv2003key PUBLIC crypto)
