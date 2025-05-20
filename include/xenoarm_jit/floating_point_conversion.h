#ifndef XENOARM_JIT_FLOATING_POINT_CONVERSION_H
#define XENOARM_JIT_FLOATING_POINT_CONVERSION_H

#include <cstdint>

namespace xenoarm_jit {
namespace fp {

/**
 * Converts a 32-bit IEEE-754 float to 80-bit x87 extended precision format
 *
 * @param src Pointer to the 32-bit source float
 * @param dst Pointer to the 80-bit destination buffer (10 bytes)
 */
void convert_f32_to_f80(const void* src, void* dst);

/**
 * Converts a 64-bit IEEE-754 double to 80-bit x87 extended precision format
 *
 * @param src Pointer to the 64-bit source double
 * @param dst Pointer to the 80-bit destination buffer (10 bytes)
 */
void convert_f64_to_f80(const void* src, void* dst);

/**
 * Converts an 80-bit x87 extended precision float to 32-bit IEEE-754 format
 *
 * @param src Pointer to the 80-bit source buffer (10 bytes)
 * @param dst Pointer to the 32-bit destination float
 */
void convert_f80_to_f32(const void* src, void* dst);

/**
 * Converts an 80-bit x87 extended precision float to 64-bit IEEE-754 format
 *
 * @param src Pointer to the 80-bit source buffer (10 bytes)
 * @param dst Pointer to the 64-bit destination double
 */
void convert_f80_to_f64(const void* src, void* dst);

/**
 * Performs a rounding conversion of an 80-bit x87 float based on the FPU rounding mode
 * 
 * @param src Pointer to the 80-bit source buffer (10 bytes)
 * @param dst Pointer to the 80-bit destination buffer (10 bytes)
 * @param control_word The FPU control word containing the rounding mode
 */
void apply_rounding(const void* src, void* dst, uint16_t control_word);

/**
 * Checks for FPU exceptions in the result of a floating-point operation
 * 
 * @param src Pointer to the 80-bit result
 * @param status_word Pointer to the FPU status word to update
 * @return true if exceptions occurred, false otherwise
 */
bool check_fpu_exceptions(const void* src, uint16_t* status_word);

} // namespace fp
} // namespace xenoarm_jit

#endif // XENOARM_JIT_FLOATING_POINT_CONVERSION_H 