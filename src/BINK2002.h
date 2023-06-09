//
// Created by neo on 6/6/2023.
//

#ifndef WINDOWSXPKG_BINK2002_H
#define WINDOWSXPKG_BINK2002_H

#include "header.h"

class BINK2002 {
    static void Unpack(
            QWORD (&pRaw)[2],
            DWORD &pChannelID,
            DWORD &pHash,
            QWORD &pSignature,
            DWORD &pAuthInfo
    );
    static void Pack(
            QWORD (&pRaw)[2],
            DWORD pChannelID,
            DWORD pHash,
            QWORD pSignature,
            DWORD pAuthInfo
    );

public:
    static bool Verify(
            EC_GROUP *eCurve,
            EC_POINT *basePoint,
            EC_POINT *publicKey,
                char (&cdKey)[25]
    );
    static void Generate(
            EC_GROUP *eCurve,
            EC_POINT *basePoint,
              BIGNUM *genOrder,
              BIGNUM *privateKey,
               DWORD pChannelID,
               DWORD pAuthInfo,
                char (&pKey)[25]
    );
};

#endif //WINDOWSXPKG_BINK2002_H
