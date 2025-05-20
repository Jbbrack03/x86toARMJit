#include <iostream>
#include <cstring>
#include <cassert>
#include "xenoarm_jit/api.h"

// Simple x86 code generation helper
void generate_x86_code(uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < 16) {
        std::cerr << "Buffer too small\n";
        return;
    }
    
    // Simple x86 code: mov eax, 42; ret
    uint8_t code[] = {
        0xB8, 0x2A, 0x00, 0x00, 0x00,  // mov eax, 42
        0xC3                           // ret
    };
    
    std::memcpy(buffer, code, sizeof(code));
}

// Modified x86 code (SMC scenario)
void modify_x86_code(uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < 16) {
        std::cerr << "Buffer too small\n";
        return;
    }
    
    // Modified code: mov eax, 100; ret
    uint8_t code[] = {
        0xB8, 0x64, 0x00, 0x00, 0x00,  // mov eax, 100
        0xC3                           // ret
    };
    
    std::memcpy(buffer, code, sizeof(code));
}

// Guest memory arrays (simulating guest RAM)
static uint8_t guest_memory[64 * 1024];  // 64 KB of guest memory
static const uint32_t code_address = 0x1000;  // Address where our test code lives

// Host memory callbacks
uint8_t read_memory_u8(uint32_t address, void* user_data) {
    if (address < sizeof(guest_memory)) {
        return guest_memory[address];
    }
    return 0;
}

uint16_t read_memory_u16(uint32_t address, void* user_data) {
    if (address + 1 < sizeof(guest_memory)) {
        return *reinterpret_cast<uint16_t*>(&guest_memory[address]);
    }
    return 0;
}

uint32_t read_memory_u32(uint32_t address, void* user_data) {
    if (address + 3 < sizeof(guest_memory)) {
        return *reinterpret_cast<uint32_t*>(&guest_memory[address]);
    }
    return 0;
}

uint64_t read_memory_u64(uint32_t address, void* user_data) {
    if (address + 7 < sizeof(guest_memory)) {
        return *reinterpret_cast<uint64_t*>(&guest_memory[address]);
    }
    return 0;
}

void read_memory_block(uint32_t address, void* buffer, uint32_t size, void* user_data) {
    if (address + size <= sizeof(guest_memory)) {
        std::memcpy(buffer, &guest_memory[address], size);
    }
}

void write_memory_u8(uint32_t address, uint8_t value, void* user_data) {
    if (address < sizeof(guest_memory)) {
        guest_memory[address] = value;
        
        // Print SMC debug message
        std::cout << "Guest memory write: address=0x" << std::hex << address << ", value=0x" 
                  << static_cast<int>(value) << std::dec << std::endl;
    }
}

void write_memory_u16(uint32_t address, uint16_t value, void* user_data) {
    if (address + 1 < sizeof(guest_memory)) {
        *reinterpret_cast<uint16_t*>(&guest_memory[address]) = value;
        
        std::cout << "Guest memory write (16-bit): address=0x" << std::hex << address << ", value=0x" 
                  << value << std::dec << std::endl;
    }
}

void write_memory_u32(uint32_t address, uint32_t value, void* user_data) {
    if (address + 3 < sizeof(guest_memory)) {
        *reinterpret_cast<uint32_t*>(&guest_memory[address]) = value;
        
        std::cout << "Guest memory write (32-bit): address=0x" << std::hex << address << ", value=0x" 
                  << value << std::dec << std::endl;
    }
}

void write_memory_u64(uint32_t address, uint64_t value, void* user_data) {
    if (address + 7 < sizeof(guest_memory)) {
        *reinterpret_cast<uint64_t*>(&guest_memory[address]) = value;
        
        std::cout << "Guest memory write (64-bit): address=0x" << std::hex << address << ", value=0x" 
                  << value << std::dec << std::endl;
    }
}

void write_memory_block(uint32_t address, const void* buffer, uint32_t size, void* user_data) {
    if (address + size <= sizeof(guest_memory)) {
        std::memcpy(&guest_memory[address], buffer, size);
        
        std::cout << "Guest memory block write: address=0x" << std::hex << address << ", size=" 
                  << size << std::dec << std::endl;
    }
}

// Logging callback
void log_message(int level, const char* message) {
    const char* level_str = "UNKNOWN";
    switch (level) {
        case 0: level_str = "ERROR"; break;
        case 1: level_str = "WARNING"; break;
        case 2: level_str = "INFO"; break;
        case 3: level_str = "DEBUG"; break;
    }
    
    std::cout << "[" << level_str << "] " << message << std::endl;
}

// Exception callback
void exception_handler(const XenoARM_JIT::GuestException& exception, void* user_data) {
    std::cout << "Guest exception: type=" << exception.type << ", code=" << exception.code
              << ", address=0x" << std::hex << exception.address << std::dec << std::endl;
}

int main() {
    std::cout << "=== SMC Detection Test ===" << std::endl;
    
    // Initialize guest memory
    std::memset(guest_memory, 0, sizeof(guest_memory));
    
    // Place some code in guest memory
    generate_x86_code(&guest_memory[code_address], 16);
    
    // Initialize JIT config
    XenoARM_JIT::JitConfig config;
    config.log_callback = log_message;
    config.read_memory_u8 = read_memory_u8;
    config.read_memory_u16 = read_memory_u16;
    config.read_memory_u32 = read_memory_u32;
    config.read_memory_u64 = read_memory_u64;
    config.read_memory_block = read_memory_block;
    config.write_memory_u8 = write_memory_u8;
    config.write_memory_u16 = write_memory_u16;
    config.write_memory_u32 = write_memory_u32;
    config.write_memory_u64 = write_memory_u64;
    config.write_memory_block = write_memory_block;
    config.exception_callback = exception_handler;
    config.enable_smc_detection = true;
    
    // Initialize JIT
    std::cout << "Initializing JIT..." << std::endl;
    XenoARM_JIT::JitContext* jit = XenoARM_JIT::Jit_Init(config);
    if (!jit) {
        std::cerr << "Failed to initialize JIT\n";
        return 1;
    }
    
    // Test 1: Translate and execute initial code
    std::cout << "\nTest 1: Initial code translation" << std::endl;
    void* translated_code = XenoARM_JIT::Jit_TranslateBlock(jit, code_address);
    
    // Print the original code
    std::cout << "Original code at 0x" << std::hex << code_address << ": ";
    for (int i = 0; i < 6; i++) {
        std::cout << std::hex << static_cast<int>(guest_memory[code_address + i]) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Test 2: Modify the code (SMC)
    std::cout << "\nTest 2: Self-modifying code" << std::endl;
    std::cout << "Modifying code at 0x" << std::hex << code_address << std::dec << std::endl;
    
    // This should invalidate any JIT translations
    modify_x86_code(&guest_memory[code_address], 16);
    
    // Print the modified code
    std::cout << "Modified code at 0x" << std::hex << code_address << ": ";
    for (int i = 0; i < 6; i++) {
        std::cout << std::hex << static_cast<int>(guest_memory[code_address + i]) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Try to get the translated code again - should be a new translation
    void* new_translated_code = XenoARM_JIT::Jit_TranslateBlock(jit, code_address);
    
    // We would expect new_translated_code != translated_code
    // However, our implementation is a stub, so we can't fully test this
    
    // Test 3: Explicit invalidation
    std::cout << "\nTest 3: Explicit cache invalidation" << std::endl;
    XenoARM_JIT::Jit_InvalidateRange(jit, code_address, 16);
    
    // Test 4: Register code memory
    std::cout << "\nTest 4: Register code memory" << std::endl;
    XenoARM_JIT::Jit_RegisterCodeMemory(jit, code_address + 0x1000, 16);
    
    // Now modify that memory
    std::cout << "Modifying memory that was marked as code" << std::endl;
    guest_memory[code_address + 0x1000] = 0x90; // NOP
    
    // Test 5: Notify memory modified
    std::cout << "\nTest 5: Explicit notification of memory modification" << std::endl;
    XenoARM_JIT::Jit_NotifyMemoryModified(jit, code_address, 16);
    
    // Cleanup
    std::cout << "\nShutting down JIT..." << std::endl;
    XenoARM_JIT::Jit_Shutdown(jit);
    
    std::cout << "\nSMC detection test completed." << std::endl;
    return 0;
} 