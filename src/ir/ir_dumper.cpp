#include "xenoarm_jit/ir_dumper.h"
#include "xenoarm_jit/ir.h"
#include <ostream>
#include <sstream>
#include <iomanip>

namespace xenoarm_jit {
namespace ir {

// Helper to convert IrDataType to string
const char* ir_data_type_to_string(IrDataType type) {
    switch (type) {
        case IrDataType::UNKNOWN: return "unknown"; // Corrected from UNDEF
        case IrDataType::I8:    return "i8";
        case IrDataType::U8:    return "u8";
        case IrDataType::I16:   return "i16";
        case IrDataType::U16:   return "u16";
        case IrDataType::I32:   return "i32";
        case IrDataType::U32:   return "u32";
        case IrDataType::I64:   return "i64";
        case IrDataType::U64:   return "u64";
        case IrDataType::PTR:   return "ptr";
        case IrDataType::F32:   return "f32";
        case IrDataType::F64:   return "f64";
        case IrDataType::F80:   return "f80";
        case IrDataType::V64_B8:  return "v64_b8";
        case IrDataType::V64_W4:  return "v64_w4";
        case IrDataType::V64_D2:  return "v64_d2";
        case IrDataType::V128_B16: return "v128_b16";
        case IrDataType::V128_W8:  return "v128_w8";
        case IrDataType::V128_D4:  return "v128_d4";
        case IrDataType::V128_Q2:  return "v128_q2";
        default:                return "unk_dtype";
    }
}

// Helper function to dump an IR operand
void dump_ir_operand(std::ostream& os, const IrOperand& operand) {
    os << ir_data_type_to_string(operand.data_type) << ":";
    switch (operand.type) {
        case IrOperandType::REGISTER:
            os << "reg" << operand.reg_idx;
            break;
        case IrOperandType::IMMEDIATE:
            os << "imm:0x" << std::hex << operand.imm_value << std::dec;
            break;
        case IrOperandType::MEMORY:
            os << "mem[";
            if (operand.mem_info.base_reg_idx != 0xFFFFFFFF) { // Corrected access
                os << "base:reg" << operand.mem_info.base_reg_idx; // Corrected access
            }
            if (operand.mem_info.index_reg_idx != 0xFFFFFFFF) { // Corrected access
                if (operand.mem_info.base_reg_idx != 0xFFFFFFFF) os << ", "; // Corrected access
                os << "idx:reg" << operand.mem_info.index_reg_idx; // Corrected access
                if (operand.mem_info.scale != 1) { // Corrected access
                    os << "*s" << static_cast<int>(operand.mem_info.scale); // Corrected access
                }
            }
            if (operand.mem_info.displacement != 0 || (operand.mem_info.base_reg_idx == 0xFFFFFFFF && operand.mem_info.index_reg_idx == 0xFFFFFFFF)) { // Corrected access
                 if (operand.mem_info.base_reg_idx != 0xFFFFFFFF || operand.mem_info.index_reg_idx != 0xFFFFFFFF) { // Corrected access
                    if (operand.mem_info.displacement >= 0) os << "+"; // Corrected access
                 }
                 os << "0x" << std::hex << operand.mem_info.displacement << std::dec; // Corrected access
            }
            os << "]";
            break;
        // TODO: Add cases for LABEL, CONDITION_CODE
        default:
            os << "unknown_operand_type";
            break;
    }
}

// Helper function to get string representation of instruction type
// Name now matches header: ir_instruction_type_to_string
const char* ir_instruction_type_to_string(IrInstructionType type) {
    switch (type) {
        // Arithmetic
        case IrInstructionType::ADD: return "ADD";
        case IrInstructionType::SUB: return "SUB";
        case IrInstructionType::ADC: return "ADC";
        case IrInstructionType::SBB: return "SBB";
        case IrInstructionType::MUL: return "MUL";
        case IrInstructionType::IMUL: return "IMUL";
        case IrInstructionType::DIV: return "DIV";
        case IrInstructionType::IDIV: return "IDIV";
        case IrInstructionType::NEG: return "NEG";
        case IrInstructionType::INC: return "INC";
        case IrInstructionType::DEC: return "DEC";
        
        // Logical
        case IrInstructionType::AND: return "AND";
        case IrInstructionType::OR: return "OR";
        case IrInstructionType::XOR: return "XOR";
        case IrInstructionType::NOT: return "NOT";
        case IrInstructionType::SHL: return "SHL";
        case IrInstructionType::SHR: return "SHR";
        case IrInstructionType::SAR: return "SAR";
        case IrInstructionType::ROL: return "ROL";
        case IrInstructionType::ROR: return "ROR";
        
        // Comparison
        case IrInstructionType::CMP: return "CMP";
        case IrInstructionType::TEST: return "TEST";
        
        // Data Movement
        case IrInstructionType::MOV: return "MOV";
        case IrInstructionType::PUSH: return "PUSH";
        case IrInstructionType::POP: return "POP";
        
        // Memory Access
        case IrInstructionType::LOAD: return "LOAD";
        case IrInstructionType::STORE: return "STORE";
        
        // Control Flow
        case IrInstructionType::JMP: return "JMP";
        case IrInstructionType::CALL: return "CALL";
        case IrInstructionType::RET: return "RET";
        case IrInstructionType::LABEL: return "LABEL";
        
        // Conditional Branches
        case IrInstructionType::BR_EQ: return "BR_EQ";
        case IrInstructionType::BR_NE: return "BR_NE";
        case IrInstructionType::BR_LT: return "BR_LT";
        case IrInstructionType::BR_LE: return "BR_LE";
        case IrInstructionType::BR_GT: return "BR_GT";
        case IrInstructionType::BR_GE: return "BR_GE";
        case IrInstructionType::BR_BL: return "BR_BL";
        case IrInstructionType::BR_BE: return "BR_BE";
        case IrInstructionType::BR_BH: return "BR_BH";
        case IrInstructionType::BR_BHE: return "BR_BHE";
        case IrInstructionType::BR_ZERO: return "BR_ZERO";
        case IrInstructionType::BR_NOT_ZERO: return "BR_NOT_ZERO";
        case IrInstructionType::BR_SIGN: return "BR_SIGN";
        case IrInstructionType::BR_NOT_SIGN: return "BR_NOT_SIGN";
        case IrInstructionType::BR_OVERFLOW: return "BR_OVERFLOW";
        case IrInstructionType::BR_NOT_OVERFLOW: return "BR_NOT_OVERFLOW";
        case IrInstructionType::BR_PARITY: return "BR_PARITY";
        case IrInstructionType::BR_NOT_PARITY: return "BR_NOT_PARITY";
        case IrInstructionType::BR_CARRY: return "BR_CARRY";
        case IrInstructionType::BR_NOT_CARRY: return "BR_NOT_CARRY";
        
        // Host Calls
        case IrInstructionType::HOST_CALL: return "HOST_CALL";
        
        // Miscellaneous
        case IrInstructionType::NOP: return "NOP";
        case IrInstructionType::DEBUG_BREAK: return "DEBUG_BREAK";
        
        // SIMD
        case IrInstructionType::VEC_MOV: return "VEC_MOV";
        case IrInstructionType::VEC_ADD_PS: return "VEC_ADD_PS";
        case IrInstructionType::VEC_SUB_PS: return "VEC_SUB_PS";
        case IrInstructionType::VEC_MUL_PS: return "VEC_MUL_PS";
        case IrInstructionType::VEC_DIV_PS: return "VEC_DIV_PS";
        case IrInstructionType::VEC_ADD_PD: return "VEC_ADD_PD";
        case IrInstructionType::VEC_SUB_PD: return "VEC_SUB_PD";
        case IrInstructionType::VEC_MUL_PD: return "VEC_MUL_PD";
        case IrInstructionType::VEC_DIV_PD: return "VEC_DIV_PD";
        case IrInstructionType::VEC_ADD_PI8: return "VEC_ADD_PI8";
        case IrInstructionType::VEC_SUB_PI8: return "VEC_SUB_PI8";
        case IrInstructionType::VEC_MUL_PI16: return "VEC_MUL_PI16";
        case IrInstructionType::VEC_ADD_W: return "VEC_ADD_W";
        
        // FPU
        case IrInstructionType::FLD: return "FLD";
        case IrInstructionType::FSTP: return "FSTP";
        case IrInstructionType::FADD: return "FADD";
        case IrInstructionType::FSUB: return "FSUB";
        case IrInstructionType::FMUL: return "FMUL";
        case IrInstructionType::FDIV: return "FDIV";
        
        default: return "UNKNOWN_INSTR";
    }
}

// Helper to dump a single IrInstruction
void dump_ir_instruction(std::ostream& os, const IrInstruction& instruction) {
    os << "    " << ir_instruction_type_to_string(instruction.type);
    for (size_t i = 0; i < instruction.operands.size(); ++i) {
        os << (i == 0 ? " " : ", ");
        dump_ir_operand(os, instruction.operands[i]);
    }
    os << std::endl;
}

// Helper to dump a single IrBasicBlock
void dump_ir_basic_block(std::ostream& os, const IrBasicBlock& block) {
    os << "  Basic Block ID: " << block.id << ":" << std::endl;
    for (const auto& instr : block.instructions) {
        dump_ir_instruction(os, instr);
    }
}

void dump_ir_function(std::ostream& os, const IrFunction& ir_func) {
    os << "IR Function at guest address: 0x" << std::hex << ir_func.guest_address << std::dec << std::endl;
    os << "Number of basic blocks: " << ir_func.basic_blocks.size() << std::endl;

    for (const auto& block : ir_func.basic_blocks) {
        dump_ir_basic_block(os, block);
    }
}

} // namespace ir
} // namespace xenoarm_jit