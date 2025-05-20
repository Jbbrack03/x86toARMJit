#ifndef XENOARM_JIT_MEMORY_MODEL_H
#define XENOARM_JIT_MEMORY_MODEL_H

#include <cstdint>
#include "xenoarm_jit/ir.h"

namespace xenoarm_jit {

// Forward declarations
namespace aarch64 {
class CodeGenerator;
}

/**
 * Memory model handler class to manage differences between 
 * x86 TSO (Total Store Order) and ARM's weaker memory model
 */
class MemoryModel {
public:
    MemoryModel();
    ~MemoryModel();
    
    enum BarrierType {
        BARRIER_NONE = 0,
        // x86 memory fence instructions
        BARRIER_MFENCE,      // Full memory fence
        BARRIER_SFENCE,      // Store fence
        BARRIER_LFENCE,      // Load fence
        // x86 implicit barriers
        BARRIER_LOCK_PREFIX, // LOCK prefix (e.g., LOCK CMPXCHG)
        BARRIER_XCHG,        // XCHG has implicit LOCK
        // ARM specific barriers
        BARRIER_DMB_ISH,     // Data Memory Barrier - Inner Shareable
        BARRIER_DSB_ISH,     // Data Synchronization Barrier - Inner Shareable
        BARRIER_ISB          // Instruction Synchronization Barrier
    };
    
    // This method is called by the IR generator when translating x86 instructions
    // that have memory ordering implications (MFENCE, SFENCE, LFENCE, LOCK prefix)
    void add_memory_barrier_to_ir(ir::IrBasicBlock* block, BarrierType barrier_type);
    
    // This method is called by the code generator to emit ARM barrier instructions
    void emit_memory_barrier(aarch64::CodeGenerator* code_gen, BarrierType barrier_type);
    
    // Analyze a memory load operation to determine if it needs ARM memory barriers
    // Returns the appropriate barrier type needed before the load
    BarrierType analyze_load_operation(const ir::IrInstruction& insn);
    
    // Analyze a memory store operation to determine if it needs ARM memory barriers
    // Returns the appropriate barrier type needed after the store
    BarrierType analyze_store_operation(const ir::IrInstruction& insn);
    
    // Helper method to determine if a memory barrier is needed between two memory operations
    bool needs_barrier_between(const ir::IrInstruction& first, const ir::IrInstruction& second);
    
private:
    // Helper methods to emit specific ARM barriers
    static void emit_arm_dmb_ish(aarch64::CodeGenerator* code_gen);  // Data Memory Barrier
    static void emit_arm_dsb_ish(aarch64::CodeGenerator* code_gen);  // Data Sync Barrier
    static void emit_arm_isb(aarch64::CodeGenerator* code_gen);      // Instruction Sync Barrier
};

} // namespace xenoarm_jit

#endif // XENOARM_JIT_MEMORY_MODEL_H 