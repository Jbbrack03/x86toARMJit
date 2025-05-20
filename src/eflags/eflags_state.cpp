#include "xenoarm_jit/eflags/eflags_state.h"
#include "logging/logger.h" // Assuming a logger exists

namespace xenoarm_jit {
namespace eflags {

// No non-inline functions to define in Phase 3 for EFlagsState

// Evaluate condition based on current flag state
bool EFlagsState::evaluate_condition(uint8_t condition) const {
    // If we have lazy flags, evaluate them first
    if (lazy_valid) {
        // This is a const method, so we need a const_cast
        const_cast<EFlagsState*>(this)->evaluate_flags();
    }

    switch (condition) {
        case CC_O:  return get_of();
        case CC_NO: return !get_of();
        case CC_B:  return get_cf();
        case CC_NB: return !get_cf();
        case CC_Z:  return get_zf();
        case CC_NZ: return !get_zf();
        case CC_BE: return get_cf() || get_zf();
        case CC_NBE: return !get_cf() && !get_zf();
        case CC_S:  return get_sf();
        case CC_NS: return !get_sf();
        case CC_P:  return get_pf();
        case CC_NP: return !get_pf();
        case CC_L:  return get_sf() != get_of();
        case CC_NL: return get_sf() == get_of();
        case CC_LE: return get_zf() || (get_sf() != get_of());
        case CC_NLE: return !get_zf() && (get_sf() == get_of());
        default:
            LOG_ERROR("Unknown condition code: " + std::to_string(condition));
            return false;
    }
}

// Store operation results for lazy flag evaluation
void EFlagsState::store_op_state(uint32_t result_value, uint32_t src1_value, uint32_t src2_value, uint8_t op_type_value) {
    result = result_value;
    src1 = src1_value;
    src2 = src2_value;
    op_type = op_type_value;
    lazy_valid = true;
    
    LOG_DEBUG("Stored operation state for lazy flag evaluation");
}

// Evaluate flags from stored operation state
void EFlagsState::evaluate_flags() {
    if (!lazy_valid) {
        LOG_WARNING("Attempted to evaluate flags with invalid lazy state");
        return;
    }
    
    // Reset all flags first to ensure clean state
    raw &= ~(0x8D5); // Clear CF, PF, AF, ZF, SF, OF

    // Set Zero Flag (ZF)
    if (result == 0) {
        set_zf(true);
    }
    
    // Set Sign Flag (SF)
    if ((result >> 31) & 1) {
        set_sf(true);
    }
    
    // Set Parity Flag (PF)
    uint8_t parity = result & 0xFF;
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    set_pf(!(parity & 1)); // PF=1 if even number of bits
    
    // Handle different operations
    switch (op_type) {
        case OP_ADD: {
            // Carry Flag (CF)
            set_cf(result < src1);
            
            // Overflow Flag (OF) - signed overflow
            uint32_t sign_src1 = (src1 >> 31) & 1;
            uint32_t sign_src2 = (src2 >> 31) & 1;
            uint32_t sign_result = (result >> 31) & 1;
            set_of((sign_src1 == sign_src2) && (sign_result != sign_src1));
            
            // Adjust Flag (AF) - rarely used, but for completeness
            set_af(((src1 ^ src2 ^ result) >> 4) & 1);
            break;
        }
        case OP_SUB:
        case OP_CMP: {
            // Carry Flag (CF)
            set_cf(src1 < src2);
            
            // Overflow Flag (OF) - signed overflow
            uint32_t sign_src1 = (src1 >> 31) & 1;
            uint32_t sign_src2 = (src2 >> 31) & 1;
            uint32_t sign_result = (result >> 31) & 1;
            set_of((sign_src1 != sign_src2) && (sign_result != sign_src1));
            
            // Adjust Flag (AF)
            set_af(((src1 ^ src2 ^ result) >> 4) & 1);
            break;
        }
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_TEST: {
            // Logical operations clear CF and OF
            set_cf(false);
            set_of(false);
            set_af(false); // Undefined, but typically cleared
            break;
        }
        case OP_SHL: {
            // Shift count (src2) is masked to 5 bits for 32-bit operands
            uint32_t shift_count = src2 & 0x1F;
            if (shift_count > 0) {
                // CF gets the last bit shifted out
                set_cf((src1 >> (32 - shift_count)) & 1);
                // OF for shift count of 1 is set to MSB(result) XOR CF
                if (shift_count == 1) {
                    set_of(((result >> 31) & 1) != get_cf());
                }
            }
            break;
        }
        case OP_SHR: {
            uint32_t shift_count = src2 & 0x1F;
            if (shift_count > 0) {
                // CF gets the last bit shifted out
                set_cf((src1 >> (shift_count - 1)) & 1);
                // OF for shift count of 1 is set to MSB of original operand
                if (shift_count == 1) {
                    set_of((src1 >> 31) & 1);
                }
            }
            break;
        }
        case OP_SAR: {
            uint32_t shift_count = src2 & 0x1F;
            if (shift_count > 0) {
                // CF gets the last bit shifted out
                set_cf((src1 >> (shift_count - 1)) & 1);
                // OF is cleared for SAR
                set_of(false);
            }
            break;
        }
        default: {
            LOG_ERROR("Unknown operation type for flag evaluation: " + std::to_string(op_type));
            break;
        }
    }
    
    // Clear lazy state
    lazy_valid = false;
    
    LOG_DEBUG("Evaluated flags from operation state");
}

} // namespace eflags
} // namespace xenoarm_jit