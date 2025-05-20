#include <gtest/gtest.h>
#include "decoder/decoder.h"

// Test fixture for decoder tests
class DecoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
    }
};

// Test basic instruction decoding
TEST_F(DecoderTest, BasicMovInstructions) {
    // Test decoding a MOV reg, reg instruction (0x89)
    uint8_t mov_r32_r32[] = {0x89, 0xC3}; // mov ebx, eax
    
    // Create a decoder instance
    Decoder decoder;
    DecodedInsn insn;
    
    // Decode the instruction
    bool success = decoder.decode(mov_r32_r32, sizeof(mov_r32_r32), insn);
    
    // Verify decoding succeeded
    ASSERT_TRUE(success);
    EXPECT_EQ(insn.opcode, X86_MOV);
    EXPECT_EQ(insn.operands[0].type, OPERAND_REGISTER);
    EXPECT_EQ(insn.operands[0].reg.id, EBX);
    EXPECT_EQ(insn.operands[1].type, OPERAND_REGISTER);
    EXPECT_EQ(insn.operands[1].reg.id, EAX);
    EXPECT_EQ(insn.length, 2);
}

TEST_F(DecoderTest, MovImmediateToRegister) {
    // Test decoding a MOV reg, imm instruction (0xB8+rd)
    uint8_t mov_r32_imm32[] = {0xB8, 0x78, 0x56, 0x34, 0x12}; // mov eax, 0x12345678
    
    Decoder decoder;
    DecodedInsn insn;
    
    bool success = decoder.decode(mov_r32_imm32, sizeof(mov_r32_imm32), insn);
    
    ASSERT_TRUE(success);
    EXPECT_EQ(insn.opcode, X86_MOV);
    EXPECT_EQ(insn.operands[0].type, OPERAND_REGISTER);
    EXPECT_EQ(insn.operands[0].reg.id, EAX);
    EXPECT_EQ(insn.operands[1].type, OPERAND_IMMEDIATE);
    EXPECT_EQ(insn.operands[1].imm.value, 0x12345678);
    EXPECT_EQ(insn.length, 5);
}

TEST_F(DecoderTest, MemoryOperands) {
    // Test decoding a MOV reg, [reg+disp] instruction
    uint8_t mov_r32_m32[] = {0x8B, 0x88, 0x78, 0x56, 0x34, 0x12}; // mov ecx, [eax+0x12345678]
    
    Decoder decoder;
    DecodedInsn insn;
    
    bool success = decoder.decode(mov_r32_m32, sizeof(mov_r32_m32), insn);
    
    ASSERT_TRUE(success);
    EXPECT_EQ(insn.opcode, X86_MOV);
    EXPECT_EQ(insn.operands[0].type, OPERAND_REGISTER);
    EXPECT_EQ(insn.operands[0].reg.id, ECX);
    EXPECT_EQ(insn.operands[1].type, OPERAND_MEMORY);
    EXPECT_EQ(insn.operands[1].mem.base, EAX);
    EXPECT_EQ(insn.operands[1].mem.index, REGISTER_NONE);
    EXPECT_EQ(insn.operands[1].mem.scale, 0);
    EXPECT_EQ(insn.operands[1].mem.disp, 0x12345678);
    EXPECT_EQ(insn.length, 6);
}

TEST_F(DecoderTest, Prefixes) {
    // Test decoding with prefixes (operand size, segment override)
    uint8_t mov_with_prefixes[] = {0x66, 0x8B, 0x01}; // mov ax, [ecx] (operand size prefix)
    
    Decoder decoder;
    DecodedInsn insn;
    
    bool success = decoder.decode(mov_with_prefixes, sizeof(mov_with_prefixes), insn);
    
    ASSERT_TRUE(success);
    EXPECT_EQ(insn.opcode, X86_MOV);
    EXPECT_EQ(insn.prefixes.operand_size, 16); // 16-bit operand size due to 0x66 prefix
    EXPECT_EQ(insn.operands[0].type, OPERAND_REGISTER);
    EXPECT_EQ(insn.operands[0].reg.id, AX);
    EXPECT_EQ(insn.operands[1].type, OPERAND_MEMORY);
    EXPECT_EQ(insn.operands[1].mem.base, ECX);
    EXPECT_EQ(insn.length, 3);
}

// Test for decoding errors/invalid instructions
TEST_F(DecoderTest, InvalidInstructions) {
    // Test an invalid opcode
    uint8_t invalid_opcode[] = {0xFF, 0xFF}; // Not a valid x86 instruction
    
    Decoder decoder;
    DecodedInsn insn;
    
    bool success = decoder.decode(invalid_opcode, sizeof(invalid_opcode), insn);
    
    // Should fail to decode
    EXPECT_FALSE(success);
} 