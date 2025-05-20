#ifndef XENOARM_JIT_FPU_TRANSCENDENTAL_HELPERS_H
#define XENOARM_JIT_FPU_TRANSCENDENTAL_HELPERS_H

#include <cstdint>

namespace xenoarm_jit {

// Helper functions for FPU transcendental operations
// These functions are used to implement the x87 FPU transcendental instructions
// They operate on 80-bit extended precision floating point values
// In a real implementation, these would be optimized assembly routines

// Sine computation helpers
void compute_sine_f80(const uint8_t* src, uint8_t* dst);
void compute_sine_large_f80(const uint8_t* src, uint8_t* dst);

// Cosine computation helpers
void compute_cosine_f80(const uint8_t* src, uint8_t* dst);
void compute_cosine_large_f80(const uint8_t* src, uint8_t* dst);

// Tangent computation helpers
void compute_tangent_f80(const uint8_t* src, uint8_t* dst);
bool compute_tangent_f80_with_status(const uint8_t* src, uint8_t* dst, uint16_t* status_flags);

// 2^x-1 computation helper
void compute_2_to_x_minus_1_f80(const uint8_t* src, uint8_t* dst);

// y * log2(x) computation helper
void compute_y_log2_x_f80(const uint8_t* x_src, const uint8_t* y_src, uint8_t* dst);

// Helper functions to load special values
void load_fpu_qnan(uint8_t* dst);
void load_fpu_positive_infinity(uint8_t* dst);
void load_fpu_negative_infinity(uint8_t* dst);
void load_fpu_constant_1(uint8_t* dst);
void load_fpu_minus_1(uint8_t* dst);
void load_fpu_constant_pi(uint8_t* dst);
void load_fpu_constant_pi_half(uint8_t* dst);
void load_fpu_constant_pi_quarter(uint8_t* dst);
void load_fpu_constant_ln2(uint8_t* dst);
void load_fpu_constant_lg2e(uint8_t* dst);

// FPU status and exception handling helpers
void set_fpu_c2_flag(uint16_t flag_value);
void set_fpu_c1_flag(uint16_t flag_value);
void set_fpu_c0_flag(uint16_t flag_value);
void set_fpu_c3_flag(uint16_t flag_value);
void handle_fpu_exception(uint16_t exception_flags);

// IEEE-754 special value checks
bool is_nan_f80(const uint8_t* src);
bool is_infinity_f80(const uint8_t* src);
bool is_positive_infinity_f80(const uint8_t* src); 
bool is_negative_infinity_f80(const uint8_t* src);
bool is_denormal_f80(const uint8_t* src);
bool is_zero_f80(const uint8_t* src);

// FPU exception flag defines - renamed to avoid conflicts with math.h macros
enum FPUExceptionFlags {
    FPU_PRECISION = 0x20,      // Precision flag
    FPU_UNDERFLOW = 0x10,      // Underflow flag
    FPU_OVERFLOW = 0x08,       // Overflow flag
    FPU_ZERODIVIDE = 0x04,     // Divide by zero flag
    FPU_DENORMAL = 0x02,       // Denormal operand flag
    FPU_INVALID = 0x01         // Invalid operation flag
};

// Enhanced precision control helpers
void apply_precision_control_f80(uint8_t* value, uint16_t control_word);
void apply_rounding_mode_f80(uint8_t* value, uint16_t control_word);
void handle_denormal_value_f80(uint8_t* value, uint16_t control_word);

} // namespace xenoarm_jit

#endif // XENOARM_JIT_FPU_TRANSCENDENTAL_HELPERS_H 