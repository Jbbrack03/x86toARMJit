#include <gtest/gtest.h>
#include <cstring>
#include "xenoarm_jit/exception_handler.h"
#include "xenoarm_jit/floating_point_conversion.h"

namespace xenoarm_jit {
namespace tests {

// Global exception information for testing
static uint32_t last_exception_vector = 0;
static uint32_t last_error_code = 0;

// Test exception callback
void test_exception_callback(uint32_t exception_vector, uint32_t error_code) {
    last_exception_vector = exception_vector;
    last_error_code = error_code;
}

class FPUExceptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register exception callback
        ExceptionHandler::set_exception_callback(test_exception_callback);
        
        // Reset globals
        last_exception_vector = 0;
        last_error_code = 0;
    }
    
    void TearDown() override {
        // Reset globals
        last_exception_vector = 0;
        last_error_code = 0;
    }
    
    // Helper to create test FP values for testing exceptions
    enum FloatType {
        NORMAL,
        ZERO,
        INF,  // Renamed from INFINITY to avoid macro conflict
        NAN_VALUE,  // Renamed from NAN to avoid macro conflict
        DENORMAL
    };
    
    void create_test_value(void* dst, FloatType type, bool sign = false) {
        uint8_t* dest = static_cast<uint8_t*>(dst);
        
        // Initialize with zeros
        std::memset(dest, 0, 10);
        
        switch (type) {
            case NORMAL:
                // Create a normal value: 1.0
                // Format: sign(1) | exponent(15) | integer(1) | fraction(63)
                // For 1.0: sign=0, exponent=0x3FFF, integer=1, fraction=0
                dest[7] = 0x80;  // Integer bit set to 1
                dest[8] = 0xFF;  // Exponent - LSB
                dest[9] = 0x3F;  // Exponent - MSB
                if (sign) dest[9] |= 0x80;  // Set sign bit if needed
                break;
                
            case ZERO:
                // Zero - already handled by memset, all zeros
                if (sign) dest[9] |= 0x80;  // Set sign bit if needed
                break;
                
            case INF:
                // Infinity: exponent all 1s, integer bit 1, fraction all 0s
                // Format: sign(1) | exponent(0x7FFF) | integer(1) | fraction(0)
                dest[7] = 0x80;  // Integer bit set to 1
                dest[8] = 0xFF;  // Exponent - LSB
                dest[9] = 0x7F;  // Exponent - MSB
                if (sign) dest[9] |= 0x80;  // Set sign bit if needed
                break;
                
            case NAN_VALUE:
                // NaN: exponent all 1s, integer bit set, non-zero fraction
                // Format: sign(1) | exponent(0x7FFF) | integer(1) | fraction(non-zero)
                dest[0] = 0x01;  // Set a bit in the fraction
                dest[7] = 0x80;  // Integer bit set
                dest[8] = 0xFF;  // Exponent - LSB
                dest[9] = 0x7F;  // Exponent - MSB
                if (sign) dest[9] |= 0x80;  // Set sign bit if needed
                break;
                
            case DENORMAL:
                // Denormal: exponent all 0s, integer bit 0, non-zero fraction
                // Format: sign(1) | exponent(0) | integer(0) | fraction(non-zero)
                dest[0] = 0x01;  // Set a bit in the fraction
                // Integer bit and exponent are already 0 from memset
                if (sign) dest[9] |= 0x80;  // Set sign bit if needed
                break;
        }
    }
};

// Test handling invalid operations
TEST_F(FPUExceptionTest, InvalidOperation) {
    // Create a NaN value
    uint8_t nan_value[10];
    create_test_value(nan_value, NAN_VALUE);
    
    // Set up status word
    uint16_t status_word = 0;
    
    // Check for exceptions
    bool has_exceptions = fp::check_fpu_exceptions(nan_value, &status_word);
    
    // Verify that exception was detected
    EXPECT_TRUE(has_exceptions);
    EXPECT_TRUE(status_word & 0x0001); // IE flag
    
    // Report the exception
    uint32_t test_eip = 0x12345678;
    EXPECT_TRUE(ExceptionHandler::report_fpu_exception(test_eip, status_word));
    
    // Check if the callback was called correctly
    EXPECT_EQ(last_exception_vector, 16); // Vector 16 for #MF
    EXPECT_EQ(last_error_code, status_word);
    EXPECT_EQ(ExceptionHandler::get_last_faulting_address(), test_eip);
}

// Test handling divide by zero
TEST_F(FPUExceptionTest, DivideByZero) {
    // Create infinity (result of divide by zero)
    uint8_t inf_value[10];
    create_test_value(inf_value, INF);
    
    // Set up status word
    uint16_t status_word = 0;
    
    // Check for exceptions
    bool has_exceptions = fp::check_fpu_exceptions(inf_value, &status_word);
    
    // Verify exception detection
    EXPECT_TRUE(has_exceptions);
    EXPECT_TRUE(status_word & 0x0004); // ZE flag
    
    // Report the exception
    uint32_t test_eip = 0x87654321;
    EXPECT_TRUE(ExceptionHandler::report_fpu_exception(test_eip, status_word));
    
    // Check callback
    EXPECT_EQ(last_exception_vector, 16);
    EXPECT_EQ(last_error_code, status_word);
}

// Test denormal operand exception
TEST_F(FPUExceptionTest, DenormalOperand) {
    // Create a denormal value
    uint8_t denorm_value[10];
    create_test_value(denorm_value, DENORMAL);
    
    // Set up status word
    uint16_t status_word = 0;
    
    // Check for exceptions
    bool has_exceptions = fp::check_fpu_exceptions(denorm_value, &status_word);
    
    // Verify exception detection
    EXPECT_TRUE(has_exceptions);
    EXPECT_TRUE(status_word & 0x0002); // DE flag
    
    // Report the exception
    uint32_t test_eip = 0x11223344;
    EXPECT_TRUE(ExceptionHandler::report_fpu_exception(test_eip, status_word));
    
    // Check callback
    EXPECT_EQ(last_exception_vector, 16);
    EXPECT_EQ(last_error_code, status_word);
}

// Test normal value (no exception)
TEST_F(FPUExceptionTest, NormalValue) {
    // Create a normal value
    uint8_t normal_value[10];
    create_test_value(normal_value, NORMAL);
    
    // Set up status word
    uint16_t status_word = 0;
    
    // Check for exceptions
    bool has_exceptions = fp::check_fpu_exceptions(normal_value, &status_word);
    
    // Verify no exceptions detected
    EXPECT_FALSE(has_exceptions);
    EXPECT_EQ(status_word, 0);
}

} // namespace tests
} // namespace xenoarm_jit 