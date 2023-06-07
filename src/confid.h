//
// Created by neo on 6/6/2023.
//

#ifndef WINDOWSXPKG_CONFID_H
#define WINDOWSXPKG_CONFID_H

#include "header.h"

// Confirmation ID generator constants
#define SUCCESS 0
#define ERR_TOO_SHORT 1
#define ERR_TOO_LARGE 2
#define ERR_INVALID_CHARACTER 3
#define ERR_INVALID_CHECK_DIGIT 4
#define ERR_UNKNOWN_VERSION 5
#define ERR_UNLUCKY 6


typedef struct {
    QWORD u[2];
    QWORD v[2];
} TDivisor;

class ConfirmationID {
    static QWORD residue_add(QWORD x, QWORD y);
    static QWORD residue_sub(QWORD x, QWORD y);
    static QWORD __umul128(QWORD a, QWORD b, QWORD* hi);
    static QWORD ui128_quotient_mod(QWORD lo, QWORD hi);
    static QWORD residue_mul(QWORD x, QWORD y);
    static QWORD residue_pow(QWORD x, QWORD y);
    static QWORD inverse(QWORD u, QWORD v);
    static QWORD residue_inv(QWORD x);
    static QWORD residue_sqrt(QWORD what);
    static int find_divisor_v(TDivisor* d);
    static int polynomial_mul(int adeg, const QWORD a[], int bdeg, const QWORD b[], int resultprevdeg, QWORD result[]);
    static int polynomial_div_monic(int adeg, QWORD a[], int bdeg, const QWORD b[], QWORD* quotient);
    static void polynomial_xgcd(int adeg, const QWORD a[3], int bdeg, const QWORD b[3], int* pgcddeg, QWORD gcd[3], int* pmult1deg, QWORD mult1[3], int* pmult2deg, QWORD mult2[3]);
    static int u2poly(const TDivisor* src, QWORD polyu[3], QWORD polyv[2]);
    static void divisor_add(const TDivisor* src1, const TDivisor* src2, TDivisor* dst);
    static void divisor_mul(const TDivisor* src, QWORD mult, TDivisor* dst);
    static void divisor_mul128(const TDivisor* src, QWORD mult_lo, QWORD mult_hi, TDivisor* dst);
    static unsigned rol(unsigned x, int shift);
    static void sha1_single_block(unsigned char input[64], unsigned char output[20]);
    static void Mix(unsigned char* buffer, size_t bufSize, const unsigned char* key, size_t keySize);
    static void Unmix(unsigned char* buffer, size_t bufSize, const unsigned char* key, size_t keySize);

public:
    static int Generate(const char* installation_id_str, char confirmation_id[49]);
    static int CLIRun();
};

#endif //WINDOWSXPKG_CONFID_H
