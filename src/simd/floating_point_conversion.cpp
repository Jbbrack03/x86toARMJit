#include "xenoarm_jit/floating_point_conversion.h"
#include "logging/logger.h"
#include <cstring>
#include <cmath>
#include <limits>

namespace xenoarm_jit {
namespace fp {

// FPU exception flags in status word
constexpr uint16_t FPU_SW_IE = 0x0001;  // Invalid operation
constexpr uint16_t FPU_SW_DE = 0x0002;  // Denormalized operand
constexpr uint16_t FPU_SW_ZE = 0x0004;  // Zero divide
constexpr uint16_t FPU_SW_OE = 0x0008;  // Overflow
constexpr uint16_t FPU_SW_UE = 0x0010;  // Underflow
constexpr uint16_t FPU_SW_PE = 0x0020;  // Precision

// FPU control word rounding mode bits
constexpr uint16_t FPU_CW_RC_NEAREST = 0x0000;  // Round to nearest (even)
constexpr uint16_t FPU_CW_RC_DOWN = 0x0400;     // Round down (toward -∞)
constexpr uint16_t FPU_CW_RC_UP = 0x0800;       // Round up (toward +∞)
constexpr uint16_t FPU_CW_RC_ZERO = 0x0C00;     // Round toward zero (truncate)
constexpr uint16_t FPU_CW_RC_MASK = 0x0C00;     // Mask for rounding control bits

// x87 FPU 80-bit format:
// sign (1 bit) | exponent (15 bits) | integer bit (1 bit) | fraction (63 bits)

void convert_f32_to_f80(const void* src, void* dst) {
    // Get the 32-bit float
    float f32_value;
    std::memcpy(&f32_value, src, sizeof(float));
    
    // Extract components
    uint32_t float_bits;
    std::memcpy(&float_bits, &f32_value, sizeof(uint32_t));
    
    uint32_t sign = (float_bits >> 31) & 0x1;
    uint32_t exponent = (float_bits >> 23) & 0xFF;
    uint32_t fraction = float_bits & 0x7FFFFF;
    
    // Handle special cases (zero, NaN, infinity)
    uint8_t x87_bytes[10] = {0};
    
    if (exponent == 0 && fraction == 0) {
        // +/- Zero
        x87_bytes[9] = sign << 7;
        // Rest are all zeros
    } 
    else if (exponent == 0xFF && fraction == 0) {
        // +/- Infinity
        x87_bytes[9] = sign << 7;
        x87_bytes[8] = 0x7F;
        x87_bytes[7] = 0xFF;
        // Explicit integer bit is 1
        x87_bytes[6] = 0x80;
    } 
    else if (exponent == 0xFF) {
        // NaN (Not a Number)
        x87_bytes[9] = sign << 7;
        x87_bytes[8] = 0x7F;
        x87_bytes[7] = 0xFF;
        
        // Keep the fraction bits and set the MSB for QNaN
        x87_bytes[6] = 0xC0 | ((fraction >> 16) & 0x3F);
        x87_bytes[5] = (fraction >> 8) & 0xFF;
        x87_bytes[4] = fraction & 0xFF;
    } 
    else {
        // Regular number
        // Adjust the exponent bias: x87's bias is 16383, float's is 127
        int16_t x87_exponent = exponent - 127 + 16383;
        
        // Set sign bit and exponent
        x87_bytes[9] = sign << 7;
        x87_bytes[9] |= (x87_exponent >> 8) & 0x7F;
        x87_bytes[8] = x87_exponent & 0xFF;
        
        // Set the explicit integer bit (always 1 for normalized float)
        x87_bytes[7] = 0x80;
        
        // Convert the 23-bit fraction to x87's 63-bit fraction
        // High 7 bits of fraction go into byte 7 (after the integer bit)
        x87_bytes[7] |= (fraction >> 16) & 0x7F;
        // Next 8 bits
        x87_bytes[6] = (fraction >> 8) & 0xFF;
        // Last 8 bits
        x87_bytes[5] = fraction & 0xFF;
        // Lower bytes are zero
    }
    
    // Copy the result to dst
    std::memcpy(dst, x87_bytes, 10);
}

void convert_f64_to_f80(const void* src, void* dst) {
    // Get the 64-bit double
    double f64_value;
    std::memcpy(&f64_value, src, sizeof(double));
    
    // Extract components
    uint64_t double_bits;
    std::memcpy(&double_bits, &f64_value, sizeof(uint64_t));
    
    uint64_t sign = (double_bits >> 63) & 0x1;
    uint64_t exponent = (double_bits >> 52) & 0x7FF;
    uint64_t fraction = double_bits & 0xFFFFFFFFFFFFFULL;
    
    // Handle special cases (zero, NaN, infinity)
    uint8_t x87_bytes[10] = {0};
    
    if (exponent == 0 && fraction == 0) {
        // +/- Zero
        x87_bytes[9] = static_cast<uint8_t>(sign << 7);
        // Rest are all zeros
    } 
    else if (exponent == 0x7FF && fraction == 0) {
        // +/- Infinity
        x87_bytes[9] = static_cast<uint8_t>(sign << 7);
        x87_bytes[8] = 0x7F;
        x87_bytes[7] = 0xFF;
        // Explicit integer bit is 1
        x87_bytes[6] = 0x80;
    } 
    else if (exponent == 0x7FF) {
        // NaN (Not a Number)
        x87_bytes[9] = static_cast<uint8_t>(sign << 7);
        x87_bytes[8] = 0x7F;
        x87_bytes[7] = 0xFF;
        
        // Keep the fraction bits and set the MSB for QNaN
        x87_bytes[6] = 0xC0 | static_cast<uint8_t>((fraction >> 45) & 0x3F);
        x87_bytes[5] = static_cast<uint8_t>((fraction >> 37) & 0xFF);
        x87_bytes[4] = static_cast<uint8_t>((fraction >> 29) & 0xFF);
        x87_bytes[3] = static_cast<uint8_t>((fraction >> 21) & 0xFF);
        x87_bytes[2] = static_cast<uint8_t>((fraction >> 13) & 0xFF);
        x87_bytes[1] = static_cast<uint8_t>((fraction >> 5) & 0xFF);
        x87_bytes[0] = static_cast<uint8_t>((fraction & 0x1F) << 3);
    } 
    else {
        // Regular number
        // Adjust the exponent bias: x87's bias is 16383, double's is 1023
        int16_t x87_exponent = static_cast<int16_t>(exponent - 1023 + 16383);
        
        // Set sign bit and exponent
        x87_bytes[9] = static_cast<uint8_t>(sign << 7);
        x87_bytes[9] |= static_cast<uint8_t>((x87_exponent >> 8) & 0x7F);
        x87_bytes[8] = static_cast<uint8_t>(x87_exponent & 0xFF);
        
        // Set the explicit integer bit (always 1 for normalized double)
        x87_bytes[7] = 0x80;
        
        // Convert the 52-bit fraction to x87's 63-bit fraction
        x87_bytes[7] |= static_cast<uint8_t>((fraction >> 45) & 0x7F);
        x87_bytes[6] = static_cast<uint8_t>((fraction >> 37) & 0xFF);
        x87_bytes[5] = static_cast<uint8_t>((fraction >> 29) & 0xFF);
        x87_bytes[4] = static_cast<uint8_t>((fraction >> 21) & 0xFF);
        x87_bytes[3] = static_cast<uint8_t>((fraction >> 13) & 0xFF);
        x87_bytes[2] = static_cast<uint8_t>((fraction >> 5) & 0xFF);
        x87_bytes[1] = static_cast<uint8_t>((fraction & 0x1F) << 3);
        // Lowest 3 bits are zero
    }
    
    // Copy the result to dst
    std::memcpy(dst, x87_bytes, 10);
}

void convert_f80_to_f32(const void* src, void* dst) {
    // Read the 80-bit x87 value
    uint8_t x87_bytes[10];
    std::memcpy(x87_bytes, src, 10);
    
    // Extract sign, exponent, and fraction
    uint32_t sign = (x87_bytes[9] >> 7) & 0x1;
    uint32_t exponent = ((x87_bytes[9] & 0x7F) << 8) | x87_bytes[8];
    uint32_t integer_bit = (x87_bytes[7] >> 7) & 0x1;
    
    // Extract the 23 MSBs of the fraction (including integer bit)
    uint32_t fraction = ((x87_bytes[7] & 0x7F) << 16) | 
                        (x87_bytes[6] << 8) | 
                        x87_bytes[5];
    
    float result;
    
    if (exponent == 0 && integer_bit == 0 && (fraction & 0x7FFFFF) == 0) {
        // Zero
        result = sign ? -0.0f : 0.0f;
    }
    else if (exponent == 0x7FFF) {
        if (integer_bit == 1 && ((x87_bytes[7] & 0x7F) == 0) && 
            x87_bytes[6] == 0 && x87_bytes[5] == 0 && 
            x87_bytes[4] == 0 && x87_bytes[3] == 0 && 
            x87_bytes[2] == 0 && x87_bytes[1] == 0 && 
            x87_bytes[0] == 0) {
            // Infinity
            result = sign ? -std::numeric_limits<float>::infinity() : 
                             std::numeric_limits<float>::infinity();
        }
        else {
            // NaN
            result = std::numeric_limits<float>::quiet_NaN();
        }
    }
    else {
        // Convert x87 exponent to float exponent
        int32_t float_exponent = static_cast<int32_t>(exponent) - 16383 + 127;
        
        if (float_exponent <= 0) {
            // Underflow to zero
            result = sign ? -0.0f : 0.0f;
        }
        else if (float_exponent >= 0xFF) {
            // Overflow to infinity
            result = sign ? -std::numeric_limits<float>::infinity() : 
                             std::numeric_limits<float>::infinity();
        }
        else {
            // Normal case
            uint32_t float_bits = (sign << 31) | (float_exponent << 23) | (fraction & 0x7FFFFF);
            std::memcpy(&result, &float_bits, sizeof(float));
        }
    }
    
    // Copy result to destination
    std::memcpy(dst, &result, sizeof(float));
}

void convert_f80_to_f64(const void* src, void* dst) {
    // Read the 80-bit x87 value
    uint8_t x87_bytes[10];
    std::memcpy(x87_bytes, src, 10);
    
    // Extract sign, exponent, and fraction
    uint64_t sign = static_cast<uint64_t>(x87_bytes[9] >> 7) & 0x1;
    uint64_t exponent = static_cast<uint64_t>((x87_bytes[9] & 0x7F) << 8) | x87_bytes[8];
    uint64_t integer_bit = static_cast<uint64_t>(x87_bytes[7] >> 7) & 0x1;
    
    // Extract the 52 MSBs of the fraction (excluding integer bit)
    uint64_t fraction = (static_cast<uint64_t>(x87_bytes[7] & 0x7F) << 45) | 
                        (static_cast<uint64_t>(x87_bytes[6]) << 37) | 
                        (static_cast<uint64_t>(x87_bytes[5]) << 29) | 
                        (static_cast<uint64_t>(x87_bytes[4]) << 21) | 
                        (static_cast<uint64_t>(x87_bytes[3]) << 13) | 
                        (static_cast<uint64_t>(x87_bytes[2]) << 5) | 
                        (static_cast<uint64_t>(x87_bytes[1]) >> 3);
    
    double result;
    
    if (exponent == 0 && integer_bit == 0 && fraction == 0) {
        // Zero
        result = sign ? -0.0 : 0.0;
    }
    else if (exponent == 0x7FFF) {
        if (integer_bit == 1 && (x87_bytes[7] & 0x7F) == 0 && 
            x87_bytes[6] == 0 && x87_bytes[5] == 0 && 
            x87_bytes[4] == 0 && x87_bytes[3] == 0 && 
            x87_bytes[2] == 0 && x87_bytes[1] == 0 && 
            x87_bytes[0] == 0) {
            // Infinity
            result = sign ? -std::numeric_limits<double>::infinity() : 
                             std::numeric_limits<double>::infinity();
        }
        else {
            // NaN
            result = std::numeric_limits<double>::quiet_NaN();
        }
    }
    else {
        // Convert x87 exponent to double exponent
        int64_t double_exponent = static_cast<int64_t>(exponent) - 16383 + 1023;
        
        if (double_exponent <= 0) {
            // Underflow to zero
            result = sign ? -0.0 : 0.0;
        }
        else if (double_exponent >= 0x7FF) {
            // Overflow to infinity
            result = sign ? -std::numeric_limits<double>::infinity() : 
                             std::numeric_limits<double>::infinity();
        }
        else {
            // Normal case
            uint64_t double_bits = (sign << 63) | (double_exponent << 52) | (fraction & 0xFFFFFFFFFFFFFULL);
            std::memcpy(&result, &double_bits, sizeof(double));
        }
    }
    
    // Copy result to destination
    std::memcpy(dst, &result, sizeof(double));
}

void apply_rounding(const void* src, void* dst, uint16_t control_word) {
    // Get the rounding mode from the control word
    uint16_t rounding_mode = control_word & FPU_CW_RC_MASK;
    
    // For now, we'll just copy the value
    // In a real implementation, we'd apply the rounding mode
    std::memcpy(dst, src, 10);
    
    // TODO: Implement proper rounding based on the mode
    switch (rounding_mode) {
        case FPU_CW_RC_NEAREST:
            // Round to nearest (even)
            LOG_DEBUG("Rounding to nearest (not fully implemented)");
            break;
            
        case FPU_CW_RC_DOWN:
            // Round down (toward -∞)
            LOG_DEBUG("Rounding down (not fully implemented)");
            break;
            
        case FPU_CW_RC_UP:
            // Round up (toward +∞)
            LOG_DEBUG("Rounding up (not fully implemented)");
            break;
            
        case FPU_CW_RC_ZERO:
            // Round toward zero (truncate)
            LOG_DEBUG("Rounding toward zero (not fully implemented)");
            break;
            
        default:
            LOG_ERROR("Unknown rounding mode: " + std::to_string(rounding_mode));
            break;
    }
}

bool check_fpu_exceptions(const void* src, uint16_t* status_word) {
    if (!src || !status_word) {
        LOG_ERROR("Invalid arguments to check_fpu_exceptions");
        return false;
    }
    
    const uint8_t* x87_bytes = static_cast<const uint8_t*>(src);
    bool has_exceptions = false;
    
    // Extract components from x87 format
    // Format: sign(1) | exponent(15) | integer(1) | fraction(63)
    uint16_t sign = (x87_bytes[9] >> 7) & 0x1;
    uint16_t exponent = ((x87_bytes[9] & 0x7F) << 8) | x87_bytes[8];
    uint16_t integer_bit = (x87_bytes[7] >> 7) & 0x1;
    
    // Build fraction value from bytes
    uint64_t fraction = (static_cast<uint64_t>(x87_bytes[7] & 0x7F) << 56) |
                        (static_cast<uint64_t>(x87_bytes[6]) << 48) |
                        (static_cast<uint64_t>(x87_bytes[5]) << 40) |
                        (static_cast<uint64_t>(x87_bytes[4]) << 32) |
                        (static_cast<uint64_t>(x87_bytes[3]) << 24) |
                        (static_cast<uint64_t>(x87_bytes[2]) << 16) |
                        (static_cast<uint64_t>(x87_bytes[1]) << 8) |
                        x87_bytes[0];
    
    // Initialize status word
    *status_word = 0;
    
    // Check for NaN: exponent all 1s and non-zero fraction or integer bit 0
    if (exponent == 0x7FFF && (!integer_bit || fraction != 0)) {
        *status_word |= FPU_SW_IE;  // Invalid operation
        has_exceptions = true;
        LOG_DEBUG("FPU exception: Invalid operation (NaN)");
    }
    
    // Check for infinity: exponent all 1s, integer bit 1, fraction 0
    if (exponent == 0x7FFF && integer_bit == 1 && fraction == 0) {
        *status_word |= FPU_SW_ZE;  // Zero divide (probable cause of infinity)
        has_exceptions = true;
        LOG_DEBUG("FPU exception: Zero divide (Infinity)");
    }
    
    // Check for denormal: exponent 0, non-zero fraction
    if (exponent == 0 && fraction != 0) {
        *status_word |= FPU_SW_DE;  // Denormalized operand
        has_exceptions = true;
        LOG_DEBUG("FPU exception: Denormalized operand");
    }
    
    // Handle overflow: exponent too large
    if (exponent > 0x7FFE && exponent < 0x7FFF) {
        *status_word |= FPU_SW_OE;  // Overflow
        has_exceptions = true;
        LOG_DEBUG("FPU exception: Overflow");
    }
    
    // Handle underflow: exponent too small but not zero
    if (exponent == 1 && fraction != 0) {
        *status_word |= FPU_SW_UE;  // Underflow
        has_exceptions = true;
        LOG_DEBUG("FPU exception: Underflow");
    }
    
    // We're not specifically checking for precision exceptions here
    // as they require more context about the operation
    
    return has_exceptions;
}

} // namespace fp
} // namespace xenoarm_jit 