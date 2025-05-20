#ifndef XENOARM_JIT_IR_H
#define XENOARM_JIT_IR_H

#include <cstdint>
#include <vector>
#include <string>
#include "decoder/decoder.h"

// IR opcodes
enum IROpcode {
    IR_NOP,
    IR_MOV_REG,   // Move register to register
    IR_MOV_IMM,   // Move immediate to register
    IR_LOAD,      // Load from memory
    IR_STORE,     // Store to memory
    IR_ADD_REG,   // Add register to register
    IR_ADD_IMM,   // Add immediate to register
    IR_SUB_REG,   // Subtract register from register
    IR_SUB_IMM,   // Subtract immediate from register
    IR_AND_REG,   // Bitwise AND register with register
    IR_AND_IMM,   // Bitwise AND register with immediate
    IR_OR_REG,    // Bitwise OR register with register
    IR_OR_IMM,    // Bitwise OR register with immediate
    IR_XOR_REG,   // Bitwise XOR register with register
    IR_XOR_IMM,   // Bitwise XOR register with immediate
    IR_NOT,       // Bitwise NOT
    IR_SHL_REG,   // Shift left by register
    IR_SHL_IMM,   // Shift left by immediate
    IR_SHR_REG,   // Shift right by register
    IR_SHR_IMM,   // Shift right by immediate
    IR_SAR_REG,   // Shift arithmetic right by register
    IR_SAR_IMM,   // Shift arithmetic right by immediate
    IR_ROL_REG,   // Rotate left by register
    IR_ROL_IMM,   // Rotate left by immediate
    IR_CMP_REG,   // Compare register with register
    IR_CMP_IMM,   // Compare register with immediate
    IR_JMP,       // Unconditional jump
    IR_JCC,       // Conditional jump
    IR_CALL,      // Call subroutine
    IR_RET,       // Return from subroutine
    IR_LOOP,      // Loop instruction
    IR_PUSH,      // Push to stack
    IR_POP,       // Pop from stack
    IR_INC,       // Increment
    IR_DEC,       // Decrement
    IR_SBB_REG,   // Subtract with borrow
    // MMX operations
    IR_MMX_MOV,   // Move MMX register
    IR_MMX_LOAD,  // Load to MMX register
    IR_MMX_STORE, // Store from MMX register
    IR_MMX_PADDB, // Add packed bytes
    IR_MMX_PADDW, // Add packed words
    IR_MMX_PMULLW,// Multiply packed words
    IR_MMX_PAND,  // Bitwise AND
    IR_MMX_POR,   // Bitwise OR
    IR_MMX_PXOR,  // Bitwise XOR
    IR_MMX_PSLLW, // Shift left logical word
    IR_MMX_PSRLW, // Shift right logical word
    // SSE operations
    IR_SSE_MOVAPS,// Move aligned packed single precision
    IR_SSE_ADDPS, // Add packed single precision
    IR_SSE_CMPEQPS,// Compare equal packed single precision
    // Other opcodes can be added as needed
    IR_INVALID
};

// IR register IDs
enum IRRegister {
    IR_REG_EAX,
    IR_REG_ECX,
    IR_REG_EDX,
    IR_REG_EBX,
    IR_REG_ESP,
    IR_REG_EBP,
    IR_REG_ESI,
    IR_REG_EDI,
    // 16-bit registers
    IR_REG_AX,
    IR_REG_CX,
    IR_REG_DX,
    IR_REG_BX,
    IR_REG_SP,
    IR_REG_BP,
    IR_REG_SI,
    IR_REG_DI,
    // 8-bit registers
    IR_REG_AL,
    IR_REG_CL,
    IR_REG_DL,
    IR_REG_BL,
    IR_REG_AH,
    IR_REG_CH,
    IR_REG_DH,
    IR_REG_BH,
    // MMX registers
    IR_REG_MM0,
    IR_REG_MM1,
    IR_REG_MM2,
    IR_REG_MM3,
    IR_REG_MM4,
    IR_REG_MM5,
    IR_REG_MM6,
    IR_REG_MM7,
    // XMM registers
    IR_REG_XMM0,
    IR_REG_XMM1,
    IR_REG_XMM2,
    IR_REG_XMM3,
    IR_REG_XMM4,
    IR_REG_XMM5,
    IR_REG_XMM6,
    IR_REG_XMM7,
    // Invalid/none
    IR_REG_NONE
};

// IR operand types
enum IROperandType {
    IR_OP_NONE,
    IR_OP_REGISTER,
    IR_OP_IMMEDIATE,
    IR_OP_MEMORY,
    IR_OP_LABEL
};

// IR register operand
struct IRRegisterOperand {
    IRRegister id;
    int size;  // Size in bits (8, 16, 32, 64, 128)
};

// IR immediate operand
struct IRImmediateOperand {
    uint64_t value;
    int size;  // Size in bits (8, 16, 32, 64)
};

// IR memory operand
struct IRMemoryOperand {
    IRRegister base;  // Base register
    IRRegister index; // Index register
    uint8_t scale;    // Scale factor
    int32_t disp;     // Displacement
    int size;         // Size in bits (8, 16, 32, 64, 128)
};

// IR label operand
struct IRLabelOperand {
    uint32_t target;  // Target address
};

// IR operand
struct IROperand {
    IROperandType type;
    union {
        IRRegisterOperand reg;
        IRImmediateOperand imm;
        IRMemoryOperand mem;
        IRLabelOperand label;
    };
};

// IR instruction
struct IRInstruction {
    IROpcode opcode;
    IROperand operands[3];  // Up to 3 operands
    uint8_t flags_mask;     // Which flags are affected
    uint8_t condition;      // Condition code for conditional instructions
};

// IR block
struct IRBlock {
    std::vector<IRInstruction> instructions;
    uint32_t start_address;  // Guest address of the block
    uint32_t end_address;    // Guest address after the block
};

// IR generator
class IRGenerator {
public:
    IRGenerator();
    ~IRGenerator();

    // Translate a decoded x86 instruction to IR
    bool translate(const DecodedInsn& insn, IRBlock& block);

private:
    // Private implementation details
    // Helper methods for translating different instruction types
};

// IR dumper for debugging/visualization
class IRDumper {
public:
    IRDumper();
    ~IRDumper();

    // Dump an IR block to a string representation
    std::string dumpBlock(const IRBlock& block);

private:
    // Private implementation details
};

#endif // XENOARM_JIT_IR_H 