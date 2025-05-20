#include <gtest/gtest.h>
#include "xenoarm_jit/decoder/decoder.h"
#include "xenoarm_jit/decoder/fpu_decoder.h"
#include "xenoarm_jit/exception_handler.h"
#include "xenoarm_jit/ir.h"
#include <vector>

namespace xenoarm_jit {
namespace tests {

class FPUDecoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        decoder = new decoder::Decoder();
        ir_func = new ir::IrFunction(0x1000); // Some test address
        basic_block = new ir::IrBasicBlock(1);
        
        // Initialize the exception handler
        ExceptionHandler::init();
    }
    
    void TearDown() override {
        delete decoder;
        delete ir_func;
        delete basic_block;
    }
    
    // Test helper: Check if an FPU instruction is decoded correctly
    bool decode_and_check_fpu_instruction(const std::vector<uint8_t>& bytes, 
                                         ir::IrInstructionType expected_type,
                                         size_t expected_operands = 1) {
        // Create a DecodedInsn to store the result
        decoder::DecodedInsn insn;
        
        // Decode the instruction
        bool result = decoder->decode(bytes.data(), bytes.size(), insn);
        
        // If decoding succeeded, generate IR
        if (result) {
            // Clear the basic block
            basic_block->instructions.clear();
            
            // Call the FPU decoder
            result = decoder::decode_fpu_instruction(decoder, &insn, ir_func, basic_block);
            
            // Check if IR was generated correctly
            if (result && !basic_block->instructions.empty()) {
                auto& ir_instr = basic_block->instructions.front();
                return (ir_instr.type == expected_type && 
                        ir_instr.operands.size() == expected_operands);
            }
        }
        
        return false;
    }
    
    decoder::Decoder* decoder;
    ir::IrFunction* ir_func;
    ir::IrBasicBlock* basic_block;
};

// Implement a custom exception callback for testing
void test_exception_callback(uint32_t exception_vector, uint32_t error_code) {
    // For testing, just record the values
    last_exception_vector = exception_vector;
    last_error_code = error_code;
}

// Global variables to store the last exception information
static uint32_t last_exception_vector = 0;
static uint32_t last_error_code = 0;

// Test decoding FLD instructions
TEST_F(FPUDecoderTest, DecodeFLD) {
    // FLD m32fp - D9 /0
    // Example: D9 05 00 00 00 00 (FLD dword ptr [0])
    std::vector<uint8_t> fld_m32 = {0xD9, 0x05, 0x00, 0x00, 0x00, 0x00};
    EXPECT_TRUE(decode_and_check_fpu_instruction(fld_m32, ir::IrInstructionType::FLD));
    
    // FLD m64fp - DD /0
    // Example: DD 05 00 00 00 00 (FLD qword ptr [0])
    std::vector<uint8_t> fld_m64 = {0xDD, 0x05, 0x00, 0x00, 0x00, 0x00};
    EXPECT_TRUE(decode_and_check_fpu_instruction(fld_m64, ir::IrInstructionType::FLD));
    
    // FLD ST(i) - D9 C0+i
    // Example: D9 C1 (FLD ST(1))
    std::vector<uint8_t> fld_st = {0xD9, 0xC1};
    EXPECT_TRUE(decode_and_check_fpu_instruction(fld_st, ir::IrInstructionType::FLD));
}

// Test decoding FSTP instructions
TEST_F(FPUDecoderTest, DecodeFSTP) {
    // FSTP m32fp - D9 /3
    // Example: D9 1D 00 00 00 00 (FSTP dword ptr [0])
    std::vector<uint8_t> fstp_m32 = {0xD9, 0x1D, 0x00, 0x00, 0x00, 0x00};
    EXPECT_TRUE(decode_and_check_fpu_instruction(fstp_m32, ir::IrInstructionType::FSTP));
    
    // FSTP m64fp - DD /3
    // Example: DD 1D 00 00 00 00 (FSTP qword ptr [0])
    std::vector<uint8_t> fstp_m64 = {0xDD, 0x1D, 0x00, 0x00, 0x00, 0x00};
    EXPECT_TRUE(decode_and_check_fpu_instruction(fstp_m64, ir::IrInstructionType::FSTP));
}

// Test decoding FADD instructions
TEST_F(FPUDecoderTest, DecodeFADD) {
    // FADD m32fp - D8 /0
    // Example: D8 05 00 00 00 00 (FADD dword ptr [0])
    std::vector<uint8_t> fadd_m32 = {0xD8, 0x05, 0x00, 0x00, 0x00, 0x00};
    EXPECT_TRUE(decode_and_check_fpu_instruction(fadd_m32, ir::IrInstructionType::FADD));
    
    // FADD ST, ST(i) - D8 C0+i
    // Example: D8 C1 (FADD ST, ST(1))
    std::vector<uint8_t> fadd_st = {0xD8, 0xC1};
    EXPECT_TRUE(decode_and_check_fpu_instruction(fadd_st, ir::IrInstructionType::FADD));
}

// Test the exception handler
TEST_F(FPUDecoderTest, ExceptionHandling) {
    // Register the test exception callback
    ExceptionHandler::set_exception_callback(test_exception_callback);
    
    // Reset the global variables
    last_exception_vector = 0;
    last_error_code = 0;
    
    // Report an FPU exception
    uint32_t test_eip = 0x12345678;
    uint32_t test_fpu_status = 0xABCD;
    
    EXPECT_TRUE(ExceptionHandler::report_fpu_exception(test_eip, test_fpu_status));
    
    // Check if the callback was called with the correct values
    EXPECT_EQ(last_exception_vector, 16); // Vector 16 for #MF
    EXPECT_EQ(last_error_code, test_fpu_status);
    
    // Check that the faulting address was recorded
    EXPECT_EQ(ExceptionHandler::get_last_faulting_address(), test_eip);
}

} // namespace tests
} // namespace xenoarm_jit 