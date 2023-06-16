CMAKE_MINIMUM_REQUIRED(VERSION 3.9)
PROJECT(WindowsXPKg)
SET(CMAKE_CXX_STANDARD 17)

SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")
SET(OPENSSL_USE_STATIC_LIBS TRUE)
SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

FIND_PACKAGE(OpenSSL REQUIRED)

IF(!OPENSSL_FOUND)
    MESSAGE(SEND_ERROR "OpenSSL Development Libraries Not Found")
    MESSAGE(SEND_ERROR "Please consult your package manager of choice to install the prerequisite")
    MESSAGE(SEND_ERROR "The package name is commonly called libssl-dev or openssl-dev depending on distribution")
    MESSAGE(FATAL_ERROR "Can not continue without OpenSSL")
ENDIF()

INCLUDE(cmake/CPM.cmake)

# Include JSON runtime library
CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.11.2
)

CPMAddPackage("gh:fmtlib/fmt#7.1.3")

CONFIGURE_FILE(keys.json keys.json COPYONLY)

ADD_LIBRARY(BINK1998 STATIC src/BINK1998.cpp)
TARGET_INCLUDE_DIRECTORIES(BINK1998 PUBLIC crypto)
TARGET_LINK_LIBRARIES(BINK1998 OpenSSL::Crypto nlohmann_json::nlohmann_json fmt)

ADD_LIBRARY(BINK2002 STATIC src/BINK2002.cpp)
TARGET_INCLUDE_DIRECTORIES(BINK2002 PUBLIC crypto)
TARGET_LINK_LIBRARIES(BINK2002 OpenSSL::Crypto nlohmann_json::nlohmann_json fmt)

ADD_LIBRARY(CONFID STATIC src/confid.cpp)
TARGET_INCLUDE_DIRECTORIES(CONFID PUBLIC crypto)
TARGET_LINK_LIBRARIES(CONFID OpenSSL::Crypto nlohmann_json::nlohmann_json fmt)

ADD_EXECUTABLE(xpkey src/main.cpp src/key.cpp src/util.cpp src/cli.cpp)
TARGET_INCLUDE_DIRECTORIES(xpkey PUBLIC crypto)
TARGET_LINK_LIBRARIES(xpkey BINK1998 BINK2002 CONFID OpenSSL::Crypto nlohmann_json::nlohmann_json fmt)
