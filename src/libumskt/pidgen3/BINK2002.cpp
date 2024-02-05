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
 * }
 */

#include "BINK2002.h"

/**
 * Packs a Windows Server 2003-like Product Key.
 *
 * @param pRaw [out] *OWORD raw product key
 **/
BOOL BINK2002::Pack(Q_OWORD *pRaw)
{
    Integer raw;
    // AuthInfo [113..104] <- Signature [103..42] <- Hash [41..11] <- Channel ID [10..1] <- Upgrade [0];
    raw += (info.AuthInfo & ((1 << 11) - 1)) << 104;
    raw += info.Signature << 42;
    raw += info.Hash << 11;
    raw += info.ChannelID << 1;
    raw += info.isUpgrade;

    raw.Encode((BYTE *)pRaw, sizeof(QWORD) * 2);

    return true;
}

/**
 * Unpacks a Windows Server 2003-like Product Key.
 *
 * @param pRaw [in] *OWORD raw product key input
 **/
BOOL BINK2002::Unpack(Q_OWORD *pRaw)
{
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Upgrade = Bit 0
    info.isUpgrade = FIRSTNBITS(pRaw->qword[0], 1);

    // Channel ID = Bits [1..10] -> 10 bits
    info.ChannelID = NEXTSNBITS(pRaw->qword[0], 10, 1);

    // Hash = Bits [11..41] -> 31 bits
    info.Hash = NEXTSNBITS(pRaw->qword[0], 31, 11);

    // Signature = Bits [42..103] -> 62 bits
    // The quad-word signature overlaps AuthInfo in bits 104 and 105,
    // hence Microsoft employs a secret technique called: Signature = HIDWORD(Signature) >> 2 | LODWORD(Signature)
    info.Signature = NEXTSNBITS(pRaw->qword[0], 30, 10) << 32 | FIRSTNBITS(pRaw->qword[1], 10) << 22 |
                     NEXTSNBITS(pRaw->qword[0], 22, 42);

    // AuthInfo = Bits [104..113] -> 10 bits
    info.AuthInfo = NEXTSNBITS(pRaw->qword[1], 10, 40);

    return true;
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
    Integer c, e, s;

    Q_OWORD pRaw;

    Integer limit;
    limit.SetBit(62);
    limit--;

    // Data segment of the RPK.
    Integer pData = info.ChannelID << 1 | info.isUpgrade;

    BOOL noSquare;

    do
    {
        ECP::Point R;

        // Generate a random number c consisting of 512 bits without any constraints.
        c.Randomize(UMSKT::rng, FieldBits);

        // R = cG
        R = eCurve.Multiply(c, genPoint);

        BYTE msgDigest[SHA::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

        // Assemble the first SHA message.
        msgBuffer[0] = 0x79;
        pMsgBuffer++;

        pData.Encode(pMsgBuffer, 2);
        pMsgBuffer += 2;

        // Convert resulting point coordinates to bytes.
        // and flip the endianness
        R.x.Encode(pMsgBuffer, FieldBytes);
        std::reverse(pMsgBuffer, pMsgBuffer + FieldBytes);
        pMsgBuffer += FieldBytes;

        R.y.Encode(pMsgBuffer, FieldBytes);
        std::reverse(pMsgBuffer, pMsgBuffer + FieldBytes);
        pMsgBuffer += FieldBytes;

        // pHash = SHA1(79 || Channel ID || R.x || R.y)
        auto digest = SHA();
        digest.Update(msgBuffer, FieldBytes);
        digest.Final(msgDigest);

        // Translate the byte digest into a 32-bit integer - this is our computed hash.
        // Truncate the hash to 31 bits.
        info.Hash = BYDWORD(msgDigest) & BITMASK(31);

        // Assemble the second SHA message.
        pMsgBuffer = msgBuffer;
        msgBuffer[0x00] = 0x5D;
        pMsgBuffer++;

        pData.Encode(pMsgBuffer, 2);
        pMsgBuffer += 2;

        info.Hash.Encode(pMsgBuffer, 4);
        pMsgBuffer += 4;

        info.AuthInfo.Encode(pMsgBuffer, 2);
        pMsgBuffer += 2;

        msgBuffer[0x09] = 0x00;
        msgBuffer[0x0A] = 0x00;

        // newSignature = SHA1(5D || Channel ID || Hash || AuthInfo || 00 00)
        digest = SHA();
        digest.Update(msgBuffer, 0x0B);
        digest.Final(msgDigest);

        // Translate the byte digest into a 64-bit integer - this is our computed intermediate signature.
        // As the signature is only 62 bits long at most, we have to truncate it by shifting the high DWORD right 2
        // bits (per spec).
        Integer iSignature = NEXTSNBITS(BYDWORD(&msgDigest[4]), 30, 2) << 32 | BYDWORD(msgDigest);

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
        e = iSignature * privateKey % genOrder;

        // s = (ek (mod n))²
        s = CryptoPP::ModularExponentiation(e, Integer::Two(), genOrder);

        // c *= 4 (c <<= 2)
        c <<= 2;

        // s += c
        s += c;

        // Around half of numbers modulo a prime are not squares -> BN_sqrt_mod fails about half of the times,
        // hence if BN_sqrt_mod returns NULL, we need to restart with a different seed.
        // s = √((ek)² + 4c (mod n))
        s = CryptoPP::ModularSquareRoot(s, genOrder);

        // s = -ek + √((ek)² + 4c) (mod n)
        s = s - e % genOrder;

        // If s is odd, add order to it.
        // The order is a prime, so it can't be even.
        if (s % Integer::Two() != 0)
        {
            // s = -ek + √((ek)² + 4c) + n
            s += genOrder;
        }

        // s /= 2 (s >>= 1)
        s >>= 1;

        // Translate resulting scalar into a 64-bit integer (the byte order is little-endian).
        info.Signature = s;

        // Pack product key.
        Pack(&pRaw);

        fmt::print(UMSKT::debug, "Generation results:\n");
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Upgrade", (bool)info.isUpgrade);
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Channel ID", info.ChannelID);
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Hash", info.Hash);
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Signature", info.Signature);
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "AuthInfo", info.AuthInfo);
        fmt::print(UMSKT::debug, "\n");

    } while (info.Signature > limit || noSquare);
    // ↑ ↑ ↑
    // The signature can't be longer than 62 bits, else it will
    // overlap with the AuthInfo segment next to it.

    // Convert bytecode to Base24 CD-key.
    base24(pKey, pRaw.byte);

    return true;
}

/**
 * Validates a Windows Server 2003-like Product Key.
 *
 * @param pKey
 **/
BOOL BINK2002::Validate(std::string &pKey)
{
    Q_OWORD bKey;

    // Convert Base24 CD-key to bytecode.
    unbase24(bKey.byte, &pKey[0]);

    // Extract product key segments from bytecode.
    Unpack(&bKey);

    Integer pData = info.ChannelID << 1 | info.isUpgrade;

    fmt::print(UMSKT::debug, "Validation results:\n");
    fmt::print(UMSKT::debug, "{:>10}: {}\n", "Upgrade", (bool)info.isUpgrade);
    fmt::print(UMSKT::debug, "{:>10}: {:d}\n", "Channel ID", info.ChannelID);
    fmt::print(UMSKT::debug, "{:>10}: {:d}\n", "Hash", info.Hash);
    fmt::print(UMSKT::debug, "{:>10}: {:d}\n", "Signature", info.Signature);
    fmt::print(UMSKT::debug, "{:>10}: {:d}\n", "AuthInfo", info.AuthInfo);
    fmt::print(UMSKT::debug, "\n");

    BYTE msgDigest[SHA::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

    // Assemble the first SHA message.
    msgBuffer[0x00] = 0x5D;
    pMsgBuffer++;

    pData.Encode(pMsgBuffer, 2);
    pMsgBuffer += 2;

    info.Hash.Encode(pMsgBuffer, 4);
    pMsgBuffer += 4;

    info.AuthInfo.Encode(pMsgBuffer, 2);
    pMsgBuffer += 2;

    msgBuffer[0x09] = 0x00;
    msgBuffer[0x0A] = 0x00;

    // newSignature = SHA1(5D || Channel ID || Hash || AuthInfo || 00 00)
    auto digest = SHA();
    digest.Update(msgBuffer, 0x0B);
    digest.Final(msgDigest);

    // Translate the byte digest into a 64-bit integer - this is our computed intermediate signature.
    // As the signature is only 62 bits long at most, we have to truncate it by shifting the high DWORD right 2 bits
    // (per spec).
    Integer iSignature = NEXTSNBITS(BYDWORD(&msgDigest[4]), 30, 2) << 32 | BYDWORD(msgDigest);

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
    Integer e = iSignature, s = info.Signature;

    // Create 2 points on the elliptic curve.
    ECP::Point p, t;

    // t = sG
    t = eCurve.Multiply(s, genPoint);

    // p = eK
    p = eCurve.Multiply(e, pubPoint);

    // p += t
    p = eCurve.Add(p, t);

    // p *= s
    p = eCurve.Multiply(s, p);

    // Assemble the second SHA message.
    pMsgBuffer = msgBuffer;
    msgBuffer[0x00] = 0x79;
    pMsgBuffer++;

    pData.Encode(pMsgBuffer, 2);
    pMsgBuffer += 2;

    p.x.Encode(pMsgBuffer, FieldBytes);
    pMsgBuffer += FieldBytes;

    p.y.Encode(pMsgBuffer, FieldBytes);
    pMsgBuffer += FieldBytes;

    // compHash = SHA1(79 || Channel ID || p.x || p.y)
    digest.Update(msgBuffer, SHAMessageLength);
    digest.Final(msgDigest);

    // Translate the byte digest into a 32-bit integer - this is our computed hash.
    // Truncate the hash to 31 bits.
    Integer compHash = BYDWORD(msgDigest) & BITMASK(31);

    // If the computed hash checks out, the key is valid.
    return compHash == info.Hash;
}
