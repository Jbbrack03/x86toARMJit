#include <iostream>
#include <cstring> // For std::memset
#include "xenoarm_jit/api.h"
#include "logging/logger.h"

// Minimal host-provided memory
uint8_t g_guest_memory[0x1000]; // 4KB of guest memory

// Minimal host-provided log callback
void HostLogCallback(int level, const char* message) {
    XenoARM_JIT::LogLevel logLevel = static_cast<XenoARM_JIT::LogLevel>(level);
    XenoARM_JIT::Logger::getInstance().log(logLevel, message);
}

// Minimal host-provided memory read callback
uint8_t HostReadMemoryCallback(uint32_t address, void* user_data) {
    // For testing purposes, translate the guest address to a local array index
    uint32_t local_address = address & 0xFFF; // Mask to get offset within 4KB array
    
    if (local_address < sizeof(g_guest_memory)) {
        std::cout << "Reading from guest address 0x" << std::hex << address 
                 << " (local offset 0x" << local_address << "): 0x" 
                 << (int)g_guest_memory[local_address] << std::dec << std::endl;
        return g_guest_memory[local_address];
    }
    
    LOG_ERROR("Attempted to read outside guest memory range!");
    std::cout << "ERROR: Attempted to read outside guest memory range: 0x" 
              << std::hex << address << std::dec << std::endl;
    return 0; // Or some indicator of an invalid read
}

// Minimal host-provided memory write callback
void HostWriteMemoryCallback(uint32_t address, uint8_t value, void* user_data) {
    // For testing purposes, translate the guest address to a local array index
    uint32_t local_address = address & 0xFFF; // Mask to get offset within 4KB array
    
    if (local_address < sizeof(g_guest_memory)) {
        g_guest_memory[local_address] = value;
        std::cout << "Writing to guest address 0x" << std::hex << address 
                 << " (local offset 0x" << local_address << "): 0x" 
                 << (int)value << std::dec << std::endl;
    } else {
        LOG_ERROR("Attempted to write outside guest memory range!");
        std::cout << "ERROR: Attempted to write outside guest memory range: 0x" 
                 << std::hex << address << std::dec << std::endl;
    }
}

int main() {
    // Set log level to DEBUG to see all messages
    XenoARM_JIT::Logger::getInstance().setLogLevel(XenoARM_JIT::DEBUG);
    
    std::cout << "Minimal XenoARM JIT Host Stub" << std::endl;
    std::cout << "With enhanced debugging enabled" << std::endl;

    // Configure the JIT
    XenoARM_JIT::JitConfig config;
    config.log_callback = HostLogCallback;
    config.read_memory_u8 = HostReadMemoryCallback;
    config.write_memory_u8 = HostWriteMemoryCallback;

    // Initialize the JIT
    XenoARM_JIT::JitContext* jit_context = XenoARM_JIT::Jit_Init(config);

    if (!jit_context) {
        LOG_FATAL("Failed to initialize XenoARM JIT!");
        return 1;
    }

    LOG_INFO("XenoARM JIT initialized successfully.");

    // --- Phase 2: Basic JIT Translation and Execution Test ---
    LOG_DEBUG("Starting Phase 2: Basic JIT Translation and Execution Test");

    // Define a simple x86 code snippet in guest memory:
    // MOV EAX, 5      ; B8 05 00 00 00
    // MOV [0x100], EAX ; A3 00 01 00 00
    // RET             ; C3
    uint32_t guest_code_address = 0x1000; // Place the code at address 0x1000 in guest memory
    uint32_t local_offset = guest_code_address & 0xFFF; // Mask to get offset within 4KB array
    
    // Zero out the entire memory first
    std::memset(g_guest_memory, 0, sizeof(g_guest_memory));
    
    // Set up the test code at the correct local offset
    g_guest_memory[local_offset + 0] = 0xB8; // MOV EAX, imm32
    g_guest_memory[local_offset + 1] = 0x05; // imm32 (5)
    g_guest_memory[local_offset + 2] = 0x00;
    g_guest_memory[local_offset + 3] = 0x00;
    g_guest_memory[local_offset + 4] = 0x00;
    g_guest_memory[local_offset + 5] = 0xA3; // MOV [disp32], EAX
    g_guest_memory[local_offset + 6] = 0x00; // disp32 (0x100)
    g_guest_memory[local_offset + 7] = 0x01;
    g_guest_memory[local_offset + 8] = 0x00;
    g_guest_memory[local_offset + 9] = 0x00;
    g_guest_memory[local_offset + 10] = 0xC3; // RET

    LOG_INFO("Guest x86 code snippet placed at address 0x" + std::to_string(guest_code_address) + ".");
    std::cout << "Guest x86 code snippet placed at address 0x" << std::hex << guest_code_address 
              << " (local offset 0x" << local_offset << ")" << std::dec << std::endl;
    
    // Verify the instruction bytes are correctly set
    std::cout << "Instruction bytes:" << std::endl;
    for (int i = 0; i < 11; i++) {
        std::cout << "Byte " << i << " at address 0x" << std::hex << (guest_code_address + i) 
                  << ": 0x" << (int)g_guest_memory[local_offset + i] << std::dec << std::endl;
    }
    
    std::cout << "About to call Jit_TranslateBlock..." << std::endl;
    // Translate the guest code block
    void* translated_code_ptr = XenoARM_JIT::Jit_TranslateBlock(jit_context, guest_code_address);
    std::cout << "Jit_TranslateBlock returned: " << translated_code_ptr << std::endl;

    if (!translated_code_ptr) {
        LOG_FATAL("Failed to translate guest code block at 0x" + std::to_string(guest_code_address) + "!");
        // Shutdown the JIT before exiting
        XenoARM_JIT::Jit_Shutdown(jit_context);
        return 1;
    }

    LOG_INFO("Guest code block translated successfully. Translated code pointer: " + std::to_string(reinterpret_cast<uintptr_t>(translated_code_ptr)) + ".");

    // Execute the translated code block using a basic dispatcher loop
    LOG_INFO("Starting execution of translated code...");

    uint64_t current_guest_address = guest_code_address;
    void* current_translated_code_ptr = translated_code_ptr;

    while (current_translated_code_ptr) {
        LOG_DEBUG("Executing translated block for guest address 0x" + std::to_string(current_guest_address));
        std::cout << "About to call Jit_ExecuteTranslatedBlock..." << std::endl;

        // Execute the current translated block
        // The translated code is expected to return the guest address of the next block to execute,
        // or a special value to indicate a return or exception.
        // For now, this is a placeholder call to the translated code.
        // The actual execution will involve casting the translated_code_ptr to a function pointer
        // and calling it with the necessary context (guest registers, memory, etc.).
        // The return value will indicate the next guest address.
        uint64_t next_guest_address = XenoARM_JIT::Jit_ExecuteTranslatedBlock(jit_context, current_translated_code_ptr); // Hypothetical execution function
        std::cout << "Jit_ExecuteTranslatedBlock returned: " << next_guest_address << std::endl;

        LOG_DEBUG("Translated block returned next guest address: 0x" + std::to_string(next_guest_address));

        if (next_guest_address == 0) { // Example: 0 indicates end of execution (e.g., RET from main block)
            LOG_INFO("Execution finished.");
            break;
        }

        std::cout << "About to call Jit_LookupBlock..." << std::endl;
        // Look up the next block in the cache
        XenoARM_JIT::TranslatedBlock* next_block = XenoARM_JIT::Jit_LookupBlock(jit_context, next_guest_address); // Hypothetical lookup function
        std::cout << "Jit_LookupBlock returned: " << next_block << std::endl;

        if (next_block) {
            // Chained: continue execution with the next translated block
            current_guest_address = next_guest_address;
            current_translated_code_ptr = next_block->code.data(); // Assuming code.data() is executable
            LOG_DEBUG("Chained to next translated block at 0x" + std::to_string(current_guest_address));
        } else {
            // Not chained: translate the next block
            LOG_DEBUG("Next block not found in cache, translating block at 0x" + std::to_string(next_guest_address));
            current_guest_address = next_guest_address;
            current_translated_code_ptr = XenoARM_JIT::Jit_TranslateBlock(jit_context, current_guest_address);
            if (!current_translated_code_ptr) {
                LOG_FATAL("Failed to translate next guest code block at 0x" + std::to_string(current_guest_address) + "!");
                // Shutdown the JIT before exiting
                XenoARM_JIT::Jit_Shutdown(jit_context);
                return 1;
            }
            LOG_DEBUG("Translated next block. Translated code pointer: " + std::to_string(reinterpret_cast<uintptr_t>(current_translated_code_ptr)) + ".");
        }
    }

    LOG_INFO("Execution loop finished.");

    // Verify the result in guest memory (will show initial value as execution is stubbed)
    uint32_t verification_address = 0x100;
    uint8_t value_at_verification_address = HostReadMemoryCallback(verification_address, nullptr);
    std::cout << "Value in guest memory at 0x" << std::hex << verification_address << std::dec << " after execution attempt: 0x" << std::hex << (int)value_at_verification_address << std::dec << std::endl;

    // Shutdown the JIT
    XenoARM_JIT::Jit_Shutdown(jit_context);
    LOG_INFO("XenoARM JIT shutdown complete.");

    return 0;
}