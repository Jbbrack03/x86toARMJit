#ifndef XENOARM_JIT_CODEGEN_AARCH64_CODE_GENERATOR_H
#define XENOARM_JIT_CODEGEN_AARCH64_CODE_GENERATOR_H

#include "xenoarm_jit/ir.h"
#include "xenoarm_jit/memory_model.h"
#include <vector>
#include <cstdint>

namespace xenoarm_jit {
namespace aarch64 {

// Forward declarations
namespace register_allocation {
    class RegisterAllocator;
}

/**
 * Code generator for AArch64 (ARM64) architecture
 * Converts IR to AArch64 machine code
 */
class CodeGenerator {
public:
    CodeGenerator();
    ~CodeGenerator();

    // Generate code for an IR function or basic block
    bool generate_code(const ir::IrFunction& function);
    bool generate_code(const ir::IrBasicBlock& block);
    
    // Get the generated machine code
    const std::vector<uint8_t>& get_code() const;
    
    // Get the size of the generated code in bytes
    size_t get_code_size() const;
    
    // Clear all generated code
    void clear();
    
    // Emit a memory barrier instruction
    void emit_memory_barrier(MemoryModel::BarrierType barrier_type);
    
    // Set the register allocator to use
    void set_register_allocator(register_allocation::RegisterAllocator* allocator);

private:
    // The generated machine code
    std::vector<uint8_t> code_;
    
    // The register allocator
    register_allocation::RegisterAllocator* register_allocator_;
    
    // Helper methods to generate AArch64 instructions
    void emit_instruction(uint32_t instruction);
    
    // Various emit_* methods for specific AArch64 instructions
    // These would be implemented in the .cpp file
};

} // namespace aarch64
} // namespace xenoarm_jit

#endif // XENOARM_JIT_CODEGEN_AARCH64_CODE_GENERATOR_H 