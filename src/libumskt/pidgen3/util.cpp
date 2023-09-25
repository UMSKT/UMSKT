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
 * @FileCreated by Andrew on 01/06/2023
 * @Maintainer Andrew
 */

#include "PIDGEN3.h"

int randomRange() {
    return 4;  // chosen by fair dice roll
               // guaranteed to be random
}

/* Convert data between endianness types. */
void PIDGEN3::endian(BYTE *data, int length) {
    for (int i = 0; i < length / 2; i++) {
        BYTE temp = data[i];
        data[i] = data[length - i - 1];
        data[length - i - 1] = temp;
    }
}

/* Initializes the elliptic curve. */
EC_GROUP* PIDGEN3::initializeEllipticCurve(
        const std::string pSel,
        const std::string aSel,
        const std::string bSel,
        const std::string generatorXSel,
        const std::string generatorYSel,
        const std::string publicKeyXSel,
        const std::string publicKeyYSel,
        EC_POINT *&genPoint,
        EC_POINT *&pubPoint
) {
    // Initialize BIGNUM and BIGNUMCTX structures.
    // BIGNUM - Large numbers
    // BIGNUMCTX - Context large numbers (temporary)
    BIGNUM *a, *b, *p, *generatorX, *generatorY, *publicKeyX, *publicKeyY;
    BN_CTX *context;

    // We're presented with an elliptic curve, a multivariable function y(x; p; a; b), where
    // y^2 % p = x^3 + ax + b % p.
    a = BN_new();
    b = BN_new();
    p = BN_new();

    // Public key will consist of the resulting (x; y) values.
    publicKeyX = BN_new();
    publicKeyY = BN_new();

    // G(x; y) is a generator function, its return value represents a point on the elliptic curve.
    generatorX = BN_new();
    generatorY = BN_new();

    // Context variable
    context = BN_CTX_new();

    /* Public data */
    BN_dec2bn(&p, pSel.c_str());
    BN_dec2bn(&a, aSel.c_str());
    BN_dec2bn(&b, bSel.c_str());
    BN_dec2bn(&generatorX, generatorXSel.c_str());
    BN_dec2bn(&generatorY, generatorYSel.c_str());

    BN_dec2bn(&publicKeyX, publicKeyXSel.c_str());
    BN_dec2bn(&publicKeyY, publicKeyYSel.c_str());

    /* Elliptic Curve calculations. */
    // The group is defined via Fp = all integers [0; p - 1], where p is prime.
    // The function EC_POINT_set_affine_coordinates() sets the x and y coordinates for the point p defined over the curve given in group.
    EC_GROUP *eCurve = EC_GROUP_new_curve_GFp(p, a, b, context);

    // Create new point for the generator on the elliptic curve and set its coordinates to (genX; genY).
    genPoint = EC_POINT_new(eCurve);
    EC_POINT_set_affine_coordinates(eCurve, genPoint, generatorX, generatorY, context);

    // Create new point for the public key on the elliptic curve and set its coordinates to (pubX; pubY).
    pubPoint = EC_POINT_new(eCurve);
    EC_POINT_set_affine_coordinates(eCurve, pubPoint, publicKeyX, publicKeyY, context);

    // If generator and public key points are not on the elliptic curve, either the generator or the public key values are incorrect.
    assert(EC_POINT_is_on_curve(eCurve, genPoint, context) == true);
    assert(EC_POINT_is_on_curve(eCurve, pubPoint, context) == true);

    // Cleanup
    BN_CTX_free(context);
    BN_free(p);
    BN_free(a);
    BN_free(b);
    BN_free(generatorX);
    BN_free(generatorY);
    BN_free(publicKeyX);
    BN_free(publicKeyY);

    return eCurve;
}

int PIDGEN3::BN_bn2lebin(const BIGNUM *a, unsigned char *to, int tolen) {
    if (a == nullptr || to == nullptr)
        return 0;

    int len = BN_bn2bin(a, to);

    if (len > tolen)
        return -1;

    // Choke point inside BN_bn2lebinpad: OpenSSL uses len instead of tolen.
    endian(to, tolen);

    return len;
}
