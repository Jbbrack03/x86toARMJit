#ifndef XENOARM_JIT_DECODER_H
#define XENOARM_JIT_DECODER_H

#include <cstdint>
#include <cstddef>

// X86 opcode definitions
enum X86Opcode {
    X86_MOV,
    X86_ADD,
    X86_SUB,
    X86_AND,
    X86_OR,
    X86_XOR,
    X86_INC,
    X86_DEC,
    X86_PUSH,
    X86_POP,
    X86_JMP,
    X86_CALL,
    X86_RET,
    X86_LOOP,
    X86_CMP,
    X86_JCC,  // Generic conditional jump
    X86_NOT,
    X86_SHL,
    X86_SHR,
    X86_SAR,
    X86_ROL,
    X86_SBB,
    // MMX instructions
    X86_MOVQ,
    X86_PADDB,
    X86_PADDW,
    X86_PMULLW,
    X86_PAND,
    X86_POR,
    X86_PXOR,
    X86_PSLLW,
    X86_PSRLW,
    // SSE instructions
    X86_MOVAPS,
    X86_ADDPS,
    X86_CMPEQPS,
    // Other opcodes can be added as needed
    X86_INVALID
};

// Register IDs
enum X86Register {
    EAX,
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
    XMM7,
    // Invalid/none
    REGISTER_NONE
};

// Operand types
enum OperandType {
    OPERAND_NONE,
    OPERAND_REGISTER,
    OPERAND_IMMEDIATE,
    OPERAND_MEMORY,
    OPERAND_SEGMENT_REGISTER
};

// Register operand
struct RegisterOperand {
    X86Register id;
    int size;  // Size in bits (8, 16, 32)
};

// Immediate operand
struct ImmediateOperand {
    uint32_t value;
    int size;  // Size in bits (8, 16, 32)
};

// Memory operand
struct MemoryOperand {
    X86Register base;  // Base register
    X86Register index; // Index register
    uint8_t scale;     // Scale factor (0, 1, 2, or 3 meaning 1, 2, 4, or 8)
    int32_t disp;      // Displacement
    int size;          // Size in bits (8, 16, 32)
};

// Instruction operand
struct Operand {
    OperandType type;
    union {
        RegisterOperand reg;
        ImmediateOperand imm;
        MemoryOperand mem;
    };
};

// Prefix information
struct PrefixInfo {
    bool rep;           // REP prefix
    bool repne;         // REPNE prefix
    uint8_t segment;    // Segment override
    uint8_t operand_size; // Operand size override (16 or 32)
    uint8_t address_size; // Address size override (16 or 32)
};

// Decoded instruction
struct DecodedInsn {
    X86Opcode opcode;
    PrefixInfo prefixes;
    Operand operands[3];  // Up to 3 operands
    uint8_t length;       // Instruction length in bytes
    uint8_t condition;    // Condition code for conditional instructions
};

// Decoder class
class Decoder {
public:
    Decoder();
    ~Decoder();

    // Decode a single instruction
    bool decode(const uint8_t* code, size_t size, DecodedInsn& insn);

private:
    // Private implementation details
    // Helper methods for decoding different instruction formats
};

#endif // XENOARM_JIT_DECODER_H 