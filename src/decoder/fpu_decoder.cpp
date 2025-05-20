#include "xenoarm_jit/decoder/decoder.h"
#include "xenoarm_jit/ir.h"
#include "logging/logger.h"

namespace xenoarm_jit {
namespace decoder {

// Helper function to decode and generate IR for x87 FPU instructions
bool decode_fpu_instruction(Decoder* decoder, x86_insn* insn, ir::IrFunction* ir_func, ir::IrBasicBlock* curr_block) {
    // Extract the opcode
    uint8_t primary_opcode = insn->opcode;
    
    // Check if this is a D8-DF opcode range (FPU opcodes)
    if (primary_opcode < 0xD8 || primary_opcode > 0xDF) {
        // Not an FPU instruction
        return false;
    }

    LOG_DEBUG("Decoding FPU instruction: Opcode=0x" + 
              std::to_string(primary_opcode) + 
              ", ModRM=" + 
              (insn->has_modrm ? std::to_string(insn->modrm) : "None"));

    // D8-DF opcodes are FPU instructions
    uint8_t modrm = insn->modrm;
    uint8_t reg_field = (modrm >> 3) & 0x7;  // bits 3-5 of ModRM
    uint8_t rm_field = modrm & 0x7;          // bits 0-2 of ModRM
    bool is_mem_op = (modrm < 0xC0);         // ModRM < 0xC0 indicates a memory operand

    // Handle based on the primary opcode
    switch (primary_opcode) {
        case 0xD9: // Various FPU instructions including FLD/FSTP
            if (is_mem_op) {
                // Memory operand
                if (reg_field == 0) {
                    // FLD m32fp - Load 32-bit float from memory
                    LOG_DEBUG("Decoded: FLD m32fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F32);
                    
                    // Generate FLD instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FLD, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                else if (reg_field == 3) {
                    // FSTP m32fp - Store 32-bit float to memory and pop
                    LOG_DEBUG("Decoded: FSTP m32fp");
                    
                    // Create memory operand for the destination
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F32);
                    
                    // Generate FSTP instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSTP, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                // Phase 8: FPU Control Word instructions
                else if (reg_field == 5) {
                    // FLDCW m16 - Load FPU Control Word
                    LOG_DEBUG("Decoded: FLDCW m16");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::I16);
                    
                    // Generate FLDCW instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FLDCW, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                else if (reg_field == 7) {
                    // FNSTCW m16 - Store FPU Control Word
                    LOG_DEBUG("Decoded: FNSTCW m16");
                    
                    // Create memory operand for the destination
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::I16);
                    
                    // Generate FNSTCW instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FNSTCW, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
            } 
            else {
                // Register operand
                if (reg_field == 0) {
                    // FLD ST(i) - Load floating-point value from ST(i) to ST(0)
                    LOG_DEBUG("Decoded: FLD ST(" + std::to_string(rm_field) + ")");
                    
                    // Create register operand for the source ST(i)
                    ir::IrOperand reg_op = ir::IrOperand::make_imm(rm_field, ir::IrDataType::F80);
                    
                    // Generate FLD instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FLD, 
                                                         std::vector<ir::IrOperand>{reg_op});
                    return true;
                }
                // FXCH, etc.
                
                // Phase 8: Transcendental functions
                if (modrm == 0xFE) {
                    // FSIN - Sine
                    LOG_DEBUG("Decoded: FSIN");
                    
                    // Generate FSIN instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSIN);
                    return true;
                }
                else if (modrm == 0xFF) {
                    // FCOS - Cosine
                    LOG_DEBUG("Decoded: FCOS");
                    
                    // Generate FCOS instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FCOS);
                    return true;
                }
                else if (modrm == 0xF2) {
                    // FPTAN - Partial tangent
                    LOG_DEBUG("Decoded: FPTAN");
                    
                    // Generate FPTAN instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FPTAN);
                    return true;
                }
                else if (modrm == 0xF0) {
                    // F2XM1 - 2^x - 1
                    LOG_DEBUG("Decoded: F2XM1");
                    
                    // Generate F2XM1 instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::F2XM1);
                    return true;
                }
                else if (modrm == 0xF1) {
                    // FYL2X - y * log2(x)
                    LOG_DEBUG("Decoded: FYL2X");
                    
                    // Generate FYL2X instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FYL2X);
                    return true;
                }
                else if (modrm == 0xFA) {
                    // FSQRT - Square root
                    LOG_DEBUG("Decoded: FSQRT");
                    
                    // Generate FSQRT instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSQRT);
                    return true;
                }
                else if (modrm == 0xFC) {
                    // FRNDINT - Round to integer
                    LOG_DEBUG("Decoded: FRNDINT");
                    
                    // Generate FRNDINT instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FRNDINT);
                    return true;
                }
                else if (modrm == 0xFD) {
                    // FSCALE - Scale by power of 2
                    LOG_DEBUG("Decoded: FSCALE");
                    
                    // Generate FSCALE instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSCALE);
                    return true;
                }
                else if (modrm == 0xF8) {
                    // FPREM - Partial remainder
                    LOG_DEBUG("Decoded: FPREM");
                    
                    // Generate FPREM instruction in IR (no operands needed)
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FPREM);
                    return true;
                }
            }
            break;
            
        case 0xDD: // Various FPU instructions including FLD/FSTP
            if (is_mem_op) {
                // Memory operand
                if (reg_field == 0) {
                    // FLD m64fp - Load 64-bit double from memory
                    LOG_DEBUG("Decoded: FLD m64fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F64);
                    
                    // Generate FLD instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FLD, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                else if (reg_field == 3) {
                    // FSTP m64fp - Store 64-bit double to memory and pop
                    LOG_DEBUG("Decoded: FSTP m64fp");
                    
                    // Create memory operand for the destination
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F64);
                    
                    // Generate FSTP instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSTP, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
            }
            break;
            
        case 0xDB: // Various FPU instructions including FLD/FSTP
            if (is_mem_op) {
                // Memory operand
                if (reg_field == 7) {
                    // FSTP m80fp - Store 80-bit extended precision to memory and pop
                    LOG_DEBUG("Decoded: FSTP m80fp");
                    
                    // Create memory operand for the destination
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F80);
                    
                    // Generate FSTP instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSTP, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                else if (reg_field == 5) {
                    // FLD m80fp - Load 80-bit extended precision from memory
                    LOG_DEBUG("Decoded: FLD m80fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F80);
                    
                    // Generate FLD instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FLD, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
            }
            break;
            
        case 0xD8: // Basic floating-point operations
            if (is_mem_op) {
                // Memory operand
                if (reg_field == 0) {
                    // FADD m32fp - Add 32-bit float from memory to ST(0)
                    LOG_DEBUG("Decoded: FADD m32fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F32);
                    
                    // Generate FADD instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FADD, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                else if (reg_field == 1) {
                    // FMUL m32fp - Multiply 32-bit float from memory with ST(0)
                    LOG_DEBUG("Decoded: FMUL m32fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F32);
                    
                    // Generate FMUL instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FMUL, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                else if (reg_field == 4) {
                    // FSUB m32fp - Subtract 32-bit float from memory from ST(0)
                    LOG_DEBUG("Decoded: FSUB m32fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F32);
                    
                    // Generate FSUB instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSUB, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                else if (reg_field == 6) {
                    // FDIV m32fp - Divide ST(0) by 32-bit float from memory
                    LOG_DEBUG("Decoded: FDIV m32fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F32);
                    
                    // Generate FDIV instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FDIV, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
            }
            else {
                // Register operand (ST(0), ST(i))
                if (reg_field == 0) {
                    // FADD ST, ST(i) - Add ST(i) to ST(0)
                    LOG_DEBUG("Decoded: FADD ST, ST(" + std::to_string(rm_field) + ")");
                    
                    // Create register operand for the source ST(i)
                    ir::IrOperand reg_op = ir::IrOperand::make_imm(rm_field, ir::IrDataType::F80);
                    
                    // Generate FADD instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FADD, 
                                                         std::vector<ir::IrOperand>{reg_op});
                    return true;
                }
                else if (reg_field == 1) {
                    // FMUL ST, ST(i) - Multiply ST(0) by ST(i)
                    LOG_DEBUG("Decoded: FMUL ST, ST(" + std::to_string(rm_field) + ")");
                    
                    // Create register operand for the source ST(i)
                    ir::IrOperand reg_op = ir::IrOperand::make_imm(rm_field, ir::IrDataType::F80);
                    
                    // Generate FMUL instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FMUL, 
                                                         std::vector<ir::IrOperand>{reg_op});
                    return true;
                }
                else if (reg_field == 4) {
                    // FSUB ST, ST(i) - Subtract ST(i) from ST(0)
                    LOG_DEBUG("Decoded: FSUB ST, ST(" + std::to_string(rm_field) + ")");
                    
                    // Create register operand for the source ST(i)
                    ir::IrOperand reg_op = ir::IrOperand::make_imm(rm_field, ir::IrDataType::F80);
                    
                    // Generate FSUB instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FSUB, 
                                                         std::vector<ir::IrOperand>{reg_op});
                    return true;
                }
                else if (reg_field == 6) {
                    // FDIV ST, ST(i) - Divide ST(0) by ST(i)
                    LOG_DEBUG("Decoded: FDIV ST, ST(" + std::to_string(rm_field) + ")");
                    
                    // Create register operand for the source ST(i)
                    ir::IrOperand reg_op = ir::IrOperand::make_imm(rm_field, ir::IrDataType::F80);
                    
                    // Generate FDIV instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FDIV, 
                                                         std::vector<ir::IrOperand>{reg_op});
                    return true;
                }
            }
            break;
            
        case 0xDC: // Similar to D8 but for 64-bit doubles
            if (is_mem_op) {
                // Memory operand
                if (reg_field == 0) {
                    // FADD m64fp - Add 64-bit double from memory to ST(0)
                    LOG_DEBUG("Decoded: FADD m64fp");
                    
                    // Create memory operand for the source
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::F64);
                    
                    // Generate FADD instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FADD, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
                // Other memory operations (FMUL, FSUB, FDIV, etc.)
            }
            else {
                // Register operations like FADD ST(i), ST
            }
            break;
            
        case 0xDF: // Various FPU instructions
            if (is_mem_op) {
                // Memory operand
                if (reg_field == 7) {
                    // FNSTSW m16 - Store FPU Status Word to memory
                    LOG_DEBUG("Decoded: FNSTSW m16");
                    
                    // Create memory operand for the destination
                    ir::IrOperand mem_op = decoder->create_memory_operand_from_insn(insn, 0, ir::IrDataType::I16);
                    
                    // Generate FNSTSW instruction in IR
                    curr_block->instructions.emplace_back(ir::IrInstructionType::FNSTSW, 
                                                         std::vector<ir::IrOperand>{mem_op});
                    return true;
                }
            }
            else if (modrm == 0xE0) {
                // FNSTSW AX - Store FPU Status Word to AX
                LOG_DEBUG("Decoded: FNSTSW AX");
                
                // Create register operand for AX (virtual register 0)
                ir::IrOperand reg_op = ir::IrOperand::make_reg(0, ir::IrDataType::I16);
                
                // Generate FNSTSW instruction in IR
                curr_block->instructions.emplace_back(ir::IrInstructionType::FNSTSW, 
                                                     std::vector<ir::IrOperand>{reg_op});
                return true;
            }
            break;
            
        // Add cases for DE, DA, etc. for other FPU instructions as needed
    }
    
    // If we reached here, this FPU instruction is not yet supported
    LOG_WARNING("Unsupported FPU instruction: Opcode=0x" + 
                std::to_string(primary_opcode) + 
                ", ModRM=0x" + 
                (insn->has_modrm ? std::to_string(insn->modrm) : "None"));
    
    return false;  // Not handled
}

} // namespace decoder
} // namespace xenoarm_jit 