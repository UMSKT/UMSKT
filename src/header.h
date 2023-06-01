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
#include <random>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#define PK_LENGTH           25
#define NULL_TERMINATOR     1

#define FIELD_BITS          384
#define FIELD_BYTES         48
#define FIELD_BITS_2003     512
#define FIELD_BYTES_2003    64

typedef unsigned char byte;
typedef unsigned int  ul32;

extern char charset[];

// util.cpp
void endian(byte *data, int length);
EC_GROUP *initializeEllipticCurve(
        const char *pSel,
        const char *aSel,
        const char *bSel,
        const char *generatorXSel,
        const char *generatorYSel,
        const char *publicKeyXSel,
        const char *publicKeyYSel,
        EC_POINT **genPoint,
        EC_POINT **pubPoint
);

// key.cpp
void unbase24(ul32 *byteSeq, const char *cdKey);
void base24(char *cdKey, ul32 *byteSeq);

// cli.cpp
void print_product_key(char *pk);
void print_product_id(ul32 *pid);

// xp.cpp
bool verifyXPKey(EC_GROUP *eCurve, EC_POINT *generator, EC_POINT *publicKey, char *cdKey);
void generateXPKey(char *pKey, EC_GROUP *eCurve, EC_POINT *generator, BIGNUM *order, BIGNUM *privateKey, ul32 *pRaw);

// server.cpp
int verify2003(EC_GROUP *ec, EC_POINT *generator, EC_POINT *public_key, char *cdkey);
void generate2003(char *pkey, EC_GROUP *ec, EC_POINT *generator, BIGNUM *order, BIGNUM *priv, ul32 *osfamily, ul32 *prefix);

#endif //WINDOWSXPKG_HEADER_H
