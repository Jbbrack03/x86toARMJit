#include <gtest/gtest.h>
#include "xenoarm_jit/ir.h"
#include "xenoarm_jit/decoder/decoder.h"

using namespace xenoarm_jit;

// Test fixture for IR tests
class IRTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
    }
};

// Test IR basic block creation and manipulation
TEST_F(IRTest, IrBasicBlock) {
    // Create a basic block
    ir::IrBasicBlock block(1);
    
    // Add a MOV instruction (eax <- ebx)
    ir::IrOperand dest_op(ir::IrOperandType::REGISTER);
    dest_op.reg_idx = 0; // EAX
    
    ir::IrOperand src_op(ir::IrOperandType::REGISTER);
    src_op.reg_idx = 1; // EBX
    
    ir::IrInstruction mov_instr(ir::IrInstructionType::MOV, {dest_op, src_op});
    block.instructions.push_back(mov_instr);
    
    // Verify the instruction was added correctly
    ASSERT_EQ(block.instructions.size(), 1);
    EXPECT_EQ(block.instructions[0].type, ir::IrInstructionType::MOV);
    EXPECT_EQ(block.instructions[0].operands.size(), 2);
    EXPECT_EQ(block.instructions[0].operands[0].type, ir::IrOperandType::REGISTER);
    EXPECT_EQ(block.instructions[0].operands[0].reg_idx, 0);
    EXPECT_EQ(block.instructions[0].operands[1].type, ir::IrOperandType::REGISTER);
    EXPECT_EQ(block.instructions[0].operands[1].reg_idx, 1);
}

// Test creating a function with multiple basic blocks
TEST_F(IRTest, IrFunction) {
    // Create a function
    ir::IrFunction func(0x1000);
    
    // Add a basic block
    ir::IrBasicBlock block1(1);
    
    // Add a MOV instruction (eax <- 42)
    ir::IrOperand dest_op(ir::IrOperandType::REGISTER);
    dest_op.reg_idx = 0; // EAX
    
    ir::IrOperand imm_op(ir::IrOperandType::IMMEDIATE);
    imm_op.imm_value = 42;
    
    ir::IrInstruction mov_instr(ir::IrInstructionType::MOV, {dest_op, imm_op});
    block1.instructions.push_back(mov_instr);
    
    // Add the block to the function
    func.basic_blocks.push_back(block1);
    
    // Add another basic block
    ir::IrBasicBlock block2(2);
    
    // Add an ADD instruction (eax += 10)
    ir::IrOperand add_dest_op(ir::IrOperandType::REGISTER);
    add_dest_op.reg_idx = 0; // EAX
    
    ir::IrOperand add_imm_op(ir::IrOperandType::IMMEDIATE);
    add_imm_op.imm_value = 10;
    
    ir::IrInstruction add_instr(ir::IrInstructionType::ADD, {add_dest_op, add_imm_op});
    block2.instructions.push_back(add_instr);
    
    // Add the second block to the function
    func.basic_blocks.push_back(block2);
    
    // Verify the function structure
    ASSERT_EQ(func.basic_blocks.size(), 2);
    EXPECT_EQ(func.basic_blocks[0].id, 1);
    EXPECT_EQ(func.basic_blocks[0].instructions.size(), 1);
    EXPECT_EQ(func.basic_blocks[0].instructions[0].type, ir::IrInstructionType::MOV);
    
    EXPECT_EQ(func.basic_blocks[1].id, 2);
    EXPECT_EQ(func.basic_blocks[1].instructions.size(), 1);
    EXPECT_EQ(func.basic_blocks[1].instructions[0].type, ir::IrInstructionType::ADD);
}

// Test memory operations
TEST_F(IRTest, IrMemoryOperations) {
    ir::IrBasicBlock block(1);
    
    // Create a LOAD instruction (ecx <- [eax+0x100])
    ir::IrOperand dest_op(ir::IrOperandType::REGISTER);
    dest_op.reg_idx = 2; // ECX
    
    ir::IrOperand addr_op(ir::IrOperandType::MEMORY);
    addr_op.mem_base = 0; // EAX
    addr_op.mem_offset = 0x100;
    
    ir::IrInstruction load_instr(ir::IrInstructionType::LOAD, {dest_op, addr_op});
    block.instructions.push_back(load_instr);
    
    // Verify the LOAD instruction
    ASSERT_EQ(block.instructions.size(), 1);
    EXPECT_EQ(block.instructions[0].type, ir::IrInstructionType::LOAD);
    EXPECT_EQ(block.instructions[0].operands.size(), 2);
    EXPECT_EQ(block.instructions[0].operands[0].type, ir::IrOperandType::REGISTER);
    EXPECT_EQ(block.instructions[0].operands[0].reg_idx, 2);
    EXPECT_EQ(block.instructions[0].operands[1].type, ir::IrOperandType::MEMORY);
    EXPECT_EQ(block.instructions[0].operands[1].mem_base, 0);
    EXPECT_EQ(block.instructions[0].operands[1].mem_offset, 0x100);
}

// Test IR block duplication and modification
TEST_F(IRTest, IrBlockManipulation) {
    // Create a block with instructions
    ir::IrBasicBlock block1(1);
    
    // Add a MOV instruction
    ir::IrOperand dest_op(ir::IrOperandType::REGISTER);
    dest_op.reg_idx = 0; // EAX
    
    ir::IrOperand src_op(ir::IrOperandType::REGISTER);
    src_op.reg_idx = 1; // EBX
    
    ir::IrInstruction mov_instr(ir::IrInstructionType::MOV, {dest_op, src_op});
    block1.instructions.push_back(mov_instr);
    
    // Create a copy of the block
    ir::IrBasicBlock block2 = block1;
    
    // Modify the copy
    block2.id = 2;
    block2.instructions[0].operands[0].reg_idx = 3; // Change destination to EDX
    
    // Verify the original is unchanged
    EXPECT_EQ(block1.id, 1);
    EXPECT_EQ(block1.instructions[0].operands[0].reg_idx, 0);
    
    // Verify the copy reflects the changes
    EXPECT_EQ(block2.id, 2);
    EXPECT_EQ(block2.instructions[0].operands[0].reg_idx, 3);
} 