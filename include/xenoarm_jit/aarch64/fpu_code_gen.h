#ifndef XENOARM_JIT_AARCH64_FPU_CODE_GEN_H
#define XENOARM_JIT_AARCH64_FPU_CODE_GEN_H

#include "xenoarm_jit/ir.h"
#include "xenoarm_jit/simd_state.h"
#include "xenoarm_jit/aarch64/arm_assembler.h"
#include "xenoarm_jit/fpu_transcendental_helpers.h"

namespace xenoarm_jit {
namespace aarch64 {

/**
 * Code generator for x87 FPU instructions
 * 
 * This class is responsible for translating IR instructions for FPU operations
 * into AArch64 machine code, using ARM NEON/VFP to emulate x87 FPU operations.
 */
class FPUCodeGenerator {
public:
    /**
     * Generate AArch64 code for an FPU instruction
     * 
     * @param instr The IR instruction to translate
     * @param assembler The ARM assembler to use for code generation
     * @param simd_state The SIMD state manager for accessing FPU state
     */
    void generate_fpu_code(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    
private:
    // Helper methods for generating specific FPU instructions
    void generate_fld(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fstp(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fadd(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fsub(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fmul(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fdiv(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    
    // Transcendental functions - added for Phase 8
    void generate_fsin(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fcos(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fptan(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_f2xm1(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fyl2x(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fpatan(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fsqrt(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fscale(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    
    // FPU control word handling - added for Phase 8
    void generate_fldcw(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fnstcw(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    void generate_fnstsw(const ir::IrInstruction& instr, ArmAssembler& assembler, simd::SIMDState& simd_state);
    
    // Denormal handling helpers - added for Phase 8
    void handle_denormal_input(ArmAssembler& assembler, int src_reg);
    void check_denormal_result(ArmAssembler& assembler, int result_reg);
    
    // Helper method for checking FPU exceptions after operations
    void generate_fpu_exception_check(ArmAssembler& assembler, bool respect_mask = true);
    
    // Helper for applying the current precision control from FPU control word - added for Phase 8
    void apply_precision_control(ArmAssembler& assembler, int src_reg, int dst_reg);
    
    // Helper for applying the current rounding mode from FPU control word - added for Phase 8
    void apply_rounding_mode(ArmAssembler& assembler);
};

} // namespace aarch64
} // namespace xenoarm_jit

#endif // XENOARM_JIT_AARCH64_FPU_CODE_GEN_H 