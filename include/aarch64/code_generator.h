#ifndef XENOARM_JIT_AARCH64_CODE_GENERATOR_H
#define XENOARM_JIT_AARCH64_CODE_GENERATOR_H

#include "ir/ir.h"
#include <cstdint>
#include <cstddef>

// AArch64 register IDs
enum AArch64Register {
    ARM64_REG_X0,
    ARM64_REG_X1,
    ARM64_REG_X2,
    ARM64_REG_X3,
    ARM64_REG_X4,
    ARM64_REG_X5,
    ARM64_REG_X6,
    ARM64_REG_X7,
    ARM64_REG_X8,
    ARM64_REG_X9,
    ARM64_REG_X10,
    ARM64_REG_X11,
    ARM64_REG_X12,
    ARM64_REG_X13,
    ARM64_REG_X14,
    ARM64_REG_X15,
    ARM64_REG_X16,
    ARM64_REG_X17,
    ARM64_REG_X18,
    ARM64_REG_X19,
    ARM64_REG_X20,
    ARM64_REG_X21,
    ARM64_REG_X22,
    ARM64_REG_X23,
    ARM64_REG_X24,
    ARM64_REG_X25,
    ARM64_REG_X26,
    ARM64_REG_X27,
    ARM64_REG_X28,
    ARM64_REG_X29, // Frame pointer
    ARM64_REG_X30, // Link register
    ARM64_REG_SP,  // Stack pointer
    // Floating-point / NEON registers
    ARM64_REG_Q0,
    ARM64_REG_Q1,
    ARM64_REG_Q2,
    ARM64_REG_Q3,
    ARM64_REG_Q4,
    ARM64_REG_Q5,
    ARM64_REG_Q6,
    ARM64_REG_Q7,
    ARM64_REG_Q8,
    ARM64_REG_Q9,
    ARM64_REG_Q10,
    ARM64_REG_Q11,
    ARM64_REG_Q12,
    ARM64_REG_Q13,
    ARM64_REG_Q14,
    ARM64_REG_Q15,
    ARM64_REG_Q16,
    ARM64_REG_Q17,
    ARM64_REG_Q18,
    ARM64_REG_Q19,
    ARM64_REG_Q20,
    ARM64_REG_Q21,
    ARM64_REG_Q22,
    ARM64_REG_Q23,
    ARM64_REG_Q24,
    ARM64_REG_Q25,
    ARM64_REG_Q26,
    ARM64_REG_Q27,
    ARM64_REG_Q28,
    ARM64_REG_Q29,
    ARM64_REG_Q30,
    ARM64_REG_Q31,
    // Invalid register
    ARM64_REG_INVALID
};

// AArch64 code generator
class AArch64CodeGenerator {
public:
    AArch64CodeGenerator(uint8_t* buffer, size_t size);
    ~AArch64CodeGenerator();

    // Generate code for an IR block
    size_t generateCode(const IRBlock& block);

    // Map X86 register to AArch64 register
    AArch64Register mapX86RegToAArch64(IRRegister irReg);

private:
    uint8_t* buffer_;      // Code buffer
    size_t buffer_size_;   // Size of the code buffer
    size_t buffer_pos_;    // Current position in the buffer

    // Helper methods for code generation
    bool genMove(const IRInstruction& instr);
    bool genLoad(const IRInstruction& instr);
    bool genStore(const IRInstruction& instr);
    bool genArithmetic(const IRInstruction& instr);
    bool genLogical(const IRInstruction& instr);
    bool genShift(const IRInstruction& instr);
    bool genCompare(const IRInstruction& instr);
    bool genJump(const IRInstruction& instr);
    bool genCall(const IRInstruction& instr);
    bool genReturn(const IRInstruction& instr);
    
    // SIMD support
    bool genMMXOp(const IRInstruction& instr);
    bool genSSEOp(const IRInstruction& instr);

    // Emit AArch64 machine code helpers
    size_t emitUint32(uint32_t value);
    
    // Helper to check if there's enough space in the buffer
    bool hasSpace(size_t bytes) const;
};

#endif // XENOARM_JIT_AARCH64_CODE_GENERATOR_H 