//
// Created by neo on 5/26/2023.
//

#ifndef WINDOWSXPKG_HEADER_H
#define WINDOWSXPKG_HEADER_H

#ifdef DEBUG
#include <cassert>
#else
#define assert(x) /* nothing */
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <random>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

// Algorithm macros
#define PK_LENGTH               25
#define NULL_TERMINATOR         1

#define FIELD_BITS              384
#define FIELD_BYTES             48
#define FIELD_BITS_2003         512
#define FIELD_BYTES_2003        64

#define SHA_MSG_LENGTH_XP       (4 + 2 * FIELD_BYTES)
#define SHA_MSG_LENGTH_2003     (3 + 2 * FIELD_BYTES_2003)

#define NEXTSNBITS(field, n, offset)   (((QWORD)field >> offset) & ((1ULL << (n)) - 1))
#define FIRSTNBITS(field, n)           NEXTSNBITS(field, n, 0)

#define BYDWORD(n)                     (*(n + 0) | *(n + 1) << 8 | *(n + 2) << 16 | *(n + 3) << 24)
#define BITMASK(n)                     ((1ULL << n) - 1)

// Confirmation ID generator constants
#define SUCCESS 0
#define ERR_TOO_SHORT 1
#define ERR_TOO_LARGE 2
#define ERR_INVALID_CHARACTER 3
#define ERR_INVALID_CHECK_DIGIT 4
#define ERR_UNKNOWN_VERSION 5
#define ERR_UNLUCKY 6

// Type definitions
typedef bool     BOOL;
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;

// Global variables
extern char pCharset[];

// util.cpp
int  BN_bn2lebin(const BIGNUM *a, unsigned char *to, int tolen); // Hello OpenSSL developers, please tell me, where is this function at?
void endian(BYTE *data, int length);

EC_GROUP *initializeEllipticCurve(
        std::string pSel,
        std::string aSel,
        std::string bSel,
        std::string generatorXSel,
        std::string generatorYSel,
        std::string publicKeyXSel,
        std::string publicKeyYSel,
        EC_POINT    *&genPoint,
        EC_POINT    *&pubPoint
);

// key.cpp
void unbase24(BYTE *byteSeq, const char *cdKey);
void base24(char *cdKey, BYTE *byteSeq);

// cli.cpp
void print_product_key(char *pk);
void print_product_id(DWORD *pid);

struct Options {
    std::string binkid;
    int channelID;
    bool verbose;
    bool help;
    bool list;
    bool error;
};

Options parseCommandLine(int argc, char* argv[]);
void showHelp(char *argv[]);

// xp.cpp
bool verifyXPKey(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        EC_POINT *publicKey,
        char     (&cdKey)[25]
);

void generateXPKey(
        EC_GROUP *eCurve,
        EC_POINT *basePoint,
        BIGNUM   *genOrder,
        BIGNUM   *privateKey,
        DWORD    pSerial,
        char     (&cdKey)[25]
);

// server.cpp

#endif //WINDOWSXPKG_HEADER_H
