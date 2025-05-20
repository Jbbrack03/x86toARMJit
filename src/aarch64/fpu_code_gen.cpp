#include "xenoarm_jit/aarch64/fpu_code_gen.h"
#include "xenoarm_jit/aarch64/arm_assembler.h"
#include "logging/logger.h"
#include <cstring>

namespace xenoarm_jit {
namespace aarch64 {

void FPUCodeGenerator::generate_fpu_code(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating AArch64 code for FPU instruction: " + std::to_string(static_cast<int>(instr.type)));
    
    switch (instr.type) {
        case ir::IrInstructionType::FLD:
            generate_fld(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FSTP:
            generate_fstp(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FADD:
            generate_fadd(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FSUB:
            generate_fsub(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FMUL:
            generate_fmul(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FDIV:
            generate_fdiv(instr, assembler, simd_state);
            break;
            
        // Added in Phase 8: Transcendental FPU instructions
        case ir::IrInstructionType::FSIN:
            generate_fsin(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FCOS:
            generate_fcos(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FPTAN:
            generate_fptan(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::F2XM1:
            generate_f2xm1(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FYL2X:
            generate_fyl2x(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FSQRT:
            generate_fsqrt(instr, assembler, simd_state);
            break;
            
        // Added in Phase 8: FPU control word instructions
        case ir::IrInstructionType::FLDCW:
            generate_fldcw(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FNSTCW:
            generate_fnstcw(instr, assembler, simd_state);
            break;
            
        case ir::IrInstructionType::FNSTSW:
            generate_fnstsw(instr, assembler, simd_state);
            break;
            
        default:
            LOG_WARNING("Unsupported FPU instruction in code generator: " + std::to_string(static_cast<int>(instr.type)));
            break;
    }
}

void FPUCodeGenerator::generate_fld(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    if (instr.operands.empty()) {
        LOG_ERROR("FLD instruction without operands");
        return;
    }
    
    const ir::IrOperand& src_op = instr.operands[0];
    
    // Implementation strategy for FLD:
    // 1. For memory operands: Load from memory, convert to 80-bit x87 format, push onto FPU stack
    // 2. For register operands: Copy ST(i) to new stack position
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0
    assembler.reserveScratchRegister(1); // x1
    assembler.reserveScratchRegister(2); // x2
    
    if (src_op.type == ir::IrOperandType::MEMORY) {
        // Memory operand - load data from memory based on operand size
        uint32_t mem_addr = src_op.mem_info.displacement;
        
        // Load the memory address into register
        assembler.emitInstruction("MOV x0, #" + std::to_string(mem_addr));
        
        // Determine data size and appropriate load function
        if (src_op.data_type == ir::IrDataType::F32) {
            // 32-bit float (single precision)
            // Call memory read callback to get the value
            assembler.emitFunctionCall("read_guest_float32");
            
            // Result now in s0. Convert to 80-bit format (in our implementation, stored in memory)
            // We need to allocate stack space for the conversion
            assembler.emitInstruction("SUB sp, sp, #16");        // Allocate stack space
            assembler.emitInstruction("STR s0, [sp]");          // Store float32 at sp
            
            // Call the conversion helper (converts float32 to 80-bit and stores in a buffer)
            assembler.emitInstruction("MOV x0, sp");            // Source address
            assembler.emitInstruction("ADD x1, sp, #8");        // Destination address (80-bit temp buffer)
            assembler.emitFunctionCall("convert_f32_to_f80");   // Call conversion helper
            
            // Push the 80-bit float onto the FPU stack
            assembler.emitInstruction("MOV x0, x1");            // x0 = address of 80-bit float
            assembler.emitFunctionCall("SIMDState::fpu_push");  // Call the helper to push onto FPU stack
            
            // Clean up the stack
            assembler.emitInstruction("ADD sp, sp, #16");
            
            LOG_DEBUG("Generated FLD from memory (F32)");
        }
        else if (src_op.data_type == ir::IrDataType::F64) {
            // 64-bit float (double precision)
            // Similar to F32 but using double precision
            assembler.emitFunctionCall("read_guest_float64");
            
            // Result now in d0. Convert to 80-bit format
            assembler.emitInstruction("SUB sp, sp, #16");        // Allocate stack space
            assembler.emitInstruction("STR d0, [sp]");          // Store float64 at sp
            
            // Call the conversion helper
            assembler.emitInstruction("MOV x0, sp");            // Source address
            assembler.emitInstruction("ADD x1, sp, #8");        // Destination address
            assembler.emitFunctionCall("convert_f64_to_f80");   // Call conversion helper
            
            // Push the 80-bit float onto the FPU stack
            assembler.emitInstruction("MOV x0, x1");            // x0 = address of 80-bit float
            assembler.emitFunctionCall("SIMDState::fpu_push");  // Call the helper
            
            // Clean up the stack
            assembler.emitInstruction("ADD sp, sp, #16");
            
            LOG_DEBUG("Generated FLD from memory (F64)");
        }
        else {
            LOG_ERROR("Unsupported data type for FLD from memory");
        }
    }
    else if (src_op.type == ir::IrOperandType::IMMEDIATE && src_op.data_type == ir::IrDataType::F80) {
        // For FLD ST(i) - copy ST(i) to new stack position
        uint8_t st_idx = static_cast<uint8_t>(src_op.imm_value);
        
        if (st_idx >= 8) {
            LOG_ERROR("Invalid FPU register index: " + std::to_string(st_idx));
            return;
        }
        
        // 1. Allocate temp buffer on stack for the 80-bit value
        assembler.emitInstruction("SUB sp, sp, #16");           // Allocate stack space
        
        // 2. Call simd_state.read_fpu_reg to get ST(i) value
        assembler.emitInstruction("MOV x0, #" + std::to_string(st_idx)); // Source register index
        assembler.emitInstruction("MOV x1, sp");                // Destination buffer
        assembler.emitFunctionCall("SIMDState::read_fpu_reg");  // Read the register
        
        // 3. Call simd_state.fpu_push to push onto stack
        assembler.emitInstruction("MOV x0, sp");                // Source buffer with 80-bit float
        assembler.emitFunctionCall("SIMDState::fpu_push");      // Push onto FPU stack
        
        // 4. Clean up
        assembler.emitInstruction("ADD sp, sp, #16");           // Restore stack
        
        LOG_DEBUG("Generated FLD ST(" + std::to_string(st_idx) + ")");
    }
    else {
        LOG_ERROR("Unsupported operand type for FLD instruction");
    }
    
    // Release scratch registers
    assembler.releaseScratchRegister(2);
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fstp(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    if (instr.operands.empty()) {
        LOG_ERROR("FSTP instruction without operands");
        return;
    }
    
    const ir::IrOperand& dst_op = instr.operands[0];
    
    // Implementation strategy for FSTP:
    // 1. For memory operands: Pop from FPU stack to NEON register, then store to memory
    // 2. For register operands: Copy from top of stack to destination register, then pop
    
    if (dst_op.type == ir::IrOperandType::MEMORY) {
        // Memory operand - store data to memory
        
        // Example ARM assembly (pseudo-code):
        // BL fpu_pop_s0           ; Call helper to pop top of FPU stack to s0
        // FMOV w0, s0             ; Move NEON register to general register
        // LDR x1, [mem_addr]      ; Load memory address into x1
        // BL guest_mem_write_f32  ; Call host function to write to guest memory
        
        LOG_DEBUG("Generated FSTP to memory");
        
        // In a real implementation, you'd generate the appropriate ARM instructions
        // based on the data type (F32, F64, F80) and memory operand details
        
        // Placeholder for ARM code generation
        // assembler.generateMemoryStore(...);
    }
    else {
        LOG_ERROR("Unsupported operand type for FSTP instruction");
    }
}

void FPUCodeGenerator::generate_fadd(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    if (instr.operands.empty()) {
        LOG_ERROR("FADD instruction without operands");
        return;
    }
    
    const ir::IrOperand& src_op = instr.operands[0];
    
    // Implementation strategy for FADD:
    // 1. Load ST(0) to ARM floating-point register
    // 2. Load source operand to another ARM floating-point register
    // 3. Perform FADD using ARM's native floating-point instructions
    // 4. Convert result back to 80-bit and update ST(0)
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0
    assembler.reserveScratchRegister(1); // x1
    assembler.reserveScratchRegister(2); // x2
    
    // Allocate stack space for temporary values
    assembler.emitInstruction("SUB sp, sp, #32");
    
    // Load ST(0) to temporary storage
    assembler.emitInstruction("MOV x0, #0");           // ST(0) has logical index 0
    assembler.emitInstruction("MOV x1, sp");           // First temp buffer at sp
    assembler.emitFunctionCall("SIMDState::read_fpu_reg"); // Get ST(0)
    
    // ARM Optimization: Convert 80-bit ST(0) to double precision directly into NEON register
    assembler.emitInstruction("MOV x0, sp");           // Source: 80-bit float
    assembler.emitInstruction("MOV x1, sp");           // Use direct register loading instead of memory
    assembler.emitFunctionCall("convert_f80_to_d0");   // New optimized helper that loads directly to d0
    
    if (src_op.type == ir::IrOperandType::MEMORY) {
        // Memory operand - load data from memory and add to ST(0)
        uint32_t mem_addr = src_op.mem_info.displacement;
        
        // Load the memory address into register
        assembler.emitInstruction("MOV x0, #" + std::to_string(mem_addr));
        
        if (src_op.data_type == ir::IrDataType::F32) {
            // ARM Optimization: Load 32-bit float directly to NEON register
            assembler.emitFunctionCall("read_guest_float32_to_s1"); // New optimized helper that loads directly to s1
            
            // Use native ARM FCVT instruction for conversion
            assembler.emitInstruction("FCVT d1, s1");
            
            // Add the values using native ARM FADD
            assembler.emitInstruction("FADD d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FADD from F32 memory");
        }
        else if (src_op.data_type == ir::IrDataType::F64) {
            // ARM Optimization: Load 64-bit double directly to NEON register
            assembler.emitFunctionCall("read_guest_float64_to_d1"); // New optimized helper that loads directly to d1
            
            // Add the values using native ARM FADD
            assembler.emitInstruction("FADD d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FADD from F64 memory");
        }
        else {
            LOG_ERROR("Unsupported data type for FADD memory");
        }
    }
    else if (src_op.type == ir::IrOperandType::IMMEDIATE && src_op.data_type == ir::IrDataType::F80) {
        // Register operand - add ST(i) to ST(0)
        uint8_t st_idx = static_cast<uint8_t>(src_op.imm_value);
        
        if (st_idx >= 8) {
            LOG_ERROR("Invalid FPU register index: " + std::to_string(st_idx));
            // Clean up and return
            assembler.emitInstruction("ADD sp, sp, #32");
            assembler.releaseScratchRegister(2);
            assembler.releaseScratchRegister(1);
            assembler.releaseScratchRegister(0);
            return;
        }
        
        // ARM Optimization: Load ST(i) directly to NEON register
        assembler.emitInstruction("MOV x0, #" + std::to_string(st_idx));
        assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d1"); // New optimized helper that loads directly to d1
        
        // Add the values using native ARM FADD
        assembler.emitInstruction("FADD d0, d0, d1");
        
        LOG_DEBUG("Generated optimized FADD ST, ST(" + std::to_string(st_idx) + ")");
    }
    else {
        LOG_ERROR("Unsupported operand type for FADD instruction");
        // Clean up and return
        assembler.emitInstruction("ADD sp, sp, #32");
        assembler.releaseScratchRegister(2);
        assembler.releaseScratchRegister(1);
        assembler.releaseScratchRegister(0);
        return;
    }
    
    // Apply FPU precision control and rounding mode
    apply_precision_control(assembler, 0, 0);
    
    // Convert result in d0 back to 80-bit format and update ST(0)
    assembler.emitInstruction("MOV x0, #0");                // ST(0) destination index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0"); // New optimized helper
    
    // Clean up stack
    assembler.emitInstruction("ADD sp, sp, #32");
    
    // After the operation, check for FPU exceptions
    generate_fpu_exception_check(assembler);
    
    // Release scratch registers
    assembler.releaseScratchRegister(2);
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fsub(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    if (instr.operands.empty()) {
        LOG_ERROR("FSUB instruction without operands");
        return;
    }
    
    const ir::IrOperand& src_op = instr.operands[0];
    
    // Implementation strategy for FSUB:
    // 1. Load ST(0) to ARM floating-point register
    // 2. Load source operand to another ARM floating-point register
    // 3. Perform FSUB using ARM's native floating-point instructions
    // 4. Convert result back to 80-bit and update ST(0)
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0
    assembler.reserveScratchRegister(1); // x1
    
    // Allocate stack space for temporary values
    assembler.emitInstruction("SUB sp, sp, #32");
    
    // Load ST(0) to d0 NEON register
    assembler.emitInstruction("MOV x0, #0");           // ST(0) has logical index 0
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d0"); // Optimized helper
    
    if (src_op.type == ir::IrOperandType::MEMORY) {
        // Memory operand - load data from memory and subtract from ST(0)
        uint32_t mem_addr = src_op.mem_info.displacement;
        
        // Load the memory address into register
        assembler.emitInstruction("MOV x0, #" + std::to_string(mem_addr));
        
        if (src_op.data_type == ir::IrDataType::F32) {
            // Load 32-bit float directly to NEON register
            assembler.emitFunctionCall("read_guest_float32_to_s1");
            
            // Convert to double using ARM FCVT
            assembler.emitInstruction("FCVT d1, s1");
            
            // Subtract using native ARM FSUB
            assembler.emitInstruction("FSUB d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FSUB from F32 memory");
        }
        else if (src_op.data_type == ir::IrDataType::F64) {
            // Load 64-bit double directly to NEON register
            assembler.emitFunctionCall("read_guest_float64_to_d1");
            
            // Subtract using native ARM FSUB
            assembler.emitInstruction("FSUB d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FSUB from F64 memory");
        }
        else {
            LOG_ERROR("Unsupported data type for FSUB memory");
        }
    }
    else if (src_op.type == ir::IrOperandType::IMMEDIATE && src_op.data_type == ir::IrDataType::F80) {
        // Register operand - subtract ST(i) from ST(0)
        uint8_t st_idx = static_cast<uint8_t>(src_op.imm_value);
        
        if (st_idx >= 8) {
            LOG_ERROR("Invalid FPU register index: " + std::to_string(st_idx));
            assembler.emitInstruction("ADD sp, sp, #32");
            assembler.releaseScratchRegister(1);
            assembler.releaseScratchRegister(0);
            return;
        }
        
        // Load ST(i) directly to NEON register
        assembler.emitInstruction("MOV x0, #" + std::to_string(st_idx));
        assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d1");
        
        // Subtract using native ARM FSUB
        assembler.emitInstruction("FSUB d0, d0, d1");
        
        LOG_DEBUG("Generated optimized FSUB ST, ST(" + std::to_string(st_idx) + ")");
    }
    else {
        LOG_ERROR("Unsupported operand type for FSUB instruction");
        assembler.emitInstruction("ADD sp, sp, #32");
        assembler.releaseScratchRegister(1);
        assembler.releaseScratchRegister(0);
        return;
    }
    
    // Apply FPU precision control and rounding mode
    apply_precision_control(assembler, 0, 0);
    
    // Update ST(0) with result in d0
    assembler.emitInstruction("MOV x0, #0");                // ST(0) destination index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    
    // Clean up stack
    assembler.emitInstruction("ADD sp, sp, #32");
    
    // Check for FPU exceptions
    generate_fpu_exception_check(assembler);
    
    // Release scratch registers
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fmul(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    if (instr.operands.empty()) {
        LOG_ERROR("FMUL instruction without operands");
        return;
    }
    
    const ir::IrOperand& src_op = instr.operands[0];
    
    // Implementation strategy for FMUL:
    // 1. Load ST(0) to ARM floating-point register
    // 2. Load source operand to another ARM floating-point register
    // 3. Perform FMUL using ARM's native floating-point instructions
    // 4. Convert result back to 80-bit and update ST(0)
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0
    assembler.reserveScratchRegister(1); // x1
    
    // Allocate stack space for temporary values
    assembler.emitInstruction("SUB sp, sp, #32");
    
    // Load ST(0) to d0 NEON register
    assembler.emitInstruction("MOV x0, #0");           // ST(0) has logical index 0
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d0"); // Optimized helper
    
    if (src_op.type == ir::IrOperandType::MEMORY) {
        // Memory operand - load data from memory and multiply with ST(0)
        uint32_t mem_addr = src_op.mem_info.displacement;
        
        // Load the memory address into register
        assembler.emitInstruction("MOV x0, #" + std::to_string(mem_addr));
        
        if (src_op.data_type == ir::IrDataType::F32) {
            // Load 32-bit float directly to NEON register
            assembler.emitFunctionCall("read_guest_float32_to_s1");
            
            // Convert to double using ARM FCVT
            assembler.emitInstruction("FCVT d1, s1");
            
            // Multiply using native ARM FMUL
            assembler.emitInstruction("FMUL d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FMUL from F32 memory");
        }
        else if (src_op.data_type == ir::IrDataType::F64) {
            // Load 64-bit double directly to NEON register
            assembler.emitFunctionCall("read_guest_float64_to_d1");
            
            // Multiply using native ARM FMUL
            assembler.emitInstruction("FMUL d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FMUL from F64 memory");
        }
        else {
            LOG_ERROR("Unsupported data type for FMUL memory");
        }
    }
    else if (src_op.type == ir::IrOperandType::IMMEDIATE && src_op.data_type == ir::IrDataType::F80) {
        // Register operand - multiply ST(0) by ST(i)
        uint8_t st_idx = static_cast<uint8_t>(src_op.imm_value);
        
        if (st_idx >= 8) {
            LOG_ERROR("Invalid FPU register index: " + std::to_string(st_idx));
            assembler.emitInstruction("ADD sp, sp, #32");
            assembler.releaseScratchRegister(1);
            assembler.releaseScratchRegister(0);
            return;
        }
        
        // Load ST(i) directly to NEON register
        assembler.emitInstruction("MOV x0, #" + std::to_string(st_idx));
        assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d1");
        
        // Multiply using native ARM FMUL
        assembler.emitInstruction("FMUL d0, d0, d1");
        
        LOG_DEBUG("Generated optimized FMUL ST, ST(" + std::to_string(st_idx) + ")");
    }
    else {
        LOG_ERROR("Unsupported operand type for FMUL instruction");
        assembler.emitInstruction("ADD sp, sp, #32");
        assembler.releaseScratchRegister(1);
        assembler.releaseScratchRegister(0);
        return;
    }
    
    // Apply FPU precision control and rounding mode
    apply_precision_control(assembler, 0, 0);
    
    // Update ST(0) with result in d0
    assembler.emitInstruction("MOV x0, #0");                // ST(0) destination index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    
    // Clean up stack
    assembler.emitInstruction("ADD sp, sp, #32");
    
    // Check for FPU exceptions
    generate_fpu_exception_check(assembler);
    
    // Release scratch registers
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fdiv(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    if (instr.operands.empty()) {
        LOG_ERROR("FDIV instruction without operands");
        return;
    }
    
    const ir::IrOperand& src_op = instr.operands[0];
    
    // Implementation strategy for FDIV:
    // 1. Load ST(0) to ARM floating-point register
    // 2. Load source operand to another ARM floating-point register
    // 3. Check for division by zero
    // 4. Perform FDIV using ARM's native floating-point instructions
    // 5. Convert result back to 80-bit and update ST(0)
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0
    assembler.reserveScratchRegister(1); // x1
    
    // Allocate stack space for temporary values
    assembler.emitInstruction("SUB sp, sp, #32");
    
    // Load ST(0) to d0 NEON register
    assembler.emitInstruction("MOV x0, #0");           // ST(0) has logical index 0
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d0"); // Optimized helper
    
    if (src_op.type == ir::IrOperandType::MEMORY) {
        // Memory operand - load data from memory and divide ST(0) by it
        uint32_t mem_addr = src_op.mem_info.displacement;
        
        // Load the memory address into register
        assembler.emitInstruction("MOV x0, #" + std::to_string(mem_addr));
        
        if (src_op.data_type == ir::IrDataType::F32) {
            // Load 32-bit float directly to NEON register
            assembler.emitFunctionCall("read_guest_float32_to_s1");
            
            // Convert to double using ARM FCVT
            assembler.emitInstruction("FCVT d1, s1");
            
            // Check for division by zero
            assembler.emitInstruction("FMOV x0, d1");
            assembler.emitInstruction("AND x0, x0, #0x7FFFFFFFFFFFFFFF"); // Clear sign bit
            assembler.emitInstruction("CBZ x0, division_by_zero");       // Branch if zero
            
            // Divide using native ARM FDIV
            assembler.emitInstruction("FDIV d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FDIV from F32 memory");
        }
        else if (src_op.data_type == ir::IrDataType::F64) {
            // Load 64-bit double directly to NEON register
            assembler.emitFunctionCall("read_guest_float64_to_d1");
            
            // Check for division by zero
            assembler.emitInstruction("FMOV x0, d1");
            assembler.emitInstruction("AND x0, x0, #0x7FFFFFFFFFFFFFFF"); // Clear sign bit
            assembler.emitInstruction("CBZ x0, division_by_zero");       // Branch if zero
            
            // Divide using native ARM FDIV
            assembler.emitInstruction("FDIV d0, d0, d1");
            
            LOG_DEBUG("Generated optimized FDIV from F64 memory");
        }
        else {
            LOG_ERROR("Unsupported data type for FDIV memory");
        }
    }
    else if (src_op.type == ir::IrOperandType::IMMEDIATE && src_op.data_type == ir::IrDataType::F80) {
        // Register operand - divide ST(0) by ST(i)
        uint8_t st_idx = static_cast<uint8_t>(src_op.imm_value);
        
        if (st_idx >= 8) {
            LOG_ERROR("Invalid FPU register index: " + std::to_string(st_idx));
            assembler.emitInstruction("ADD sp, sp, #32");
            assembler.releaseScratchRegister(1);
            assembler.releaseScratchRegister(0);
            return;
        }
        
        // Load ST(i) directly to NEON register
        assembler.emitInstruction("MOV x0, #" + std::to_string(st_idx));
        assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d1");
        
        // Check for division by zero
        assembler.emitInstruction("FMOV x0, d1");
        assembler.emitInstruction("AND x0, x0, #0x7FFFFFFFFFFFFFFF"); // Clear sign bit
        assembler.emitInstruction("CBZ x0, division_by_zero");       // Branch if zero
        
        // Divide using native ARM FDIV
        assembler.emitInstruction("FDIV d0, d0, d1");
        
        LOG_DEBUG("Generated optimized FDIV ST, ST(" + std::to_string(st_idx) + ")");
    }
    else {
        LOG_ERROR("Unsupported operand type for FDIV instruction");
        assembler.emitInstruction("ADD sp, sp, #32");
        assembler.releaseScratchRegister(1);
        assembler.releaseScratchRegister(0);
        return;
    }
    
    // Skip division by zero handler
    assembler.emitInstruction("B division_done");
    
    // Division by zero handler
    assembler.emitLabel("division_by_zero");
    // Set the divide by zero flag in the FPU status word
    assembler.emitInstruction("MOV x0, #4"); // Divide by zero exception
    assembler.emitFunctionCall("handle_fpu_exception");
    
    // Load infinity with the same sign as the dividend
    assembler.emitInstruction("FMOV x0, d0");                  // Get dividend sign bit
    assembler.emitInstruction("AND x0, x0, #0x8000000000000000"); // Extract sign bit
    assembler.emitInstruction("MOV x1, #0x7FF0000000000000");    // Infinity exponent and mantissa
    assembler.emitInstruction("ORR x0, x0, x1");               // Combine to get signed infinity
    assembler.emitInstruction("FMOV d0, x0");                  // Load infinity to d0
    
    // Continue with result handling
    assembler.emitLabel("division_done");
    
    // Apply FPU precision control and rounding mode
    apply_precision_control(assembler, 0, 0);
    
    // Update ST(0) with result in d0
    assembler.emitInstruction("MOV x0, #0");                // ST(0) destination index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    
    // Clean up stack
    assembler.emitInstruction("ADD sp, sp, #32");
    
    // Check for FPU exceptions
    generate_fpu_exception_check(assembler);
    
    // Release scratch registers
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fsin(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for FSIN instruction");
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for temp
    assembler.reserveScratchRegister(1); // x1 for temp
    
    // 1. Allocate stack space for temp buffer (80-bit float)
    assembler.emitInstruction("SUB sp, sp, #16");
    
    // 2. Pop the top value from FPU stack into the temp buffer
    assembler.emitInstruction("MOV x0, sp");            // Destination buffer
    assembler.emitFunctionCall("SIMDState::fpu_pop");   // Pop top value to buffer
    
    // 3. Check if the input is within valid range for FSIN
    // In x87, the valid range is approximately [-2^63, 2^63]
    // Values outside this range may have reduced precision
    assembler.emitInstruction("LDR d0, [sp]");          // Load as double for range check
    assembler.emitInstruction("FCMP d0, d0");           // Check for NaN
    assembler.emitInstruction("B.VS sine_invalid");     // Branch if NaN
    
    // For very large values, generate a partial tangent reduction
    // This is a simplified version - for Phase 8, we limit precision guarantees
    // to reasonable input ranges
    assembler.emitInstruction("FABS d1, d0");           // |value|
    assembler.emitInstruction("LDR d2, =549755813888"); // 2^39 (large threshold)
    assembler.emitInstruction("FCMP d1, d2");
    assembler.emitInstruction("B.GT sine_large_value"); // Handle large value reduction
    
    // 4. Call runtime helper for sine computation
    assembler.emitInstruction("MOV x0, sp");                // Input buffer
    assembler.emitInstruction("ADD x1, sp, #8");           // Output buffer
    assembler.emitFunctionCall("compute_sine_f80");        // Helper function
    
    // 5. Push the result back onto the FPU stack
    assembler.emitInstruction("MOV x0, x1");               // Source buffer with result
    assembler.emitFunctionCall("SIMDState::fpu_push");     // Push result onto FPU stack
    
    // 6. Skip error handling paths and clean up
    assembler.emitInstruction("B sine_done");
    
    // Handle invalid input (NaN)
    assembler.emitLabel("sine_invalid");
    assembler.emitInstruction("MOV x0, #1");               // Invalid operation exception
    assembler.emitFunctionCall("handle_fpu_exception");    // Set appropriate FPU exception flags
    
    // Load a QNaN onto the stack
    assembler.emitInstruction("MOV x0, sp");               // Destination
    assembler.emitFunctionCall("load_fpu_qnan");           // Load QNaN value
    assembler.emitFunctionCall("SIMDState::fpu_push");     // Push QNaN onto FPU stack
    assembler.emitInstruction("B sine_done");
    
    // Handle very large values (partial reduction)
    assembler.emitLabel("sine_large_value");
    // For Phase 8, we implement a partial range reduction for large values
    assembler.emitInstruction("MOV x0, sp");               // Input buffer
    assembler.emitInstruction("ADD x1, sp, #8");           // Output buffer
    assembler.emitFunctionCall("compute_sine_large_f80");  // Specialized helper for large values
    
    // Push the result back onto the FPU stack
    assembler.emitInstruction("MOV x0, x1");               // Source buffer with result
    assembler.emitFunctionCall("SIMDState::fpu_push");     // Push result onto FPU stack
    
    // Cleanup
    assembler.emitLabel("sine_done");
    assembler.emitInstruction("ADD sp, sp, #16");          // Free stack space
    
    // Release scratch registers
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fcos(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for FCOS instruction");
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for temp
    assembler.reserveScratchRegister(1); // x1 for temp
    
    // 1. Allocate stack space for temp buffer (80-bit float)
    assembler.emitInstruction("SUB sp, sp, #16");
    
    // 2. Pop the top value from FPU stack into the temp buffer
    assembler.emitInstruction("MOV x0, sp");            // Destination buffer
    assembler.emitFunctionCall("SIMDState::fpu_pop");   // Pop top value to buffer
    
    // 3. Check if the input is within valid range for FCOS
    assembler.emitInstruction("LDR d0, [sp]");          // Load as double for range check
    assembler.emitInstruction("FCMP d0, d0");           // Check for NaN
    assembler.emitInstruction("B.VS cosine_invalid");   // Branch if NaN
    
    // For very large values, generate a partial tangent reduction
    assembler.emitInstruction("FABS d1, d0");           // |value|
    assembler.emitInstruction("LDR d2, =549755813888"); // 2^39 (large threshold)
    assembler.emitInstruction("FCMP d1, d2");
    assembler.emitInstruction("B.GT cosine_large_value"); // Handle large value reduction
    
    // 4. Call runtime helper for cosine computation
    assembler.emitInstruction("MOV x0, sp");                // Input buffer
    assembler.emitInstruction("ADD x1, sp, #8");           // Output buffer
    assembler.emitFunctionCall("compute_cosine_f80");      // Helper function
    
    // 5. Push the result back onto the FPU stack
    assembler.emitInstruction("MOV x0, x1");               // Source buffer with result
    assembler.emitFunctionCall("SIMDState::fpu_push");     // Push result onto FPU stack
    
    // 6. Skip error handling paths and clean up
    assembler.emitInstruction("B cosine_done");
    
    // Handle invalid input (NaN)
    assembler.emitLabel("cosine_invalid");
    assembler.emitInstruction("MOV x0, #1");               // Invalid operation exception
    assembler.emitFunctionCall("handle_fpu_exception");    // Set appropriate FPU exception flags
    
    // Load a QNaN onto the stack
    assembler.emitInstruction("MOV x0, sp");               // Destination
    assembler.emitFunctionCall("load_fpu_qnan");           // Load QNaN value
    assembler.emitFunctionCall("SIMDState::fpu_push");     // Push QNaN onto FPU stack
    assembler.emitInstruction("B cosine_done");
    
    // Handle very large values (partial reduction)
    assembler.emitLabel("cosine_large_value");
    // For Phase 8, we implement a partial range reduction for large values
    assembler.emitInstruction("MOV x0, sp");               // Input buffer
    assembler.emitInstruction("ADD x1, sp, #8");           // Output buffer
    assembler.emitFunctionCall("compute_cosine_large_f80");  // Specialized helper for large values
    
    // Push the result back onto the FPU stack
    assembler.emitInstruction("MOV x0, x1");               // Source buffer with result
    assembler.emitFunctionCall("SIMDState::fpu_push");     // Push result onto FPU stack
    
    // Cleanup
    assembler.emitLabel("cosine_done");
    assembler.emitInstruction("ADD sp, sp, #16");          // Free stack space
    
    // Release scratch registers
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fptan(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for FPTAN instruction");
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for temp
    assembler.reserveScratchRegister(1); // x1 for temp
    assembler.reserveScratchRegister(2); // x2 for status flags
    
    // 1. First, check for stack overflow - we need 2 free slots (one for result, one for 1.0)
    // Get FPU TOP value
    assembler.emitFunctionCall("SIMDState::get_fpu_top");
    
    // Calculate new TOP value (TOP - 1) & 7 for first push
    assembler.emitInstruction("SUB w1, w0, #1");
    assembler.emitInstruction("AND w1, w1, #7");
    
    // Check if the stack has room for first push - check tag of (TOP - 1)
    assembler.emitInstruction("MOV x0, x1");  // Put new TOP in x0 for function parameter
    assembler.emitFunctionCall("SIMDState::get_tag_for_register");
    
    // Compare with EMPTY (0)
    assembler.emitInstruction("CMP w0, #3");  // EMPTY is 3, not 0
    assembler.emitInstruction("B.NE fptan_stack_overflow_check_second");
    
    // Calculate new TOP value (TOP - 2) & 7 for second push
    assembler.emitFunctionCall("SIMDState::get_fpu_top");
    assembler.emitInstruction("SUB w1, w0, #2");
    assembler.emitInstruction("AND w1, w1, #7");
    
    // Check if the stack has room for second push - check tag of (TOP - 2)
    assembler.emitInstruction("MOV x0, x1");  // Put new TOP-2 in x0 for function parameter
    assembler.emitFunctionCall("SIMDState::get_tag_for_register");
    
    // Compare with EMPTY (0)
    assembler.emitInstruction("CMP w0, #3");  // EMPTY is 3, not 0
    assembler.emitInstruction("B.NE fptan_stack_overflow");
    assembler.emitInstruction("B fptan_stack_check_done");
    
    // If first check failed, still need to check second position
    assembler.emitLabel("fptan_stack_overflow_check_second");
    assembler.emitFunctionCall("SIMDState::get_fpu_top");
    assembler.emitInstruction("SUB w1, w0, #2");
    assembler.emitInstruction("AND w1, w1, #7");
    
    // Check if the stack has room for second push - check tag of (TOP - 2)
    assembler.emitInstruction("MOV x0, x1");  // Put new TOP-2 in x0 for function parameter
    assembler.emitFunctionCall("SIMDState::get_tag_for_register");
    
    // Compare with EMPTY (0)
    assembler.emitInstruction("CMP w0, #3");  // EMPTY is 3, not 0
    assembler.emitInstruction("B.NE fptan_stack_overflow");
    
    assembler.emitLabel("fptan_stack_check_done");
    
    // 2. Check if the current TOP register is empty (stack underflow)
    assembler.emitFunctionCall("SIMDState::get_fpu_top");
    assembler.emitFunctionCall("SIMDState::get_tag_for_register");
    
    // Compare with EMPTY (3)
    assembler.emitInstruction("CMP w0, #3");  // EMPTY is 3
    assembler.emitInstruction("B.EQ fptan_stack_underflow");
    
    // 3. Allocate stack space for operation
    assembler.emitInstruction("SUB sp, sp, #32"); // Space for 80-bit value and status flags
    
    // 4. Load the value from ST(0) directly into ARM register for efficient handling
    assembler.emitInstruction("MOV x0, #0"); // ST(0)
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d0"); // Read ST(0) directly to d0
    
    // 5. Check for special cases - NaN, Infinity
    assembler.emitInstruction("FMOV d1, d0");
    assembler.emitInstruction("FCMP d0, d0");
    assembler.emitInstruction("B.VS fptan_invalid_value"); // Branch if unordered (NaN)
    
    assembler.emitInstruction("FABS d1, d0");
    assembler.emitInstruction("FMOV d2, #inf");
    assembler.emitInstruction("FCMP d1, d2");
    assembler.emitInstruction("B.EQ fptan_invalid_value"); // Branch if infinite
    
    // 6. Check for very large values (outside FPTAN valid range)
    assembler.emitInstruction("LDR d2, =1.0e10"); // Load constant for comparison
    assembler.emitInstruction("FCMP d1, d2");
    assembler.emitInstruction("B.GT fptan_out_of_range"); // Value too large for FPTAN
    
    // 7. Set up parameter buffers for compute_tangent_f80_with_status
    // Convert d0 to 80-bit format and store in stack buffer
    assembler.emitInstruction("MOV x0, sp"); // Destination buffer
    assembler.emitFunctionCall("convert_d0_to_f80"); // Convert d0 to 80-bit format
    
    // Set up other parameters
    assembler.emitInstruction("ADD x1, sp, #10"); // Result buffer
    assembler.emitInstruction("ADD x2, sp, #20"); // Status flags
    assembler.emitInstruction("STR xzr, [x2]");   // Initialize status flags to 0
    
    // 8. Call compute_tangent_f80_with_status
    assembler.emitFunctionCall("compute_tangent_f80_with_status");
    
    // 9. Check the return value and C2 flag
    assembler.emitInstruction("CMP w0, #0");
    assembler.emitInstruction("B.EQ fptan_incomplete"); // If function returned false
    
    assembler.emitInstruction("LDR w0, [sp, #20]"); // Load status flags
    assembler.emitInstruction("AND w1, w0, #4");    // Check C2 flag
    assembler.emitInstruction("CMP w1, #4");
    assembler.emitInstruction("B.EQ fptan_incomplete"); // C2 flag set means operation incomplete
    
    // 10. Operation completed successfully, update FPU stack
    // Pop ST(0) to remove the original value
    assembler.emitFunctionCall("SIMDState::pop_without_result");
    
    // Load tangent result into d0
    assembler.emitInstruction("ADD x0, sp, #10"); // Result buffer
    assembler.emitFunctionCall("convert_f80_to_d0"); // Convert 80-bit result to d0
    
    // Push tan(x) result
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    
    // Push 1.0 onto the FPU stack
    assembler.emitInstruction("FMOV d0, #1.0");
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    
    // Update status word flags from our computed status
    assembler.emitInstruction("LDR w0, [sp, #20]"); // Load status flags
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // Skip error handling
    assembler.emitInstruction("B fptan_done");
    
    // --- Error handling paths ---
    
    // Handle stack overflow
    assembler.emitLabel("fptan_stack_overflow");
    assembler.emitInstruction("MOV w0, #0x0002"); // C1 flag for stack overflow
    assembler.emitInstruction("ORR w0, w0, #0x0001"); // Invalid operation flag
    assembler.emitInstruction("ORR w0, w0, #0x0080"); // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    assembler.emitInstruction("B fptan_done");
    
    // Handle stack underflow
    assembler.emitLabel("fptan_stack_underflow");
    assembler.emitInstruction("MOV w0, #0x0040"); // C3 flag for stack underflow
    assembler.emitInstruction("ORR w0, w0, #0x0001"); // Invalid operation flag
    assembler.emitInstruction("ORR w0, w0, #0x0080"); // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    assembler.emitInstruction("B fptan_done");
    
    // Handle invalid value (NaN or Infinity)
    assembler.emitLabel("fptan_invalid_value");
    // Pop the invalid value
    assembler.emitFunctionCall("SIMDState::pop_without_result");
    
    // Load QNaN into d0
    assembler.emitInstruction("FMOV d0, #NaN");
    
    // Push QNaN result
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    
    // Push 1.0
    assembler.emitInstruction("FMOV d0, #1.0");
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    
    // Set invalid operation flag
    assembler.emitInstruction("MOV w0, #1"); // Invalid operation flag
    assembler.emitInstruction("ORR w0, w0, #0x0002"); // C1 flag for invalid operation
    assembler.emitInstruction("ORR w0, w0, #0x0080"); // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    assembler.emitInstruction("B fptan_done");
    
    // Handle range value in range but operation incomplete (C2 flag)
    assembler.emitLabel("fptan_incomplete");
    // Set C2 flag (result is not complete)
    assembler.emitInstruction("MOV w0, #4"); // C2 flag
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    assembler.emitInstruction("B fptan_done");
    
    // Handle out of range value (too large for FPTAN)
    assembler.emitLabel("fptan_out_of_range");
    // Set C2 flag to indicate partial result
    assembler.emitInstruction("MOV w0, #4"); // C2 flag
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    assembler.emitInstruction("B fptan_done");
    
    // Cleanup
    assembler.emitLabel("fptan_done");
    assembler.emitInstruction("ADD sp, sp, #32");  // Free stack space
    
    // Release scratch registers
    assembler.releaseScratchRegister(2);
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_f2xm1(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for F2XM1 instruction");
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for temp
    assembler.reserveScratchRegister(1); // x1 for temp
    assembler.reserveScratchRegister(2); // x2 for status flags
    
    // 1. First check for stack underflow - verify ST(0) is not empty
    assembler.emitFunctionCall("SIMDState::get_fpu_top");
    assembler.emitFunctionCall("SIMDState::get_tag_for_register");
    
    // Compare with EMPTY (3)
    assembler.emitInstruction("CMP w0, #3");  // EMPTY is 3
    assembler.emitInstruction("B.EQ f2xm1_stack_underflow");
    
    // 2. Allocate stack space for temp buffer and status flags
    assembler.emitInstruction("SUB sp, sp, #32");
    
    // 3. Load the value from ST(0) directly to the ARM register
    assembler.emitInstruction("MOV x0, #0");  // ST(0) index
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d0");  // Read ST(0) to d0
    
    // 4. Check if the input is valid for F2XM1 (in range [-1, 1])
    assembler.emitInstruction("FMOV d1, #1.0");   // Load 1.0
    assembler.emitInstruction("FNEG d2, d1");     // Load -1.0
    
    // Check for NaN
    assembler.emitInstruction("FCMP d0, d0");     // Compare with itself
    assembler.emitInstruction("B.VS f2xm1_invalid_input");  // Branch if unordered (NaN)
    
    // Check if value < -1.0
    assembler.emitInstruction("FCMP d0, d2");     // Compare with -1.0
    assembler.emitInstruction("B.LT f2xm1_out_of_range_low");  // Branch if < -1.0
    
    // Check if value > 1.0
    assembler.emitInstruction("FCMP d0, d1");     // Compare with 1.0
    assembler.emitInstruction("B.GT f2xm1_out_of_range_high"); // Branch if > 1.0
    
    // 5. Convert d0 to 80-bit format and store in stack buffer
    assembler.emitInstruction("MOV x0, sp");      // Destination buffer
    assembler.emitFunctionCall("convert_d0_to_f80");  // Convert to 80-bit
    
    // 6. Call helper function to compute 2^x-1
    assembler.emitInstruction("MOV x0, sp");      // Source buffer
    assembler.emitInstruction("ADD x1, sp, #10"); // Result buffer
    assembler.emitFunctionCall("compute_2_to_x_minus_1_f80");
    
    // 7. Load result back to d0
    assembler.emitInstruction("ADD x0, sp, #10"); // Result buffer
    assembler.emitFunctionCall("convert_f80_to_d0");  // Load to d0
    
    // 8. Update ST(0) with the result
    assembler.emitInstruction("MOV x0, #0");      // ST(0) index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    
    // 9. Clear any condition code flags (C0, C1, C2, C3)
    assembler.emitInstruction("MOV x0, #0");      // Clear all CC flags
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // 10. Skip error handling
    assembler.emitInstruction("B f2xm1_done");
    
    // --- Error handling paths ---
    
    // Handle stack underflow
    assembler.emitLabel("f2xm1_stack_underflow");
    assembler.emitInstruction("MOV w0, #0x0041"); // Invalid operation + C3 flag (stack underflow)
    assembler.emitInstruction("ORR w0, w0, #0x0080"); // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    assembler.emitInstruction("B f2xm1_done_no_stack");
    
    // Handle invalid input (NaN)
    assembler.emitLabel("f2xm1_invalid_input");
    assembler.emitInstruction("MOV w0, #0x0001"); // Invalid operation flag
    assembler.emitInstruction("ORR w0, w0, #0x0002"); // C1 flag
    assembler.emitInstruction("ORR w0, w0, #0x0080"); // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // Return NaN as result
    assembler.emitInstruction("FMOV d0, #NaN");
    assembler.emitInstruction("MOV x0, #0");      // ST(0) index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    assembler.emitInstruction("B f2xm1_done");
    
    // Handle out of range low (x < -1.0)
    assembler.emitLabel("f2xm1_out_of_range_low");
    assembler.emitInstruction("MOV w0, #0x0002"); // C1 flag to indicate clipping
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // Return -1.0 (saturated value for x < -1.0)
    assembler.emitInstruction("FMOV d0, #-1.0");
    assembler.emitInstruction("MOV x0, #0");      // ST(0) index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    assembler.emitInstruction("B f2xm1_done");
    
    // Handle out of range high (x > 1.0)
    assembler.emitLabel("f2xm1_out_of_range_high");
    assembler.emitInstruction("MOV w0, #0x0002"); // C1 flag to indicate clipping
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // Return 1.0 (saturated value for x > 1.0)
    assembler.emitInstruction("FMOV d0, #1.0");
    assembler.emitInstruction("MOV x0, #0");      // ST(0) index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    
    // Cleanup
    assembler.emitLabel("f2xm1_done");
    assembler.emitInstruction("ADD sp, sp, #32"); // Free stack space
    
    assembler.emitLabel("f2xm1_done_no_stack");
    
    // Release scratch registers
    assembler.releaseScratchRegister(2);
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fsqrt(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    // Implementation strategy for FSQRT:
    // 1. Load ST(0) to ARM floating-point register
    // 2. Check for negative input (which would result in an invalid operation)
    // 3. Perform FSQRT using ARM's native floating-point instructions
    // 4. Convert result back to 80-bit and update ST(0)
    
    LOG_DEBUG("Generating optimized FSQRT instruction");
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0
    
    // Allocate stack space for temporary values
    assembler.emitInstruction("SUB sp, sp, #16");
    
    // Load ST(0) to d0 NEON register
    assembler.emitInstruction("MOV x0, #0");           // ST(0) has logical index 0
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d0"); // Optimized helper
    
    // Check for negative input
    assembler.emitInstruction("FMOV x0, d0");
    assembler.emitInstruction("TST x0, #0x8000000000000000"); // Test sign bit
    assembler.emitInstruction("B.NE sqrt_negative_input");   // Branch if sign bit set
    
    // Calculate square root using ARM FSQRT instruction
    assembler.emitInstruction("FSQRT d0, d0");
    
    // Skip negative input handler
    assembler.emitInstruction("B sqrt_done");
    
    // Negative input handler
    assembler.emitLabel("sqrt_negative_input");
    // Set the invalid operation flag in the FPU status word
    assembler.emitInstruction("MOV x0, #1"); // Invalid operation exception
    assembler.emitFunctionCall("handle_fpu_exception");
    
    // Load QNaN as result
    assembler.emitInstruction("LDR x0, =0x7FF8000000000000"); // QNaN
    assembler.emitInstruction("FMOV d0, x0");
    
    // Continue with result handling
    assembler.emitLabel("sqrt_done");
    
    // Apply FPU precision control and rounding mode
    apply_precision_control(assembler, 0, 0);
    
    // Update ST(0) with result in d0
    assembler.emitInstruction("MOV x0, #0");                // ST(0) destination index
    assembler.emitFunctionCall("SIMDState::write_fpu_reg_from_d0");
    
    // Clean up stack
    assembler.emitInstruction("ADD sp, sp, #16");
    
    // Check for FPU exceptions
    generate_fpu_exception_check(assembler);
    
    // Release scratch registers
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fyl2x(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for FYL2X instruction");
    
    // FYL2X computes: ST(1) * log2(ST(0)), pops both, and pushes the result
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for temp
    assembler.reserveScratchRegister(1); // x1 for temp
    assembler.reserveScratchRegister(2); // x2 for status flags
    
    // 1. First check that we have at least 2 values on the stack (ST(0) and ST(1) not empty)
    // Get TOP value
    assembler.emitFunctionCall("SIMDState::get_fpu_top");
    
    // Store TOP value for later use
    assembler.emitInstruction("MOV w3, w0");  // Save TOP in w3
    
    // Check if ST(0) is empty
    assembler.emitFunctionCall("SIMDState::get_tag_for_register");
    assembler.emitInstruction("CMP w0, #3");  // EMPTY is 3
    assembler.emitInstruction("B.EQ fyl2x_stack_underflow");
    
    // Calculate ST(1) index and check if it's empty
    assembler.emitInstruction("ADD w0, w3, #1");  // ST(1) = (TOP + 1) & 7
    assembler.emitInstruction("AND w0, w0, #7");
    assembler.emitFunctionCall("SIMDState::get_tag_for_register");
    assembler.emitInstruction("CMP w0, #3");  // EMPTY is 3
    assembler.emitInstruction("B.EQ fyl2x_stack_underflow");
    
    // 2. Allocate stack space for temp buffers (two 80-bit floats and status)
    assembler.emitInstruction("SUB sp, sp, #32");
    
    // 3. Load ST(0) directly to d0
    assembler.emitInstruction("MOV x0, #0");  // ST(0) logical index
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d0");
    
    // 4. Load ST(1) directly to d1
    assembler.emitInstruction("MOV x0, #1");  // ST(1) logical index
    assembler.emitFunctionCall("SIMDState::read_fpu_reg_to_d1");
    
    // 5. Check for invalid inputs
    
    // Check if ST(0) is NaN
    assembler.emitInstruction("FCMP d0, d0");
    assembler.emitInstruction("B.VS fyl2x_invalid_input");  // Branch if unordered (NaN)
    
    // Check if ST(1) is NaN
    assembler.emitInstruction("FCMP d1, d1");
    assembler.emitInstruction("B.VS fyl2x_invalid_input");  // Branch if unordered (NaN)
    
    // Check if ST(0) <= 0 (invalid for logarithm)
    assembler.emitInstruction("FCMP d0, #0.0");
    assembler.emitInstruction("B.LE fyl2x_st0_invalid");  // Branch if <= 0
    
    // 6. Convert operands to 80-bit format
    // ST(0) to first temp buffer
    assembler.emitInstruction("MOV x0, sp");  // Destination buffer
    assembler.emitFunctionCall("convert_d0_to_f80");
    
    // ST(1) to second temp buffer
    assembler.emitInstruction("FMOV d0, d1");  // Copy d1 to d0 for conversion
    assembler.emitInstruction("ADD x0, sp, #10");  // Destination buffer
    assembler.emitFunctionCall("convert_d0_to_f80");
    
    // 7. Call helper function to compute y * log2(x)
    assembler.emitInstruction("MOV x0, sp");      // x buffer (ST0)
    assembler.emitInstruction("ADD x1, sp, #10"); // y buffer (ST1)
    assembler.emitInstruction("ADD x2, sp, #20"); // result buffer
    assembler.emitFunctionCall("compute_y_log2_x_f80");
    
    // 8. Pop ST(0) and ST(1) from the stack
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(0)
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(1), which is now ST(0)
    
    // 9. Load result back to d0
    assembler.emitInstruction("ADD x0, sp, #20");  // Result buffer
    assembler.emitFunctionCall("convert_f80_to_d0");  // Load to d0
    
    // 10. Push the result onto the FPU stack
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    
    // 11. Clear condition code flags
    assembler.emitInstruction("MOV w0, #0");  // Clear all CC flags
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // 12. Skip error handling
    assembler.emitInstruction("B fyl2x_done");
    
    // --- Error handling paths ---
    
    // Handle stack underflow
    assembler.emitLabel("fyl2x_stack_underflow");
    assembler.emitInstruction("MOV w0, #0x0041");  // Invalid operation + C3 flag (stack underflow)
    assembler.emitInstruction("ORR w0, w0, #0x0080");  // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    assembler.emitInstruction("B fyl2x_done_no_stack");
    
    // Handle invalid input (NaN in either operand)
    assembler.emitLabel("fyl2x_invalid_input");
    assembler.emitInstruction("MOV w0, #0x0001");  // Invalid operation flag
    assembler.emitInstruction("ORR w0, w0, #0x0002");  // C1 flag
    assembler.emitInstruction("ORR w0, w0, #0x0080");  // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // Pop both stack values
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(0)
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(1), which is now ST(0)
    
    // Push QNaN result
    assembler.emitInstruction("FMOV d0, #NaN");
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    assembler.emitInstruction("B fyl2x_done");
    
    // Handle invalid ST(0) (<=0 for logarithm)
    assembler.emitLabel("fyl2x_st0_invalid");
    // Check if ST(0) is zero (dividing by zero) or negative (invalid operation)
    assembler.emitInstruction("FCMP d0, #0.0");
    assembler.emitInstruction("B.EQ fyl2x_divide_by_zero");  // Branch if == 0
    
    // Handle negative ST(0)
    assembler.emitInstruction("MOV w0, #0x0001");  // Invalid operation flag
    assembler.emitInstruction("ORR w0, w0, #0x0002");  // C1 flag
    assembler.emitInstruction("ORR w0, w0, #0x0080");  // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // Pop both stack values
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(0)
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(1), which is now ST(0)
    
    // Push QNaN result
    assembler.emitInstruction("FMOV d0, #NaN");
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    assembler.emitInstruction("B fyl2x_done");
    
    // Handle divide by zero (ST(0) = 0)
    assembler.emitLabel("fyl2x_divide_by_zero");
    assembler.emitInstruction("MOV w0, #0x0004");  // Divide by zero flag
    assembler.emitInstruction("ORR w0, w0, #0x0002");  // C1 flag
    assembler.emitInstruction("ORR w0, w0, #0x0080");  // Error summary bit
    assembler.emitFunctionCall("SIMDState::update_status_word_flags");
    
    // Check the sign of ST(1) to determine the sign of infinity
    assembler.emitInstruction("FCMP d1, #0.0");
    assembler.emitInstruction("B.LT fyl2x_negative_infinity");  // Branch if < 0
    
    // Pop both stack values
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(0)
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(1), which is now ST(0)
    
    // Push positive infinity result
    assembler.emitInstruction("LDR d0, =0x7FF0000000000000");  // +Infinity
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    assembler.emitInstruction("B fyl2x_done");
    
    // Handle negative infinity result
    assembler.emitLabel("fyl2x_negative_infinity");
    // Pop both stack values
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(0)
    assembler.emitFunctionCall("SIMDState::pop_without_result");  // Pop ST(1), which is now ST(0)
    
    // Push negative infinity result
    assembler.emitInstruction("LDR d0, =0xFFF0000000000000");  // -Infinity
    assembler.emitFunctionCall("SIMDState::push_from_d0");
    
    // Cleanup
    assembler.emitLabel("fyl2x_done");
    assembler.emitInstruction("ADD sp, sp, #32");  // Free stack space
    
    assembler.emitLabel("fyl2x_done_no_stack");
    
    // Release scratch registers
    assembler.releaseScratchRegister(2);
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fldcw(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for FLDCW instruction");
    
    if (instr.operands.empty()) {
        LOG_ERROR("FLDCW instruction without operands");
        return;
    }
    
    const ir::IrOperand& src_op = instr.operands[0];
    
    // FLDCW loads a 16-bit control word from memory
    if (src_op.type != ir::IrOperandType::MEMORY) {
        LOG_ERROR("FLDCW requires memory operand");
        return;
    }
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for memory address
    assembler.reserveScratchRegister(1); // x1 for control word value
    
    // Load the memory address into register
    uint32_t mem_addr = src_op.mem_info.displacement;
    assembler.emitInstruction("MOV x0, #" + std::to_string(mem_addr));
    
    // Read the 16-bit control word from guest memory
    assembler.emitFunctionCall("read_guest_u16");
    
    // Result is now in w0. Call the helper to set FPU control word
    assembler.emitFunctionCall("SIMDState::set_fpu_control_word");
    
    // Release scratch registers
    assembler.releaseScratchRegister(1);
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fnstcw(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for FNSTCW instruction");
    
    if (instr.operands.empty()) {
        LOG_ERROR("FNSTCW instruction without operands");
        return;
    }
    
    const ir::IrOperand& dst_op = instr.operands[0];
    
    // FNSTCW stores the 16-bit control word to memory
    if (dst_op.type != ir::IrOperandType::MEMORY) {
        LOG_ERROR("FNSTCW requires memory operand");
        return;
    }
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for memory address/control word value
    
    // Get the current FPU control word
    assembler.emitFunctionCall("SIMDState::get_fpu_control_word");
    
    // Save the control word (in w0) to guest memory
    uint32_t mem_addr = dst_op.mem_info.displacement;
    assembler.emitInstruction("MOV x1, #" + std::to_string(mem_addr));
    assembler.emitFunctionCall("write_guest_u16");
    
    // Release scratch register
    assembler.releaseScratchRegister(0);
}

void FPUCodeGenerator::generate_fnstsw(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state) {
    LOG_DEBUG("Generating ARM code for FNSTSW instruction");
    
    if (instr.operands.empty()) {
        LOG_ERROR("FNSTSW instruction without operands");
        return;
    }
    
    const ir::IrOperand& dst_op = instr.operands[0];
    
    // Reserve scratch registers
    assembler.reserveScratchRegister(0); // x0 for memory address/status word value
    
    // Get the current FPU status word
    assembler.emitFunctionCall("SIMDState::get_fpu_status_word");
    
    // The result is in w0. Save it based on the operand type
    if (dst_op.type == ir::IrOperandType::MEMORY) {
        // Store to memory
        uint32_t mem_addr = dst_op.mem_info.displacement;
        assembler.emitInstruction("MOV x1, #" + std::to_string(mem_addr));
        assembler.emitFunctionCall("write_guest_u16");
    } 
    else if (dst_op.type == ir::IrOperandType::REGISTER && dst_op.reg_idx == 0) {
        // Store to AX register (FNSTSW AX)
        // This is a special form of the instruction that stores the status word in AX
        assembler.emitFunctionCall("set_x86_ax_register");
    }
    else {
        LOG_ERROR("FNSTSW requires memory operand or AX register");
    }
    
    // Release scratch register
    assembler.releaseScratchRegister(0);
}

// Handle denormal input (Phase 8 enhancement)
void FPUCodeGenerator::handle_denormal_input(ArmAssembler& assembler, int src_reg) {
    LOG_DEBUG("Generating code for denormal input handling");
    
    // Check if the input is denormal
    assembler.emitInstruction("FABS d1, d" + std::to_string(src_reg));  // Get absolute value
    assembler.emitInstruction("FMOV d2, #0.0");                    // Zero for comparison
    assembler.emitInstruction("LDR d3, =2.2250738585072014e-308"); // Min normalized double
    
    // Compare with zero
    assembler.emitInstruction("FCMP d1, d2");
    assembler.emitInstruction("B.EQ skip_denormal_check");          // Skip if zero
    
    // Compare with smallest normalized number
    assembler.emitInstruction("FCMP d1, d3");
    assembler.emitInstruction("B.GE skip_denormal_check");          // Skip if normalized
    
    // Handle denormal based on current FPU control word
    assembler.emitFunctionCall("SIMDState::get_fpu_control_word");
    assembler.emitInstruction("AND w0, w0, #0x0002");              // Extract denormal control bit
    assembler.emitInstruction("CBZ w0, denormal_handle_as_zero");  // If bit 1 is 0, handle as zero
    
    // Handle as normal (ARM default behavior, no need to do anything)
    assembler.emitInstruction("B skip_denormal_check");
    
    // Handle as zero
    assembler.emitLabel("denormal_handle_as_zero");
    assembler.emitInstruction("FMOV d" + std::to_string(src_reg) + ", #0.0");
    
    // Raise denormal operand exception
    assembler.emitInstruction("MOV w0, #8");                       // Denormal operand flag
    assembler.emitFunctionCall("handle_fpu_exception");            // Set exception flag
    
    assembler.emitLabel("skip_denormal_check");
}

// Check result for denormal value and handle it according to FPU control word (Phase 8)
void FPUCodeGenerator::check_denormal_result(ArmAssembler& assembler, int result_reg) {
    LOG_DEBUG("Generating code for denormal result checking");
    
    // Check if the result is denormal
    assembler.emitInstruction("FABS d1, d" + std::to_string(result_reg));  // Get absolute value
    assembler.emitInstruction("FMOV d2, #0.0");                    // Zero for comparison
    assembler.emitInstruction("LDR d3, =2.2250738585072014e-308"); // Min normalized double
    
    // Compare with zero
    assembler.emitInstruction("FCMP d1, d2");
    assembler.emitInstruction("B.EQ skip_denormal_result_check");  // Skip if zero
    
    // Compare with smallest normalized number
    assembler.emitInstruction("FCMP d1, d3");
    assembler.emitInstruction("B.GE skip_denormal_result_check");  // Skip if normalized
    
    // Handle denormal based on current FPU control word
    assembler.emitFunctionCall("SIMDState::get_fpu_control_word");
    assembler.emitInstruction("AND w0, w0, #0x0002");              // Extract denormal control bit
    assembler.emitInstruction("CBZ w0, denormal_result_to_zero");  // If bit 1 is 0, handle as zero
    
    // Handle as normal (ARM default behavior, just set the exception flag)
    assembler.emitInstruction("MOV w0, #8");                       // Denormal operand flag
    assembler.emitFunctionCall("handle_fpu_exception");            // Set exception flag
    assembler.emitInstruction("B skip_denormal_result_check");
    
    // Handle as zero
    assembler.emitLabel("denormal_result_to_zero");
    assembler.emitInstruction("FMOV d" + std::to_string(result_reg) + ", #0.0");
    
    // Raise underflow and denormal operand exceptions
    assembler.emitInstruction("MOV w0, #0x10");                    // Underflow flag
    assembler.emitFunctionCall("handle_fpu_exception");            // Set exception flag
    assembler.emitInstruction("MOV w0, #8");                       // Denormal operand flag
    assembler.emitFunctionCall("handle_fpu_exception");            // Set exception flag
    
    assembler.emitLabel("skip_denormal_result_check");
}

// Apply precision control as specified in FPU control word (Phase 8)
void FPUCodeGenerator::apply_precision_control(ArmAssembler& assembler, int src_reg, int dst_reg) {
    LOG_DEBUG("Generating code for precision control");
    
    // Get precision control bits from FPU control word
    assembler.emitFunctionCall("SIMDState::get_fpu_control_word");
    assembler.emitInstruction("LSR w0, w0, #8");                   // Shift right to get PC bits
    assembler.emitInstruction("AND w0, w0, #0x03");                // Extract PC bits (0-3)
    
    // Branch based on precision control value
    assembler.emitInstruction("CBZ w0, pc_extended_precision");    // PC=0: 80-bit extended precision
    assembler.emitInstruction("CMP w0, #1");
    assembler.emitInstruction("B.EQ pc_single_precision");         // PC=1: 32-bit single precision
    assembler.emitInstruction("CMP w0, #2");
    assembler.emitInstruction("B.EQ pc_reserved");                 // PC=2: reserved (default to double)
    assembler.emitInstruction("CMP w0, #3");
    assembler.emitInstruction("B.EQ pc_double_precision");         // PC=3: 64-bit double precision
    
    // Extended precision (80-bit) - This is the default, no conversion needed
    assembler.emitLabel("pc_extended_precision");
    if (src_reg != dst_reg) {
        assembler.emitInstruction("FMOV d" + std::to_string(dst_reg) + ", d" + std::to_string(src_reg));
    }
    assembler.emitInstruction("B pc_done");
    
    // Single precision (32-bit)
    assembler.emitLabel("pc_single_precision");
    // Convert to single and back to double to apply precision loss
    assembler.emitInstruction("FCVT s4, d" + std::to_string(src_reg));
    assembler.emitInstruction("FCVT d" + std::to_string(dst_reg) + ", s4");
    assembler.emitInstruction("B pc_done");
    
    // Reserved (treat as double precision)
    assembler.emitLabel("pc_reserved");
    // Fall through to double precision
    
    // Double precision (64-bit) - ARM FP is already double, no conversion needed
    assembler.emitLabel("pc_double_precision");
    if (src_reg != dst_reg) {
        assembler.emitInstruction("FMOV d" + std::to_string(dst_reg) + ", d" + std::to_string(src_reg));
    }
    
    assembler.emitLabel("pc_done");
}

// Apply the rounding mode from FPU control word (Phase 8)
void FPUCodeGenerator::apply_rounding_mode(ArmAssembler& assembler) {
    LOG_DEBUG("Generating code for rounding mode control");
    
    // Get rounding control bits from FPU control word
    assembler.emitFunctionCall("SIMDState::get_fpu_control_word");
    assembler.emitInstruction("LSR w0, w0, #10");                  // Shift right to get RC bits
    assembler.emitInstruction("AND w0, w0, #0x03");                // Extract RC bits (0-3)
    
    // Set FPCR rounding mode bits (22-23)
    assembler.emitInstruction("MRS x1, FPCR");                     // Get current FPCR
    assembler.emitInstruction("BIC x1, x1, #0xC00000");            // Clear rounding mode bits
    
    // Map x87 rounding mode to ARM FPCR rounding mode
    // x87 RC: 0=nearest, 1=down, 2=up, 3=truncate
    // ARM RM: 0=nearest, 1=plus inf, 2=minus inf, 3=zero
    assembler.emitInstruction("CMP w0, #0");
    assembler.emitInstruction("B.EQ rm_nearest");
    assembler.emitInstruction("CMP w0, #1");
    assembler.emitInstruction("B.EQ rm_down");
    assembler.emitInstruction("CMP w0, #2");
    assembler.emitInstruction("B.EQ rm_up");
    assembler.emitInstruction("CMP w0, #3");
    assembler.emitInstruction("B.EQ rm_truncate");
    
    // Round to nearest (even)
    assembler.emitLabel("rm_nearest");
    // ARM default mode is round to nearest, no need to change bits
    assembler.emitInstruction("B rm_apply");
    
    // Round down (toward -∞)
    assembler.emitLabel("rm_down");
    assembler.emitInstruction("ORR x1, x1, #0x800000");            // RM=10 (minus inf)
    assembler.emitInstruction("B rm_apply");
    
    // Round up (toward +∞)
    assembler.emitLabel("rm_up");
    assembler.emitInstruction("ORR x1, x1, #0x400000");            // RM=01 (plus inf)
    assembler.emitInstruction("B rm_apply");
    
    // Truncate (toward 0)
    assembler.emitLabel("rm_truncate");
    assembler.emitInstruction("ORR x1, x1, #0xC00000");            // RM=11 (zero)
    
    // Apply the rounding mode
    assembler.emitLabel("rm_apply");
    assembler.emitInstruction("MSR FPCR, x1");                     // Update FPCR
}

void FPUCodeGenerator::generate_fpu_exception_check(ArmAssembler& assembler, bool respect_mask) {
    // Call helper to check for FPU exceptions and handle them according to mask
    assembler.emitInstruction("MOV w0, #" + std::to_string(respect_mask ? 1 : 0));
    assembler.emitFunctionCall("SIMDState::check_and_handle_fpu_exceptions");
}

} // namespace aarch64
} // namespace xenoarm_jit 