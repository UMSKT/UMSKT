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
 *
  * @History {
 *   Algorithm was initially written and open sourced by z22
 *   and uploaded to GitHub by TheMCHK in August of 2019
 *
 *   Endermanch (Andrew) rewrote the algorithm in May of 2023
 *   Neo ported Endermanch's algorithm to CryptoPP in February of 2024
 * }
 */

#include "BINK2002.h"

/**
 * Packs a Windows Server 2003-like Product Key.
 *
 * @param ki PIDGEN3::KeyInfo struct to pack
 * @return Integer representation of the Product Key
 */
Integer BINK2002::Pack(const KeyInfo &ki)
{
    // AuthInfo [113..104] <- Signature [103..42] <- Hash [41..11] <- Channel ID [10..1] <- Upgrade [0];
    Integer raw = CryptoPP::Crop(ki.AuthInfo, 10) << 104 | CryptoPP::Crop(ki.Signature, 62) << 42 |
                  CryptoPP::Crop(ki.Hash, 31) << 11 | CryptoPP::Crop(ki.ChannelID, 10) << 1 | ki.isUpgrade;

    if (debug)
    {
        fmt::print(debug, "pack: {:x}\n\n", raw);
    }

    return raw;
}

/**
 * Unpacks a Windows Server 2003-like Product Key.
 *
 * @param raw Integer representation of the product key
 * @return unpacked PIDGEN3::KeyInfo struct
 */
BINK2002::KeyInfo BINK2002::Unpack(const Integer &raw)
{
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.
    KeyInfo ki;

    // Upgrade = Bit 0
    ki.isUpgrade = CryptoPP::Crop(raw, 1).ConvertToLong();

    // Channel ID = Bits [1..10] -> 10 bits
    ki.ChannelID = CryptoPP::Crop(raw >> 1, 10);

    // Hash = Bits [11..41] -> 30 bits
    ki.Hash = CryptoPP::Crop(raw >> 11, 31);

    // Signature = Bits [42..103] -> 62 bits
    // The quad-word signature overlaps AuthInfo in bits 104 and 105,
    // hence Microsoft employs a secret technique called: Signature = HIDWORD(Signature) >> 2 | LODWORD(Signature)
    ki.Signature = CryptoPP::Crop(raw >> 42, 62);

    // AuthInfo = Bits [104..113] -> 10 bits
    ki.AuthInfo = CryptoPP::Crop(raw >> 104, 10);

    return ki;
}

/**
 * Generates a Windows Server 2003-like Product Key.
 *
 * @param info
 * @param pKey
 * @return
 */
BOOL BINK2002::Generate(std::string &pKey)
{
    // copy the starting state from the class
    KeyInfo ki = info;
    SHA1 sha1;

    Integer c, e, s, pRaw;

    if (!ki.Rand.IsZero())
    {
        c = ki.Rand;
    }

    // Data segment of the RPK.
    Integer pData = ki.ChannelID << 1 | ki.isUpgrade;

    BOOL noSquare;

    do
    {
        ECP::Point R;

        if (ki.Rand.IsZero())
        {
            // Generate a random number c consisting of 512 bits without any constraints.
            c.Randomize(UMSKT::rng, FieldBits);
        }

        // R = cG
        R = eCurve.Multiply(c, genPoint);
        if (debug)
        {
            fmt::print(debug, "c: {:x}\n\n", c);
            fmt::print(debug, "R[x,y] [{:x},\n{:x}]\n\n", R.x, R.y);
        }

        BYTE msgDigest[SHA1::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

        // Assemble the first SHA message.
        *pMsgBuffer = 0x79;
        pMsgBuffer++;

        pMsgBuffer = EncodeN(pData, pMsgBuffer, 2);

        // Convert resulting point coordinates to bytes.
        // and flip the endianness
        pMsgBuffer = EncodeN(R.x, pMsgBuffer, FieldBytes);
        EncodeN(R.y, pMsgBuffer, FieldBytes);

        // pHash = SHA1(79 || Channel ID || R.x || R.y)
        sha1.CalculateDigest(msgDigest, msgBuffer, SHAMessageLength);

        if (debug)
        {
            fmt::print(debug, "msgBuffer[1]: ");
            for (BYTE b : msgBuffer)
            {
                fmt::print(debug, "{:x}", b);
            }
            fmt::print(debug, "\n\n");

            fmt::print(debug, "msgDigest[1]: ");
            for (BYTE b : msgDigest)
            {
                fmt::print(debug, "{:x}", b);
            }
            fmt::print(debug, "\n\n");
        }

        // Translate the byte sha1 into a 32-bit integer - this is our computed hash.
        // Truncate the hash to 31 bits.
        ki.Hash = CryptoPP::Crop(IntegerN(msgDigest), 31);

        if (verbose)
        {
            BYTE buf[8];
            sha1.CalculateTruncatedDigest(buf, sizeof(buf), msgBuffer, SHAMessageLength);

            fmt::print(verbose, "truncated buffer: ");
            for (BYTE b : buf)
            {
                fmt::print(verbose, "{:x}", b);
            }
            fmt::print(verbose, "\n\n");

            DWORD h0 = ((DWORD)buf[0] | ((DWORD)buf[1] << 8) | ((DWORD)buf[2] << 16) | ((DWORD)buf[3] << 24));
            DWORD h1 =
                ((((DWORD)buf[4]) | ((DWORD)buf[5] << 8) | ((DWORD)buf[6] << 16) | ((DWORD)buf[7] << 24)) >> (32 - 19))
                << 1;

            h1 |= (h0 >> 31) & 1;

            fmt::print(verbose, "h0,1: {:x} {:x}\n\n", h0, h1);

            ki.Serial = IntegerN(h1);

            fmt::print(verbose, "serial: {:d}\n\n", ki.Serial);
        }

        // Assemble the second SHA message.
        pMsgBuffer = msgBuffer;
        msgBuffer[0x00] = 0x5D;
        pMsgBuffer++;

        pMsgBuffer = EncodeN(pData, pMsgBuffer, 2);
        pMsgBuffer = EncodeN(ki.Hash, pMsgBuffer, 4);
        pMsgBuffer = EncodeN(ki.AuthInfo, pMsgBuffer, 2);

        *pMsgBuffer = 0x00;
        pMsgBuffer++;

        *pMsgBuffer = 0x00;
        pMsgBuffer++;

        // newSignature = SHA1(5D || Channel ID || Hash || AuthInfo || 00 00)
        sha1.CalculateDigest(msgDigest, msgBuffer, pMsgBuffer - msgBuffer);

        if (debug)
        {
            fmt::print(debug, "msgBuffer[2]: ");
            for (BYTE b : msgBuffer)
            {
                fmt::print(debug, "{:x}", b);
            }
            fmt::print(debug, "\n\n");

            fmt::print(debug, "msgDigest[2]: ");
            for (BYTE b : msgDigest)
            {
                fmt::print(debug, "{:x}", b);
            }
            fmt::print(debug, "\n\n");
        }

        // Translate the byte sha1 into a 64-bit integer - this is our computed intermediate signature.
        // As the signature is only 62 bits long at most, we have to truncate it by shifting the high DWORD right 2
        // bits (per spec).
        QWORD iSignature = NEXTSNBITS(BYDWORD(&msgDigest[4]), 30, 2) << 32 | BYDWORD(msgDigest);

        /*
         *
         * Scalars:
         *  c = Random multiplier
         *  e = Intermediate Signature
         *  s = Signature
         *  n = Order of G
         *  k = Private Key
         *
         * Points:
         *  G(x, y) = Generator (Base Point)
         *  R(x, y) = Random derivative of the generator
         *  K(x, y) = Public Key
         *
         * Equation:
         *  s(sG + eK) = R (mod p)
         *  ↓ K = kG; R = cG ↓
         *
         *  s(sG + ekG) = cG (mod p)
         *  s(s + ek)G = cG (mod p)
         *  ↓ G cancels out, the scalar arithmetic shrinks to order n ↓
         *
         *  s(s + ek) = c (mod n)
         *  s² + (ek)s - c = 0 (mod n)
         *  ↓ This is a quadratic equation in respect to the signature ↓
         *
         *  s = (-ek ± √((ek)² + 4c)) / 2 (mod n)
         */

        // e = ek (mod n)
        e = CryptoPP::ModularMultiplication(IntegerN(iSignature), privateKey, genOrder);

        // s = (ek (mod n))²
        s = CryptoPP::ModularExponentiation(e, Integer::Two(), genOrder);

        // c *= 4 (c <<= 2)
        c *= 4;

        // s += c
        s += c;

        // Around half of numbers modulo a prime are not squares -> BN_sqrt_mod fails about half of the times,
        // hence if BN_sqrt_mod returns NULL, we need to restart with a different seed.
        // s = √((ek)² + 4c (mod n))
        s = CryptoPP::ModularSquareRoot(s, genOrder);
        noSquare = s.IsZero();

        // s = -ek + √((ek)² + 4c) (mod n)
        s -= e;
        s %= genOrder;

        // If s is odd, add order to it.
        // The order is a prime, so it can't be even.
        if (s % Integer::Two() != 0)
        {
            // s = -ek + √((ek)² + 4c) + n
            s += genOrder;
        }

        // s /= 2 (s >>= 1)
        s /= 2;

        // Translate resulting scalar into a 64-bit integer (the byte order is little-endian).
        ki.Signature = s;

        // Pack product key.
        pRaw = Pack(ki);

        if (verbose)
        {
            fmt::print(verbose, "Generation results:\n");
            fmt::print(verbose, "{:>10}: {}\n", "Upgrade", (bool)ki.isUpgrade);
            fmt::print(verbose, "{:>10}: {}\n", "Channel ID", ki.ChannelID);
            fmt::print(verbose, "{:>10}: {:x}\n", "Hash", ki.Hash);
            fmt::print(verbose, "{:>10}: {:x}\n", "Signature", ki.Signature);
            fmt::print(verbose, "{:>10}: {:x}\n", "AuthInfo", ki.AuthInfo);
            fmt::print(verbose, "\n");
        }
    } while ((ki.Signature.BitCount() > 62 || noSquare) && ki.Rand.IsZero());
    // ↑ ↑ ↑
    // The signature can't be longer than 62 bits, else it will
    // overlap with the AuthInfo segment next to it.
    // Convert bytecode to Base24 CD-key.
    pKey = base24(pRaw);

    info = ki;

    return true;
}

/**
 * Validates a Windows Server 2003-like Product Key.
 *
 * @param pKey
 **/
BOOL BINK2002::Validate(const std::string &pKey)
{
    Integer pRaw;
    SHA1 sha1;

    // Convert Base24 CD-key to bytecode.
    pRaw = unbase24(pKey);

    // Extract product key segments from bytecode.
    KeyInfo ki = Unpack(pRaw);

    Integer pData = ki.ChannelID << 1 | ki.isUpgrade;

    if (verbose)
    {
        fmt::print(verbose, "Validation results:\n");
        fmt::print(verbose, "{:>10}: {}\n", "Upgrade", (bool)ki.isUpgrade);
        fmt::print(verbose, "{:>10}: {}\n", "Channel ID", ki.ChannelID);
        fmt::print(verbose, "{:>10}: {:x}\n", "Hash", ki.Hash);
        fmt::print(verbose, "{:>10}: {:x}\n", "Signature", ki.Signature);
        fmt::print(verbose, "{:>10}: {:x}\n", "AuthInfo", ki.AuthInfo);
        fmt::print(verbose, "\n");
    }

    BYTE msgDigest[SHA1::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

    // Assemble the first SHA message.
    msgBuffer[0x00] = 0x5D;
    pMsgBuffer++;

    pMsgBuffer = EncodeN(pData, pMsgBuffer, 2);
    pMsgBuffer = EncodeN(ki.Hash, pMsgBuffer, 4);
    pMsgBuffer = EncodeN(ki.AuthInfo, pMsgBuffer, 2);

    *pMsgBuffer = 0x00;
    pMsgBuffer++;

    *pMsgBuffer = 0x00;
    pMsgBuffer++;

    // newSignature = SHA1(5D || Channel ID || Hash || AuthInfo || 00 00)
    sha1.CalculateDigest(msgDigest, msgBuffer, pMsgBuffer - msgBuffer);

    if (debug)
    {
        auto intDigest = IntegerN(msgDigest);
        fmt::print(debug, "\nhash 1: {:x}\n\n", intDigest);
    }

    // Translate the byte sha1 into a 64-bit integer - this is our computed intermediate signature.
    // As the signature is only 62 bits long at most, we have to truncate it by shifting the high DWORD right 2 bits
    // (per spec).
    QWORD iSignature = NEXTSNBITS(BYDWORD(&msgDigest[4]), 30, 2) << 32 | BYDWORD(msgDigest);

    /*
     *
     * Scalars:
     *  e = Hash
     *  s = Schnorr Signature
     *
     * Points:
     *  G(x, y) = Generator (Base Point)
     *  K(x, y) = Public Key
     *
     * Equation:
     *  P = s(sG + eK)
     *
     */
    Integer e = IntegerN(iSignature), s = ki.Signature;

    // Create 2 points on the elliptic curve.
    ECP::Point P, t;

    // t = sG
    t = eCurve.Multiply(s, genPoint);

    // P = eK
    P = eCurve.Multiply(e, pubPoint);

    // P += t
    P = eCurve.Add(P, t);

    // P *= s
    P = eCurve.Multiply(s, P);

    if (debug)
    {
        fmt::print(debug, "P[x,y]: [{:x},\n{:x}]\n\n", P.x, P.y);
    }

    // Assemble the second SHA message.
    pMsgBuffer = msgBuffer;
    msgBuffer[0x00] = 0x79;
    pMsgBuffer++;

    pMsgBuffer = EncodeN(pData, pMsgBuffer, 2);
    pMsgBuffer = EncodeN(P.x, pMsgBuffer, FieldBytes);
    EncodeN(P.y, pMsgBuffer, FieldBytes);

    // compHash = SHA1(79 || Channel ID || P.x || P.y)
    sha1.CalculateDigest(msgDigest, msgBuffer, SHAMessageLength);

    auto intDigest = IntegerN(msgDigest);
    if (debug)
    {
        fmt::print(debug, "hash 2: {:x}\n\n", intDigest);
    }

    // Translate the byte sha1 into a 32-bit integer - this is our computed hash.
    // Truncate the hash to 31 bits.
    Integer compHash = CryptoPP::Crop(intDigest, 31);

    info = ki;

    // If the computed hash checks out, the key is valid.
    return compHash == ki.Hash;
}
