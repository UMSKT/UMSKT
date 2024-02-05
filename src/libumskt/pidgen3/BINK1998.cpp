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

#include "BINK1998.h"

/**
 * Packs a Windows XP-like Product Key.
 *
 * @param pRaw [in] *QWORD[2] raw product key input
 **/
BOOL BINK1998::Pack(Q_OWORD *pRaw)
{
    // The quantity of information the key provides is 114 bits.
    // We're storing it in 2 64-bit quad-words with 14 trailing bits.
    // 64 * 2 = 128

    // Signature [114..59] <- Hash [58..31] <- Serial [30..1] <- Upgrade [0]
    Integer raw;
    raw += info.Signature << 59;
    raw += (info.Hash & BITMASK(28)) << 31;
    raw += info.Serial << 1;
    raw += info.isUpgrade;

    raw.Encode(pRaw->byte, sizeof(Q_OWORD));

    return true;
}

/**
 * Unpacks a Windows XP-like Product Key.
 *
 * @param pRaw [out] *QWORD[2] raw product key output
 **/
BOOL BINK1998::Unpack(Q_OWORD *pRaw)
{
    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Upgrade = Bit 0
    info.isUpgrade = FIRSTNBITS(pRaw->qword[0], 1);

    // Serial = Bits [1..30] -> 30 bits
    info.Serial = NEXTSNBITS(pRaw->qword[0], 30, 1);

    // Hash = Bits [31..58] -> 28 bits
    info.Hash = NEXTSNBITS(pRaw->qword[0], 28, 31);

    // Signature = Bits [59..113] -> 56 bits
    info.Signature = FIRSTNBITS(pRaw->qword[1], 51) << 5 | NEXTSNBITS(pRaw->qword[0], 5, 59);

    return true;
}

/**
 * Generates a Windows XP-like Product Key.
 *
 * @param pKey [out]
 *
 * @return true on success, false on fail
 */
BOOL BINK1998::Generate(std::string &pKey)
{
    Integer c, s;

    Q_OWORD pRaw;

    // Data segment of the RPK.
    Integer pData = info.Serial << 1 | info.isUpgrade;

    // prepare the private key for generation
    privateKey -= genOrder;

    Integer limit;
    limit.SetBit(55);
    limit -= 1;

    do
    {
        ECP::Point R;

        // Generate a random number c consisting of 384 bits without any constraints.
        c.Randomize(UMSKT::rng, FieldBits);

        // Pick a random derivative of the base point on the elliptic curve.
        // R = cG;
        R = eCurve.Multiply(c, genPoint);

        // Acquire its coordinates.
        // x = R.x; y = R.y;

        BYTE msgDigest[SHA::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

        // Assemble the SHA message.
        pData.Encode((CryptoPP::byte *)msgBuffer, 4);
        pMsgBuffer += 4;

        R.x.Encode(pMsgBuffer, FieldBytes);
        pMsgBuffer += FieldBytes;

        R.y.Encode(pMsgBuffer, FieldBytes);
        pMsgBuffer += FieldBytes;

        // pHash = SHA1(pSerial || R.x || R.y)
        auto digest = SHA();
        digest.Update(msgBuffer, SHAMessageLength);
        digest.Final(msgDigest);

        // Translate the byte digest into a 32-bit integer - this is our computed pHash.
        // Truncate the pHash to 28 bits.

        info.Hash.Decode(msgDigest, SHAMessageLength);
        info.Hash = BYDWORD(msgDigest) >> 4 & BITMASK(28);

        /*
         *
         * Scalars:
         *  c = Random multiplier
         *  e = Hash
         *  s = Signature
         *  n = Order of G
         *  k = Private Key
         *
         * Points:
         *  G(x, y) = Generator (Base Point)
         *  R(x, y) = Random derivative of the generator
         *  K(x, y) = Public Key
         *
         * We need to find the signature s that satisfies the equation with a given hash:
         *  P = sG + eK
         *  s = ek + c (mod n) <- computation optimization
         */

        // s = ek;
        s = privateKey * info.Hash;

        // s += c (mod n)
        s = s + c % genOrder;

        // Translate resulting scalar into a 64-bit integer (the byte order is little-endian).
        info.Signature = s;

        // Pack product key.
        Pack(&pRaw);

        auto serial = fmt::format("{:d}", info.Serial);
        fmt::print(UMSKT::debug, "Generation results:\n");
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Upgrade", (bool)info.isUpgrade);
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Channel ID", serial.substr(0, 3));
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Sequence", serial.substr(3));
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Hash", info.Hash);
        fmt::print(UMSKT::debug, "{:>10}: {}\n", "Signature", info.Signature);
        fmt::print(UMSKT::debug, "\n");

    } while (info.Signature > limit);
    // ↑ ↑ ↑
    // The signature can't be longer than 55 bits, else it will
    // make the CD-key longer than 25 characters.

    // Convert bytecode to Base24 CD-key.
    base24(pKey, pRaw.byte);

    return true;
}

/**
 * Validate a Windows XP-like Product Key.
 *
 * @param pKey [in]
 *
 * @return true if provided key validates against loaded curve
 */
BOOL BINK1998::Validate(std::string &pKey)
{
    if (pKey.length() != 25)
    {
        return false;
    }

    Q_OWORD pRaw;

    // Convert Base24 CD-key to bytecode.
    unbase24(pRaw.byte, pKey);

    // Extract RPK, hash and signature from bytecode.
    Unpack(&pRaw);

    auto serial = fmt::format("{:d}", info.Serial);
    fmt::print(UMSKT::debug, "Validation results:\n");
    fmt::print(UMSKT::debug, "{:>10}: {}\n", "Upgrade", (bool)info.isUpgrade);
    fmt::print(UMSKT::debug, "{:>10}: {}\n", "Channel ID", serial.substr(0, 3));
    fmt::print(UMSKT::debug, "{:>10}: {}\n", "Sequence", serial.substr(3));
    fmt::print(UMSKT::debug, "{:>10}: {}\n", "Hash", info.Hash);
    fmt::print(UMSKT::debug, "{:>10}: {}\n", "Signature", info.Signature);
    fmt::print(UMSKT::debug, "\n");

    Integer pData = info.Serial << 1 | info.isUpgrade;

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
     *  P = sG + eK
     *
     */

    Integer e = info.Hash, s;
    s.Decode((BYTE *)&info.Signature, sizeof(info.Signature));

    // Create 2 points on the elliptic curve.
    ECP::Point t, P;

    // t = sG
    t = eCurve.Multiply(s, genPoint);

    // P = eK
    P = eCurve.Multiply(e, pubPoint);

    // P += t
    P = eCurve.Add(P, t);

    BYTE msgDigest[SHA::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

    // Convert resulting point coordinates to bytes.

    // Assemble the SHA message.
    pData.Encode(pMsgBuffer, 4);
    pMsgBuffer += 4;

    P.x.Encode(pMsgBuffer, FieldBytes);
    pMsgBuffer += FieldBytes;

    P.y.Encode(pMsgBuffer, FieldBytes);
    pMsgBuffer += FieldBytes;

    // compHash = SHA1(pSerial || P.x || P.y)
    auto digest = SHA();
    digest.Update(msgBuffer, SHAMessageLength);
    digest.Final(msgDigest);

    // Translate the byte digest into a 32-bit integer - this is our computed hash.
    // Truncate the hash to 28 bits.
    Integer compHash = BYDWORD(msgDigest) >> 4 & BITMASK(28);

    // If the computed hash checks out, the key is valid.
    return compHash == info.Hash;
}
