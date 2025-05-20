#ifndef XENOARM_JIT_DECODER_H
#define XENOARM_JIT_DECODER_H

#include <cstdint>
#include <cstddef>
#include "xenoarm_jit/ir.h"

namespace xenoarm_jit {
namespace decoder {

// Forward declaration
struct x86_insn;

// Define x86 instruction enums
enum X86Opcode {
    X86_INVALID = 0,
    X86_MOV,
    X86_ADD,
    X86_SUB,
    // ... other instructions
    X86_MOVQ,
    X86_MOVAPS,
    // FPU instructions (D8-DF)
    X86_FLD = 0xD9,
    X86_FSTP = 0xD9,
    X86_FADD = 0xD8,
    X86_FMUL = 0xD8,
    X86_FSUB = 0xD8,
    X86_FDIV = 0xD8
};

enum X86Register {
    REGISTER_NONE = -1,
    EAX = 0,
    ECX,
    EDX,
    EBX,
    ESP,
    EBP,
    ESI,
    EDI,
    // 16-bit registers
    AX,
    CX,
    DX,
    BX,
    SP,
    BP,
    SI,
    DI,
    // 8-bit registers
    AL,
    CL,
    DL,
    BL,
    AH,
    CH,
    DH,
    BH,
    // MMX registers
    MM0,
    MM1,
    MM2,
    MM3,
    MM4,
    MM5,
    MM6,
    MM7,
    // XMM registers
    XMM0,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7
};

enum OperandType {
    OPERAND_INVALID = 0,
    OPERAND_REGISTER,
    OPERAND_IMMEDIATE,
    OPERAND_MEMORY
};

struct RegisterOperand {
    X86Register id;
    uint8_t size; // In bits (8, 16, 32, 64, 80, 128)
};

struct ImmediateOperand {
    int64_t value;
    uint8_t size; // In bits
};

struct MemoryOperand {
    X86Register base;
    X86Register index;
    uint8_t scale;
    int32_t disp;
    uint8_t size; // In bits
};

struct Operand {
    OperandType type;
    union {
        RegisterOperand reg;
        ImmediateOperand imm;
        MemoryOperand mem;
    };
};

struct InstructionPrefixes {
    bool address_size;
    uint8_t operand_size; // 0=default (32), 16, 32
    bool rep;
    bool repne;
    uint8_t segment; // CS=1, DS=2, ES=3, FS=4, GS=5, SS=6, 0=none
};

// Simplified x86 instruction representation
struct x86_insn {
    uint8_t opcode;
    uint8_t modrm;
    uint8_t sib;
    bool has_modrm;
    bool has_sib;
    bool has_displacement;
    int32_t displacement;
    uint8_t length;
    InstructionPrefixes prefixes;
    Operand operands[4];
    int num_operands;
};

using DecodedInsn = x86_insn;

// Main decoder class
class Decoder {
public:
    Decoder();
    ~Decoder();
    
    // Decode a single x86 instruction from code
    bool decode(const uint8_t* code, size_t size, DecodedInsn& insn);
    
    // Helper method to create memory operands for IR generation
    ir::IrOperand create_memory_operand_from_insn(const x86_insn* insn, int operand_idx, ir::IrDataType data_type);
};

} // namespace decoder
} // namespace xenoarm_jit

#endif // XENOARM_JIT_DECODER_H 