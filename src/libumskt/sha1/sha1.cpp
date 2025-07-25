#include "sha1.h"
#include <cstring>
#include <bit>

namespace umskt {

// Rotate left operation
static inline uint32_t rotl(uint32_t value, unsigned int bits) {
    return (value << bits) | (value >> (32 - bits));
}

SHA1::SHA1() {
    reset();
}

void SHA1::reset() {
    std::memcpy(state, INIT_STATE, sizeof(state));
    totalBytes = 0;
    bufferSize = 0;
}

void SHA1::update(const uint8_t* data, size_t len) {
    while (len > 0) {
        size_t copy = std::min(BLOCK_SIZE - bufferSize, len);
        std::memcpy(buffer + bufferSize, data, copy);
        bufferSize += copy;
        data += copy;
        len -= copy;
        
        if (bufferSize == BLOCK_SIZE) {
            processBlock(buffer);
            totalBytes += BLOCK_SIZE;
            bufferSize = 0;
        }
    }
}

SHA1::Digest SHA1::finalize() {
    // Total size including padding must be a multiple of 64 bytes
    totalBytes += bufferSize;
    
    // Add padding byte
    buffer[bufferSize++] = 0x80;
    
    // If there isn't enough room for the length (8 bytes), process this block and start a new one
    if (bufferSize > BLOCK_SIZE - 8) {
        std::memset(buffer + bufferSize, 0, BLOCK_SIZE - bufferSize);
        processBlock(buffer);
        bufferSize = 0;
    }
    
    // Pad with zeros and add 64-bit length (in bits)
    std::memset(buffer + bufferSize, 0, BLOCK_SIZE - 8 - bufferSize);
    uint64_t bitCount = totalBytes * 8;
    
    // Store length in big-endian format
    for (int i = 7; i >= 0; --i) {
        buffer[BLOCK_SIZE - 1 - i] = static_cast<uint8_t>(bitCount >> (i * 8));
    }
    
    processBlock(buffer);
    
    // Convert state to bytes (big-endian)
    Digest digest;
    for (int i = 0; i < 5; ++i) {
        digest[i*4] = static_cast<uint8_t>(state[i] >> 24);
        digest[i*4 + 1] = static_cast<uint8_t>(state[i] >> 16);
        digest[i*4 + 2] = static_cast<uint8_t>(state[i] >> 8);
        digest[i*4 + 3] = static_cast<uint8_t>(state[i]);
    }
    
    return digest;
}

void SHA1::processBlock(const uint8_t* block) {
    // Convert block to 16 32-bit words (big-endian)
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i*4]) << 24) |
               (static_cast<uint32_t>(block[i*4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i*4 + 2]) << 8) |
               static_cast<uint32_t>(block[i*4 + 3]);
    }
    
    // Extend 16 words to 80 words
    for (int i = 16; i < 80; ++i) {
        w[i] = rotl(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    // Initialize working variables
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    
    // Main loop
    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }
        else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }
        else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }
        else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        
        uint32_t temp = rotl(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = temp;
    }
    
    // Update state
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

SHA1::Digest SHA1::hash(const uint8_t* data, size_t len) {
    SHA1 sha1;
    sha1.update(data, len);
    return sha1.finalize();
}

} // namespace umskt 