/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2024 UMSKT Contributors (et.al.)
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
#include "BINK1998.h"
#include "BINK2002.h"

/**
 * PID 3.0 Product Key Character Set
 */
const std::string PIDGEN3::pKeyCharset = "BCDFGHJKMPQRTVWXY2346789";

/**
 * Maximum Field size for BINK 1998
 */
const DWORD32 PIDGEN3::MaxSizeBINK1998 = BINK1998::FieldBits + 1;

/**
 * RFC 1149.5 specifies 4 as the standard IEEE-vetted random number.
 *
 * see also: https://xkcd.com/221/
 *
 * @return 4
 */
int getRandomNumber()
{
    return 4; // chosen by fair dice roll
              // guaranteed to be random
}

/**
 * Initializes the elliptic curve
 *
 * @param pSel          [in] prime
 * @param aSel          [in] a
 * @param bSel          [in] b
 * @param generatorXSel [in] G[x]
 * @param generatorYSel [in] G[y]
 * @param publicKeyXSel [in] pub[x]
 * @param publicKeyYSel [in] pub[y]
 * @param genOrderSel   [in] computed order of G
 * @param privateKeySel [in] computed private key
 *
 * @return true on success, false on fail
 */
BOOL PIDGEN3::LoadEllipticCurve(const std::string &BinkIDSel, const std::string &pSel, const std::string &aSel,
                                const std::string &bSel, const std::string &generatorXSel,
                                const std::string &generatorYSel, const std::string &publicKeyXSel,
                                const std::string &publicKeyYSel, const std::string &genOrderSel,
                                const std::string &privateKeySel)
{
    // We cannot produce a valid key without knowing the private key k. The reason for this is that
    // we need the result of the function K(x; y) = kG(x; y).

    // We can, however, validate any given key using the available public key: {p, a, b, G, K}.
    // genOrder the order of the generator G, a value we have to reverse -> Schoof's Algorithm.

    BINKID = IntegerHexS(BinkIDSel);

    // We're presented with an elliptic curve, a multivariable function y(x; p; a; b), where
    // y^2 % p = x^3 + ax + b % p.
    auto p = IntegerS(pSel), a = IntegerS(aSel), b = IntegerS(bSel);

    // Public key will consist of the resulting (x; y) values.
    auto generatorX = IntegerS(generatorXSel), generatorY = IntegerS(generatorYSel);

    // G(x; y) is a generator function, its return value represents a point on the elliptic curve.
    auto publicKeyX = IntegerS(publicKeyXSel), publicKeyY = IntegerS(publicKeyYSel);

    /* Computed Data */
    genOrder = IntegerS(genOrderSel);
    privateKey = IntegerS(privateKeySel);

    /* Elliptic Curve calculations. */
    // The group is defined via Fp = all integers [0; p - 1], where p is prime.
    // The function EC_POINT_set_affine_coordinates() sets the x and y coordinates for the point p defined over the
    // curve given in group.
    eCurve = ECP(p, a, b);

    // Create new point N for the generator on the elliptic curve and set its coordinates to (genX; genY).
    genPoint = ECP::Point(generatorX, generatorY);

    // Create new point Q for the public key on the elliptic curve and set its coordinates to (pubX; pubY).
    pubPoint = ECP::Point(publicKeyX, publicKeyY);

    // If generator and public key points are not on the elliptic curve, either the generator or the public key values
    // are incorrect.
    assert(eCurve.VerifyPoint(genPoint) == true);
    assert(eCurve.VerifyPoint(pubPoint) == true);

    return true;
}

/**
 * Instantiates a PID 3.0 generator based on a given field on the heap
 *
 * @param field
 * @return PIDGEN3 based on the field type
 */
PIDGEN3 *PIDGEN3::Factory(const std::string &field)
{
    if (checkFieldStrIsBink1998(field))
    {
        return new BINK1998();
    }
    return new BINK2002();
}

/**
 * Factory-style Generate function, checks the currently instantiated field
 * creates the correct PIDGEN for the field type using the copy constructor
 * and invokes its Generate()
 *
 * @param pKey
 * @return successfulness
 */
BOOL PIDGEN3::Generate(std::string &pKey)
{
    if (checkFieldIsBink1998())
    {
        auto p3 = BINK1998(this);
        return p3.Generate(pKey);
    }

    auto p3 = BINK2002(this);
    return p3.Generate(pKey);
}

/**
 * Factory style Validate function, see Generate() for more info
 *
 * @param pKey
 * @return successfulness
 */
BOOL PIDGEN3::Validate(const std::string &pKey)
{
    if (checkFieldIsBink1998())
    {
        auto p3 = BINK1998(this);
        return p3.Validate(pKey);
    }

    auto p3 = BINK2002(this);
    return p3.Validate(pKey);
}

/**
 * Converts from byte sequence to the CD-key.
 *
 * @param seq Integer representation
 * @return std::string CDKey output
 **/
std::string PIDGEN3::base24(Integer &seq)
{
    std::string cdKey;
    cdKey.reserve(PK_LENGTH);

    // Divide z by 24 and convert the remainder to a CD-key char.
    Integer r, q, a = seq;
    for (int i = PK_LENGTH - 1; i >= 0; i--)
    {
        Integer::Divide(r, q, a, (WORD)pKeyCharset.length());
        cdKey.insert(cdKey.begin(), pKeyCharset[r.ConvertToLong()]);
        a = q;
    }

    return cdKey;
}

/**
 * Converts from CD-key to a byte sequence.
 *
 * @param cdKey std::string CDKey to convert
 * @return Integer raw representation of the CDKey
 **/
Integer PIDGEN3::unbase24(const std::string &cdKey)
{
    Integer result;

    for (char ch : cdKey)
    {
        auto val = std::find(pKeyCharset.begin(), pKeyCharset.end(), ch);

        // character is not in set, return early
        if (val == pKeyCharset.end())
        {
            return result;
        }

        // add the weighted sum to result
        result *= (int)pKeyCharset.length();
        result += (int)(val - pKeyCharset.begin());
    }

    return result;
}

/**
 * Takes the currently loaded Class-level KeyInfo and calculates the check digit for display.
 *
 * Algorithm directly taken from PIDGEN
 *
 * @return std::string representation of the Product ID as Displayed on the Product
 */
std::string PIDGEN3::StringifyProductID()
{
    if (info.isOEM)
    {
        Integer OEMID = info.ChannelID * Integer(100);
        OEMID += ((info.Serial / (MaxSerial / TEN)) * TEN);
        OEMID += GenerateMod7(OEMID);

        Integer Serial = info.Serial % (MaxSerial / TEN);

        DWORD32 iOEMID = OEMID.ConvertToLong(), iSerial = Serial.ConvertToLong();
        return fmt::format("PPPPP-OEM-{:07d}-{:05d}", iOEMID, iSerial);
    }
    else
    {
        DWORD32 ChannelID = info.ChannelID.ConvertToLong(),
                Serial = (info.Serial * TEN + GenerateMod7(info.Serial)).ConvertToLong(),
                BinkID = (BINKID / Integer::Two()).ConvertToLong();
        return fmt::format("PPPPP-{:03d}-{:07d}-{:d}xxx", ChannelID, Serial, BinkID);
    }
}

/**
 * Checks to see if the currently instantiated PIDGEN3 object has a
 * field size greater than the maximum known BINK1998 size.
 *
 * @return boolean value
 */
BOOL PIDGEN3::checkFieldIsBink1998()
{
    // is fieldSize < max?
    return (eCurve.FieldSize().BitCount() < MaxSizeBINK1998);
}

/**
 * Checks if a given field, in a std::string, is greater than
 * the maximum known BINK1998 size
 *
 * @param keyin std::string representation of a Field
 * @return boolean value
 */
BOOL PIDGEN3::checkFieldStrIsBink1998(std::string keyin)
{
    auto check = IntegerS(keyin);

    // is fieldSize < max?
    return (check.BitCount() < MaxSizeBINK1998);
}

/**
 * Prints a product key to stdout
 *
 * @param pk std::string to print
 */
std::string PIDGEN3::StringifyKey(const std::string &pKey)
{
    assert(pKey.length() >= PK_LENGTH);

    return fmt::format("{}-{}-{}-{}-{}", pKey.substr(0, 5), pKey.substr(5, 5), pKey.substr(10, 5), pKey.substr(15, 5),
                       pKey.substr(20, 5));
}

/**
 * std::BinaryOperation compatible accumulator for validating/stripping an input string against the PIDGEN3 charset
 *
 * @param accumulator
 * @param currentChar
 * @return
 */
std::string INLINE PIDGEN3::ValidateStringKeyInputCharset(std::string &accumulator, char currentChar)
{
    char cchar = (char)::toupper(currentChar);
    if (std::find(pKeyCharset.begin(), pKeyCharset.end(), cchar) != pKeyCharset.end())
    {
        accumulator.push_back(cchar);
    }
    return accumulator;
}

/**
 *
 * @param in_key
 * @param out_key
 * @return
 */
BOOL PIDGEN3::ValidateKeyString(const std::string &in_key, std::string &out_key)
{
    // copy out the product key stripping out extraneous characters
    out_key = std::accumulate(in_key.begin(), in_key.end(), std::string(), ValidateStringKeyInputCharset);

    // only return true if we've handled exactly PK_LENGTH chars
    return (out_key.length() == PK_LENGTH);
}
