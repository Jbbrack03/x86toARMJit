#include "xenoarm_jit/aarch64/arm_assembler.h"
#include "logging/logger.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

namespace xenoarm_jit {
namespace aarch64 {

// Class static members
std::unordered_set<int> ArmAssembler::used_registers_;
std::unordered_map<std::string, int> ArmAssembler::named_registers_;
std::vector<std::string> ArmAssembler::register_names_ = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
    "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30", "sp"
};

ArmAssembler::ArmAssembler() {
    // Initialize register state
    used_registers_.clear();
    
    // Reserve registers that should not be directly allocated by the JIT
    reserveScratchRegister(29);  // x29 - frame pointer
    reserveScratchRegister(30);  // x30 - link register
    reserveScratchRegister(31);  // sp - stack pointer
    
    // Default aliases for commonly used registers
    named_registers_["fp"] = 29;  // x29
    named_registers_["lr"] = 30;  // x30
}

ArmAssembler::~ArmAssembler() {
    // Clear register state
    used_registers_.clear();
    named_registers_.clear();
}

void ArmAssembler::emitInstruction(const std::string& instruction) {
    LOG_DEBUG("Emitting instruction: " + instruction);
    // Implementation would add the instruction to a buffer
    
    // In a real implementation, we'd parse the instruction here and
    // ensure register usage is tracked correctly
}

void ArmAssembler::emitFunctionCall(const std::string& function) {
    LOG_DEBUG("Emitting function call: " + function);
    // Implementation would add a call to this function
    
    // In the actual implementation, we would:
    // 1. Save any live registers according to ABI
    // 2. Set up arguments
    // 3. Generate BL instruction
    // 4. Handle return value
    // 5. Restore saved registers
}

void ArmAssembler::emitLabel(const std::string& label) {
    LOG_DEBUG("Emitting label: " + label);
    // Implementation would add this label to the assembly
}

void ArmAssembler::reserveScratchRegister(int reg) {
    if (reg >= 0 && reg < 32) {
        LOG_DEBUG("Reserving scratch register: " + register_names_[reg]);
        used_registers_.insert(reg);
    } else {
        LOG_ERROR("Invalid register number in reserveScratchRegister: " + std::to_string(reg));
    }
}

void ArmAssembler::releaseScratchRegister(int reg) {
    if (reg >= 0 && reg < 32) {
        LOG_DEBUG("Releasing scratch register: " + register_names_[reg]);
        used_registers_.erase(reg);
    } else {
        LOG_ERROR("Invalid register number in releaseScratchRegister: " + std::to_string(reg));
    }
}

// Get an available scratch register
int ArmAssembler::getFreeScratchRegister() {
    // Look for an available register (x0-x17 are caller-saved)
    for (int i = 0; i < 18; i++) {
        if (used_registers_.find(i) == used_registers_.end()) {
            reserveScratchRegister(i);
            return i;
        }
    }
    
    // If no registers are available, report an error
    LOG_ERROR("No free scratch registers available");
    return -1;
}

// Get a named register, allocating if necessary
int ArmAssembler::getNamedRegister(const std::string& name) {
    // Check if this named register already exists
    auto it = named_registers_.find(name);
    if (it != named_registers_.end()) {
        return it->second;
    }
    
    // If not, allocate a new register
    int reg = getFreeScratchRegister();
    if (reg >= 0) {
        named_registers_[name] = reg;
    }
    return reg;
}

// Convert a floating-point value from 80-bit format to ARM NEON register
void ArmAssembler::emitConvertF80ToNEON(const std::string& src_mem, const std::string& dst_reg) {
    LOG_DEBUG("Converting 80-bit float to NEON register: " + src_mem + " -> " + dst_reg);
    
    // Generate optimized assembly for 80-bit to NEON conversion
    // This avoids intermediate memory operations for better performance
    
    // Load the significand (64 bits) into a GPR
    emitInstruction("LDR x0, [" + src_mem + "]");
    
    // Load the exponent and sign (16 bits)
    emitInstruction("LDRH w1, [" + src_mem + ", #8]");
    
    // Extract sign bit (bit 15 of the exponent word)
    emitInstruction("LSR w2, w1, #15");        // Extract sign bit to w2
    emitInstruction("LSL w2, w2, #31");        // Position sign bit for float
    
    // Extract and adjust the exponent (bits 0-14)
    emitInstruction("AND w1, w1, #0x7FFF");    // Mask off sign bit
    emitInstruction("SUB w1, w1, #16383");     // Unbias x87 exponent (x87 bias is 16383)
    emitInstruction("ADD w1, w1, #1023");      // Rebias for IEEE-754 double (bias is 1023)
    emitInstruction("LSL w1, w1, #20");        // Position exponent for double
    
    // Create the IEEE-754 double format in x3
    emitInstruction("ORR w3, w2, w1");         // Combine sign and exponent
    emitInstruction("LSR x1, x0, #11");        // Shift significand for double format (dropping explicit bit)
    emitInstruction("BFI x3, x1, #0, #52");    // Insert significand into result
    
    // Store the result to NEON register
    emitInstruction("FMOV " + dst_reg + ", x3");
}

// Convert a NEON register to 80-bit float format
void ArmAssembler::emitConvertNEONToF80(const std::string& src_reg, const std::string& dst_mem) {
    LOG_DEBUG("Converting NEON register to 80-bit float: " + src_reg + " -> " + dst_mem);
    
    // Generate optimized assembly for NEON to 80-bit conversion
    
    // Extract the bits from double to GPR
    emitInstruction("FMOV x0, " + src_reg);
    
    // Extract sign bit (bit 63)
    emitInstruction("LSR x1, x0, #63");        // Extract sign bit
    emitInstruction("LSL x1, x1, #15");        // Position sign bit for x87 format
    
    // Extract and adjust the exponent (bits 52-62)
    emitInstruction("LSR x2, x0, #52");        // Shift right to isolate exponent
    emitInstruction("AND x2, x2, #0x7FF");     // Mask out to get 11-bit exponent
    emitInstruction("SUB x2, x2, #1023");      // Unbias double exponent
    emitInstruction("ADD x2, x2, #16383");     // Rebias for x87 (16383 bias)
    emitInstruction("ORR x1, x1, x2");         // Combine sign and exponent into x1
    
    // Extract the significand (bits 0-51)
    emitInstruction("AND x0, x0, #0xFFFFFFFFFFFFF"); // Mask out to get 52-bit significand
    emitInstruction("ORR x0, x0, #0x8000000000000"); // Set explicit integer bit (bit 63)
    emitInstruction("LSL x0, x0, #11");        // Shift for 80-bit format
    
    // Store the result
    emitInstruction("STR x0, [" + dst_mem + "]");               // Store significand
    emitInstruction("STRH w1, [" + dst_mem + ", #8]");          // Store exponent+sign
}

// Emit code to transfer directly between NEON registers and X87 registers
void ArmAssembler::emitDirectRegisterTransfer(int fpu_reg, const std::string& neon_reg, bool is_read) {
    if (is_read) {
        // Reading from FPU register to NEON register
        LOG_DEBUG("Direct transfer from FPU register " + std::to_string(fpu_reg) + " to " + neon_reg);
        
        // Generate efficient code to read from FPU register memory area directly to NEON register
        emitInstruction("MOV w0, #" + std::to_string(fpu_reg));
        emitInstruction("BL SIMDState::get_fpu_reg_addr");  // Get address of FPU register in x0
        emitConvertF80ToNEON("x0", neon_reg);
    } else {
        // Writing from NEON register to FPU register
        LOG_DEBUG("Direct transfer from " + neon_reg + " to FPU register " + std::to_string(fpu_reg));
        
        // Generate efficient code to write from NEON register directly to FPU register memory area
        emitInstruction("MOV w0, #" + std::to_string(fpu_reg));
        emitInstruction("BL SIMDState::get_fpu_reg_addr");  // Get address of FPU register in x0
        emitConvertNEONToF80(neon_reg, "x0");
        
        // Update the tag for the FPU register
        emitInstruction("MOV w0, #" + std::to_string(fpu_reg));
        emitInstruction("BL SIMDState::update_tag");
    }
}

// Emit code to perform an FPU operation directly on NEON registers
void ArmAssembler::emitDirectFPUOperation(const std::string& op, const std::string& src_reg, const std::string& dst_reg) {
    LOG_DEBUG("Direct FPU operation: " + op + " " + dst_reg + ", " + src_reg);
    
    // Generate optimized ARM instruction for FPU operation
    if (op == "add") {
        emitInstruction("FADD " + dst_reg + ", " + dst_reg + ", " + src_reg);
    } else if (op == "sub") {
        emitInstruction("FSUB " + dst_reg + ", " + dst_reg + ", " + src_reg);
    } else if (op == "mul") {
        emitInstruction("FMUL " + dst_reg + ", " + dst_reg + ", " + src_reg);
    } else if (op == "div") {
        emitInstruction("FDIV " + dst_reg + ", " + dst_reg + ", " + src_reg);
    } else if (op == "sqrt") {
        emitInstruction("FSQRT " + dst_reg + ", " + src_reg);
    } else {
        LOG_ERROR("Unsupported direct FPU operation: " + op);
    }
}

// Emit code to set precision control
void ArmAssembler::emitSetPrecisionControl(int precision) {
    LOG_DEBUG("Setting precision control to " + std::to_string(precision));
    
    // Generate code to set precision control bits in FPU control word
    emitInstruction("MOV w0, #" + std::to_string(precision & 3));
    emitInstruction("BL SIMDState::set_precision_control");
}

// Emit code to set rounding mode
void ArmAssembler::emitSetRoundingMode(int mode) {
    LOG_DEBUG("Setting rounding mode to " + std::to_string(mode));
    
    // Generate code to set rounding mode in FPU control word
    emitInstruction("MOV w0, #" + std::to_string(mode & 3));
    emitInstruction("BL SIMDState::set_rounding_mode");
    
    // Set ARM FPCR rounding bits based on x87 rounding mode
    emitInstruction("MRS x0, FPCR");
    emitInstruction("BIC x0, x0, #0xC00000");  // Clear rounding mode bits
    
    // Map x87 rounding mode to ARM format
    if (mode == 0) {
        // Round to nearest (even) - ARM default
    } else if (mode == 1) {
        // Round down (toward -∞)
        emitInstruction("ORR x0, x0, #0x800000");  // Set bits for round toward -∞
    } else if (mode == 2) {
        // Round up (toward +∞)
        emitInstruction("ORR x0, x0, #0x400000");  // Set bits for round toward +∞
    } else if (mode == 3) {
        // Round toward zero (truncate)
        emitInstruction("ORR x0, x0, #0xC00000");  // Set bits for round toward zero
    }
    
    emitInstruction("MSR FPCR, x0");  // Update ARM floating-point control register
}

// Emit code to apply precision control and rounding to a register value
void ArmAssembler::emitApplyPrecisionAndRounding(const std::string& reg) {
    LOG_DEBUG("Applying precision control and rounding to " + reg);
    
    // Get current precision control setting
    emitInstruction("BL SIMDState::get_fpu_control_word");
    emitInstruction("LSR w1, w0, #8");    // Shift to get PC bits
    emitInstruction("AND w1, w1, #3");    // Mask to get 2-bit PC field
    
    // Apply precision control
    emitInstruction("CMP w1, #0");
    emitInstruction("B.EQ pc_skip");      // Skip if PC=0 (extended precision)
    
    emitInstruction("CMP w1, #1");
    emitInstruction("B.NE pc_check_double");
    
    // Single precision (32-bit)
    emitInstruction("FCVT s16, " + reg);  // Convert to single
    emitInstruction("FCVT " + reg + ", s16");  // Convert back to double
    emitInstruction("B pc_done");
    
    // Double precision (64-bit)
    emitInstruction("pc_check_double:");
    // No action needed for double precision in ARM (already double)
    
    emitInstruction("pc_skip:");
    emitInstruction("pc_done:");
    
    // Apply rounding mode (already set in FPCR by set_rounding_mode)
    // Just need a dummy operation to enforce rounding
    emitInstruction("FADD " + reg + ", " + reg + ", #0.0");
}

} // namespace aarch64
} // namespace xenoarm_jit 