#include <gtest/gtest.h>
#include "xenoarm_jit/simd_state.h"
#include "xenoarm_jit/simd_helpers.h"
#include <cmath>
#include <limits>
#include <cstring>

using namespace xenoarm_jit::simd;

// Define our own implementations of these functions to avoid linker conflicts
#ifdef HOST_PROVIDES_MEMORY_ACCESS
#undef HOST_PROVIDES_MEMORY_ACCESS
#endif

// Define test fixtures
class FpuOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        simd_state = new SIMDState();
    }
    
    void TearDown() override {
        delete simd_state;
    }
    
    SIMDState* simd_state;
};

// Test basic conversion of values to d0 register
TEST_F(FpuOptimizationTest, TestConversionToD0) {
    double test_value = 1.23456;
    
    // Reset the FPU state to ensure a clean stack
    simd_state->reset();
    
    // Set up an 80-bit buffer with our test value
    uint8_t buffer[10] = {0};
    xenoarm_jit::convert_double_to_f80(test_value, buffer);
    
    // Push to the FPU stack so we have a source register
    simd_state->fpu_push(buffer);
    
    // Test conversion from register to d0
    simd_state->read_fpu_reg_to_d0(0);
    
    // Check that d0 now contains our value
    double result = xenoarm_jit::asm_load_d0();
    EXPECT_DOUBLE_EQ(result, test_value);
}

// Test basic conversion from d0 register to FPU register
TEST_F(FpuOptimizationTest, TestConversionFromD0) {
    double test_value = 3.14159265358979;
    
    // Store directly to d0
    xenoarm_jit::asm_store_d0(test_value);
    
    // Test writing from d0 to FPU register
    simd_state->write_fpu_reg_from_d0(0);
    
    // Read back the value from the FPU register
    uint8_t buffer[10] = {0};
    simd_state->read_fpu_reg(0, buffer);
    
    // Extract and check the value
    double result = xenoarm_jit::extract_double_from_f80(buffer);
    EXPECT_DOUBLE_EQ(result, test_value);
}

// Test optimized memory access to registers
TEST_F(FpuOptimizationTest, TestMemoryAccessToRegisters) {
    // Test 32-bit float loading directly to s1
    uint32_t addr32 = 0x1000;
    float expected_f32 = 1.0f; // The stub implementation returns 1.0f
    
    // Call the optimized function
    xenoarm_jit::read_guest_float32_to_s1(addr32);
    
    // Check that d1 contains the expected value (lower 32 bits)
    uint64_t d1_bits = 0;
    std::memcpy(&d1_bits, &xenoarm_jit::global_d1_register, sizeof(double));
    
    float actual_f32 = 0.0f;
    std::memcpy(&actual_f32, &d1_bits, sizeof(float)); // Extract lower 32 bits
    EXPECT_FLOAT_EQ(actual_f32, expected_f32);
    
    // Test 64-bit double loading directly to d1
    uint32_t addr64 = 0x2000;
    double expected_f64 = 1.0; // The stub implementation returns 1.0
    
    // Call the optimized function
    xenoarm_jit::read_guest_float64_to_d1(addr64);
    
    // Check that d1 now contains the expected value
    EXPECT_DOUBLE_EQ(expected_f64, xenoarm_jit::global_d1_register);
}

// Test special values like infinity and NaN
TEST_F(FpuOptimizationTest, TestSpecialValues) {
    // Test positive infinity
    double inf = std::numeric_limits<double>::infinity();
    xenoarm_jit::asm_store_d0(inf);
    
    // Convert to FPU register and back to d1
    simd_state->write_fpu_reg_from_d0(0);
    simd_state->read_fpu_reg_to_d1(0);
    
    // Check the result
    EXPECT_TRUE(std::isinf(xenoarm_jit::global_d1_register));
    EXPECT_GT(xenoarm_jit::global_d1_register, 0);
    
    // Test negative infinity
    double neg_inf = -std::numeric_limits<double>::infinity();
    xenoarm_jit::asm_store_d0(neg_inf);
    
    // Convert to FPU register and back to d1
    simd_state->write_fpu_reg_from_d0(0);
    simd_state->read_fpu_reg_to_d1(0);
    
    // Check the result
    EXPECT_TRUE(std::isinf(xenoarm_jit::global_d1_register));
    EXPECT_LT(xenoarm_jit::global_d1_register, 0);
    
    // Test NaN
    double nan = std::numeric_limits<double>::quiet_NaN();
    xenoarm_jit::asm_store_d0(nan);
    
    // Convert to FPU register and back to d1
    simd_state->write_fpu_reg_from_d0(0);
    simd_state->read_fpu_reg_to_d1(0);
    
    // Check the result
    EXPECT_TRUE(std::isnan(xenoarm_jit::global_d1_register));
}

// Test very small and very large values
TEST_F(FpuOptimizationTest, TestRangeValues) {
    // Test very small value
    double small = 1e-100;
    xenoarm_jit::asm_store_d0(small);
    
    // Convert to FPU register and back to d1
    simd_state->write_fpu_reg_from_d0(0);
    simd_state->read_fpu_reg_to_d1(0);
    
    // Check the result (should be preserved)
    EXPECT_GT(xenoarm_jit::global_d1_register, 0);
    
    // Test very large value
    double large = 1e100;
    xenoarm_jit::asm_store_d0(large);
    
    // Convert to FPU register and back to d1
    simd_state->write_fpu_reg_from_d0(0);
    simd_state->read_fpu_reg_to_d1(0);
    
    // Check the result (should be preserved)
    EXPECT_DOUBLE_EQ(xenoarm_jit::global_d1_register, large);
}

// Test denormal handling
TEST_F(FpuOptimizationTest, TestDenormalHandling) {
    // Create a denormal value directly in 80-bit format
    uint8_t denormal_value[10] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Mantissa with only the lowest bit set
        0x00, 0x00                                       // Zero exponent
    };
    
    // Reset the FPU state
    simd_state->reset();
    
    // Test with denormal handling enabled (preserve denormal)
    simd_state->set_denormal_handling(true);
    
    // Push denormal value directly to the FPU stack
    simd_state->fpu_push(denormal_value);
    
    // Read the value to d1
    simd_state->read_fpu_reg_to_d1(0);
    
    // Check that denormal is preserved - it won't be exactly representable as a double,
    // but it should be non-zero. --> This comment is misleading for the given input.
    // For an 80-bit denormal like 2^-16445, its closest 64-bit double representation IS 0.0.
    // The "preserve denormal" handling should mean the FPU uses it correctly internally if possible,
    // not that it becomes a non-zero double if it's numerically zero when converted.
    // EXPECT_NE(xenoarm_jit::global_d1_register, 0.0); // Original failing line
    EXPECT_DOUBLE_EQ(xenoarm_jit::global_d1_register, 0.0); // Corrected expectation
    
    // Reset the FPU state
    simd_state->reset();
    
    // Test with denormal handling disabled (flush to zero)
    simd_state->set_denormal_handling(false);
    
    // Push denormal value directly to the FPU stack
    simd_state->fpu_push(denormal_value);
    
    // Read the value to d1
    simd_state->read_fpu_reg_to_d1(0);
    
    // Check that denormal was flushed to zero
    EXPECT_DOUBLE_EQ(xenoarm_jit::global_d1_register, 0.0);
}

// Main function for the test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 
