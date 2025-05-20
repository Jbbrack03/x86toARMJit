#include "xenoarm_jit/aarch64/code_generator.h"
#include "logger.h"
#include <iostream> // For std::cerr and std::endl
#include "xenoarm_jit/eflags/eflags_state.h" // Include EFLAGS header
#include <vector> // Include vector for emit_instruction
#include <unordered_map> // Include for unordered_map
#include "xenoarm_jit/register_allocation/register_allocator.h" // Include for RegisterMapping
#include <cstring>

namespace xenoarm_jit {
namespace aarch64 {

CodeGenerator::CodeGenerator() {
    LOG_DEBUG("AArch64 CodeGenerator created.");
    // TODO: Initialize EFLAGS state location (e.g., allocate a dedicated register or memory)
}

CodeGenerator::~CodeGenerator() {
    LOG_DEBUG("AArch64 CodeGenerator destroyed.");
}

// Helper to emit a 32-bit AArch64 instruction
void CodeGenerator::emit_instruction(std::vector<uint8_t>& code, uint32_t instruction) {
    // AArch64 instructions are 32 bits (4 bytes) and typically little-endian
    code.push_back(static_cast<uint8_t>(instruction & 0xFF));
    code.push_back(static_cast<uint8_t>((instruction >> 8) & 0xFF));
    code.push_back(static_cast<uint8_t>((instruction >> 16) & 0xFF));
    code.push_back(static_cast<uint8_t>((instruction >> 24) & 0xFF));
}

// Helper to get the physical register index for a virtual register
uint32_t CodeGenerator::get_physical_reg(
    uint32_t virtual_reg,
    const std::unordered_map<uint32_t, register_allocation::RegisterMapping>& register_map
) const {
    auto it = register_map.find(virtual_reg);
    if (it != register_map.end()) {
        if (it->second.type == register_allocation::PhysicalRegisterType::GPR) {
            return it->second.gpr_physical_reg_idx;
        } else {
            return it->second.neon_physical_reg_idx;
        }
    }
    LOG_ERROR("Virtual register " + std::to_string(virtual_reg) + " not found in register map.");
    return 0; // Default to a safe register (e.g., W0/V0) in case of error
}

// Helper to get the physical register index for the EFLAGS state
uint32_t CodeGenerator::get_eflags_reg() const {
    // TODO: Return the physical register allocated for EFLAGS
    return 28; // Example: Use X28/W28 for EFLAGS
}


std::vector<uint8_t> CodeGenerator::generate(
    const std::vector<ir::IrInstruction>& ir_instructions,
    const std::unordered_map<uint32_t, register_allocation::RegisterMapping>& register_map
) {
    LOG_DEBUG("Generating AArch64 code from IR.");
    std::vector<uint8_t> compiled_code;

    // TODO: Load initial EFLAGS state into the dedicated register/memory location

    for (const auto& instruction : ir_instructions) {
        // !!!!! DEBUGGING: Check for VEC_ADD_PS explicitly !!!!!
        if (instruction.type == ir::IrInstructionType::VEC_ADD_PS) {
            LOG_DEBUG("DEBUG: Explicit IF detected IR_VEC_ADD_PS. instruction.type cast to int: " + std::to_string(static_cast<int>(instruction.type)));
        } else {
            // Log if type is 63 but didn't match VEC_ADD_PS in the if
            if (static_cast<int>(instruction.type) == 63) {
                 LOG_ERROR("DEBUG: Explicit IF MISSED VEC_ADD_PS, but instruction.type IS 63. instruction.type cast to int: " + std::to_string(static_cast<int>(instruction.type)));
            }
        }
        // !!!!! END DEBUGGING !!!!!

        switch (instruction.type) {
            case ir::IrInstructionType::MOV: {
                // Assuming MOV with two operands: dest, src
                if (instruction.operands.size() == 2) {
                    const auto& dest_op = instruction.operands[0];
                    const auto& src_op = instruction.operands[1];

                    if (dest_op.type == ir::IrOperandType::REGISTER) {
                        uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);

                        if (src_op.type == ir::IrOperandType::IMMEDIATE) {
                            // MOV Rd, #imm (simplified, only handles small immediates)
                            uint64_t imm = src_op.imm_value;
                            if (imm < 0x1000) { // Check if immediate fits in 12 bits for ADD
                                // ADD Rd, XZR, #imm
                                // ADD (immediate): 0x91000000 | (imm << 10) | (Rn << 5) | Rd
                                // Rn = 31 (XZR/WZR)
                                uint32_t aarch64_inst = 0x91000000 | ((imm & 0xFFF) << 10) | (31 << 5) | dest_reg;
                                emit_instruction(compiled_code, aarch64_inst);
                                LOG_DEBUG("Generated AArch64 ADD (MOV imm) for IR_MOV.");
                            } else {
                                LOG_ERROR("IR_MOV with immediate too large for simple generation.");
                                // TODO: Handle larger immediates using MOVZ/MOVN/MOVK sequences
                            }
                        } else if (src_op.type == ir::IrOperandType::REGISTER) {
                            // MOV Rd, Rn (ORR Rd, RZ, Rn)
                            // ORR (shifted register): 0x2A0003E0 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                            // Use ORR Rd, RZ, Rn where RZ is zero register (31)
                            uint32_t src_reg = get_physical_reg(src_op.reg_idx, register_map);
                            uint32_t aarch64_inst = 0x2A0003E0 | (src_reg << 16) | (0 << 10) | (31 << 5) | dest_reg;
                             emit_instruction(compiled_code, aarch64_inst);
                             LOG_DEBUG("Generated AArch64 ORR (MOV reg) for IR_MOV.");
                         } else {
                              LOG_ERROR("Unsupported source operand type for IR_MOV.");
                         }
                     } else {
                         LOG_ERROR("Unsupported destination operand type for IR_MOV.");
                     }
                 } else {
                     LOG_ERROR("IR_MOV instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::ADD: {
                 // Assuming ADD with three operands: dest, op1, op2 (dest = op1 + op2)
                  if (instruction.operands.size() == 3) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];
                     const auto& op2 = instruction.operands[2];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // ADDS Rd, Rn, Rm (shifted register, shift = 0) - Updates flags
                         // 0x0B000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                         uint32_t aarch64_inst = 0x0B000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 ADDS for IR_ADD.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, CF, OF, PF, AF)

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_ADD.");
                     }
                 } else {
                     LOG_ERROR("IR_ADD instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::SUB: {
                 // Assuming SUB with three operands: dest, op1, op2 (dest = op1 - op2)
                  if (instruction.operands.size() == 3) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];
                     const auto& op2 = instruction.operands[2];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // SUBS Rd, Rn, Rm (shifted register, shift = 0) - Updates flags
                         // 0x6B000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                         uint32_t aarch64_inst = 0x6B000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 SUBS for IR_SUB.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, CF, OF, PF, AF)

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_SUB.");
                     }
                 } else {
                     LOG_ERROR("IR_SUB instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::ADC: {
                 // Assuming ADC with three operands: dest, op1, op2 (dest = op1 + op2 + CF)
                  if (instruction.operands.size() == 3) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];
                     const auto& op2 = instruction.operands[2];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // ADCS Rd, Rn, Rm (shifted register, shift = 0) - Updates flags
                         // 0x1B000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                         uint32_t aarch64_inst = 0x1B000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 ADCS for IR_ADC.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, CF, OF, PF, AF)

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_ADC.");
                     }
                 } else {
                     LOG_ERROR("IR_ADC instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::SBB: {
                 // Assuming SBB with three operands: dest, op1, op2 (dest = op1 - op2 - CF)
                  if (instruction.operands.size() == 3) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];
                     const auto& op2 = instruction.operands[2];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // SBCS Rd, Rn, Rm (shifted register, shift = 0) - Updates flags
                         // 0x5B000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                         uint32_t aarch64_inst = 0x5B000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 SBCS for IR_SBB.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, CF, OF, PF, AF)

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_SBB.");
                     }
                 } else {
                     LOG_ERROR("IR_SBB instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::AND: {
                 // Assuming AND with three operands: dest, op1, op2 (dest = op1 & op2)
                  if (instruction.operands.size() == 3) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];
                     const auto& op2 = instruction.operands[2];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // ANDS Rd, Rn, Rm (shifted register, shift = 0) - Updates flags (NZCV)
                         // 0x1A000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                         uint32_t aarch64_inst = 0x1A000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 ANDS for IR_AND.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, PF) - CF, OF, AF are undefined

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_AND.");
                     }
                 } else {
                     LOG_ERROR("IR_AND instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::OR: {
                 // Assuming OR with three operands: dest, op1, op2 (dest = op1 | op2)
                  if (instruction.operands.size() == 3) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];
                     const auto& op2 = instruction.operands[2];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // ORRS Rd, Rn, Rm (shifted register, shift = 0) - Updates flags (NZCV)
                         // 0x3A000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                         uint32_t aarch64_inst = 0x3A000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 ORRS for IR_OR.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, PF) - CF, OF, AF are undefined

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_OR.");
                     }
                 } else {
                     LOG_ERROR("IR_OR instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::XOR: {
                 // Assuming XOR with three operands: dest, op1, op2 (dest = op1 ^ op2)
                  if (instruction.operands.size() == 3) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];
                     const auto& op2 = instruction.operands[2];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // EOR Rd, Rn, Rm (shifted register, shift = 0) - Does NOT update flags
                         // 0x4A000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rd
                         uint32_t aarch64_inst = 0x4A000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 EOR for IR_XOR.");
                         // TODO: Manually update x86 EFLAGS (ZF, SF, PF) based on result

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_XOR.");
                     }
                 } else {
                     LOG_ERROR("IR_XOR instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::NOT: {
                 // Assuming NOT with two operands: dest, op1 (dest = ~op1)
                  if (instruction.operands.size() == 2) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);

                         // MVN Rd, Rm (shifted register, shift = 0) - Does NOT update flags
                         // 0x4A0003E0 | (Rm << 16) | (shift << 10) | (31 << 5) | Rd (ORR with inverted)
                         // Use ORN Rd, RZ, Rm
                         uint32_t aarch64_inst = 0x2A0003E0 | (op1_reg << 16) | (0 << 10) | (31 << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 ORN (NOT) for IR_NOT.");
                         // TODO: Manually update x86 EFLAGS (ZF, SF, PF) based on result

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_NOT.");
                     }
                 } else {
                     LOG_ERROR("IR_NOT instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::NEG: {
                 // Assuming NEG with two operands: dest, op1 (dest = -op1)
                  if (instruction.operands.size() == 2) {
                     const auto& dest_op = instruction.operands[0];
                     const auto& op1 = instruction.operands[1];

                     if (dest_op.type == ir::IrOperandType::REGISTER &&
                         op1.type == ir::IrOperandType::REGISTER) {

                         uint32_t dest_reg = get_physical_reg(dest_op.reg_idx, register_map);
                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);

                         // NEGS Rd, Rm (shifted register, shift = 0) - Updates flags
                         // 0x6B000000 | (Rm << 16) | (shift << 10) | (31 << 5) | Rd (SUBS Rd, XZR, Rm)
                         uint32_t aarch64_inst = 0x6B000000 | (op1_reg << 16) | (0 << 10) | (31 << 5) | dest_reg;
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 NEGS for IR_NEG.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, CF, OF, PF, AF)

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_NEG.");
                     }
                 } else {
                     LOG_ERROR("IR_NEG instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::CMP: {
                 // Assuming CMP with two operands: op1, op2 (op1 - op2, updates flags)
                  if (instruction.operands.size() == 2) {
                     const auto& op1 = instruction.operands[0];
                     const auto& op2 = instruction.operands[1];

                     if (op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // CMP Rn, Rm (shifted register, shift = 0) - Updates flags
                         // SUBS XZR, Rn, Rm (0x6B00001F | (Rm << 16) | (shift << 10) | (Rn << 5))
                         // Use WZR (31) for 32-bit comparison
                         uint32_t aarch64_inst = 0x6B00001F | (op2_reg << 16) | (0 << 10) | (op1_reg << 5);
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 SUBS (CMP) for IR_CMP.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, CF, OF, PF, AF)

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_CMP.");
                     }
                 } else {
                     LOG_ERROR("IR_CMP instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::TEST: {
                 // Assuming TEST with two operands: op1, op2 (op1 & op2, updates flags)
                  if (instruction.operands.size() == 2) {
                     const auto& op1 = instruction.operands[0];
                     const auto& op2 = instruction.operands[1];

                     if (op1.type == ir::IrOperandType::REGISTER &&
                         op2.type == ir::IrOperandType::REGISTER) {

                         uint32_t op1_reg = get_physical_reg(op1.reg_idx, register_map);
                         uint32_t op2_reg = get_physical_reg(op2.reg_idx, register_map);

                         // ANDS Rn, Rm (shifted register, shift = 0) - Updates flags (NZCV)
                         // 0x1A000000 | (Rm << 16) | (shift << 10) | (Rn << 5) | Rn (dest is same as op1 for flags)
                         uint32_t aarch64_inst = 0x1A000000 | (op2_reg << 16) | (0 << 10) | (op1_reg << 5) | op1_reg; // Dest is op1 for flags
                         emit_instruction(compiled_code, aarch64_inst);
                         LOG_DEBUG("Generated AArch64 ANDS (TEST) for IR_TEST.");
                         // TODO: Map AArch64 flags (NZCV) to x86 EFLAGS (ZF, SF, PF) - CF, OF, AF are undefined

                     } else {
                         LOG_ERROR("Unsupported operand types for IR_TEST.");
                     }
                 } else {
                     LOG_ERROR("IR_TEST instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::SHL: // Shift Left
             case ir::IrInstructionType::SHR: // Shift Right (Logical)
             case ir::IrInstructionType::SAR: // Shift Right (Arithmetic)
             case ir::IrInstructionType::ROL: // Rotate Left
             case ir::IrInstructionType::ROR: { // Rotate Right
                 // Assuming shift/rotate with two operands: dest/src, count
                  if (instruction.operands.size() == 2) {
                     const auto& rm_op = instruction.operands[0];
                     const auto& count_op = instruction.operands[1];

                     if (rm_op.type == ir::IrOperandType::REGISTER &&
                         count_op.type == ir::IrOperandType::IMMEDIATE) {

                         uint32_t rm_reg = get_physical_reg(rm_op.reg_idx, register_map);
                         uint64_t shift_count = count_op.imm_value;

                         // TODO: Implement AArch64 shift/rotate instructions and EFLAGS updates
                         LOG_DEBUG("Generated placeholder for shift/rotate instruction.");
                         // Example for LSL (Logical Shift Left): 0x13000000 | (imm << 10) | (Rm << 5) | Rd
                         // Need to handle different shift types and update flags (CF, ZF, SF, PF, OF)
                     } else {
                         LOG_ERROR("Unsupported operand types for shift/rotate instruction.");
                     }
                 } else {
                     LOG_ERROR("Shift/rotate instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::JMP: {
                 // Assuming JMP with one operand: target_label or target_address
                  if (instruction.operands.size() == 1) {
                     const auto& target_op = instruction.operands[0];

                     if (target_op.type == ir::IrOperandType::IMMEDIATE) {
                         // This is a direct jump to a guest address (relative displacement)
                         // We need to generate AArch64 code to jump to the target address
                         // (or a dispatcher stub if not chained).
                         LOG_DEBUG("Generated placeholder for unconditional jump.");
                         // TODO: Implement unconditional jump generation based on target address
                         // This will involve emitting a B instruction or jumping to a dispatcher stub.
                         // Need to handle saving the return address.
                     } else {
                          LOG_ERROR("Unsupported operand type for unconditional jump.");
                     }
                 } else {
                     LOG_ERROR("Unconditional jump instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::CALL: {
                 // Assuming CALL with one operand: target_label or target_address
                  if (instruction.operands.size() == 1) {
                     const auto& target_op = instruction.operands[0];

                     if (target_op.type == ir::IrOperandType::IMMEDIATE) {
                         // This is a direct call to a guest address (relative displacement)
                         // We need to generate AArch64 code to call the target address
                         // (or a dispatcher stub).
                         LOG_DEBUG("Generated placeholder for CALL.");
                         // TODO: Implement CALL generation based on target address
                         // This will involve emitting a BL instruction or calling a dispatcher stub.
                         // Need to handle saving the return address.
                     } else {
                          LOG_ERROR("Unsupported operand type for CALL.");
                     }
                 } else {
                     LOG_ERROR("CALL instruction has incorrect number of operands.");
                 }
                 break;
             }
             case ir::IrInstructionType::RET: {
                 // RET instruction
                 // D65F03C0
                 uint32_t aarch64_inst = 0xD65F03C0;
                 emit_instruction(compiled_code, aarch64_inst);
                 LOG_DEBUG("Generated AArch64 RET for IR_RET.");
                 break;
             }
             case ir::IrInstructionType::BR_EQ: // Conditional Branch: Branch if Equal (ZF=1)
             case ir::IrInstructionType::BR_NE: // Conditional Branch: Branch if Not Equal (ZF=0)
             case ir::IrInstructionType::BR_LT: // Conditional Branch: Branch if Less Than (SF!=OF)
             case ir::IrInstructionType::BR_LE: // Conditional Branch: Branch if Less Than or Equal (ZF=1 or SF!=OF)
             case ir::IrInstructionType::BR_GT: // Conditional Branch: Branch if Greater Than (ZF=0 and SF=OF)
             case ir::IrInstructionType::BR_GE: // Conditional Branch: Branch if Greater Than or Equal (SF=OF)
             case ir::IrInstructionType::BR_BL: // Conditional Branch: Branch if Below (CF=1) - Unsigned
             case ir::IrInstructionType::BR_BE: // Conditional Branch: Branch if Below or Equal (CF=1 or ZF=1) - Unsigned
             case ir::IrInstructionType::BR_BH: // Conditional Branch: Branch if Higher (CF=0 and ZF=0) - Unsigned
             case ir::IrInstructionType::BR_BHE: // Conditional Branch: Branch if Higher or Equal (CF=0) - Unsigned
             case ir::IrInstructionType::BR_ZERO: // Conditional Branch: Branch if Zero (ZF=1)
             case ir::IrInstructionType::BR_NOT_ZERO: // Conditional Branch: Branch if Not Zero (ZF=0)
             case ir::IrInstructionType::BR_SIGN: // Conditional Branch: Branch if Sign (SF=1)
             case ir::IrInstructionType::BR_NOT_SIGN: // Conditional Branch: Branch if Not Sign (SF=0)
             case ir::IrInstructionType::BR_OVERFLOW: // Conditional Branch: Branch if Overflow (OF=1)
             case ir::IrInstructionType::BR_NOT_OVERFLOW: // Conditional Branch: Branch if Not Overflow (OF=0)
             case ir::IrInstructionType::BR_PARITY: // Conditional Branch: Branch if Parity (PF=1)
             case ir::IrInstructionType::BR_NOT_PARITY: { // Conditional Branch: Branch if Not Parity (PF=0)
                 // Assuming conditional branch with two operands: target_label, condition_code (deprecated) or target_label
                 // The condition code is now implicit in the IR instruction type.
                  if (instruction.operands.size() == 1) {
                     const auto& target_op = instruction.operands[0];

                     if (target_op.type == ir::IrOperandType::IMMEDIATE) {
                         // This is a conditional jump to a guest address (relative displacement)
                         // We need to generate AArch64 code to jump to the target address
                         // (or a dispatcher stub if not chained), based on the EFLAGS state.
                         LOG_DEBUG("Generated placeholder for conditional jump.");
                         // TODO: Implement conditional jump generation based on target address and IR type
                         // This will involve reading the emulated EFLAGS state, setting AArch64 flags,
                         // and emitting a B.cond instruction or jumping to a dispatcher stub.
                     } else {
                          LOG_ERROR("Unsupported operand type for conditional jump.");
                     }
                 } else {
                     LOG_ERROR("Conditional jump instruction has incorrect number of operands.");
                 }
                 break;
             }

            case ir::IrInstructionType::VEC_ADD_W: {
                // Assuming VEC_ADD_W with three operands: dest, op1, op2 (dest = op1 + op2)
                if (instruction.operands.size() == 3) {
                    const auto& dest_op = instruction.operands[0];
                    const auto& op1 = instruction.operands[1];
                    const auto& op2 = instruction.operands[2];

                    if (dest_op.type == ir::IrOperandType::REGISTER &&
                        op1.type == ir::IrOperandType::REGISTER &&
                        op2.type == ir::IrOperandType::REGISTER) {
                        // Register to Register (MMX to MMX)
                        uint32_t dest_vreg = dest_op.reg_idx;
                        uint32_t op1_vreg = op1.reg_idx;
                        uint32_t op2_vreg = op2.reg_idx;
                        uint32_t dest_nreg = get_physical_reg(dest_vreg, register_map);
                        uint32_t op1_nreg = get_physical_reg(op1_vreg, register_map);
                        uint32_t op2_nreg = get_physical_reg(op2_vreg, register_map);

                        // ADD Vd.4H, Vn.4H, Vm.4H (Packed Add 16-bit integers)
                        // 0x0E200800 | (Vm << 16) | (Vn << 5) | Vd
                        uint32_t aarch64_inst = 0x0E200800 | (op2_nreg << 16) | (op1_nreg << 5) | dest_nreg;
                        emit_instruction(compiled_code, aarch64_inst);
                        LOG_DEBUG("Generated AArch64 ADD (NEON .4H) for IR_VEC_ADD_W (reg, reg, reg).");

                    } else if (dest_op.type == ir::IrOperandType::REGISTER &&
                               op1.type == ir::IrOperandType::REGISTER &&
                               op2.type == ir::IrOperandType::MEMORY) {
                        // Register to Memory (MMX, MMX, Memory) - dest is MMX, op1 is MMX, op2 is Memory
                        uint32_t dest_vreg = dest_op.reg_idx;
                        uint32_t op1_vreg = op1.reg_idx;
                        const auto& mem_info = op2.mem_info;
                        uint32_t dest_nreg = get_physical_reg(dest_vreg, register_map);
                        uint32_t op1_nreg = get_physical_reg(op1_vreg, register_map);
                        uint32_t base_gpr = get_physical_reg(mem_info.base_reg_idx, register_map);
                        // uint32_t index_gpr = get_physical_reg(mem_info.index_reg_idx, register_map); // Index not used in simple LD1

                        // Need to load memory operand into a temporary NEON register first
                        // LD1 {Vt.8B}, [Xn] (Load one 8-byte structure from base register) - MMX is 64-bit (8 bytes)
                        // 0x0D400400 | (V << 5) | Rn
                        // Use a temporary NEON register (e.g., V31)
                        uint32_t temp_nreg = 31; // Placeholder for a temporary NEON register
                        uint32_t load_inst = 0x0D400400 | (temp_nreg << 5) | base_gpr;
                        emit_instruction(compiled_code, load_inst);
                        LOG_DEBUG("Generated AArch64 LD1 (NEON load) for IR_VEC_ADD_W (mem operand).");
                        // TODO: Handle displacement and SIB addressing modes for memory operands

                        // Now perform the addition with the temporary register
                        // ADD Vd.4H, Vn.4H, Vm.4H
                        // 0x0E200800 | (Vm << 16) | (Vn << 5) | Vd
                        uint32_t add_inst = 0x0E200800 | (temp_nreg << 16) | (op1_nreg << 5) | dest_nreg;
                        emit_instruction(compiled_code, add_inst);
                        LOG_DEBUG("Generated AArch64 ADD (NEON .4H) for IR_VEC_ADD_W (reg, reg, mem).");

                    } else {
                         LOG_ERROR("Unsupported operand types for IR_VEC_ADD_W.");
                    }
                } else {
                    LOG_ERROR("IR_VEC_ADD_W instruction has incorrect number of operands.");
                }
                break;
            }

            case ir::IrInstructionType::VEC_ADD_PS: {
                // Assuming VEC_ADD_PS with three operands: dest, op1, op2 (dest = op1 + op2)
                if (instruction.operands.size() == 3) {
                    const auto& dest_op = instruction.operands[0];
                    const auto& op1 = instruction.operands[1];
                    const auto& op2 = instruction.operands[2];

                    if (dest_op.type == ir::IrOperandType::REGISTER &&
                        op1.type == ir::IrOperandType::REGISTER &&
                        op2.type == ir::IrOperandType::REGISTER) {
                        // Register to Register (XMM to XMM)
                        uint32_t dest_vreg = dest_op.reg_idx;
                        uint32_t op1_vreg = op1.reg_idx;
                        uint32_t op2_vreg = op2.reg_idx;
                        uint32_t dest_nreg = get_physical_reg(dest_vreg, register_map);
                        uint32_t op1_nreg = get_physical_reg(op1_vreg, register_map);
                        uint32_t op2_nreg = get_physical_reg(op2_vreg, register_map);

                        // FADD Vd.4S, Vn.4S, Vm.4S (Packed Add Single-Precision Floating-Point)
                        // 0x0E201800 | (Vm << 16) | (Vn << 5) | Vd
                        uint32_t aarch64_inst = 0x0E201800 | (op2_nreg << 16) | (op1_nreg << 5) | dest_nreg;
                        emit_instruction(compiled_code, aarch64_inst);
                        LOG_DEBUG("Generated AArch64 FADD (NEON .4S) for IR_VEC_ADD_PS (reg, reg, reg).");

                    } else if (dest_op.type == ir::IrOperandType::REGISTER &&
                               op1.type == ir::IrOperandType::REGISTER &&
                               op2.type == ir::IrOperandType::MEMORY) {
                        // Register to Memory (XMM, XMM, Memory) - dest is XMM, op1 is XMM, op2 is Memory
                        uint32_t dest_vreg = dest_op.reg_idx;
                        uint32_t op1_vreg = op1.reg_idx;
                        const auto& mem_info = op2.mem_info;
                        uint32_t dest_nreg = get_physical_reg(dest_vreg, register_map);
                        uint32_t op1_nreg = get_physical_reg(op1_vreg, register_map);
                        uint32_t base_gpr = get_physical_reg(mem_info.base_reg_idx, register_map);
                        // uint32_t index_gpr = get_physical_reg(mem_info.index_reg_idx, register_map); // Index not used in simple LD1

                        // Need to load memory operand into a temporary NEON register first
                        // LD1 {Vt.4S}, [Xn] (Load one 16-byte structure from base register) - SSE is 128-bit (16 bytes)
                        // 0x0D400400 | (V << 5) | Rn
                        // Use a temporary NEON register (e.g., V31)
                        uint32_t temp_nreg = 31; // Placeholder for a temporary NEON register
                        uint32_t load_inst = 0x0D400400 | (temp_nreg << 5) | base_gpr;
                        emit_instruction(compiled_code, load_inst);
                        LOG_DEBUG("Generated AArch64 LD1 (NEON load) for IR_VEC_ADD_PS (mem operand).");
                        // TODO: Handle displacement and SIB addressing modes for memory operands

                        // Now perform the addition with the temporary register
                        // FADD Vd.4S, Vn.4S, Vm.4S
                        // 0x0E201800 | (Vm << 16) | (Vn << 5) | Vd
                        uint32_t add_inst = 0x0E201800 | (temp_nreg << 16) | (op1_nreg << 5) | dest_nreg;
                        emit_instruction(compiled_code, add_inst);
                        LOG_DEBUG("Generated AArch64 FADD (NEON .4S) for IR_VEC_ADD_PS (reg, reg, mem).");

                    } else {
                         LOG_ERROR("Unsupported operand types for IR_VEC_ADD_PS.");
                    }
                } else {
                    LOG_ERROR("IR_VEC_ADD_PS instruction has incorrect number of operands.");
                }
                break;
            }

            case ir::IrInstructionType::VEC_MOV: {
                // Assuming VEC_MOV with two operands: dest, src
                if (instruction.operands.size() == 2) {
                    const auto& dest_op = instruction.operands[0];
                    const auto& src_op = instruction.operands[1];

                    if (dest_op.type == ir::IrOperandType::REGISTER && src_op.type == ir::IrOperandType::REGISTER) {
                        // Register to Register (XMM to XMM)
                        uint32_t dest_vreg = dest_op.reg_idx;
                        uint32_t src_vreg = src_op.reg_idx;
                        uint32_t dest_nreg = get_physical_reg(dest_vreg, register_map);
                        uint32_t src_nreg = get_physical_reg(src_vreg, register_map);

                        // MOV Vd.16B, Vn.16B (Vector Move)
                        // 0x4E201C00 | (Vn << 5) | Vd
                        uint32_t aarch64_inst = 0x4E201C00 | (src_nreg << 5) | dest_nreg;
                        emit_instruction(compiled_code, aarch64_inst);
                        LOG_DEBUG("Generated AArch64 MOV (NEON reg) for IR_VEC_MOV.");

                    } else if (dest_op.type == ir::IrOperandType::REGISTER && src_op.type == ir::IrOperandType::MEMORY) {
                        // Memory to Register (Memory to XMM)
                        uint32_t dest_vreg = dest_op.reg_idx;
                        const auto& mem_info = src_op.mem_info;
                        uint32_t dest_nreg = get_physical_reg(dest_vreg, register_map);
                        uint32_t base_gpr = get_physical_reg(mem_info.base_reg_idx, register_map);
                        // uint32_t index_gpr = get_physical_reg(mem_info.index_reg_idx, register_map); // Index not used in simple LD1

                        // LD1 {Vt.16B}, [Xn] (Load one 16-byte structure from base register)
                        // 0x4C407800 | (Vt << 5) | Xn
                        uint32_t aarch64_inst = 0x4C407800 | (dest_nreg << 5) | base_gpr;
                        emit_instruction(compiled_code, aarch64_inst);
                        LOG_DEBUG("Generated AArch64 LD1 (NEON load) for IR_VEC_MOV.");
                        // TODO: Handle displacement and SIB addressing modes for memory operands

                    } else if (dest_op.type == ir::IrOperandType::MEMORY && src_op.type == ir::IrOperandType::REGISTER) {
                        // Register to Memory (XMM to Memory)
                        const auto& mem_info = dest_op.mem_info;
                        uint32_t src_vreg = src_op.reg_idx;
                        uint32_t src_nreg = get_physical_reg(src_vreg, register_map);
                        uint32_t base_gpr = get_physical_reg(mem_info.base_reg_idx, register_map);
                        // uint32_t index_gpr = get_physical_reg(mem_info.index_reg_idx, register_map); // Index not used in simple ST1

                        // ST1 {Vt.16B}, [Xn] (Store one 16-byte structure to base register)
                        // 0x4C007800 | (Vt << 5) | Xn
                        uint32_t aarch64_inst = 0x4C007800 | (src_nreg << 5) | base_gpr;
                        emit_instruction(compiled_code, aarch64_inst);
                        LOG_DEBUG("Generated AArch64 ST1 (NEON store) for IR_VEC_MOV.");
                        // TODO: Handle displacement and SIB addressing modes for memory operands

                    } else {
                         LOG_ERROR("Unsupported operand types for IR_VEC_MOV.");
                    }
                } else {
                    LOG_ERROR("IR_VEC_MOV instruction has incorrect number of operands.");
                }
                break;
            }

            default:
                LOG_ERROR("Unsupported IR instruction type for AArch64 generation. Type: " + std::to_string(static_cast<int>(instruction.type)));
                // For unsupported instructions, emit a trap or an illegal instruction
                // This is a placeholder; a proper implementation would handle all required IR types.
                emit_instruction(compiled_code, 0x00000000); // HLT #0 (effectively a trap)
                break;
        }
    }

    // TODO: Save final EFLAGS state from the dedicated register/memory location

    LOG_DEBUG("Finished AArch64 code generation.");
    return compiled_code;
}

// Update function definitions to use the correct namespace
void CodeGenerator::patch_branch(translation_cache::TranslatedBlock* source_block, 
                             const translation_cache::TranslatedBlock::ControlFlowExit& exit, 
                             translation_cache::TranslatedBlock* target_block) {
    // Calculate the relative branch offset (in units of instructions, not bytes)
    // AArch64 branches use pc-relative addressing with units of 4 bytes (32-bit instructions)
    int64_t relative_offset = (reinterpret_cast<int64_t>(target_block->code.data()) - 
                              reinterpret_cast<int64_t>(source_block->code.data() + 
                              exit.instruction_offset)) / 4;

    if (relative_offset > 0x7FFFFF || relative_offset < -0x800000) {
        // Offset is too large for immediate branch; we need a register-based solution
        LOG_ERROR("Branch offset too large for direct branch. Need to implement long branches.");
    } else {
        // Get a pointer to the instruction to patch (previously emitted B or BL)
        uint32_t* instruction_ptr = reinterpret_cast<uint32_t*>(source_block->code.data() + 
                                   exit.instruction_offset);
        
        // Patch a B (Unconditional Branch) instruction
        // Format: 0x14000000 | imm26  (imm26 is a 26-bit signed immediate, units of 4 bytes)
        // We take the lower 26 bits of the relative offset
        *instruction_ptr = 0x14000000 | (relative_offset & 0x3FFFFFF);
        
        LOG_DEBUG("Patched branch instruction at offset " + std::to_string(exit.instruction_offset) + 
                 " in source block.");
    }
}

void CodeGenerator::patch_branch_false(translation_cache::TranslatedBlock* source_block, 
                                   const translation_cache::TranslatedBlock::ControlFlowExit& exit, 
                                   translation_cache::TranslatedBlock* target_block_false) {
    // For conditional branches, we need to patch the branch-not-taken target
    // This would typically be implemented as a B.cond instruction
    
    LOG_ERROR("patch_branch_false is not yet implemented");
    // TODO: Implement patch_branch_false for conditional branches
}


} // namespace aarch64
} // namespace xenoarm_jit