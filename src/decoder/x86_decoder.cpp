#include "xenoarm_jit/decoder.h"
#include "logging/logger.h"
#include <iostream>

namespace xenoarm_jit {
namespace decoder {

X86Decoder::X86Decoder() {
    // Initialize decoder state
    LOG_DEBUG("X86Decoder created");
}

X86Decoder::~X86Decoder() {
    // Clean up decoder state
    LOG_DEBUG("X86Decoder destroyed");
}

ir::IrFunction X86Decoder::decode_block(const uint8_t* guest_code, uint64_t guest_address, size_t max_bytes) {
    LOG_DEBUG("Decoding block at address 0x" + std::to_string(guest_address));
    
    // Create a new function with a single basic block
    ir::IrFunction func(guest_address);
    
    ir::IrBasicBlock block(0);  // First block has ID 0
    
    // Process instructions until we hit a terminator or reach max_bytes
    size_t offset = 0;
    while (offset < max_bytes) {
        // Try to decode a single instruction
        size_t bytes_read = 0;
        std::vector<ir::IrInstruction> instructions = decode_instruction(guest_code + offset, bytes_read, max_bytes - offset);
        
        // If we couldn't decode an instruction, stop processing
        if (bytes_read == 0 || instructions.empty()) {
            LOG_WARNING("Failed to decode instruction at offset " + std::to_string(offset));
            break;
        }
        
        // Add decoded instructions to the basic block
        for (const auto& instr : instructions) {
            block.instructions.push_back(instr);
            
            // If this is a terminator instruction, end the block
            if (instr.type == ir::IrInstructionType::JMP || 
                instr.type == ir::IrInstructionType::CALL || 
                instr.type == ir::IrInstructionType::RET ||
                instr.type == ir::IrInstructionType::BR_EQ ||
                instr.type == ir::IrInstructionType::BR_NE ||
                // Add other branch types as needed
                false) {
                
                offset += bytes_read;
                goto end_block;  // Break out of both loops
            }
        }
        
        // Move to the next instruction
        offset += bytes_read;
    }
    
end_block:
    // Add the block to the function
    func.basic_blocks.push_back(block);
    
    LOG_DEBUG("Decoded " + std::to_string(offset) + " bytes into " + 
              std::to_string(func.basic_blocks.size()) + " basic blocks with " +
              std::to_string(block.instructions.size()) + " instructions");
    
    return func;
}

std::vector<ir::IrInstruction> X86Decoder::decode_instruction(const uint8_t* instruction_bytes, size_t& bytes_read, size_t max_bytes_for_instruction) {
    // This is a simplified decoder for demonstration purposes
    std::vector<ir::IrInstruction> result;
    
    // Ensure we have at least 1 byte to read
    if (max_bytes_for_instruction < 1) {
        bytes_read = 0;
        return result;
    }
    
    // Example: Decode a MOV instruction (0xB8 + register number)
    if (instruction_bytes[0] >= 0xB8 && instruction_bytes[0] <= 0xBF) {
        // MOV r32, imm32
        if (max_bytes_for_instruction >= 5) {  // Need 5 bytes for the full instruction
            // Destination operand is a register
            ir::IrOperand dest = ir::IrOperand::make_reg(
                instruction_bytes[0] - 0xB8,  // Register is encoded in the opcode
                ir::IrDataType::I32
            );
            
            // Source operand is an immediate
            ir::IrOperand src = ir::IrOperand::make_imm(
                instruction_bytes[1] | 
                (instruction_bytes[2] << 8) | 
                (instruction_bytes[3] << 16) | 
                (instruction_bytes[4] << 24),
                ir::IrDataType::I32
            );
            
            // Create the instruction
            std::vector<ir::IrOperand> operands = {dest, src};
            ir::IrInstruction instr(ir::IrInstructionType::MOV, operands);
            
            bytes_read = 5;
            result.push_back(instr);
        } else {
            bytes_read = 0;  // Not enough bytes available
        }
    } else {
        // For testing, just create a NOP for any other instruction
        ir::IrInstruction nop(ir::IrInstructionType::NOP);
        bytes_read = 1;  // Assume all other instructions are 1 byte (not realistic)
        result.push_back(nop);
    }
    
    return result;
}

} // namespace decoder
} // namespace xenoarm_jit 