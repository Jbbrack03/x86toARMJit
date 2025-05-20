#include "xenoarm_jit/simd_helpers.h"
#include "xenoarm_jit/simd_state.h"
#include "logging/logger.h"
#include "xenoarm_jit/fpu_transcendental_helpers.h"  // Include for is_nan_f80, is_infinity_f80, is_zero_f80
#include <cstring>
#include <limits>
#include <cmath>

namespace xenoarm_jit {

// Global variables to simulate direct register access
// In a real implementation, these would be actual ARM NEON registers
double global_d0_register = 0.0;
double global_d1_register = 0.0;

// Store a double value directly to NEON register d0
void asm_store_d0(double value) {
    // In a real implementation, this would be an assembly function
    global_d0_register = value;
}

// Load a double value from NEON register d0
double asm_load_d0() {
    // In a real implementation, this would be an assembly function
    LOG_DEBUG("Loaded " + std::to_string(global_d0_register) + " from d0");
    return global_d0_register;
}

// Store a double value directly to NEON register d1
void asm_store_d1(double value) {
    // In a real implementation, this would be an assembly function
    global_d1_register = value;
    LOG_DEBUG("Stored " + std::to_string(value) + " to d1");
}

// Load a double value from NEON register d1
double asm_load_d1() {
    // In a real implementation, this would be an assembly function
    LOG_DEBUG("Loaded " + std::to_string(global_d1_register) + " from d1");
    return global_d1_register;
}

// Store a float value directly to NEON register s1 (lower half of d1)
void asm_store_s1(float value) {
    // In a real implementation, this would be an assembly function
    // For simulating, we'll just store it in the lower 32 bits of d1
    uint64_t bits;
    std::memcpy(&bits, &global_d1_register, sizeof(double));
    
    uint32_t float_bits;
    std::memcpy(&float_bits, &value, sizeof(float));
    
    // Replace lower 32 bits
    bits = (bits & 0xFFFFFFFF00000000ULL) | float_bits;
    
    std::memcpy(&global_d1_register, &bits, sizeof(double));
    LOG_DEBUG("Stored float " + std::to_string(value) + " to s1");
}

// Convert a floating point value from 80-bit extended precision to double
double extract_double_from_f80(const uint8_t* src) {
    // Check for special cases first
    if (is_nan_f80(src)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (is_infinity_f80(src)) {
        // Extract sign bit and return appropriate infinity
        bool negative = (src[9] & 0x80) != 0;
        return negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }
    if (is_zero_f80(src)) {
        // Extract sign bit and return appropriate zero
        bool negative = (src[9] & 0x80) != 0;
        return negative ? -0.0 : 0.0;
    }
    
    // Extract the sign bit (bit 79)
    bool sign = (src[9] & 0x80) != 0;
    
    // Extract the exponent (bits 64-78)
    uint16_t exponent_bits;
    std::memcpy(&exponent_bits, src + 8, 2);
    exponent_bits &= 0x7FFF; // Mask out the sign bit
    
    // Extract the significand (bits 0-63)
    uint64_t significand;
    std::memcpy(&significand, src, 8);
    
    // Adjust the exponent bias from 16383 to 1023 for double precision
    int16_t adjusted_exponent = exponent_bits - 16383 + 1023;
    
    // Handle underflow and overflow in the conversion
    if (adjusted_exponent < 0) {
        // Underflow to zero (or denormal, but we're simplifying to zero)
        return sign ? -0.0 : 0.0;
    }
    if (adjusted_exponent > 2047) {
        // Overflow to infinity
        return sign ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }
    
    // Shift the significand right by 11 bits (80-bit uses 64-bit significand, double uses 53-bit)
    // First mask out the integer bit (bit 63) which is explicit in 80-bit but implicit in double
    significand &= 0x7FFFFFFFFFFFFFFF;
    
    // Shift right to convert to the 52-bit double precision significand
    significand >>= 11;
    
    // Construct the IEEE 754 double precision value
    uint64_t double_bits = 0;
    
    // Set the sign bit (bit 63)
    if (sign) {
        double_bits |= (1ULL << 63);
    }
    
    // Set the exponent bits (bits 52-62)
    double_bits |= static_cast<uint64_t>(adjusted_exponent) << 52;
    
    // Set the significand bits (bits 0-51)
    double_bits |= significand;
    
    // Convert the bit pattern to a double
    double result;
    std::memcpy(&result, &double_bits, 8);
    
    return result;
}

// Convert double to 80-bit float format
void convert_double_to_f80(double value, uint8_t* buffer) {
    // Handle special cases
    if (std::isnan(value)) {
        // Generate a quiet NaN
        std::memset(buffer, 0, 10);
        // Set the significand to a non-zero value (a canonical quiet NaN)
        buffer[0] = 0x01;
        // Set the exponent to all 1s (0x7FFF)
        buffer[8] = 0xFF;
        buffer[9] = 0x7F;
        return;
    }
    
    if (std::isinf(value)) {
        // Generate an infinity with the appropriate sign
        std::memset(buffer, 0, 10);
        // Set the exponent to all 1s (0x7FFF)
        buffer[8] = 0xFF;
        buffer[9] = 0x7F;
        // Set the sign bit if negative
        if (value < 0) {
            buffer[9] |= 0x80;
        }
        return;
    }
    
    if (value == 0.0) {
        // Generate a zero with the appropriate sign
        std::memset(buffer, 0, 10);
        // Set the sign bit if negative zero
        if (std::signbit(value)) {
            buffer[9] |= 0x80;
        }
        return;
    }
    
    // Extract the sign bit
    bool sign = std::signbit(value);
    
    // Get the absolute value to work with
    double abs_value = std::abs(value);
    
    // Extract the bit pattern of the double
    uint64_t double_bits;
    std::memcpy(&double_bits, &abs_value, 8);
    
    // Extract the exponent and significand from the double
    uint64_t significand = double_bits & 0x000FFFFFFFFFFFFF; // Bits 0-51
    uint16_t exponent = (double_bits >> 52) & 0x7FF;        // Bits 52-62
    
    // Convert to 80-bit format
    
    // Adjust exponent bias (80-bit uses 16383, double uses 1023)
    uint16_t adjusted_exponent = exponent - 1023 + 16383;
    
    // The significand - explicit integer bit (set to 1 for normalized values)
    // and shift to fit in the 64-bit significand field of x87 format
    uint64_t extended_significand = (significand << 11) | (1ULL << 63);
    
    // Handle denormals in double format (exponent == 0)
    if (exponent == 0) {
        // No implicit 1 bit for denormals, just shift the significand
        extended_significand = significand << 11;
        // Use the smallest valid exponent for 80-bit format
        adjusted_exponent = 0;
    }
    
    // Store the significand (bytes 0-7)
    std::memcpy(buffer, &extended_significand, 8);
    
    // Store the exponent and sign (bytes 8-9)
    uint16_t exponent_sign = adjusted_exponent;
    if (sign) {
        exponent_sign |= 0x8000; // Set the sign bit
    }
    
    std::memcpy(buffer + 8, &exponent_sign, 2);
}

// Convert 80-bit float to double and store directly in d0
void convert_f80_to_d0(const uint8_t* src) {
    // Extract the value and store to d0
    double value = extract_double_from_f80(src);
    asm_store_d0(value);
}

// Convert double in d0 to 80-bit float
void convert_d0_to_f80(uint8_t* dst) {
    double value = asm_load_d0();
    convert_double_to_f80(value, dst);
}

// Read a 32-bit float from guest memory directly into s1
void read_guest_float32_to_s1(uint32_t address) {
    // In a real implementation, this would call the memory read function
    // and store the result directly to s1
    float value = read_guest_float32(address);
    
    // Store to s1 (d1 holds two single-precision values)
    asm_store_s1(value);
    LOG_DEBUG("Read float32 " + std::to_string(value) + " from address " + std::to_string(address) + " to s1");
}

// Read a 64-bit double from guest memory directly into d1
void read_guest_float64_to_d1(uint32_t address) {
    // In a real implementation, this would call the memory read function
    // and store the result directly to d1
    double value = read_guest_float64(address);
    asm_store_d1(value);
    LOG_DEBUG("Read float64 " + std::to_string(value) + " from address " + std::to_string(address) + " to d1");
}

// Optimized FPU operations using ARM NEON registers directly
// These would normally be implemented as inline assembly or intrinsics for optimal performance

// Add d0 + d1 -> d0
void asm_fadd_d0_d1() {
    // In a real implementation with ARM assembly, this would be:
    // "FADD d0, d0, d1"
    
    // For our simulated implementation:
    double result = global_d0_register + global_d1_register;
    global_d0_register = result;
    
    LOG_DEBUG("FADD d0, d0, d1: " + std::to_string(result));
}

// Subtract d0 - d1 -> d0
void asm_fsub_d0_d1() {
    // In a real implementation with ARM assembly, this would be:
    // "FSUB d0, d0, d1"
    
    // For our simulated implementation:
    double result = global_d0_register - global_d1_register;
    global_d0_register = result;
    
    LOG_DEBUG("FSUB d0, d0, d1: " + std::to_string(result));
}

// Multiply d0 * d1 -> d0
void asm_fmul_d0_d1() {
    // In a real implementation with ARM assembly, this would be:
    // "FMUL d0, d0, d1"
    
    // For our simulated implementation:
    double result = global_d0_register * global_d1_register;
    global_d0_register = result;
    
    LOG_DEBUG("FMUL d0, d0, d1: " + std::to_string(result));
}

// Divide d0 / d1 -> d0
void asm_fdiv_d0_d1() {
    // In a real implementation with ARM assembly, this would be:
    // "FDIV d0, d0, d1"
    
    // For our simulated implementation:
    double result = global_d0_register / global_d1_register;
    global_d0_register = result;
    
    LOG_DEBUG("FDIV d0, d0, d1: " + std::to_string(result));
}

// Square root of d0 -> d0
void asm_fsqrt_d0() {
    // In a real implementation with ARM assembly, this would be:
    // "FSQRT d0, d0"
    
    // For our simulated implementation:
    double result = std::sqrt(global_d0_register);
    global_d0_register = result;
    
    LOG_DEBUG("FSQRT d0: " + std::to_string(result));
}

// Implementing read_guest_float functions that are needed by the tests
// These would normally be provided by the host environment
#ifndef HOST_PROVIDES_MEMORY_ACCESS
extern "C" {
    float read_guest_float32(uint32_t address) {
        // Stub implementation returning a fixed value
        LOG_DEBUG("Reading float32 from guest address: " + std::to_string(address));
        return 1.0f;
    }

    double read_guest_float64(uint32_t address) {
        // Stub implementation returning a fixed value
        LOG_DEBUG("Reading float64 from guest address: " + std::to_string(address));
        return 1.0;
    }
}
#endif // HOST_PROVIDES_MEMORY_ACCESS

// Check if a double value is denormal according to IEEE-754
bool is_denormal(double value) {
    // Denormal numbers are non-zero, very small values where the exponent is zero
    
    // Get the raw bits of the double
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    
    // Extract the exponent bits (11 bits starting from bit 52)
    uint64_t exponent = (bits >> 52) & 0x7FF;
    
    // Extract the mantissa bits (lower 52 bits)
    uint64_t mantissa = bits & 0xFFFFFFFFFFFFF;
    
    // Check if exponent is 0 (denormal) and mantissa is non-zero
    return (exponent == 0 && mantissa != 0);
}

// Apply precision control to an 80-bit floating point value based on FPU control word
void apply_precision_control_f80(uint8_t* value, uint16_t control_word) {
    // Extract the precision control bits from the control word (bits 8-9)
    uint16_t precision_control = (control_word >> 8) & 0x03;
    
    // Skip if set to extended precision (PC=11), as no change is needed
    if (precision_control == 0x03) {
        return;
    }
    
    // Convert to appropriate precision and back
    double temp_value = extract_double_from_f80(value);
    
    if (precision_control == 0x00) {
        // Single precision (PC=00)
        float single_value = static_cast<float>(temp_value);
        temp_value = static_cast<double>(single_value);
    }
    // Double precision (PC=10) doesn't need further conversion since we're already using double

    // Convert back to 80-bit format
    convert_double_to_f80(temp_value, value);
}

} // namespace xenoarm_jit 