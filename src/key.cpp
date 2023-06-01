//
// Created by neo on 5/26/2023.
//

#include "header.h"

/* Converts from CD-key to a byte sequence. */
void unbase24(ul32 *byteSeq, const char *cdKey) {
    byte pDecodedKey[PK_LENGTH + NULL_TERMINATOR]{};
    BIGNUM *y = BN_new();

    BN_zero(y);

    // Remove dashes from the CD-key and put it into a Base24 byte array.
    for (int i = 0, k = 0; i < strlen(cdKey) && k < PK_LENGTH; i++) {
        for (int j = 0; j < 24; j++) {
            if (cdKey[i] != '-' && cdKey[i] == charset[j]) {
                pDecodedKey[k++] = j;
                break;
            }
        }
    }

    // Empty byte sequence.
    memset(byteSeq, 0, 16);

    // Calculate the weighed sum of byte array elements.
    for (int i = 0; i < PK_LENGTH; i++) {
        BN_mul_word(y, PK_LENGTH - 1);
        BN_add_word(y, pDecodedKey[i]);
    }

    // Acquire length.
    int n = BN_num_bytes(y);

    // Place the generated code into the byte sequence.
    BN_bn2bin(y, (byte *)byteSeq);
    BN_free(y);

    // Reverse the byte sequence.
    endian((byte *) byteSeq, n);
}

/* Converts from byte sequence to the CD-key. */
void base24(char *cdKey, ul32 *byteSeq) {
    byte rbyteSeq[16];
    BIGNUM *z;

    // Copy byte sequence to the reversed byte sequence.
    memcpy(rbyteSeq, byteSeq, sizeof(rbyteSeq));

    // Skip trailing zeroes and reverse y.
    int length;

    for (length = 15; rbyteSeq[length] == 0; length--);
    endian(rbyteSeq, ++length);

    // Convert reversed byte sequence to BigNum z.
    z = BN_bin2bn(rbyteSeq, length, nullptr);

    // Divide z by 24 and convert the remainder to a CD-key char.
    cdKey[25] = 0;

    for (int i = 24; i >= 0; i--)
        cdKey[i] = charset[BN_div_word(z, 24)];

    BN_free(z);
}