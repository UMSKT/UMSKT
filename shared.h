//
// Created by neo on 5/26/2023.
//

#ifndef WINDOWSXPKG_SHARED_H
#define WINDOWSXPKG_SHARED_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

#endif //WINDOWSXPKG_SHARED_H
