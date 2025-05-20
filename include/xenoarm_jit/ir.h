#ifndef XENOARM_JIT_IR_H
#define XENOARM_JIT_IR_H

#include <cstdint>
#include <vector>
#include <string>

namespace xenoarm_jit {
namespace ir {

// Define the types of data the IR can handle
enum class IrDataType {
    UNKNOWN,
    I8, U8,
    I16, U16,
    I32, U32,
    I64, U64,
    // Floating point and vector types
    F32, F64, F80, // x87 FPU (Phase 5)
    V64_B8, V64_W4, V64_D2, // MMX (Phase 4)
    V128_B16, V128_W8, V128_D4, V128_Q2, // SSE (Phase 4)
    PTR // Host pointer size
};

// Define the types of operands an IR instruction can take
enum class IrOperandType {
    REGISTER,
    IMMEDIATE,
    MEMORY, // Base + Displacement, or Base + Index*Scale + Displacement
    LABEL, // For control flow within the IR
    CONDITION_CODE // For conditional branches (deprecated, replaced by specific BR_*)
};

// Forward declaration for IrBasicBlock
struct IrBasicBlock;

// Represents a memory operand
struct MemoryOperand {
    uint32_t base_reg_idx;  // Index of the base register (or -1 if no base)
    uint32_t index_reg_idx; // Index of the index register (or -1 if no index)
    uint8_t scale;          // Scale for the index register (1, 2, 4, 8)
    int32_t displacement;   // Displacement value
};

// Represents an operand for an IR instruction
struct IrOperand {
    IrOperandType type;
    IrDataType data_type;

    union {
        uint32_t reg_idx;       // Virtual register index
        uint64_t imm_value;     // Immediate value (use largest size to accommodate)
        MemoryOperand mem_info; // Memory operand details
        uint32_t label_id;      // Target label ID for branches
        uint32_t cond_code;     // Condition code (deprecated)
    };
    
    // Direct memory operand fields for easier access
    uint32_t mem_base;
    int32_t mem_offset;
    uint32_t mem_size;

    // Default constructor
    IrOperand() 
        : type(IrOperandType::IMMEDIATE), 
          data_type(IrDataType::UNKNOWN), 
          imm_value(0), 
          mem_base(0), 
          mem_offset(0), 
          mem_size(0) {}
    
    // Constructor that initializes with an operand type
    IrOperand(IrOperandType op_type) 
        : type(op_type), 
          data_type(IrDataType::UNKNOWN), 
          imm_value(0), 
          mem_base(0), 
          mem_offset(0), 
          mem_size(0) {}

    // Constructor for register operand
    static IrOperand make_reg(uint32_t reg_idx, IrDataType data_type) {
        IrOperand op;
        op.type = IrOperandType::REGISTER;
        op.data_type = data_type;
        op.reg_idx = reg_idx;
        return op;
    }

    // Constructor for immediate operand
    static IrOperand make_imm(uint64_t value, IrDataType data_type) {
        IrOperand op;
        op.type = IrOperandType::IMMEDIATE;
        op.data_type = data_type;
        op.imm_value = value;
        return op;
    }

    // Constructor for memory operand
    static IrOperand make_mem(uint32_t base_reg_idx, uint32_t index_reg_idx, uint8_t scale, int32_t displacement, IrDataType data_type) {
        IrOperand op;
        op.type = IrOperandType::MEMORY;
        op.data_type = data_type;
        op.mem_info = {base_reg_idx, index_reg_idx, scale, displacement};
        return op;
    }

    // TODO: Add constructors for LABEL operands later
};

// Define the types of IR instructions
enum class IrInstructionType {
    // Arithmetic
    ADD, SUB, ADC, SBB, MUL, IMUL, DIV, IDIV, NEG, INC, DEC,
    // Logical
    AND, OR, XOR, NOT, SHL, SHR, SAR, ROL, ROR,
    // Comparison
    CMP, TEST, // CMP and TEST
    // Data Movement
    MOV, PUSH, POP,
    // Memory Access
    LOAD, STORE,
    // Control Flow
    JMP, // Unconditional jump
    CALL, RET, LABEL,
    // Conditional Branches (based on EFLAGS)
    BR_EQ, BR_NE, BR_LT, BR_LE, BR_GT, BR_GE, // Signed
    BR_BL, BR_BE, BR_BH, BR_BHE, // Unsigned (Below/Above)
    BR_ZERO, BR_NOT_ZERO, // ZF
    BR_SIGN, BR_NOT_SIGN, // SF
    BR_OVERFLOW, BR_NOT_OVERFLOW, // OF
    BR_PARITY, BR_NOT_PARITY, // PF
    BR_CARRY, BR_NOT_CARRY, // CF
    BR_COND, // Generic conditional branch with condition code
    // Flag Operations (may be implicit in arithmetic/compare)
    // SET_CF, GET_ZF, etc.
    UPDATE_EFLAGS_ADD, // Update EFLAGS for ADD operation
    UPDATE_EFLAGS_SUB, // Update EFLAGS for SUB operation
    UPDATE_EFLAGS_AND, // Update EFLAGS for AND operation
    UPDATE_EFLAGS_OR, // Update EFLAGS for OR operation
    UPDATE_EFLAGS_XOR, // Update EFLAGS for XOR operation
    UPDATE_EFLAGS_SHL, // Update EFLAGS for SHL operation
    UPDATE_EFLAGS_SHR, // Update EFLAGS for SHR operation
    UPDATE_EFLAGS_SAR, // Update EFLAGS for SAR operation
    UPDATE_EFLAGS_TEST, // Update EFLAGS for TEST operation
    UPDATE_EFLAGS_CMP, // Update EFLAGS for CMP operation
    // Host Calls
    HOST_CALL,
    // Miscellaneous
    NOP, DEBUG_BREAK,
    // Memory barriers
    MEM_FENCE, // Memory fence instruction for x86 memory model support

    // SIMD (MMX & SSE)
    VEC_MOV, // Generic vector move (e.g., MOVDQA, MOVUPS)
    VEC_ADD_PS, VEC_SUB_PS, VEC_MUL_PS, VEC_DIV_PS, // SSE Packed Single-Precision Floating-Point
    VEC_ADD_PD, VEC_SUB_PD, VEC_MUL_PD, VEC_DIV_PD, // SSE Packed Double-Precision Floating-Point
    VEC_ADD_PI8, VEC_SUB_PI8, VEC_MUL_PI16, // MMX/SSE Integer SIMD
    VEC_ADD_W, // Packed Add Words (for PADDW)
    // Add more SIMD instructions as needed

    // FPU (x87) (Phase 5)
    FLD, FSTP, FADD, FSUB, FMUL, FDIV, // Basic x87 instructions
    
    // Extended FPU instructions (Phase 8)
    FSIN,       // Sine
    FCOS,       // Cosine
    FPTAN,      // Partial Tangent
    F2XM1,      // 2^x - 1
    FYL2X,      // y * log2(x)
    FPATAN,     // Partial Arctangent
    FSQRT,      // Square Root
    FSCALE,     // Scale by power of 2
    FPREM,      // Partial Remainder
    FRNDINT,    // Round to Integer
    FXCH,       // Exchange ST(0) and ST(i)
    FINCSTP,    // Increment FPU Stack Pointer
    FDECSTP,    // Decrement FPU Stack Pointer
    
    // FPU Control Word Instructions (Phase 8)
    FLDCW,      // Load FPU Control Word
    FNSTCW,     // Store FPU Control Word
    FNSTSW,     // Store FPU Status Word
};

// Represents a single IR instruction
struct IrInstruction {
    IrInstructionType type;
    std::vector<IrOperand> operands;

    // Constructor
    IrInstruction(IrInstructionType type, const std::vector<IrOperand>& operands = {})
        : type(type), operands(operands) {}
};

// Represents a basic block of IR instructions
struct IrBasicBlock {
    uint32_t id; // Unique identifier for the block
    std::vector<IrInstruction> instructions;
    // TODO: Add predecessors/successors for control flow graph in later phases

    // Constructor
    IrBasicBlock(uint32_t id) : id(id) {}
};

// Represents a translated function in IR
struct IrFunction {
    uint64_t guest_address; // The original guest address of the function
    std::vector<IrBasicBlock> basic_blocks;

    // Constructor
    IrFunction(uint64_t address) : guest_address(address) {}
};

} // namespace ir
} // namespace xenoarm_jit

#endif // XENOARM_JIT_IR_H