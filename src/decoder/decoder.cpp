#include "decoder/decoder.h"
#include <cstring>
#include <iostream>

Decoder::Decoder() {
    // Initialize decoder
}

Decoder::~Decoder() {
    // Cleanup
}

bool Decoder::decode(const uint8_t* code, size_t size, DecodedInsn& insn) {
    if (!code || size < 1) {
        return false;
    }
    
    // Reset the instruction
    memset(&insn, 0, sizeof(DecodedInsn));
    
    // Initialize prefixes with defaults
    insn.prefixes.operand_size = 32; // Default to 32-bit operand size
    insn.prefixes.address_size = 32; // Default to 32-bit address size
    
    // Track current position in the instruction stream
    size_t pos = 0;
    
    // Process prefixes
    bool has_prefix = true;
    while (has_prefix && pos < size) {
        switch (code[pos]) {
            case 0xF0: // LOCK
            case 0xF2: // REPNE/REPNZ
                insn.prefixes.repne = true;
                pos++;
                break;
            case 0xF3: // REP/REPE/REPZ
                insn.prefixes.rep = true;
                pos++;
                break;
            case 0x26: // ES override
            case 0x2E: // CS override
            case 0x36: // SS override
            case 0x3E: // DS override
            case 0x64: // FS override
            case 0x65: // GS override
                insn.prefixes.segment = code[pos] & 0x0F;
                pos++;
                break;
            case 0x66: // Operand size override
                insn.prefixes.operand_size = 16;
                pos++;
                break;
            case 0x67: // Address size override
                insn.prefixes.address_size = 16;
                pos++;
                break;
            default:
                has_prefix = false;
                break;
        }
    }
    
    // Not enough bytes for opcode
    if (pos >= size) {
        return false;
    }
    
    // Process opcode and operands
    uint8_t opcode = code[pos++];
    
    // Handle basic MOV instructions for testing
    if (opcode == 0x89) { // MOV r/m32, r32
        if (pos >= size) return false;
        
        uint8_t modrm = code[pos++];
        uint8_t mod = (modrm >> 6) & 0x3;
        uint8_t reg = (modrm >> 3) & 0x7;
        uint8_t rm = modrm & 0x7;
        
        insn.opcode = X86_MOV;
        insn.length = pos;
        
        // Destination operand (r/m)
        if (mod == 3) { // Register to register
            insn.operands[0].type = OPERAND_REGISTER;
            insn.operands[0].reg.id = static_cast<X86Register>(rm);
            insn.operands[0].reg.size = 32;
            
            // Source operand (reg)
            insn.operands[1].type = OPERAND_REGISTER;
            insn.operands[1].reg.id = static_cast<X86Register>(reg);
            insn.operands[1].reg.size = 32;
        } else {
            // Memory operand not fully implemented for simplicity
            return false;
        }
        
        return true;
    } 
    else if ((opcode & 0xF8) == 0xB8) { // MOV r32, imm32
        if (pos + 3 >= size) return false;
        
        uint8_t reg = opcode & 0x7;
        uint32_t imm = code[pos] | (code[pos+1] << 8) | (code[pos+2] << 16) | (code[pos+3] << 24);
        pos += 4;
        
        insn.opcode = X86_MOV;
        insn.length = pos;
        
        // Destination operand (register)
        insn.operands[0].type = OPERAND_REGISTER;
        insn.operands[0].reg.id = static_cast<X86Register>(reg);
        insn.operands[0].reg.size = 32;
        
        // Source operand (immediate)
        insn.operands[1].type = OPERAND_IMMEDIATE;
        insn.operands[1].imm.value = imm;
        insn.operands[1].imm.size = 32;
        
        return true;
    }
    else if (opcode == 0x8B) { // MOV r32, r/m32
        if (pos >= size) return false;
        
        uint8_t modrm = code[pos++];
        uint8_t mod = (modrm >> 6) & 0x3;
        uint8_t reg = (modrm >> 3) & 0x7;
        uint8_t rm = modrm & 0x7;
        
        insn.opcode = X86_MOV;
        
        // Destination operand (reg)
        insn.operands[0].type = OPERAND_REGISTER;
        
        // Handle 16-bit operand size prefix
        if (insn.prefixes.operand_size == 16) {
            insn.operands[0].reg.id = static_cast<X86Register>(AX + reg); // Map to 16-bit register
            insn.operands[0].reg.size = 16;
        } else {
            insn.operands[0].reg.id = static_cast<X86Register>(reg);
            insn.operands[0].reg.size = 32;
        }
        
        // Source operand (r/m)
        if (mod == 3) { // Register
            insn.operands[1].type = OPERAND_REGISTER;
            
            if (insn.prefixes.operand_size == 16) {
                insn.operands[1].reg.id = static_cast<X86Register>(AX + rm); // Map to 16-bit register
                insn.operands[1].reg.size = 16;
            } else {
                insn.operands[1].reg.id = static_cast<X86Register>(rm);
                insn.operands[1].reg.size = 32;
            }
        } else { 
            // Memory operand
            insn.operands[1].type = OPERAND_MEMORY;
            insn.operands[1].mem.base = static_cast<X86Register>(rm);
            insn.operands[1].mem.index = REGISTER_NONE;
            insn.operands[1].mem.scale = 0;
            
            // Displacement
            if (mod == 1) { // 8-bit displacement
                if (pos >= size) return false;
                insn.operands[1].mem.disp = static_cast<int8_t>(code[pos++]); // Sign extend
            } else if (mod == 2) { // 32-bit displacement
                if (pos + 3 >= size) return false;
                insn.operands[1].mem.disp = code[pos] | (code[pos+1] << 8) | 
                                          (code[pos+2] << 16) | (code[pos+3] << 24);
                pos += 4;
            }
            
            insn.operands[1].mem.size = (insn.prefixes.operand_size == 16) ? 16 : 32;
        }
        
        insn.length = pos;
        return true;
    }
    else if (opcode == 0x66 && pos < size && code[pos] == 0x8B) { // 16-bit operand override + MOV
        // Already processed the prefix, so just handle the MOV
        return decode(code + 1, size - 1, insn);
    }
    
    // Unrecognized or unimplemented opcode
    insn.opcode = X86_INVALID;
    return false;
}
