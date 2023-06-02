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
#include <iostream>
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


extern char charset[];

// util.cpp
void endian(uint8_t *data, int length);
EC_GROUP *initializeEllipticCurve(
        const std::string pSel,
        const std::string aSel,
        const std::string bSel,
        const std::string generatorXSel,
        const std::string generatorYSel,
        const std::string publicKeyXSel,
        const std::string publicKeyYSel,
        EC_POINT **genPoint,
        EC_POINT **pubPoint
);

// key.cpp
void unbase24(uint32_t *byteSeq, const char *cdKey);
void base24(char *cdKey, uint32_t *byteSeq);

// cli.cpp
void print_product_key(char *pk);
void print_product_id(uint32_t *pid);

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
bool verifyXPKey(EC_GROUP *eCurve, EC_POINT *generator, EC_POINT *publicKey, char *cdKey);
void generateXPKey(char *pKey, EC_GROUP *eCurve, EC_POINT *generator, BIGNUM *order, BIGNUM *privateKey, uint32_t *pRaw);

// server.cpp
int verify2003(EC_GROUP *ec, EC_POINT *generator, EC_POINT *public_key, char *cdkey);
void generate2003(char *pkey, EC_GROUP *ec, EC_POINT *generator, BIGNUM *order, BIGNUM *priv, uint32_t *osfamily, uint32_t *prefix);

#endif //WINDOWSXPKG_HEADER_H
