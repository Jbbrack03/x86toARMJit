#ifndef XENOARM_JIT_TRANSLATION_CACHE_TRANSLATION_CACHE_H
#define XENOARM_JIT_TRANSLATION_CACHE_TRANSLATION_CACHE_H

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <set>
#include <functional>

namespace xenoarm_jit {
namespace translation_cache {

// Forward declarations
class CodeGenerator;

// Represents a block of translated AArch64 code
struct TranslatedBlock {
    uint64_t guest_address; // Original x86 address
    uint32_t guest_size;    // Size of original x86 code block
    std::vector<uint8_t> code; // Generated AArch64 code
    void* code_ptr;         // Executable pointer to the code (after copying to executable memory)
    bool is_linked;         // Whether this block is linked to other blocks

    // Define the types of control flow exits from a translated block
    enum class ControlFlowExitType {
        UNKNOWN,
        JMP, // Unconditional jump
        BR_COND, // Conditional branch (needs taken and not-taken targets)
        CALL,
        RET,
        FALLTHROUGH, // Implicit fallthrough to the next instruction
        INDIRECT_JMP, // Jump to dynamic target
        INDIRECT_CALL // Call to dynamic target
    };

    // Represents a control flow exit from a translated block
    struct ControlFlowExit {
        ControlFlowExitType type;
        uint64_t target_guest_address; // Target address for JMP, CALL, FALLTHROUGH
        uint64_t target_guest_address_false; // Target address for the false case of BR_COND
        size_t instruction_offset; // Offset within the code vector where the branch instruction is located
        bool is_patched; // Whether this exit has been patched to directly link to the target
    };

    // For tracking blocks that use this block in their chain
    std::set<TranslatedBlock*> incoming_links;

    std::vector<ControlFlowExit> exits; // Control flow exits from this block
    
    // Constructor
    TranslatedBlock(uint64_t addr, uint32_t size) 
        : guest_address(addr), guest_size(size), code_ptr(nullptr), is_linked(false) {}
};

class TranslationCache {
public:
    TranslationCache();
    ~TranslationCache();

    // Looks up a translated block by guest address
    TranslatedBlock* lookup(uint64_t guest_address);

    // Stores a translated block
    void store(TranslatedBlock* block);
    
    // Chain blocks wherever possible
    void chain_blocks(TranslatedBlock* block, std::function<void(TranslatedBlock*, TranslatedBlock*, const TranslatedBlock::ControlFlowExit&)> patch_callback);
    
    // Invalidate a single block
    void invalidate(uint64_t guest_address);
    
    // Invalidate a range of addresses
    void invalidate_range(uint64_t start_address, uint64_t end_address);
    
    // Get statistics
    size_t get_block_count() const { return cache_.size(); }
    size_t get_chained_block_count() const;
    
    // Flush the entire cache
    void flush();

private:
    // Map from guest address to translated block
    std::unordered_map<uint64_t, TranslatedBlock*> cache_;
    
    // Break links to and from a specific block
    void unchain_block(TranslatedBlock* block);
};

} // namespace translation_cache
} // namespace xenoarm_jit

#endif // XENOARM_JIT_TRANSLATION_CACHE_TRANSLATION_CACHE_H