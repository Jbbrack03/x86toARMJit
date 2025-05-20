#ifndef XENOARM_JIT_API_H
#define XENOARM_JIT_API_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct JitInstance JitInstance;

// Memory read/write callbacks
typedef uint8_t (*ReadMemory8Callback)(void* context, uint32_t address);
typedef uint32_t (*ReadMemory32Callback)(void* context, uint32_t address);
typedef void (*WriteMemory8Callback)(void* context, uint32_t address, uint8_t value);
typedef void (*WriteMemory32Callback)(void* context, uint32_t address, uint32_t value);

// JIT initialization parameters
typedef struct {
    void* hostContextPtr;              // Host-specific context
    ReadMemory8Callback readMemory8Callback;
    ReadMemory32Callback readMemory32Callback;
    WriteMemory8Callback writeMemory8Callback;
    WriteMemory32Callback writeMemory32Callback;
} JitInitParams;

// Create a new JIT instance
JitInstance* jitCreate(const JitInitParams* params);

// Destroy a JIT instance
void jitDestroy(JitInstance* jit);

// Compile code at guest address and return host code pointer
void* jitGetHostCodeForGuestAddress(JitInstance* jit, uint32_t guestAddress);

// Execute compiled code
void jitExecute(JitInstance* jit, void* hostCode);

// Invalidate a range of guest code
void jitInvalidateCache(JitInstance* jit, uint32_t guestAddress, size_t size);

// Invalidate all cached code
void jitInvalidateAllCache(JitInstance* jit);

// Chain two blocks together
bool jitChainBlocks(JitInstance* jit, uint32_t fromAddress, uint32_t toAddress);

// Register access
uint32_t jitGetGuestRegister(JitInstance* jit, int regIndex);
void jitSetGuestRegister(JitInstance* jit, int regIndex, uint32_t value);

// EIP access
uint32_t jitGetGuestEIP(JitInstance* jit);
void jitSetGuestEIP(JitInstance* jit, uint32_t value);

// EFLAGS access
uint32_t jitGetGuestEflags(JitInstance* jit);
void jitSetGuestEflags(JitInstance* jit, uint32_t value);

// MMX register access
uint64_t jitGetGuestMMXRegister(JitInstance* jit, int regIndex);
void jitSetGuestMMXRegister(JitInstance* jit, int regIndex, uint64_t value);

// XMM register access
void jitGetGuestXMMRegister(JitInstance* jit, int regIndex, uint8_t* value);
void jitSetGuestXMMRegister(JitInstance* jit, int regIndex, const uint8_t* value);

#ifdef __cplusplus
}
#endif

#endif // XENOARM_JIT_API_H 