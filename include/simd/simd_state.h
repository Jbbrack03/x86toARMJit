#ifndef XENOARM_JIT_SIMD_STATE_H
#define XENOARM_JIT_SIMD_STATE_H

#include <cstdint>
#include <array>
#include <vector>

namespace xenoarm_jit {
namespace simd {

// Define the FPU exception flags
enum FPUExceptionFlags {
    FPU_INVALID = 0x0001,    // Invalid operation
    FPU_DENORMAL = 0x0002,   // Denormal operand
    FPU_ZERO_DIV = 0x0004,   // Divide by zero
    FPU_OVERFLOW = 0x0008,   // Numeric overflow
    FPU_UNDERFLOW = 0x0010,  // Numeric underflow
    FPU_PRECISION = 0x0020   // Inexact result (precision)
};

// Define the tag values for x87 registers
enum class X87TagStatus : uint8_t {
    VALID = 0,    // Valid number
    ZERO = 1,     // Zero
    SPECIAL = 2,  // NaN, infinity, or denormal
    EMPTY = 3     // Empty register
};

// Define the different SIMD operation modes
enum class SIMDMode {
    FPU,  // x87 FPU mode
    MMX   // MMX mode
};

// Structure for an x87 register
struct X87Register {
    uint8_t data[10]; // 80-bit floating point value
    X87TagStatus tag;
};

// Class to manage MMX, SSE, and x87 FPU state
class SIMDState {
public:
    SIMDState();
    ~SIMDState();

    // Reset all state
    void reset();

    // XMM register access
    void read_xmm_reg(uint8_t reg_idx, uint8_t* data);
    void write_xmm_reg(uint8_t reg_idx, const uint8_t* data);

    // MMX register access
    void read_mmx_reg(uint8_t reg_idx, uint8_t* data);
    void write_mmx_reg(uint8_t reg_idx, const uint8_t* data);

    // FPU register access
    void read_fpu_reg(uint8_t logical_reg_idx, uint8_t* data);
    void write_fpu_reg(uint8_t logical_reg_idx, const uint8_t* data);

    // FPU stack operations
    void fpu_push(const uint8_t* data = nullptr);
    void fpu_pop(uint8_t* data = nullptr);
    
    // Optimized ARM-specific FPU operations
    // Pop the top value from stack without storing the result
    void pop_without_result();
    
    // Push a value from ARM d0 register directly to the FPU stack
    void push_from_d0();
    
    // Push a value from ARM d1 register directly to the FPU stack
    void push_from_d1();
    
    // Update FPU status word with specified flags
    void update_status_word_flags(uint16_t flags);

    // FPU status/control word access
    uint16_t get_fpu_status_word() const;
    void set_fpu_status_word(uint16_t status);
    uint16_t get_fpu_control_word() const;
    void set_fpu_control_word(uint16_t control);
    uint16_t get_fpu_tag_word() const;
    void set_fpu_tag_word(uint16_t tag);

    // FPU stack pointer access
    uint8_t get_fpu_top() const;
    void set_fpu_top(uint8_t top);

    // FPU tag access
    X87TagStatus get_tag_for_register(uint8_t physical_reg_idx) const;
    void set_tag_for_register(uint8_t physical_reg_idx, X87TagStatus tag);

    // Conversion between logical and physical register indices
    uint8_t logical_to_physical(uint8_t logical_idx) const;
    uint8_t physical_to_logical(uint8_t physical_idx) const;

    // Mode switching
    void switch_to_fpu_mode();
    void switch_to_mmx_mode();
    SIMDMode get_current_mode() const;

    // Tag word maintenance
    void update_tag(uint8_t reg_idx);

    // FPU computational operations
    void compute_sine();
    void compute_cosine();
    void compute_tangent();
    bool compute_tangent_with_status(double input);
    void compute_2_to_x_minus_1();
    void compute_y_log2_x();
    
    // FPU control word operations
    void set_precision_control(uint8_t pc_value);
    void set_rounding_mode(uint8_t mode);
    void set_denormal_handling(bool handle_as_normal);
    void set_flush_denormals_to_zero(bool flush_to_zero);
    
    // Denormal handling
    bool is_denormal(double value);
    void handle_denormal_input(int stack_position);
    void apply_precision_control(int stack_position);
    
    // Optimized register access for ARM
    void read_fpu_reg_to_d0(uint8_t logical_reg_idx);
    void read_fpu_reg_to_d1(uint8_t logical_reg_idx);
    void write_fpu_reg_from_d0(uint8_t logical_reg_idx);
    void write_fpu_reg_from_d1(uint8_t logical_reg_idx);
    
    // Helper for extracting double from register
    double extract_double_from_reg(uint8_t physical_idx);
    
    // Public access to registers for testing
    std::array<X87Register, 8> x87_registers;
    uint8_t xmm_registers[8][16];
    
    // FPU control state
    uint16_t fpu_control_word;
    uint16_t fpu_status_word;
    uint16_t fpu_tag_word;
    uint8_t fpu_top;
    
private:
    // Current SIMD operation mode
    SIMDMode current_mode;
};

// External declarations for helper functions
extern uint16_t fpu_status_word;
extern uint16_t fpu_control_word;

} // namespace simd
} // namespace xenoarm_jit

#endif // XENOARM_JIT_SIMD_STATE_H 