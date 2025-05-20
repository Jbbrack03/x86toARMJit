#include <gtest/gtest.h>
#include "aarch64/code_generator.h"
#include "ir/ir.h"

// Test fixture for AArch64 code generator tests
class CodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
        code_buffer = new uint8_t[CODE_BUFFER_SIZE];
        memset(code_buffer, 0, CODE_BUFFER_SIZE);
        code_gen = new AArch64CodeGenerator(code_buffer, CODE_BUFFER_SIZE);
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
        delete[] code_buffer;
        delete code_gen;
    }

    // Constants
    static constexpr size_t CODE_BUFFER_SIZE = 4096;

    // Test members
    uint8_t* code_buffer;
    AArch64CodeGenerator* code_gen;
};

// Test register mapping
TEST_F(CodeGenTest, RegisterMapping) {
    // Test that x86 registers map to AArch64 registers correctly
    EXPECT_EQ(code_gen->mapX86RegToAArch64(IR_REG_EAX), ARM64_REG_X0);
    EXPECT_EQ(code_gen->mapX86RegToAArch64(IR_REG_ECX), ARM64_REG_X1);
    EXPECT_EQ(code_gen->mapX86RegToAArch64(IR_REG_EDX), ARM64_REG_X2);
    EXPECT_EQ(code_gen->mapX86RegToAArch64(IR_REG_EBX), ARM64_REG_X3);
}

// Test basic move instruction
TEST_F(CodeGenTest, GenerateMovReg) {
    IRBlock block;
    
    // Create a MOV reg, reg instruction
    IRInstruction instr;
    instr.opcode = IR_MOV_REG;
    instr.operands[0].type = IR_OP_REGISTER;
    instr.operands[0].reg.id = IR_REG_EAX;  // dst
    instr.operands[1].type = IR_OP_REGISTER;
    instr.operands[1].reg.id = IR_REG_EBX;  // src
    block.instructions.push_back(instr);
    
    // Generate code
    size_t code_size = code_gen->generateCode(block);
    
    // Verify code was generated
    ASSERT_GT(code_size, 0);
    
    // In AArch64, MOV between registers should generate a "mov x0, x3" instruction
    // This is 0xAA0303E0 in AArch64 machine code (MOV X0, X3)
    uint32_t* arm_code = reinterpret_cast<uint32_t*>(code_buffer);
    EXPECT_EQ(arm_code[0], 0xAA0303E0);
}

// Test move immediate instruction
TEST_F(CodeGenTest, GenerateMovImm) {
    IRBlock block;
    
    // Create a MOV reg, imm instruction
    IRInstruction instr;
    instr.opcode = IR_MOV_IMM;
    instr.operands[0].type = IR_OP_REGISTER;
    instr.operands[0].reg.id = IR_REG_EAX;  // dst
    instr.operands[1].type = IR_OP_IMMEDIATE;
    instr.operands[1].imm.value = 42;  // immediate value
    block.instructions.push_back(instr);
    
    // Generate code
    size_t code_size = code_gen->generateCode(block);
    
    // Verify code was generated
    ASSERT_GT(code_size, 0);
    
    // Check generated instruction
    // For 64-bit immediate loads, it might use multiple instructions
    // but we can check if it's non-zero at minimum
    uint32_t* arm_code = reinterpret_cast<uint32_t*>(code_buffer);
    EXPECT_NE(arm_code[0], 0);
}

// Test memory load instruction
TEST_F(CodeGenTest, GenerateLoad) {
    IRBlock block;
    
    // Create a LOAD instruction (load into EAX from [EBX])
    IRInstruction instr;
    instr.opcode = IR_LOAD;
    instr.operands[0].type = IR_OP_REGISTER;
    instr.operands[0].reg.id = IR_REG_EAX;  // dst
    instr.operands[1].type = IR_OP_REGISTER;
    instr.operands[1].reg.id = IR_REG_EBX;  // address
    block.instructions.push_back(instr);
    
    // Generate code
    size_t code_size = code_gen->generateCode(block);
    
    // Verify code was generated
    ASSERT_GT(code_size, 0);
    
    // In AArch64, LDR should be generated
    // We're not checking specific encoding as it may vary
    // Just ensuring non-zero code
    uint32_t* arm_code = reinterpret_cast<uint32_t*>(code_buffer);
    EXPECT_NE(arm_code[0], 0);
}

// Test memory store instruction
TEST_F(CodeGenTest, GenerateStore) {
    IRBlock block;
    
    // Create a STORE instruction (store EAX to [EBX])
    IRInstruction instr;
    instr.opcode = IR_STORE;
    instr.operands[0].type = IR_OP_REGISTER;
    instr.operands[0].reg.id = IR_REG_EBX;  // dst address
    instr.operands[1].type = IR_OP_REGISTER;
    instr.operands[1].reg.id = IR_REG_EAX;  // value
    block.instructions.push_back(instr);
    
    // Generate code
    size_t code_size = code_gen->generateCode(block);
    
    // Verify code was generated
    ASSERT_GT(code_size, 0);
    
    // In AArch64, STR should be generated
    // We're not checking specific encoding as it may vary
    uint32_t* arm_code = reinterpret_cast<uint32_t*>(code_buffer);
    EXPECT_NE(arm_code[0], 0);
}

// Test basic arithmetic operations
TEST_F(CodeGenTest, GenerateArithmeticOps) {
    IRBlock block;
    
    // Create an ADD reg, reg instruction
    IRInstruction add_instr;
    add_instr.opcode = IR_ADD_REG;
    add_instr.operands[0].type = IR_OP_REGISTER;
    add_instr.operands[0].reg.id = IR_REG_EAX;  // dst/src1
    add_instr.operands[1].type = IR_OP_REGISTER;
    add_instr.operands[1].reg.id = IR_REG_EBX;  // src2
    block.instructions.push_back(add_instr);
    
    // Generate code
    size_t code_size = code_gen->generateCode(block);
    
    // Verify code was generated
    ASSERT_GT(code_size, 0);
    
    // In AArch64, ADD should be generated
    // We're not checking specific encoding as it may vary
    uint32_t* arm_code = reinterpret_cast<uint32_t*>(code_buffer);
    EXPECT_NE(arm_code[0], 0);
}

// Test code buffer bounds checking
TEST_F(CodeGenTest, CodeBufferBoundsChecking) {
    // Create a block with many instructions to test buffer overflow handling
    IRBlock large_block;
    
    // Fill with many MOV instructions to potentially overflow the buffer
    for (int i = 0; i < 10000; i++) {
        IRInstruction instr;
        instr.opcode = IR_MOV_IMM;
        instr.operands[0].type = IR_OP_REGISTER;
        instr.operands[0].reg.id = IR_REG_EAX;
        instr.operands[1].type = IR_OP_IMMEDIATE;
        instr.operands[1].imm.value = i;
        large_block.instructions.push_back(instr);
    }
    
    // This should either return 0 (failed) or a value <= buffer size
    size_t code_size = code_gen->generateCode(large_block);
    
    // Either code generation failed safely (returns 0)
    // or it successfully generated code that fits in the buffer
    EXPECT_TRUE(code_size == 0 || code_size <= CODE_BUFFER_SIZE);
} 