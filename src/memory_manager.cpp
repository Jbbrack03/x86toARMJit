#include "xenoarm_jit/memory_manager.h"
#include "xenoarm_jit/translation_cache/translation_cache.h"
#include "logging/logger.h"
#include <sys/mman.h>
#include <cstring>
#include <algorithm>

namespace xenoarm_jit {

MemoryManager::MemoryManager(translation_cache::TranslationCache* tc, size_t page_size)
    : translation_cache_(tc), page_size_(page_size) {
    LOG_DEBUG("MemoryManager created with page size: " + std::to_string(page_size_));
}

MemoryManager::~MemoryManager() {
    LOG_DEBUG("MemoryManager destroyed");
}

bool MemoryManager::initialize() {
    LOG_INFO("Initializing MemoryManager");
    
    // Check if translation cache is valid
    if (!translation_cache_) {
        LOG_ERROR("Translation cache is null, cannot initialize MemoryManager");
        return false;
    }
    
    return true;
}

// Memory access callbacks (used internally by JIT)
uint8_t MemoryManager::read_u8(uint32_t guest_address) {
    if (host_read_u8_) {
        return host_read_u8_(guest_address);
    }
    LOG_ERROR("Host read_u8 callback not set");
    return 0;
}

uint16_t MemoryManager::read_u16(uint32_t guest_address) {
    if (host_read_u16_) {
        return host_read_u16_(guest_address);
    }
    LOG_ERROR("Host read_u16 callback not set");
    return 0;
}

uint32_t MemoryManager::read_u32(uint32_t guest_address) {
    if (host_read_u32_) {
        return host_read_u32_(guest_address);
    }
    LOG_ERROR("Host read_u32 callback not set");
    return 0;
}

uint64_t MemoryManager::read_u64(uint32_t guest_address) {
    if (host_read_u64_) {
        return host_read_u64_(guest_address);
    }
    LOG_ERROR("Host read_u64 callback not set");
    return 0;
}

void MemoryManager::read_block(uint32_t guest_address, void* host_buffer, uint32_t size) {
    if (host_read_block_) {
        host_read_block_(guest_address, host_buffer, size);
        return;
    }
    
    // Fallback to byte-by-byte reads if block read not available
    uint8_t* buffer = static_cast<uint8_t*>(host_buffer);
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = read_u8(guest_address + i);
    }
}

void MemoryManager::write_u8(uint32_t guest_address, uint8_t value) {
    // Check if this write affects a code page, handle SMC if necessary
    uint32_t page_addr = align_to_page(guest_address);
    bool is_code_page = false;
    
    {
        std::lock_guard<std::mutex> lock(pages_mutex_);
        auto it = pages_.find(page_addr);
        if (it != pages_.end() && it->second.has_translated_code) {
            is_code_page = true;
        }
    }
    
    if (is_code_page) {
        // This is SMC - handle it before the write
        LOG_INFO("SMC detected: Writing to code page at address 0x" + 
                 std::to_string(guest_address) + ", invalidating translations");
        
        // Unprotect the page temporarily
        int old_protection = get_protection(page_addr);
        protect_guest_memory(page_addr, page_size_, PROT_READ | PROT_WRITE);
        
        // Invalidate any translations for this page
        invalidate_translations_for_page(page_addr);
        
        // Do the write
        if (host_write_u8_) {
            host_write_u8_(guest_address, value);
        } else {
            LOG_ERROR("Host write_u8 callback not set");
        }
        
        // Reprotect with original protection
        protect_guest_memory(page_addr, page_size_, old_protection);
        
        // Synchronize instruction cache with data cache
        // This is crucial for ARM processors when modifying code
        insert_data_sync_barrier();       // DSB - ensure all memory writes are complete
        insert_instruction_sync_barrier(); // ISB - flush pipeline
    } else {
        // Normal write to data page
        if (host_write_u8_) {
            host_write_u8_(guest_address, value);
        } else {
            LOG_ERROR("Host write_u8 callback not set");
        }
    }
}

void MemoryManager::write_u16(uint32_t guest_address, uint16_t value) {
    // For simplicity in this implementation, handle as two 8-bit writes
    // A real implementation would be more optimized
    uint8_t low_byte = static_cast<uint8_t>(value & 0xFF);
    uint8_t high_byte = static_cast<uint8_t>((value >> 8) & 0xFF);
    
    // Handle byte order (assuming little-endian x86)
    write_u8(guest_address, low_byte);
    write_u8(guest_address + 1, high_byte);
}

void MemoryManager::write_u32(uint32_t guest_address, uint32_t value) {
    // Check if this write affects a code page, handle SMC if necessary
    uint32_t page_addr = align_to_page(guest_address);
    bool is_code_page = false;
    
    {
        std::lock_guard<std::mutex> lock(pages_mutex_);
        auto it = pages_.find(page_addr);
        if (it != pages_.end() && it->second.has_translated_code) {
            is_code_page = true;
        }
    }
    
    if (is_code_page) {
        // Handle as SMC, similar to write_u8
        LOG_INFO("SMC detected: Writing 32-bit value to code page at address 0x" + 
                 std::to_string(guest_address) + ", invalidating translations");
        
        // Unprotect the page temporarily
        int old_protection = get_protection(page_addr);
        protect_guest_memory(page_addr, page_size_, PROT_READ | PROT_WRITE);
        
        // Invalidate any translations for this page
        invalidate_translations_for_page(page_addr);
        
        // Do the write
        if (host_write_u32_) {
            host_write_u32_(guest_address, value);
        } else {
            LOG_ERROR("Host write_u32 callback not set");
        }
        
        // Reprotect with original protection
        protect_guest_memory(page_addr, page_size_, old_protection);
        
        // Synchronize caches
        insert_data_sync_barrier();
        insert_instruction_sync_barrier();
    } else {
        // Normal write to data page
        if (host_write_u32_) {
            host_write_u32_(guest_address, value);
        } else {
            LOG_ERROR("Host write_u32 callback not set");
        }
    }
}

void MemoryManager::write_u64(uint32_t guest_address, uint64_t value) {
    // Handle as two 32-bit writes
    uint32_t low_dword = static_cast<uint32_t>(value & 0xFFFFFFFF);
    uint32_t high_dword = static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF);
    
    // Assuming little-endian x86
    write_u32(guest_address, low_dword);
    write_u32(guest_address + 4, high_dword);
}

void MemoryManager::write_block(uint32_t guest_address, const void* host_buffer, uint32_t size) {
    // Check if this write crosses or is within a code page
    uint32_t start_page = align_to_page(guest_address);
    uint32_t end_page = align_to_page(guest_address + size - 1);
    bool is_smc = false;
    std::vector<uint32_t> affected_code_pages;
    
    // Identify affected code pages
    {
        std::lock_guard<std::mutex> lock(pages_mutex_);
        for (uint32_t page = start_page; page <= end_page; page += page_size_) {
            auto it = pages_.find(page);
            if (it != pages_.end() && it->second.has_translated_code) {
                affected_code_pages.push_back(page);
                is_smc = true;
            }
        }
    }
    
    if (is_smc) {
        LOG_INFO("SMC detected: Block write affecting " + 
                 std::to_string(affected_code_pages.size()) + " code pages");
        
        // For each affected code page, handle SMC
        for (uint32_t page : affected_code_pages) {
            // Temporarily unprotect
            int old_prot = get_protection(page);
            protect_guest_memory(page, page_size_, PROT_READ | PROT_WRITE);
            
            // Invalidate translations
            invalidate_translations_for_page(page);
            
            // Reprotect after write
            protect_guest_memory(page, page_size_, old_prot);
        }
        
        // Do the write
        if (host_write_block_) {
            host_write_block_(guest_address, host_buffer, size);
        } else {
            // Fallback to byte by byte
            const uint8_t* buffer = static_cast<const uint8_t*>(host_buffer);
            for (uint32_t i = 0; i < size; i++) {
                write_u8(guest_address + i, buffer[i]);
            }
        }
        
        // Synchronize caches
        insert_data_sync_barrier();
        insert_instruction_sync_barrier();
    } else {
        // Normal write to data pages
        if (host_write_block_) {
            host_write_block_(guest_address, host_buffer, size);
        } else {
            // Fallback to byte by byte
            const uint8_t* buffer = static_cast<const uint8_t*>(host_buffer);
            for (uint32_t i = 0; i < size; i++) {
                write_u8(guest_address + i, buffer[i]);
            }
        }
    }
}

bool MemoryManager::protect_guest_memory(uint32_t guest_address, uint32_t size, int protection) {
    uint32_t aligned_addr = align_to_page(guest_address);
    uint32_t aligned_size = ((size + page_size_ - 1) / page_size_) * page_size_;
    
    LOG_DEBUG("Protecting guest memory range 0x" + std::to_string(aligned_addr) + 
              " - 0x" + std::to_string(aligned_addr + aligned_size - 1) + 
              " with protection flags: " + std::to_string(protection));
    
    // Update our local page protection info
    std::lock_guard<std::mutex> lock(pages_mutex_);
    for (uint32_t addr = aligned_addr; addr < aligned_addr + aligned_size; addr += page_size_) {
        // Create or update the page info
        auto& page = pages_[addr];
        page.guest_address = addr;
        page.size = page_size_;
        page.protection = protection;
        // don't modify has_translated_code flag
    }
    
    return true;
}

int MemoryManager::get_protection(uint32_t guest_address) {
    uint32_t aligned_addr = align_to_page(guest_address);
    
    std::lock_guard<std::mutex> lock(pages_mutex_);
    auto it = pages_.find(aligned_addr);
    if (it != pages_.end()) {
        return it->second.protection;
    }
    
    // Default to read/write
    return PROT_READ | PROT_WRITE;
}

void MemoryManager::register_code_page(uint32_t guest_address, uint32_t size) {
    uint32_t aligned_addr = align_to_page(guest_address);
    uint32_t aligned_size = ((size + page_size_ - 1) / page_size_) * page_size_;
    
    LOG_INFO("Registering code page(s) from 0x" + std::to_string(aligned_addr) + 
             " to 0x" + std::to_string(aligned_addr + aligned_size - 1));
    
    std::lock_guard<std::mutex> lock(pages_mutex_);
    for (uint32_t addr = aligned_addr; addr < aligned_addr + aligned_size; addr += page_size_) {
        // Create or update the page info
        auto& page = pages_[addr];
        page.guest_address = addr;
        page.size = page_size_;
        page.has_translated_code = true;
        
        // If the page wasn't specifically protected before, mark it read-only now for SMC detection
        if (page.protection == 0 || page.protection == (PROT_READ | PROT_WRITE)) {
            page.protection = PROT_READ;
        }
    }
}

void MemoryManager::notify_memory_modified(uint32_t guest_address, uint32_t size) {
    uint32_t aligned_addr = align_to_page(guest_address);
    uint32_t aligned_size = ((size + page_size_ - 1) / page_size_) * page_size_;
    
    LOG_DEBUG("Guest memory modified at 0x" + std::to_string(guest_address) + 
              ", size: " + std::to_string(size));
    
    // Check if any affected pages have translated code
    std::vector<uint32_t> code_pages;
    {
        std::lock_guard<std::mutex> lock(pages_mutex_);
        for (uint32_t addr = aligned_addr; addr < aligned_addr + aligned_size; addr += page_size_) {
            auto it = pages_.find(addr);
            if (it != pages_.end() && it->second.has_translated_code) {
                code_pages.push_back(addr);
            }
        }
    }
    
    // Invalidate translations for all affected code pages
    for (uint32_t page_addr : code_pages) {
        LOG_INFO("Invalidating code page at 0x" + std::to_string(page_addr) + 
                 " due to memory modification");
        invalidate_translations_for_page(page_addr);
    }
}

void MemoryManager::handle_protection_fault(uint32_t guest_address) {
    uint32_t page_addr = align_to_page(guest_address);
    
    LOG_WARNING("Protection fault at guest address 0x" + std::to_string(guest_address) + 
                " (page 0x" + std::to_string(page_addr) + ")");
    
    // Get the current page info
    MemoryPage page;
    bool is_code_page = false;
    {
        std::lock_guard<std::mutex> lock(pages_mutex_);
        auto it = pages_.find(page_addr);
        if (it != pages_.end()) {
            page = it->second;
            is_code_page = page.has_translated_code;
        }
    }
    
    if (is_code_page) {
        // This is likely SMC! Handle it
        LOG_INFO("SMC detected: Protection fault in code page");
        
        // Temporarily make the page writable
        int old_protection = page.protection;
        protect_guest_memory(page_addr, page_size_, PROT_READ | PROT_WRITE);
        
        // Invalidate any translations for this page
        invalidate_translations_for_page(page_addr);
        
        // The guest memory access can now proceed (will be retried by the host)
        
        // After a short delay, restore the protection
        // In a real implementation, you might want to do this on a background thread
        // or after the current instruction completes
        protect_guest_memory(page_addr, page_size_, old_protection);
    } else {
        LOG_ERROR("Protection fault in non-code page - unexpected!");
    }
}

// ARM Memory barrier helpers
void MemoryManager::insert_data_memory_barrier() {
    // Generate ARM DMB instruction
    asm volatile("dmb ish" ::: "memory");
}

void MemoryManager::insert_data_sync_barrier() {
    // Generate ARM DSB instruction
    asm volatile("dsb ish" ::: "memory");
}

void MemoryManager::insert_instruction_sync_barrier() {
    // Generate ARM ISB instruction
    asm volatile("isb" ::: "memory");
}

void MemoryManager::set_host_memory_callbacks(
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
) {
    host_read_u8_ = read_u8_cb;
    host_read_u16_ = read_u16_cb;
    host_read_u32_ = read_u32_cb;
    host_read_u64_ = read_u64_cb;
    host_read_block_ = read_block_cb;
    
    host_write_u8_ = write_u8_cb;
    host_write_u16_ = write_u16_cb;
    host_write_u32_ = write_u32_cb;
    host_write_u64_ = write_u64_cb;
    host_write_block_ = write_block_cb;
    
    LOG_INFO("Host memory callbacks registered");
}

// Helper methods
uint32_t MemoryManager::align_to_page(uint32_t address) {
    return address & ~(page_size_ - 1);
}

bool MemoryManager::is_page_protected(uint32_t guest_address) {
    uint32_t page_addr = align_to_page(guest_address);
    
    std::lock_guard<std::mutex> lock(pages_mutex_);
    auto it = pages_.find(page_addr);
    if (it != pages_.end()) {
        return (it->second.protection & PROT_WRITE) == 0;
    }
    
    return false;
}

bool MemoryManager::invalidate_translations_for_page(uint32_t guest_address) {
    if (!translation_cache_) {
        LOG_ERROR("Translation cache is null, cannot invalidate translations");
        return false;
    }
    
    uint32_t page_addr = align_to_page(guest_address);
    
    // Invalidate all translations in this page
    translation_cache_->invalidate_range(page_addr, page_addr + page_size_ - 1);
    
    // Mark the page as dirty
    {
        std::lock_guard<std::mutex> lock(pages_mutex_);
        auto it = pages_.find(page_addr);
        if (it != pages_.end()) {
            it->second.is_dirty = true;
        }
    }
    
    return true;
}

void MemoryManager::reprotect_page(uint32_t guest_address, int new_protection) {
    uint32_t page_addr = align_to_page(guest_address);
    
    protect_guest_memory(page_addr, page_size_, new_protection);
}

} // namespace xenoarm_jit 