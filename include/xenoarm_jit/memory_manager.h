#ifndef XENOARM_JIT_MEMORY_MANAGER_H
#define XENOARM_JIT_MEMORY_MANAGER_H

#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <functional>
#include <vector>
#include <mutex>
#include <memory>

namespace xenoarm_jit {

// Forward declarations
namespace translation_cache {
class TranslationCache;
}

// Memory protection flags
enum MemoryProtectionFlags {
    PROT_NONE  = 0,
    PROT_READ  = 1 << 0,
    PROT_WRITE = 1 << 1,
    PROT_EXEC  = 1 << 2
};

// Memory page information
struct MemoryPage {
    uint32_t guest_address;        // Starting guest address (page-aligned)
    uint32_t size;                 // Size of the page in bytes
    int protection;                // Current protection flags
    bool has_translated_code;      // Whether this page contains translated code
    bool is_dirty;                 // Whether this page has been modified
};

/**
 * Memory Manager class that handles guest memory mapping, 
 * SMC detection, and memory barrier insertion
 */
class MemoryManager {
public:
    // Constructor takes a translation cache and page size
    MemoryManager(translation_cache::TranslationCache* tc, size_t page_size = 4096);
    ~MemoryManager();

    // Initialize the memory manager
    bool initialize();

    // Memory access callbacks (used internally by JIT)
    uint8_t read_u8(uint32_t guest_address);
    uint16_t read_u16(uint32_t guest_address);
    uint32_t read_u32(uint32_t guest_address);
    uint64_t read_u64(uint32_t guest_address);
    
    void write_u8(uint32_t guest_address, uint8_t value);
    void write_u16(uint32_t guest_address, uint16_t value);
    void write_u32(uint32_t guest_address, uint32_t value);
    void write_u64(uint32_t guest_address, uint64_t value);
    
    void read_block(uint32_t guest_address, void* host_buffer, uint32_t size);
    void write_block(uint32_t guest_address, const void* host_buffer, uint32_t size);

    // Memory protection management
    bool protect_guest_memory(uint32_t guest_address, uint32_t size, int protection);
    int get_protection(uint32_t guest_address);

    // Register a page as containing translated code
    void register_code_page(uint32_t guest_address, uint32_t size);
    
    // Notify that a guest memory page may have been modified
    void notify_memory_modified(uint32_t guest_address, uint32_t size);

    // Process protection fault (called by signal handler or exception handler)
    void handle_protection_fault(uint32_t guest_address);

    // ARM Memory barrier helpers
    void insert_data_memory_barrier();       // DMB - Data Memory Barrier
    void insert_data_sync_barrier();         // DSB - Data Synchronization Barrier
    void insert_instruction_sync_barrier();  // ISB - Instruction Synchronization Barrier

    // Register host memory callbacks
    using HostReadU8Callback = std::function<uint8_t(uint32_t)>;
    using HostReadU16Callback = std::function<uint16_t(uint32_t)>;
    using HostReadU32Callback = std::function<uint32_t(uint32_t)>;
    using HostReadU64Callback = std::function<uint64_t(uint32_t)>;
    using HostReadBlockCallback = std::function<void(uint32_t, void*, uint32_t)>;
    
    using HostWriteU8Callback = std::function<void(uint32_t, uint8_t)>;
    using HostWriteU16Callback = std::function<void(uint32_t, uint16_t)>;
    using HostWriteU32Callback = std::function<void(uint32_t, uint32_t)>;
    using HostWriteU64Callback = std::function<void(uint32_t, uint64_t)>;
    using HostWriteBlockCallback = std::function<void(uint32_t, const void*, uint32_t)>;

    void set_host_memory_callbacks(
        HostReadU8Callback read_u8_cb,
        HostReadU16Callback read_u16_cb,
        HostReadU32Callback read_u32_cb,
        HostReadU64Callback read_u64_cb,
        HostReadBlockCallback read_block_cb,
        HostWriteU8Callback write_u8_cb,
        HostWriteU16Callback write_u16_cb,
        HostWriteU32Callback write_u32_cb,
        HostWriteU64Callback write_u64_cb,
        HostWriteBlockCallback write_block_cb
    );

private:
    translation_cache::TranslationCache* translation_cache_;
    size_t page_size_;
    
    // Host memory callbacks
    HostReadU8Callback host_read_u8_;
    HostReadU16Callback host_read_u16_;
    HostReadU32Callback host_read_u32_;
    HostReadU64Callback host_read_u64_;
    HostReadBlockCallback host_read_block_;
    
    HostWriteU8Callback host_write_u8_;
    HostWriteU16Callback host_write_u16_;
    HostWriteU32Callback host_write_u32_;
    HostWriteU64Callback host_write_u64_;
    HostWriteBlockCallback host_write_block_;
    
    // Guest memory page map
    std::unordered_map<uint32_t, MemoryPage> pages_;
    std::mutex pages_mutex_;  // Protect access to pages_
    
    // Helper methods
    uint32_t align_to_page(uint32_t address);
    bool is_page_protected(uint32_t guest_address);
    bool invalidate_translations_for_page(uint32_t guest_address);
    void reprotect_page(uint32_t guest_address, int new_protection);
};

} // namespace xenoarm_jit

#endif // XENOARM_JIT_MEMORY_MANAGER_H 