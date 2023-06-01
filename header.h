//
// Created by neo on 5/26/2023.
//

#ifndef WINDOWSXPKG_HEADER_H
#define WINDOWSXPKG_HEADER_H

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <unordered_map>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#include "bink.h"

#define PK_LENGTH           25
#define NULL_TERMINATOR     1

#define FIELD_BITS          384
#define FIELD_BYTES         48
#define FIELD_BITS_2003     512
#define FIELD_BYTES_2003    64

typedef unsigned char byte;
typedef unsigned long ul32;

extern char charset[];

// util.cpp
void endian(byte *data, int length);

// key.cpp
void unbase24(ul32 *byteSeq, char *cdKey);
void base24(char *cdKey, ul32 *byteSeq);

// cli.cpp
void print_product_key(char *pk);
void print_product_id(ul32 *pid);

// xp.cpp
bool verifyXPKey(EC_GROUP *eCurve, EC_POINT *generator, EC_POINT *publicKey, char *cdKey);
void generateXPKey(char *pKey, EC_GROUP *eCurve, EC_POINT *generator, BIGNUM *order, BIGNUM *privateKey, ul32 *pRaw);

// server.cpp

struct ECDLP_Params {
    //         p,           a,           b
    std::tuple<std::string, std::string, std::string> E;

    //         x,           y
    std::tuple<std::string, std::string> K;

    //         x,           y
    std::tuple<std::string, std::string> G;

    std::string n;
    std::string k;
};

struct ProductID {
    uint8_t SiteID;
    uint16_t Serial;
};

extern std::unordered_map<std::string, std::unordered_map<int, std::string>> Products;
extern std::unordered_map<std::string, ECDLP_Params> BINKData;
void initBink();

#endif //WINDOWSXPKG_HEADER_H
