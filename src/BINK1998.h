//
// Created by neo on 6/6/2023.
//

#ifndef WINDOWSXPKG_BINK1998_H
#define WINDOWSXPKG_BINK1998_H

#include "header.h"

class BINK1998 {
    static void Unpack(
            QWORD (&pRaw)[2],
            DWORD &pSerial,
            DWORD &pHash,
            QWORD &pSignature
    );
    static void Pack(
            QWORD (&pRaw)[2],
            DWORD pSerial,
            DWORD pHash,
            QWORD pSignature
    );

public:
    static bool Verify(
            EC_GROUP *eCurve,
            EC_POINT *basePoint,
            EC_POINT *publicKey,
                char (&pKey)[25]
    );
    static void Generate(
            EC_GROUP *eCurve,
            EC_POINT *basePoint,
              BIGNUM *genOrder,
              BIGNUM *privateKey,
               DWORD pSerial,
                char (&pKey)[25]
    );
};

#endif //WINDOWSXPKG_BINK1998_H
