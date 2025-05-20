#ifndef XENOARM_JIT_SIMD_STATE_H
#define XENOARM_JIT_SIMD_STATE_H

#include <cstdint>
#include <array>
#include <string>

namespace xenoarm_jit {
namespace tests {
    // Forward declaration for friendship
    class FPUTranscendentalTest;
}

namespace simd {

// FPU control and status word - accessible to other modules
extern uint16_t fpu_status_word;
extern uint16_t fpu_control_word;

// Structure to hold a register value (XMM, MMX, or FPU)
struct RegisterValue {
    uint8_t data[16]; // 128 bits for XMM registers (MMX and FPU use fewer bits)
    
    // Constructor to initialize to zero
    RegisterValue() {
        for (int i = 0; i < 16; i++) {
            data[i] = 0;
        }
    }
};

// X87 FPU tag word status values
enum class X87TagStatus {
    VALID = 0,
    ZERO = 1,
    SPECIAL = 2, // NaN, infinity, denormal
    EMPTY = 3
};

// X87 FPU data register
struct X87Register {
    uint8_t data[10]; // 80-bit floating point value in internal x87 format
    X87TagStatus tag;
    
    X87Register() : tag(X87TagStatus::EMPTY) {
        for (int i = 0; i < 10; i++) {
            data[i] = 0;
        }
    }
};

// CPU mode for MMX/FPU state
enum class SIMDMode {
    FPU,    // Using x87 FPU
    MMX,    // Using MMX
    MIXED   // Mixed state (transitioning)
};

// SIMD state manager for MMX, SSE, and x87 FPU registers
// Handles the aliasing between MM0-MM7 and the x87 FPU stack
class SIMDState {
public:
    SIMDState();
    ~SIMDState();
    
    // FPU control word access
    uint16_t get_fpu_control_word() const { return fpu_control_word; }
    void set_fpu_control_word(uint16_t value) { fpu_control_word = value; }
    
    // FPU status word access
    uint16_t get_fpu_status_word() const { return fpu_status_word; }
    void set_fpu_status_word(uint16_t value) { fpu_status_word = value; }
    
    // Get x87 top-of-stack pointer
    uint8_t get_fpu_top() const { return fpu_top; }
    
    // Set x87 top-of-stack pointer
    void set_fpu_top(uint8_t top) {
        // Update the internal member variable
        fpu_top = top & 0x7;
        
        // Update the status word bits 11-13
        fpu_status_word &= ~(0x7 << 11);
        fpu_status_word |= (fpu_top & 0x7) << 11;
    }
    
    // Phase 8: FPU control settings
    void set_precision_control(uint8_t pc_value);
    void set_denormal_handling(bool handle_as_normal);
    void set_rounding_mode(uint8_t mode);
    
    // Phase 8: Additional methods for testing
    bool is_denormal(double value);
    void set_flush_denormals_to_zero(bool flush_to_zero);
    void handle_denormal_input(int stack_position);
    void apply_precision_control(int stack_position);
    void apply_rounding_mode();
    void round_to_integer();
    
    // Phase 8: Transcendental function helpers for testing
    void compute_sine();
    void compute_cosine();
    bool compute_tangent_with_status(double input);
    void compute_tangent();
    void compute_2_to_x_minus_1();
    void compute_y_log2_x();
    
    // Phase 8: Double precision helpers for testing
    void push_double(double value);
    double pop_double();
    
    // Convert logical register index to physical stack position
    uint8_t logical_to_physical(uint8_t logical_index) const {
        return (get_fpu_top() + logical_index) & 0x7;
    }
    
    // Register access methods for SSE
    void read_xmm_reg(uint8_t reg_idx, uint8_t* data);
    void write_xmm_reg(uint8_t reg_idx, const uint8_t* data);
    
    // Register access methods for MMX
    void read_mmx_reg(uint8_t reg_idx, uint8_t* data);
    void write_mmx_reg(uint8_t reg_idx, const uint8_t* data);
    
    // Register access methods for x87 FPU (80-bit)
    void read_fpu_reg(uint8_t reg_idx, uint8_t* data);
    void write_fpu_reg(uint8_t reg_idx, const uint8_t* data);
    
    // Push value onto FPU stack
    void fpu_push(const uint8_t* data);
    
    // Pop value from FPU stack
    void fpu_pop(uint8_t* data = nullptr);
    
    // Get current mode (FPU, MMX, or MIXED)
    SIMDMode get_mode() const { return current_mode; }
    
    // Switch to MMX mode (when executing an MMX instruction)
    void switch_to_mmx_mode();
    
    // Switch to FPU mode (when executing an FPU instruction)
    void switch_to_fpu_mode();
    
    // Reset the SIMD state
    void reset();
    
    // FPU tag word access
    uint16_t get_fpu_tag_word() const { return fpu_tag_word; }
    void set_fpu_tag_word(uint16_t tw) { fpu_tag_word = tw; }
    
    // Update the tag for a specific register
    void update_tag(uint8_t reg_idx);
    
    // Phase 8 completion: ARM Optimization - Direct FPU register access
    // These functions optimize FPU register access by directly working with
    // ARM NEON registers (d0, d1, etc.)
    void read_fpu_reg_to_d0(uint8_t logical_reg_idx);
    void read_fpu_reg_to_d1(uint8_t logical_reg_idx);
    void write_fpu_reg_from_d0(uint8_t logical_reg_idx);
    void write_fpu_reg_from_d1(uint8_t logical_reg_idx);
    
    // Phase 8 completion: Enhanced stack operations with direct register access
    void pop_without_result();      // Pop without returning the value, just adjust stack
    void push_from_d0();           // Push value from d0 register onto FPU stack
    void push_from_d1();           // Push value from d1 register onto FPU stack
    void update_status_word_flags(uint16_t flags); // Update status word with specific flags
    
    // External function declarations for memory access optimizations
    // These are implemented in the simd_helpers.cpp file
    friend void convert_f80_to_d0(const uint8_t* src);
    friend void convert_d0_to_f80(uint8_t* dst);
    friend void read_guest_float32_to_s1(uint32_t address);
    friend void read_guest_float64_to_d1(uint32_t address);

    // Extract a double value directly from a register without popping it
    double extract_double_from_reg(uint8_t physical_idx);
    
    // Friend class declarations for testing
    friend class xenoarm_jit::tests::FPUTranscendentalTest;

public:
    // Make these accessible for testing
    std::array<X87Register, 8> x87_registers;
    uint16_t fpu_tag_word;

private:
    // Current mode of operation
    SIMDMode current_mode;
    
    // x87 FPU control and status words
    uint16_t fpu_control_word;
    uint16_t fpu_status_word;
    
    // SSE XMM registers (8 registers for Original Xbox, each 128 bits or 16 bytes)
    std::array<uint8_t[16], 8> xmm_registers;
    
    // FPU status
    uint8_t fpu_top;           // Top of stack pointer (0-7)
};

} // namespace simd
} // namespace xenoarm_jit

#endif // XENOARM_JIT_SIMD_STATE_H 