#ifndef XENOARM_JIT_TRANSLATION_CACHE_H
#define XENOARM_JIT_TRANSLATION_CACHE_H

#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <vector>

// A translated code block
struct TranslatedBlock {
    uint32_t guest_address;    // Guest address of the block
    size_t size;               // Size of the block in bytes
    void* host_code;           // Host code pointer
    bool is_linked;            // Whether this block is linked to other blocks
};

// Translation cache
class TranslationCache {
public:
    TranslationCache(size_t codeSize);
    ~TranslationCache();

    // Allocate space for a new block
    uint8_t* allocateBlock(uint32_t guestAddress, size_t size);

    // Register a completed block
    bool registerBlock(uint32_t guestAddress, size_t size, void* hostCode);

    // Look up a block by guest address
    void* lookupBlock(uint32_t guestAddress);

    // Invalidate a range of guest addresses
    void invalidate(uint32_t guestAddress, size_t size);

    // Invalidate all blocks
    void invalidateAll();

    // Chain two blocks together
    bool chainBlocks(uint32_t fromAddress, uint32_t toAddress);

private:
    uint8_t* code_buffer_;     // Code buffer
    size_t code_buffer_size_;  // Size of the code buffer
    size_t code_buffer_pos_;   // Current position in the code buffer

    // Map of guest addresses to translated blocks
    std::unordered_map<uint32_t, TranslatedBlock> blocks_;

    // Reset the cache
    void reset();
};

#endif // XENOARM_JIT_TRANSLATION_CACHE_H 