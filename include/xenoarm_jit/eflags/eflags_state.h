#ifndef XENOARM_JIT_EFLAGS_EFLAGS_STATE_H
#define XENOARM_JIT_EFLAGS_EFLAGS_STATE_H

#include <cstdint>

namespace xenoarm_jit {
namespace eflags {

// Flag evaluation strategy
enum class FlagEvaluationStrategy {
    IMMEDIATE,  // Compute flags immediately after operation
    LAZY        // Compute flags only when needed
};

// Represents the state of the x86 EFLAGS register
// Only includes flags relevant to V1.0 scope (integer and control flow)
struct EFlagsState {
    uint32_t raw; // Raw EFLAGS value

    // For lazy flag evaluation
    uint32_t result;     // Last operation result
    uint32_t src1;       // First operand of last operation
    uint32_t src2;       // Second operand of last operation
    uint8_t op_type;     // Type of last operation (ADD, SUB, etc.)
    bool lazy_valid;     // Whether lazy state is valid

    // Individual flags (extracted from 'raw' or updated directly)
    // These can be accessed as bitfields or through helper functions
    // For simplicity in Phase 3, we might primarily work with the raw value
    // and implement flag updates/reads in the code generator.

    // Common flags:
    // CF (Carry Flag): bit 0
    // PF (Parity Flag): bit 2
    // AF (Adjust Flag): bit 4 (usually not needed for V1.0)
    // ZF (Zero Flag): bit 6
    // SF (Sign Flag): bit 7
    // TF (Trap Flag): bit 8 (for debugging, maybe not V1.0)
    // IF (Interrupt Enable Flag): bit 9 (for interrupts, maybe not V1.0)
    // DF (Direction Flag): bit 10 (for string ops)
    // OF (Overflow Flag): bit 11
    // IOPL (I/O Privilege Level): bits 12-13 (for protected mode, not V1.0)
    // NT (Nested Task Flag): bit 14 (for protected mode, not V1.0)
    // RF (Resume Flag): bit 16 (for debugging, maybe not V1.0)
    // VM (Virtual 8086 Mode Flag): bit 17 (not V1.0)
    // AC (Alignment Check Flag): bit 18 (maybe not V1.0)
    // VIF (Virtual Interrupt Flag): bit 19 (not V1.0)
    // VIP (Virtual Interrupt Pending): bit 20 (not V1.0)
    // ID (ID Flag): bit 21 (for CPUID, maybe not V1.0)

    // Helper functions to get/set individual flags (optional for Phase 3, can be done in code generator)
    bool get_cf() const { return (raw >> 0) & 1; }
    void set_cf(bool val) { if (val) raw |= (1 << 0); else raw &= ~(1 << 0); }

    bool get_pf() const { return (raw >> 2) & 1; }
    void set_pf(bool val) { if (val) raw |= (1 << 2); else raw &= ~(1 << 2); }

    bool get_af() const { return (raw >> 4) & 1; }
    void set_af(bool val) { if (val) raw |= (1 << 4); else raw &= ~(1 << 4); }

    bool get_zf() const { return (raw >> 6) & 1; }
    void set_zf(bool val) { if (val) raw |= (1 << 6); else raw &= ~(1 << 6); }

    bool get_sf() const { return (raw >> 7) & 1; }
    void set_sf(bool val) { if (val) raw |= (1 << 7); else raw &= ~(1 << 7); }

    bool get_df() const { return (raw >> 10) & 1; }
    void set_df(bool val) { if (val) raw |= (1 << 10); else raw &= ~(1 << 10); }

    bool get_of() const { return (raw >> 11) & 1; }
    void set_of(bool val) { if (val) raw |= (1 << 11); else raw &= ~(1 << 11); }

    // Constructor
    EFlagsState() : raw(0x2), result(0), src1(0), src2(0), op_type(0), lazy_valid(false) {
        // Default state: bit 1 is always 1
    }

    // Evaluate condition based on current flag state
    // Used for Jcc instructions
    bool evaluate_condition(uint8_t condition) const;

    // Store operation results for lazy flag evaluation
    void store_op_state(uint32_t result, uint32_t src1, uint32_t src2, uint8_t op_type);

    // Evaluate flags from stored operation state
    void evaluate_flags();

    // Op types for lazy evaluation
    enum OpType {
        OP_ADD = 0,
        OP_SUB = 1,
        OP_AND = 2,
        OP_OR = 3,
        OP_XOR = 4,
        OP_SHL = 5,
        OP_SHR = 6,
        OP_SAR = 7,
        OP_TEST = 8,
        OP_CMP = 9
    };

    // Condition codes for Jcc instructions
    enum ConditionCode {
        CC_O = 0x0,    // Overflow
        CC_NO = 0x1,   // Not overflow
        CC_B = 0x2,    // Below (CF=1)
        CC_NB = 0x3,   // Not below (CF=0)
        CC_Z = 0x4,    // Zero (ZF=1)
        CC_NZ = 0x5,   // Not zero (ZF=0)
        CC_BE = 0x6,   // Below or equal (CF=1 or ZF=1)
        CC_NBE = 0x7,  // Not below or equal (CF=0 and ZF=0)
        CC_S = 0x8,    // Sign (SF=1)
        CC_NS = 0x9,   // Not sign (SF=0)
        CC_P = 0xA,    // Parity (PF=1)
        CC_NP = 0xB,   // Not parity (PF=0)
        CC_L = 0xC,    // Less than (SF!=OF)
        CC_NL = 0xD,   // Not less than (SF=OF)
        CC_LE = 0xE,   // Less than or equal (ZF=1 or SF!=OF)
        CC_NLE = 0xF   // Not less than or equal (ZF=0 and SF=OF)
    };
};

} // namespace eflags
} // namespace xenoarm_jit

#endif // XENOARM_JIT_EFLAGS_EFLAGS_STATE_H