//
// Created by neo on 5/26/2023.
//

#ifndef WINDOWSXPKG_SHARED_H
#define WINDOWSXPKG_SHARED_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>
#include <vector>
#include <unordered_map>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

extern uint8_t cset[];

void endian(uint8_t *data, int len);
void unbase24(uint32_t *x, uint8_t *c);
void base24(uint8_t *c, uint32_t *x);
void print_product_key(uint8_t *pk);
void print_product_id(uint32_t *pid);

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

#endif //WINDOWSXPKG_SHARED_H
