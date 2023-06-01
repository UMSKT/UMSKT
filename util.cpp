//
// Created by Andrew on 01/06/2023.
//

#include "header.h"

/* Convert data between endianness types. */
void endian(byte *data, int length) {
    for (int i = 0; i < length / 2; i++) {
        byte temp = data[i];
        data[i] = data[length - i - 1];
        data[length - i - 1] = temp;
    }
}
