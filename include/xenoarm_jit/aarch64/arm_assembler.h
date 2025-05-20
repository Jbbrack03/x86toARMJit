#ifndef XENOARM_JIT_AARCH64_ARM_ASSEMBLER_H
#define XENOARM_JIT_AARCH64_ARM_ASSEMBLER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace xenoarm_jit {
namespace aarch64 {

/**
 * Class responsible for generating ARM AArch64 assembly code
 * 
 * Enhanced in Phase 8 for direct register access and optimization
 */
class ArmAssembler {
public:
    ArmAssembler();
    ~ArmAssembler();
    
    /**
     * Emit an ARM instruction
     * @param instruction The instruction to emit
     */
    void emitInstruction(const std::string& instruction);
    
    /**
     * Emit a function call
     * @param function The function to call
     */
    void emitFunctionCall(const std::string& function);
    
    /**
     * Emit a label in the code
     * @param label The label to emit
     */
    void emitLabel(const std::string& label);
    
    /**
     * Reserve a scratch register
     * @param reg The register to reserve
     */
    void reserveScratchRegister(int reg);
    
    /**
     * Release a scratch register
     * @param reg The register to release
     */
    void releaseScratchRegister(int reg);
    
    /**
     * Get a free scratch register
     * @return The allocated register number, or -1 if none available
     */
    int getFreeScratchRegister();
    
    /**
     * Get or allocate a named register
     * @param name The name to associate with the register
     * @return The register number, or -1 if allocation failed
     */
    int getNamedRegister(const std::string& name);
    
    /**
     * Emit code to convert an 80-bit float to a NEON register
     * @param src_mem Source memory location containing 80-bit float
     * @param dst_reg Destination NEON register (e.g., "d0")
     */
    void emitConvertF80ToNEON(const std::string& src_mem, const std::string& dst_reg);
    
    /**
     * Emit code to convert a NEON register to 80-bit float
     * @param src_reg Source NEON register (e.g., "d0")
     * @param dst_mem Destination memory location for 80-bit float
     */
    void emitConvertNEONToF80(const std::string& src_reg, const std::string& dst_mem);
    
    /**
     * Emit code to transfer directly between NEON registers and X87 registers
     * @param fpu_reg Logical FPU register index (0-7)
     * @param neon_reg The NEON register name (e.g., "d0", "d1")
     * @param is_read True for FPU->NEON, false for NEON->FPU
     */
    void emitDirectRegisterTransfer(int fpu_reg, const std::string& neon_reg, bool is_read);
    
    /**
     * Emit optimized code for FPU operations using direct register access
     * @param op The operation to perform (e.g., "fadd", "fmul", "fsqrt")
     * @param src_reg Source NEON register
     * @param dst_reg Destination NEON register
     */
    void emitDirectFPUOperation(const std::string& op, const std::string& src_reg, const std::string& dst_reg);
    
    /**
     * Set precision control for floating-point operations
     * @param precision Precision control bits (0=single, 1=reserved, 2=double, 3=extended)
     */
    void emitSetPrecisionControl(int precision);
    
    /**
     * Set rounding mode for floating-point operations
     * @param mode Rounding mode (0=nearest, 1=down, 2=up, 3=zero)
     */
    void emitSetRoundingMode(int mode);
    
    /**
     * Apply precision control and rounding to a register
     * @param reg The register to apply precision control to
     */
    void emitApplyPrecisionAndRounding(const std::string& reg);
    
private:
    // Static members for tracking register usage
    static std::unordered_set<int> used_registers_;
    static std::unordered_map<std::string, int> named_registers_;
    static std::vector<std::string> register_names_;
};

} // namespace aarch64
} // namespace xenoarm_jit

#endif // XENOARM_JIT_AARCH64_ARM_ASSEMBLER_H 