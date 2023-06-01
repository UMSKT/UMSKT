CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
PROJECT(WindowsXPKg)
SET(CMAKE_CXX_STANDARD 17)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

if (!OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL Development Libraries Not Found")
endif()

# generate bink.h
add_custom_command(
        OUTPUT bink.h
        COMMAND ${PROJECT_SOURCE_DIR}/convert_keys_to_cpp.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS bink.h
)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")
ADD_EXECUTABLE(xpkey main.cpp xp.cpp key.cpp util.cpp cli.cpp bink.h)
TARGET_INCLUDE_DIRECTORIES(xpkey PUBLIC crypto)
TARGET_LINK_LIBRARIES(xpkey PUBLIC crypto)
add_dependencies(xpkey bink.h)

ADD_EXECUTABLE(srv2003key server.cpp key.cpp util.cpp cli.cpp bink.h)
TARGET_INCLUDE_DIRECTORIES(srv2003key PUBLIC crypto)
TARGET_LINK_LIBRARIES(srv2003key PUBLIC crypto)
add_dependencies(srv2003key bink.h)