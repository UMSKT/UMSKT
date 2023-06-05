CMAKE_MINIMUM_REQUIRED(VERSION 3.9)
PROJECT(WindowsXPKg)
SET(CMAKE_CXX_STANDARD 17)

# TODO: commenting out static builds for now
#SET(OPENSSL_USE_STATIC_LIBS TRUE)
FIND_PACKAGE(OpenSSL REQUIRED)

IF(!OPENSSL_FOUND)
    MESSAGE(FATAL_ERROR "OpenSSL Development Libraries Not Found")
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

#SET(BUILD_SHARED_LIBS OFF)
#SET(CMAKE_EXE_LINKER_FLAGS "-static")
ADD_EXECUTABLE(xpkey src/main.cpp src/xp.cpp src/key.cpp src/util.cpp src/cli.cpp)
TARGET_INCLUDE_DIRECTORIES(xpkey PUBLIC crypto)
TARGET_LINK_LIBRARIES(xpkey PUBLIC OpenSSL::Crypto nlohmann_json::nlohmann_json fmt)

ADD_EXECUTABLE(srv2003key src/server.cpp src/key.cpp src/util.cpp src/cli.cpp)
TARGET_INCLUDE_DIRECTORIES(srv2003key PUBLIC crypto)
TARGET_LINK_LIBRARIES(srv2003key PUBLIC OpenSSL::Crypto nlohmann_json::nlohmann_json fmt)

ADD_EXECUTABLE(xpactivate src/confid.cpp)
TARGET_INCLUDE_DIRECTORIES(xpactivate PUBLIC crypto)
TARGET_LINK_LIBRARIES(xpactivate PUBLIC OpenSSL::Crypto nlohmann_json::nlohmann_json fmt)