#include "ir/ir.h"
#include <sstream>

IRGenerator::IRGenerator() {
    // Initialize IR generator
}

IRGenerator::~IRGenerator() {
    // Clean up IR generator
}

bool IRGenerator::translate(const DecodedInsn& insn, IRBlock& block) {
    // In a full implementation, this would translate x86 instructions to IR
    // For test purposes, we'll implement a minimal translation
    
    switch (insn.opcode) {
        case X86_MOV: {
            if (insn.operands[0].type == OPERAND_REGISTER && insn.operands[1].type == OPERAND_REGISTER) {
                // MOV reg, reg
                IRInstruction ir_instr;
                ir_instr.opcode = IR_MOV_REG;
                
                // Map source operand
                ir_instr.operands[0].type = IR_OP_REGISTER;
                if (insn.operands[0].reg.id >= EAX && insn.operands[0].reg.id <= EDI) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_EAX + (insn.operands[0].reg.id - EAX));
                } else if (insn.operands[0].reg.id >= AX && insn.operands[0].reg.id <= DI) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_AX + (insn.operands[0].reg.id - AX));
                } else if (insn.operands[0].reg.id >= MM0 && insn.operands[0].reg.id <= MM7) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_MM0 + (insn.operands[0].reg.id - MM0));
                } else if (insn.operands[0].reg.id >= XMM0 && insn.operands[0].reg.id <= XMM7) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_XMM0 + (insn.operands[0].reg.id - XMM0));
                }
                ir_instr.operands[0].reg.size = insn.operands[0].reg.size;
                
                // Map destination operand
                ir_instr.operands[1].type = IR_OP_REGISTER;
                if (insn.operands[1].reg.id >= EAX && insn.operands[1].reg.id <= EDI) {
                    ir_instr.operands[1].reg.id = static_cast<IRRegister>(IR_REG_EAX + (insn.operands[1].reg.id - EAX));
                } else if (insn.operands[1].reg.id >= AX && insn.operands[1].reg.id <= DI) {
                    ir_instr.operands[1].reg.id = static_cast<IRRegister>(IR_REG_AX + (insn.operands[1].reg.id - AX));
                } else if (insn.operands[1].reg.id >= MM0 && insn.operands[1].reg.id <= MM7) {
                    ir_instr.operands[1].reg.id = static_cast<IRRegister>(IR_REG_MM0 + (insn.operands[1].reg.id - MM0));
                } else if (insn.operands[1].reg.id >= XMM0 && insn.operands[1].reg.id <= XMM7) {
                    ir_instr.operands[1].reg.id = static_cast<IRRegister>(IR_REG_XMM0 + (insn.operands[1].reg.id - XMM0));
                }
                ir_instr.operands[1].reg.size = insn.operands[1].reg.size;
                
                block.instructions.push_back(ir_instr);
                return true;
            }
            else if (insn.operands[0].type == OPERAND_REGISTER && insn.operands[1].type == OPERAND_IMMEDIATE) {
                // MOV reg, imm
                IRInstruction ir_instr;
                ir_instr.opcode = IR_MOV_IMM;
                
                // Map register operand
                ir_instr.operands[0].type = IR_OP_REGISTER;
                if (insn.operands[0].reg.id >= EAX && insn.operands[0].reg.id <= EDI) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_EAX + (insn.operands[0].reg.id - EAX));
                } else if (insn.operands[0].reg.id >= AX && insn.operands[0].reg.id <= DI) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_AX + (insn.operands[0].reg.id - AX));
                }
                ir_instr.operands[0].reg.size = insn.operands[0].reg.size;
                
                // Map immediate operand
                ir_instr.operands[1].type = IR_OP_IMMEDIATE;
                ir_instr.operands[1].imm.value = insn.operands[1].imm.value;
                ir_instr.operands[1].imm.size = insn.operands[1].imm.size;
                
                block.instructions.push_back(ir_instr);
                return true;
            }
            else if (insn.operands[0].type == OPERAND_REGISTER && insn.operands[1].type == OPERAND_MEMORY) {
                // MOV reg, [mem]
                IRInstruction ir_instr;
                ir_instr.opcode = IR_LOAD;
                
                // Map register operand
                ir_instr.operands[0].type = IR_OP_REGISTER;
                if (insn.operands[0].reg.id >= EAX && insn.operands[0].reg.id <= EDI) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_EAX + (insn.operands[0].reg.id - EAX));
                } else if (insn.operands[0].reg.id >= AX && insn.operands[0].reg.id <= DI) {
                    ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_AX + (insn.operands[0].reg.id - AX));
                }
                ir_instr.operands[0].reg.size = insn.operands[0].reg.size;
                
                // Map memory address into register
                ir_instr.operands[1].type = IR_OP_REGISTER;
                if (insn.operands[1].mem.base >= EAX && insn.operands[1].mem.base <= EDI) {
                    ir_instr.operands[1].reg.id = static_cast<IRRegister>(IR_REG_EAX + (insn.operands[1].mem.base - EAX));
                }
                ir_instr.operands[1].reg.size = 32; // Address is 32-bit
                
                block.instructions.push_back(ir_instr);
                return true;
            }
            break;
        }
        
        case X86_MOVQ: {
            // MMX MOVQ instruction
            IRInstruction ir_instr;
            ir_instr.opcode = IR_MMX_MOV;
            
            // Setup operands
            if (insn.operands[0].type == OPERAND_REGISTER && insn.operands[1].type == OPERAND_REGISTER) {
                // Register to register
                ir_instr.operands[0].type = IR_OP_REGISTER;
                ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_MM0 + (insn.operands[0].reg.id - MM0));
                ir_instr.operands[0].reg.size = 64;
                
                ir_instr.operands[1].type = IR_OP_REGISTER;
                ir_instr.operands[1].reg.id = static_cast<IRRegister>(IR_REG_MM0 + (insn.operands[1].reg.id - MM0));
                ir_instr.operands[1].reg.size = 64;
                
                block.instructions.push_back(ir_instr);
                return true;
            }
            break;
        }
        
        case X86_MOVAPS: {
            // SSE MOVAPS instruction
            IRInstruction ir_instr;
            ir_instr.opcode = IR_SSE_MOVAPS;
            
            // Setup operands
            if (insn.operands[0].type == OPERAND_REGISTER && insn.operands[1].type == OPERAND_REGISTER) {
                // Register to register
                ir_instr.operands[0].type = IR_OP_REGISTER;
                ir_instr.operands[0].reg.id = static_cast<IRRegister>(IR_REG_XMM0 + (insn.operands[0].reg.id - XMM0));
                ir_instr.operands[0].reg.size = 128;
                
                ir_instr.operands[1].type = IR_OP_REGISTER;
                ir_instr.operands[1].reg.id = static_cast<IRRegister>(IR_REG_XMM0 + (insn.operands[1].reg.id - XMM0));
                ir_instr.operands[1].reg.size = 128;
                
                block.instructions.push_back(ir_instr);
                return true;
            }
            break;
        }
        
        default:
            // Unsupported instruction
            return false;
    }
    
    return false;
}

// IR Dumper implementation
IRDumper::IRDumper() {
    // Initialize dumper
}

IRDumper::~IRDumper() {
    // Clean up dumper
}

std::string IRDumper::dumpBlock(const IRBlock& block) {
    std::stringstream ss;
    
    ss << "IR Block at 0x" << std::hex << block.start_address;
    ss << " (Instruction count: " << std::dec << block.instructions.size() << ")\n";
    
    for (size_t i = 0; i < block.instructions.size(); ++i) {
        const IRInstruction& instr = block.instructions[i];
        
        ss << "  [" << i << "] ";
        
        switch (instr.opcode) {
            case IR_MOV_REG:
                ss << "MOV_REG ";
                if (instr.operands[0].type == IR_OP_REGISTER) {
                    ss << "REG" << instr.operands[0].reg.id << ", ";
                }
                if (instr.operands[1].type == IR_OP_REGISTER) {
                    ss << "REG" << instr.operands[1].reg.id;
                }
                break;
                
            case IR_MOV_IMM:
                ss << "MOV_IMM ";
                if (instr.operands[0].type == IR_OP_REGISTER) {
                    ss << "REG" << instr.operands[0].reg.id << ", ";
                }
                if (instr.operands[1].type == IR_OP_IMMEDIATE) {
                    ss << "0x" << std::hex << instr.operands[1].imm.value << std::dec;
                }
                break;
                
            case IR_LOAD:
                ss << "LOAD ";
                if (instr.operands[0].type == IR_OP_REGISTER) {
                    ss << "REG" << instr.operands[0].reg.id << ", ";
                }
                if (instr.operands[1].type == IR_OP_REGISTER) {
                    ss << "[REG" << instr.operands[1].reg.id << "]";
                }
                break;
                
            case IR_MMX_MOV:
                ss << "MMX_MOV ";
                if (instr.operands[0].type == IR_OP_REGISTER && instr.operands[0].reg.id >= IR_REG_MM0 && 
                    instr.operands[0].reg.id <= IR_REG_MM7) {
                    ss << "MM" << (instr.operands[0].reg.id - IR_REG_MM0) << ", ";
                }
                if (instr.operands[1].type == IR_OP_REGISTER && instr.operands[1].reg.id >= IR_REG_MM0 && 
                    instr.operands[1].reg.id <= IR_REG_MM7) {
                    ss << "MM" << (instr.operands[1].reg.id - IR_REG_MM0);
                }
                break;
                
            case IR_SSE_MOVAPS:
                ss << "SSE_MOVAPS ";
                if (instr.operands[0].type == IR_OP_REGISTER && instr.operands[0].reg.id >= IR_REG_XMM0 && 
                    instr.operands[0].reg.id <= IR_REG_XMM7) {
                    ss << "XMM" << (instr.operands[0].reg.id - IR_REG_XMM0) << ", ";
                }
                if (instr.operands[1].type == IR_OP_REGISTER && instr.operands[1].reg.id >= IR_REG_XMM0 && 
                    instr.operands[1].reg.id <= IR_REG_XMM7) {
                    ss << "XMM" << (instr.operands[1].reg.id - IR_REG_XMM0);
                }
                break;
                
            default:
                ss << "UNKNOWN";
                break;
        }
        
        ss << "\n";
    }
    
    return ss.str();
} 