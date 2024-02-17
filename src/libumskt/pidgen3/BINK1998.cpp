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

#include "BINK1998.h"

/**
 * Packs a Windows XP-like Product Key.
 *
 * @param ki
 * @return Integer representation of KeyInfo
 */
Integer BINK1998::Pack(const KeyInfo &ki)
{
    // The quantity of information the key provides is 114 bits.
    // We're storing it in 2 64-bit quad-words with 14 trailing bits.
    // 64 * 2 = 128
    auto serial = (ki.ChannelID * MaxSerial) + ki.Serial;

    // Signature [114..59] <- Hash [58..31] <- Serial [30..1] <- Upgrade [0]
    Integer raw = CryptoPP::Crop(ki.Signature, 56) << 59 | CryptoPP::Crop(ki.Hash, 28) << 31 |
                  CryptoPP::Crop(serial, 30) << 1 | ki.isUpgrade;

    if (debug)
    {
        fmt::print(debug, "pack: {:x}\n\n", raw);
    }

    return raw;
}

/**
 * Unpacks a Windows XP-like Product Key.
 *
 * @param raw Integer to unpack
 * @return populated PIDGEN3::KeyInfo struct
 */
BINK1998::KeyInfo BINK1998::Unpack(const Integer &raw)
{
    KeyInfo ki;

    // We're assuming that the quantity of information within the product key is at most 114 bits.
    // log2(24^25) = 114.

    // Upgrade = Bit 0
    ki.isUpgrade = CryptoPP::Crop(raw, 1).ConvertToLong();

    // Serial = Bits [1..30] -> 30 bits
    auto serialPack = CryptoPP::Crop((raw >> 1), 30);
    ki.Serial = serialPack % MaxSerial;
    ki.ChannelID = ((serialPack - ki.Serial) / MaxSerial);

    // Hash = Bits [31..58] -> 28 bits
    ki.Hash = CryptoPP::Crop((raw >> 31), 28);

    // Signature = Bits [59..113] -> 56 bits
    ki.Signature = CryptoPP::Crop((raw >> 59), 56);

    return ki;
}

/**
 * Generates a Windows XP-like Product Key.
 *
 * @param pKey [out]
 * @return true on success, false on fail
 */
BOOL BINK1998::Generate(std::string &pKey)
{
    Integer c, s, pRaw;
    SHA1 sha1;

    // copy initial state from object
    auto ki = info;

    // Data segment of the RPK.
    Integer serialPack = (ki.ChannelID * MaxSerial) + ki.Serial;
    Integer pData = (serialPack << 1) | ki.isUpgrade;

    // prepare the private key for generation
    privateKey = genOrder - privateKey;

    do
    {
        ECP::Point R;

        // Generate a random number c consisting of 384 bits without any constraints.
        c.Randomize(UMSKT::rng, FieldBits);

        // Pick a random derivative of the base point on the elliptic curve.
        // R = cG;
        R = eCurve.Multiply(c, genPoint);
        if (debug)
        {
            fmt::print(debug, "c: {:x}\n\n", c);
            fmt::print(debug, "R[x,y] [{:x},\n{:x}]\n\n", R.x, R.y);
        }

        // Acquire its coordinates.
        // x = R.x; y = R.y;

        BYTE msgDigest[SHA1::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

        // Assemble the SHA message.
        pMsgBuffer = EncodeN(pData, pMsgBuffer, 4);
        pMsgBuffer = EncodeN(R.x, pMsgBuffer, FieldBytes);
        EncodeN(R.y, pMsgBuffer, FieldBytes);

        // pHash = SHA1(pSerial || R.x || R.y)
        sha1.CalculateDigest(msgDigest, msgBuffer, sizeof(msgBuffer));

        if (debug)
        {
            fmt::print(debug, "msgBuffer: ");
            for (BYTE b : msgBuffer)
            {
                fmt::print(debug, "{:x}", b);
            }
            fmt::print(debug, "\n\n");

            fmt::print(debug, "msgDigest: ");
            for (BYTE b : msgDigest)
            {
                fmt::print(debug, "{:x}", b);
            }
            fmt::print(debug, "\n\n");
        }

        // Translate the byte digest into a 32-bit integer - this is our computed pHash.
        // Truncate the pHash to 28 bits.

        ki.Hash = IntegerN(msgDigest, 4) >> 4;
        ki.Hash = CryptoPP::Crop(ki.Hash, 28);

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
        s = privateKey * ki.Hash;

        // s += c (mod n)
        s += c;
        s %= genOrder;

        // Translate resulting scalar into an Integer.
        ki.Signature = s;

        // Pack product key.
        pRaw = Pack(ki);

        if (verbose)
        {
            fmt::print(verbose, "Generation results:\n");
            fmt::print(verbose, "{:>10}: {}\n", "Upgrade", (bool)ki.isUpgrade);
            fmt::print(verbose, "{:>10}: {}\n", "Channel ID", ki.ChannelID);
            fmt::print(verbose, "{:>10}: {}\n", "Sequence", ki.Serial);
            fmt::print(verbose, "{:>10}: {:x}\n", "Hash", ki.Hash);
            fmt::print(verbose, "{:>10}: {:x}\n", "Signature", ki.Signature);
            fmt::print(verbose, "\n");
        }

    } while (ki.Signature.BitCount() > 55);
    // ↑ ↑ ↑
    // The signature can't be longer than 55 bits, else it will
    // make the CD-key longer than 25 characters.

    // Convert bytecode to Base24 CD-key.
    pKey = base24(pRaw);

    info = ki;

    return true;
}

/**
 * Validate a Windows XP-like Product Key.
 *
 * @param pKey [in]
 *
 * @return true if provided key validates against loaded curve
 */
BOOL BINK1998::Validate(const std::string &pKey)
{
    if (pKey.length() != 25)
    {
        return false;
    }

    // Convert Base24 CD-key to bytecode.
    Integer pRaw = unbase24(pKey);
    SHA1 sha1;

    // Extract RPK, hash and signature from bytecode.
    KeyInfo ki = Unpack(pRaw);

    if (verbose)
    {
        fmt::print(UMSKT::verbose, "Validation results:\n");
        fmt::print(UMSKT::verbose, "{:>10}: {}\n", "Upgrade", (bool)ki.isUpgrade);
        fmt::print(UMSKT::verbose, "{:>10}: {}\n", "Channel ID", ki.ChannelID);
        fmt::print(UMSKT::verbose, "{:>10}: {}\n", "Sequence", ki.Serial);
        fmt::print(UMSKT::verbose, "{:>10}: {:x}\n", "Hash", ki.Hash);
        fmt::print(UMSKT::verbose, "{:>10}: {:x}\n", "Signature", ki.Signature);
        fmt::print(UMSKT::verbose, "\n");
    }

    Integer serialPack = (ki.ChannelID * MaxSerial) + ki.Serial;
    Integer pData = serialPack << 1 | ki.isUpgrade;

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

    Integer e = ki.Hash, s = ki.Signature;

    // Create 2 points on the elliptic curve.
    ECP::Point t, P;

    // t = sG
    t = eCurve.Multiply(s, genPoint);

    // P = eK
    P = eCurve.Multiply(e, pubPoint);

    // P += t
    P = eCurve.Add(P, t);

    if (debug)
    {
        fmt::print("\nP[x,y]: [{:x},\n{:x}]\n\n", P.x, P.y);
    }

    BYTE msgDigest[SHA1::DIGESTSIZE], msgBuffer[SHAMessageLength], *pMsgBuffer = msgBuffer;

    // Convert resulting point coordinates to bytes.

    // Assemble the SHA message.
    pMsgBuffer = EncodeN(pData, pMsgBuffer, 4);
    pMsgBuffer = EncodeN(P.x, pMsgBuffer, FieldBytes);
    EncodeN(P.y, pMsgBuffer, FieldBytes);

    // compHash = SHA1(pSerial || P.x || P.y)
    sha1.CalculateDigest(msgDigest, msgBuffer, SHAMessageLength);

    auto intDigest = IntegerN(msgDigest);
    if (debug)
    {
        fmt::print(debug, "hash: {:x}\n\n", intDigest);
    }

    info = ki;

    // Translate the byte sha1 into a 32-bit integer - this is our computed hash.
    // Truncate the hash to 28 bits.
    Integer compHash = CryptoPP::Crop(intDigest >> 4, 28);

    // If the computed hash checks out, the key is valid.
    return compHash == ki.Hash;
}
