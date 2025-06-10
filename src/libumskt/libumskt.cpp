/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2023 UMSKT Contributors (et.al.)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @FileCreated by Neo on 6/25/2023
 * @Maintainer Neo
 */

#include "libumskt.h"
#include "confid/confid.h"
#include "pidgen3/PIDGEN3.h"
#include "pidgen3/BINK1998.h"
#include "pidgen3/BINK2002.h"
#include "pidgen2/PIDGEN2.h"

FNEXPORT int ConfirmationID_Generate(const char* installation_id_str, char confirmation_id[49], int mode, std::string productid, bool bypassVersion) {
    return ConfirmationID::Generate(installation_id_str, confirmation_id, mode, productid, bypassVersion);
}

FNEXPORT EC_GROUP* PIDGEN3_initializeEllipticCurve(char* pSel, char* aSel, char* bSel, char* generatorXSel, char* generatorYSel, char* publicKeyXSel, char* publicKeyYSel, EC_POINT *&genPoint, EC_POINT *&pubPoint) {
    return PIDGEN3::initializeEllipticCurve(pSel, aSel, bSel, generatorXSel, generatorYSel, publicKeyXSel, publicKeyYSel, genPoint, pubPoint);
}

FNEXPORT bool PIDGEN3_BINK1998_Verify(EC_GROUP *eCurve, EC_POINT *basePoint, EC_POINT *publicKey, char (&pKey)[25]) {
    return PIDGEN3::BINK1998::Verify(eCurve, basePoint, publicKey, pKey);
}

FNEXPORT void PIDGEN3_BINK1998_Generate(EC_GROUP *eCurve, EC_POINT *basePoint, BIGNUM *genOrder, BIGNUM *privateKey, DWORD pSerial, BOOL pUpgrade,char (&pKey)[25]) {
    return PIDGEN3::BINK1998::Generate(eCurve, basePoint, genOrder, privateKey, pSerial, pUpgrade, pKey);
}

FNEXPORT bool PIDGEN3_BINK2002_Verify(EC_GROUP *eCurve, EC_POINT *basePoint, EC_POINT *publicKey, char (&cdKey)[25]) {
    return PIDGEN3::BINK2002::Verify(eCurve, basePoint, publicKey, cdKey);
}

FNEXPORT void PIDGEN3_BINK2002_Generate(EC_GROUP *eCurve, EC_POINT *basePoint, BIGNUM *genOrder, BIGNUM *privateKey, DWORD pChannelID, DWORD pAuthInfo, BOOL pUpgrade, char (&pKey)[25]) {
    return PIDGEN3::BINK2002::Generate(eCurve, basePoint, genOrder, privateKey, pChannelID, pAuthInfo, pUpgrade, pKey);
}

FNEXPORT int PIDGEN2_GenerateRetail(char* channelID, char* &keyout) {
    return PIDGEN2::GenerateRetail(channelID, keyout);
}

FNEXPORT int PIDGEN2_GenerateOEM(char* year, char* day, char* oem, char* keyout) {
    return PIDGEN2::GenerateOEM(year, day, oem, keyout);
}

// RNG utility functions
int UMSKT::umskt_rand_bytes(unsigned char *buf, int num) {
#if UMSKT_RNG_DJGPP
    // DOS-compatible RNG using DJGPP's random() function
    static bool initialized = false;
    if (!initialized) {
        // Get initial seed from multiple sources for better entropy
        struct timeval tv;
        gettimeofday(&tv, NULL);
        
        // Combine microseconds with BIOS timer ticks
        unsigned long ticks = *(volatile unsigned long *)0x0040001CL;
        int seed = (int)((tv.tv_sec ^ tv.tv_usec) ^ (ticks * 100000));
        
        // Initialize both random() and rand() with different seeds
        srandom(seed);
        srand(seed ^ 0x1234ABCD); // Use a different seed for rand
        
        initialized = true;
    }
    
    for (int i = 0; i < num; i++) {
        // Use random() for better randomness, especially in lower bits
        buf[i] = (unsigned char)(random() & 0xFF);
        
        // Mix in rand() as an additional source
        buf[i] ^= (unsigned char)(rand() & 0xFF);
    }
    return 1;
#else
    // Use OpenSSL's RAND_bytes for non-DOS systems
    return RAND_bytes(buf, num);
#endif
}

int UMSKT::umskt_bn_rand(BIGNUM *rnd, int bits, int top, int bottom) {
#if UMSKT_RNG_DJGPP
    // DOS-compatible RNG implementation for BIGNUMs
    unsigned char *buf = (unsigned char *)malloc((bits + 7) / 8);
    if (!buf) return 0;
    
    // Generate random bytes
    umskt_rand_bytes(buf, (bits + 7) / 8);
    
    // Convert to BIGNUM
    if (!BN_bin2bn(buf, (bits + 7) / 8, rnd)) {
        free(buf);
        return 0;
    }
    
    free(buf);
    
    // Apply top/bottom constraints like BN_rand does
    if (top != -1) {
        if (top) {
            if (bits == 0) {
                BN_zero(rnd);
                return 1;
            }
            BN_set_bit(rnd, bits - 1);
        }
        BN_mask_bits(rnd, bits);
    }
    
    if (bottom) {
        BN_set_bit(rnd, 0);
    }
    
    return 1;
#else
    // Use OpenSSL's BN_rand for non-DOS systems
    return BN_rand(rnd, bits, top, bottom);
#endif
}
