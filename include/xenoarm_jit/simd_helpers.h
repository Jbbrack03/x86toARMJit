#ifndef XENOARM_JIT_SIMD_HELPERS_H
#define XENOARM_JIT_SIMD_HELPERS_H

#include <cstdint>
#include <array>

namespace xenoarm_jit {

// ARM NEON register direct access functions
// These functions would typically be implemented in assembly
// For this implementation, we simulate them with global variables

// Store/load functions for ARM NEON registers
void asm_store_d0(double value);
double asm_load_d0();
void asm_store_d1(double value);
double asm_load_d1();
void asm_store_s1(float value);

// Conversion functions between 80-bit x87 format and double precision
void convert_f80_to_d0(const uint8_t* src);
void convert_d0_to_f80(uint8_t* dst);
double extract_double_from_f80(const uint8_t* buffer);
void convert_double_to_f80(double value, uint8_t* buffer);

// Optimized memory access functions that read directly into ARM NEON registers
void read_guest_float32_to_s1(uint32_t address);
void read_guest_float64_to_d1(uint32_t address);

// Optimized FPU operations using ARM NEON registers
// These operate directly on ARM NEON registers d0 and d1
void asm_fadd_d0_d1(); // d0 = d0 + d1
void asm_fsub_d0_d1(); // d0 = d0 - d1
void asm_fmul_d0_d1(); // d0 = d0 * d1
void asm_fdiv_d0_d1(); // d0 = d0 / d1
void asm_fsqrt_d0();   // d0 = sqrt(d0)

// Memory access functions (declared here, implemented by host)
extern "C" float read_guest_float32(uint32_t address);
extern "C" double read_guest_float64(uint32_t address);

// Global variables used to simulate NEON registers
extern double global_d0_register;
extern double global_d1_register;

// Check if a 80-bit floating point value is NaN (Not-a-Number)
bool is_nan_f80(const uint8_t* src);

// Check if a 80-bit floating point value is infinity
bool is_infinity_f80(const uint8_t* src);

// Check if a 80-bit floating point value is zero (positive or negative)
bool is_zero_f80(const uint8_t* src);

// Check if a 80-bit floating point value is denormal
bool is_denormal_f80(const uint8_t* src);

// Check if a double value is denormal according to IEEE-754 
bool is_denormal(double value);

// Convert a 64-bit double value to 80-bit extended precision
void convert_double_to_f80(double value, uint8_t* dst);

// Extract a double value from 80-bit extended precision format
double extract_double_from_f80(const uint8_t* src);

// Apply precision control to an 80-bit floating point value based on FPU control word
void apply_precision_control_f80(uint8_t* value, uint16_t control_word);

} // namespace xenoarm_jit

#endif // XENOARM_JIT_SIMD_HELPERS_H 