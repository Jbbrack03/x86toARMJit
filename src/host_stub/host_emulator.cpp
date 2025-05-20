#include "xenoarm_jit/host_stub/host_emulator.h"
#include "../logging/logger.h"
#include <iostream>
#include <cstring>
#include <vector>

namespace xenoarm_jit {
namespace host_stub {

// Initialize static member
HostEmulator* HostEmulator::current_instance_ = nullptr;

HostEmulator::HostEmulator() 
    : jit_context_(nullptr),
      guest_memory_(nullptr),
      guest_memory_size_(0),
      entry_point_(0)
{
    LOG_DEBUG("HostEmulator created.");
    // Set the current instance for callback redirection
    current_instance_ = this;
}

HostEmulator::~HostEmulator() {
    if (guest_memory_) {
        delete[] guest_memory_;
        guest_memory_ = nullptr;
    }
    
    if (jit_context_) {
        XenoARM_JIT::Jit_Shutdown(jit_context_);
        jit_context_ = nullptr;
    }
    
    LOG_DEBUG("HostEmulator destroyed.");
    
    // Clear the current instance if it's this one
    if (current_instance_ == this) {
        current_instance_ = nullptr;
    }
}

bool HostEmulator::initialize(size_t memory_size) {
    if (memory_size == 0) {
        LOG_ERROR("Cannot initialize host emulator with zero memory size.");
        return false;
    }
    
    if (guest_memory_) {
        delete[] guest_memory_;
    }
    
    try {
        guest_memory_ = new uint8_t[memory_size];
        guest_memory_size_ = memory_size;
        std::memset(guest_memory_, 0, memory_size);
        LOG_DEBUG("Allocated " + std::to_string(memory_size) + " bytes for guest memory.");
        return true;
    } catch (const std::bad_alloc&) {
        LOG_ERROR("Failed to allocate " + std::to_string(memory_size) + " bytes for guest memory.");
        guest_memory_ = nullptr;
        guest_memory_size_ = 0;
        return false;
    }
}

bool HostEmulator::initializeJIT() {
    if (!guest_memory_ || guest_memory_size_ == 0) {
        LOG_ERROR("Cannot initialize JIT without guest memory.");
        return false;
    }
    
    // Initialize JIT API
    XenoARM_JIT::JitConfig config;
    config.log_callback = logCallbackWrapper;
    config.read_memory_callback = readMemoryCallbackWrapper;
    config.write_memory_callback = writeMemoryCallbackWrapper;
    
    jit_context_ = XenoARM_JIT::Jit_Init(config);
    if (!jit_context_) {
        LOG_ERROR("Failed to create JIT context.");
        return false;
    }
    
    LOG_DEBUG("JIT initialized successfully.");
    return true;
}

bool HostEmulator::loadProgram(const uint8_t* program, size_t size, uint64_t load_address) {
    if (!guest_memory_ || guest_memory_size_ == 0) {
        LOG_ERROR("Cannot load program without guest memory.");
        return false;
    }
    
    if (load_address + size > guest_memory_size_) {
        LOG_ERROR("Program would exceed guest memory bounds.");
        return false;
    }
    
    std::memcpy(guest_memory_ + load_address, program, size);
    entry_point_ = load_address;
    LOG_DEBUG("Loaded program of size " + std::to_string(size) + " bytes at address 0x" + 
              std::to_string(load_address) + ".");
    return true;
}

bool HostEmulator::run() {
    if (!jit_context_) {
        LOG_ERROR("Cannot run without initialized JIT.");
        return false;
    }
    
    LOG_DEBUG("Starting execution at entry point 0x" + std::to_string(entry_point_) + ".");
    
    // First, translate the block at the entry point
    void* translated_code = XenoARM_JIT::Jit_TranslateBlock(jit_context_, static_cast<uint32_t>(entry_point_));
    if (!translated_code) {
        LOG_ERROR("Failed to translate block at entry point 0x" + std::to_string(entry_point_) + ".");
        return false;
    }
    
    // Now execute the translated block
    LOG_DEBUG("Executing translated block at entry point 0x" + std::to_string(entry_point_) + ".");
    uint64_t next_address = XenoARM_JIT::Jit_ExecuteTranslatedBlock(jit_context_, translated_code);
    
    // This is a very simple dispatcher - for testing we're just executing one block
    if (next_address == 0) {
        LOG_DEBUG("Execution completed successfully.");
        return true;
    } else {
        LOG_ERROR("Execution failed or returned unexpected next address: 0x" + std::to_string(next_address) + ".");
        return false;
    }
}

bool HostEmulator::memoryReadCallback(uint64_t address, void* buffer, size_t size) {
    if (!guest_memory_ || address + size > guest_memory_size_) {
        LOG_ERROR("Memory read out of bounds: address=0x" + std::to_string(address) + 
                 ", size=" + std::to_string(size) + ".");
        return false;
    }
    
    std::memcpy(buffer, guest_memory_ + address, size);
    return true;
}

bool HostEmulator::memoryWriteCallback(uint64_t address, const void* buffer, size_t size) {
    if (!guest_memory_ || address + size > guest_memory_size_) {
        LOG_ERROR("Memory write out of bounds: address=0x" + std::to_string(address) + 
                 ", size=" + std::to_string(size) + ".");
        return false;
    }
    
    std::memcpy(guest_memory_ + address, buffer, size);
    return true;
}

bool HostEmulator::exceptionCallback(uint64_t address, uint32_t exception_code) {
    LOG_ERROR("Guest exception at address 0x" + std::to_string(address) + 
             ", code=0x" + std::to_string(exception_code) + ".");
    return false; // Don't handle exception, terminate execution
}

// Static callback wrappers

uint8_t HostEmulator::readMemoryCallbackWrapper(uint32_t address) {
    if (!current_instance_ || !current_instance_->guest_memory_ || 
        address >= current_instance_->guest_memory_size_) {
        LOG_ERROR("Invalid memory read: address=0x" + std::to_string(address) + ".");
        return 0;
    }
    
    return current_instance_->guest_memory_[address];
}

void HostEmulator::writeMemoryCallbackWrapper(uint32_t address, uint8_t value) {
    if (!current_instance_ || !current_instance_->guest_memory_ ||
        address >= current_instance_->guest_memory_size_) {
        LOG_ERROR("Invalid memory write: address=0x" + std::to_string(address) + 
                 ", value=0x" + std::to_string(value) + ".");
        return;
    }
    
    current_instance_->guest_memory_[address] = value;
}

void HostEmulator::logCallbackWrapper(int level, const char* message) {
    // Map JIT log levels to our logger levels
    switch (level) {
        case 0: // Error
            LOG_ERROR(message);
            break;
        case 1: // Warning
            LOG_WARNING(message);
            break;
        case 2: // Info
            LOG_INFO(message);
            break;
        case 3: // Debug
        default:
            LOG_DEBUG(message);
            break;
    }
}

} // namespace host_stub
} // namespace xenoarm_jit 