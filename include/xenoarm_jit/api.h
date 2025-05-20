#ifndef XENOARM_JIT_API_H
#define XENOARM_JIT_API_H

#include <cstdint>
#include <cstddef> // For size_t
#include "xenoarm_jit/translation_cache/translation_cache.h"
#include "xenoarm_jit/register_allocation/register_allocator.h"
#include "xenoarm_jit/aarch64/code_generator.h"
#include "xenoarm_jit/decoder.h" // Include for X86Decoder
#include "xenoarm_jit/memory_manager.h" // Include for memory manager
#include "xenoarm_jit/memory_model.h" // Include for memory model
#include "xenoarm_jit/signal_handler.h" // Include for signal handler

namespace XenoARM_JIT {

// Forward declaration of the JIT context/state
struct JitContext;

// Alias the translated block type for simplicity in the API
using TranslatedBlock = xenoarm_jit::translation_cache::TranslatedBlock;

// Exception types for guest CPU
enum GuestExceptionType {
    EXCEPTION_NONE = 0,
    EXCEPTION_DIVIDE_BY_ZERO,
    EXCEPTION_DEBUG,
    EXCEPTION_BREAKPOINT,
    EXCEPTION_OVERFLOW,
    EXCEPTION_BOUND_RANGE,
    EXCEPTION_INVALID_OPCODE,
    EXCEPTION_DEVICE_NOT_AVAILABLE,
    EXCEPTION_DOUBLE_FAULT,
    EXCEPTION_SEGMENT_OVERRUN,
    EXCEPTION_INVALID_TSS,
    EXCEPTION_SEGMENT_NOT_PRESENT,
    EXCEPTION_STACK_FAULT,
    EXCEPTION_GENERAL_PROTECTION,
    EXCEPTION_PAGE_FAULT,
    EXCEPTION_X87_FLOATING_POINT,
    EXCEPTION_ALIGNMENT_CHECK,
    EXCEPTION_MACHINE_CHECK,
    EXCEPTION_SIMD_FLOATING_POINT
};

// Structure for guest exception information
struct GuestException {
    GuestExceptionType type;
    uint32_t code;
    uint32_t address;
};

// Function pointer types for host callbacks
// The host emulator will provide implementations for these
typedef void (*LogCallback)(int level, const char* message);

// Memory read callbacks
typedef uint8_t (*ReadMemoryU8Callback)(uint32_t address, void* user_data);
typedef uint16_t (*ReadMemoryU16Callback)(uint32_t address, void* user_data);
typedef uint32_t (*ReadMemoryU32Callback)(uint32_t address, void* user_data);
typedef uint64_t (*ReadMemoryU64Callback)(uint32_t address, void* user_data);
typedef void (*ReadMemoryBlockCallback)(uint32_t address, void* buffer, uint32_t size, void* user_data);

// Memory write callbacks
typedef void (*WriteMemoryU8Callback)(uint32_t address, uint8_t value, void* user_data);
typedef void (*WriteMemoryU16Callback)(uint32_t address, uint16_t value, void* user_data);
typedef void (*WriteMemoryU32Callback)(uint32_t address, uint32_t value, void* user_data);
typedef void (*WriteMemoryU64Callback)(uint32_t address, uint64_t value, void* user_data);
typedef void (*WriteMemoryBlockCallback)(uint32_t address, const void* buffer, uint32_t size, void* user_data);

// Guest exception callback
typedef void (*GuestExceptionCallback)(const GuestException& exception, void* user_data);

// Log level constants
enum LogLevels {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
};

// Memory barrier type constants
enum BarrierType {
    BARRIER_TYPE_NONE = 0,
    BARRIER_TYPE_FULL = 1,  // Full memory barrier (read/write)
    BARRIER_TYPE_STORE = 2, // Store barrier
    BARRIER_TYPE_LOAD = 3   // Load barrier
};

// Error handling constants
enum JitErrorCodes {
    JIT_ERROR_NONE = 0,
    JIT_ERROR_INVALID_PARAMETER = 1,
    JIT_ERROR_MEMORY_ALLOCATION = 2,
    JIT_ERROR_TRANSLATION_FAILED = 3,
    JIT_ERROR_EXECUTION_FAILED = 4,
    JIT_ERROR_NOT_IMPLEMENTED = 5
};

// Configuration structure for the JIT
struct JitConfig {
    // User data for callbacks
    void* user_data;
    
    // Logging
    LogCallback log_callback;
    
    // Memory access
    ReadMemoryU8Callback read_memory_u8;
    ReadMemoryU16Callback read_memory_u16;
    ReadMemoryU32Callback read_memory_u32;
    ReadMemoryU64Callback read_memory_u64;
    ReadMemoryBlockCallback read_memory_block;
    
    WriteMemoryU8Callback write_memory_u8;
    WriteMemoryU16Callback write_memory_u16;
    WriteMemoryU32Callback write_memory_u32;
    WriteMemoryU64Callback write_memory_u64;
    WriteMemoryBlockCallback write_memory_block;
    
    // Exception handling
    GuestExceptionCallback exception_callback;
    
    // Code cache size (in bytes)
    size_t code_cache_size;
    
    // Memory page size (for SMC detection)
    size_t page_size;
    
    // Enable SMC detection
    bool enable_smc_detection;
    
    // Memory model settings
    bool conservative_memory_model; // If true, use more memory barriers for compatibility
    
    // Constructor with defaults
    JitConfig() 
        : user_data(nullptr), 
          log_callback(nullptr),
          read_memory_u8(nullptr), read_memory_u16(nullptr),
          read_memory_u32(nullptr), read_memory_u64(nullptr),
          read_memory_block(nullptr),
          write_memory_u8(nullptr), write_memory_u16(nullptr),
          write_memory_u32(nullptr), write_memory_u64(nullptr),
          write_memory_block(nullptr),
          exception_callback(nullptr),
          code_cache_size(16 * 1024 * 1024), // 16MB default
          page_size(4096), // 4KB default
          enable_smc_detection(true),
          conservative_memory_model(true)
    {}
};

// Definition of the JIT context/state
struct JitContext {
    // Configuration provided by the host
    JitConfig config;

    // JIT components
    xenoarm_jit::decoder::X86Decoder* decoder;
    xenoarm_jit::translation_cache::TranslationCache* translation_cache;
    xenoarm_jit::register_allocation::RegisterAllocator* register_allocator;
    xenoarm_jit::aarch64::CodeGenerator* code_generator;
    xenoarm_jit::MemoryManager* memory_manager;
    xenoarm_jit::MemoryModel* memory_model;
    
    // Guest CPU state (registers, flags, etc.)
    // This would be defined in a separate header
    void* cpu_state;
};

// Initialize the JIT
// Returns a JitContext handle on success, nullptr on failure
JitContext* Jit_Init(const JitConfig& config);

// Shutdown the JIT and free resources
void Jit_Shutdown(JitContext* context);

// Translate a block of guest code at the given guest_address
// Returns a pointer to the translated host code on success, nullptr on failure
void* Jit_TranslateBlock(JitContext* context, uint32_t guest_address);

// Execute the translated code block
// This function will jump into the JITted code
// The JITted code is expected to eventually return control to the host
void Jit_ExecuteBlock(JitContext* context, void* translated_code_ptr);

// Execute the translated code block and return the next guest address
// This is used by the host_stub to implement a simple dispatcher
uint32_t Jit_ExecuteTranslatedBlock(JitContext* context, void* translated_code_ptr);

// Look up a translated block in the Translation Cache
// Returns a pointer to the translated block on success, nullptr if not found
void* Jit_LookupBlock(JitContext* context, uint32_t guest_address);

// Invalidate a range of guest memory in the translation cache
void Jit_InvalidateRange(JitContext* context, uint32_t guest_address, size_t size);

// Register memory as containing code that may be JITted
// This is used for SMC detection
void Jit_RegisterCodeMemory(JitContext* context, uint32_t guest_address, size_t size);

// Notify the JIT that a range of guest memory has been modified
// This allows the JIT to invalidate any cached translations
void Jit_NotifyMemoryModified(JitContext* context, uint32_t guest_address, size_t size);

// Handle a memory protection fault (e.g., from a signal handler)
// Returns true if the fault was handled, false otherwise
bool Jit_HandleProtectionFault(JitContext* context, uint32_t fault_address);

// Get the current guest CPU state
// The actual implementation would use a specific guest CPU state structure
bool Jit_GetGuestCpuState(JitContext* context, void* state, size_t size);

// Set the current guest CPU state
// The actual implementation would use a specific guest CPU state structure
bool Jit_SetGuestCpuState(JitContext* context, const void* state, size_t size);

// Get the current value of a guest CPU register
uint32_t Jit_GetGuestRegister(JitContext* context, int reg_index);

// Set the value of a guest CPU register
void Jit_SetGuestRegister(JitContext* context, int reg_index, uint32_t value);

// Get the current guest EFLAGS register
uint32_t Jit_GetGuestEflags(JitContext* context);

// Set the guest EFLAGS register
void Jit_SetGuestEflags(JitContext* context, uint32_t eflags);

// Get/Set MMX register (64-bit)
bool Jit_GetGuestMMXRegister(JitContext* context, int reg_idx, uint64_t* value_out);
bool Jit_SetGuestMMXRegister(JitContext* context, int reg_idx, uint64_t value_in);

// Get/Set XMM register (128-bit)
// value_out must point to a 16-byte buffer
bool Jit_GetGuestXMMRegister(JitContext* context, int reg_idx, void* value_out);
// value_in must point to a 16-byte buffer
bool Jit_SetGuestXMMRegister(JitContext* context, int reg_idx, const void* value_in);

// Get information about the JIT
// e.g., version, capabilities, stats
bool Jit_GetInfo(JitContext* context, void* info, size_t size);

// New API functions added in Phase 6
// Insert a memory barrier in the generated code
bool Jit_InsertMemoryBarrier(JitContext* context, BarrierType barrier_type);

// Get the last error that occurred in the JIT
int Jit_GetLastError(JitContext* context);

// Get a string description of an error code
const char* Jit_GetErrorString(int error_code);

// Set the log level for JIT operations
bool Jit_SetLogLevel(JitContext* context, int level);

// Enable or disable debug output
bool Jit_EnableDebugOutput(JitContext* context, bool enable);

} // namespace XenoARM_JIT

#endif // XENOARM_JIT_API_H