#ifndef XENOARM_JIT_HOST_STUB_HOST_EMULATOR_H
#define XENOARM_JIT_HOST_STUB_HOST_EMULATOR_H

#include "xenoarm_jit/api.h"
#include <cstdint>
#include <cstddef>

namespace xenoarm_jit {
namespace host_stub {

/**
 * A simple host emulator for testing the XenoARM JIT.
 * It provides a basic implementation of memory and exception callbacks.
 */
class HostEmulator {
public:
    HostEmulator();
    ~HostEmulator();
    
    /**
     * Initializes the host emulator with the specified guest memory size.
     * 
     * @param memory_size The size of the guest memory to allocate, in bytes.
     * @return true if initialization succeeded, false otherwise.
     */
    bool initialize(size_t memory_size);
    
    /**
     * Initializes the JIT and registers callbacks.
     * 
     * @return true if JIT initialization succeeded, false otherwise.
     */
    bool initializeJIT();
    
    /**
     * Loads a program into guest memory.
     * 
     * @param program Pointer to the program data.
     * @param size Size of the program data, in bytes.
     * @param load_address Address in guest memory to load the program at.
     * @return true if the program was loaded successfully, false otherwise.
     */
    bool loadProgram(const uint8_t* program, size_t size, uint64_t load_address);
    
    /**
     * Runs the loaded program from the entry point.
     * 
     * @return true if execution succeeded, false if an error occurred.
     */
    bool run();
    
    /**
     * Gets the JIT context.
     * 
     * @return Pointer to the JIT context, or nullptr if not initialized.
     */
    XenoARM_JIT::JitContext* getJITContext() { return jit_context_; }
    
    /**
     * Gets a pointer to the guest memory.
     * 
     * @return Pointer to the guest memory, or nullptr if not initialized.
     */
    uint8_t* getGuestMemory() { return guest_memory_; }
    
    /**
     * Gets the size of the guest memory.
     * 
     * @return Size of the guest memory, in bytes.
     */
    size_t getGuestMemorySize() { return guest_memory_size_; }
    
private:
    XenoARM_JIT::JitContext* jit_context_; // JIT context
    uint8_t* guest_memory_;                // Guest memory buffer
    size_t guest_memory_size_;             // Size of guest memory
    uint64_t entry_point_;                 // Program entry point
    
    /**
     * Memory read callback function.
     * 
     * @param address Guest address to read from.
     * @param buffer Host buffer to read into.
     * @param size Number of bytes to read.
     * @return true if the read was successful, false otherwise.
     */
    bool memoryReadCallback(uint64_t address, void* buffer, size_t size);
    
    /**
     * Memory write callback function.
     * 
     * @param address Guest address to write to.
     * @param buffer Host buffer containing the data to write.
     * @param size Number of bytes to write.
     * @return true if the write was successful, false otherwise.
     */
    bool memoryWriteCallback(uint64_t address, const void* buffer, size_t size);
    
    /**
     * Exception callback function.
     * 
     * @param address Guest address where the exception occurred.
     * @param exception_code Exception code.
     * @return true if the exception was handled, false to terminate execution.
     */
    bool exceptionCallback(uint64_t address, uint32_t exception_code);
    
    // Callback wrappers for the JIT API
    static uint8_t readMemoryCallbackWrapper(uint32_t address);
    static void writeMemoryCallbackWrapper(uint32_t address, uint8_t value);
    static void logCallbackWrapper(int level, const char* message);
    
    // Static pointer to the current emulator instance for callback redirection
    static HostEmulator* current_instance_;
};

} // namespace host_stub
} // namespace xenoarm_jit

#endif // XENOARM_JIT_HOST_STUB_HOST_EMULATOR_H 