#ifndef UMSKT_SHA1_H
#define UMSKT_SHA1_H

#include "../../typedefs.h"
#include <cstdint>
#include <array>
#include <string>

// OpenSSL-compatible constants
#define SHA_DIGEST_LENGTH 20
#define SHA_CBLOCK    64
#define SHA_LBLOCK    16

namespace umskt {

class SHA1 {
public:
    static constexpr size_t DIGEST_SIZE = SHA_DIGEST_LENGTH;  // SHA1 produces a 160-bit (20-byte) hash
    using Digest = std::array<uint8_t, DIGEST_SIZE>;

    SHA1();
    
    // Update the hash with more data
    void update(const uint8_t* data, size_t len);
    
    // Finalize and get the hash
    Digest finalize();
    
    // Reset the hash state
    void reset();
    
    // Convenience method to hash data in one call
    static Digest hash(const uint8_t* data, size_t len);

private:
    static constexpr size_t BLOCK_SIZE = SHA_CBLOCK;  // SHA1 operates on 512-bit (64-byte) blocks
    static constexpr uint32_t INIT_STATE[5] = {
        0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
    };

    void processBlock(const uint8_t* block);
    
    uint32_t state[5];           // Hash state
    uint8_t buffer[BLOCK_SIZE];  // Input buffer
    uint64_t totalBytes;         // Total bytes processed
    size_t bufferSize;          // Current bytes in buffer
};

// OpenSSL-compatible function
inline void SHA1_wrapper(const unsigned char* data, size_t len, unsigned char* digest) {
    auto result = SHA1::hash(data, len);
    std::copy(result.begin(), result.end(), digest);
}

} // namespace umskt

#define SHA1_DIGEST(d,n,md) umskt::SHA1_wrapper((d),(n),(md))

#endif // UMSKT_SHA1_H 