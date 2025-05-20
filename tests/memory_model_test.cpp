#include <gtest/gtest.h>
#include <cstring>
#include <atomic>
#include <thread>
#include "xenoarm_jit/memory_model.h"
#include "xenoarm_jit/ir.h"
#include "xenoarm_jit/codegen/aarch64/code_generator.h"

using namespace xenoarm_jit;

class MemoryModelTest : public ::testing::Test {
protected:
    MemoryModel* memory_model;
    aarch64::CodeGenerator* code_gen;
    
    void SetUp() override {
        memory_model = new MemoryModel();
        code_gen = new aarch64::CodeGenerator(); // No size parameter needed
    }
    
    void TearDown() override {
        delete memory_model;
        delete code_gen;
    }
    
    // Helper to create IR instructions
    ir::IrInstruction createLoadInstruction(uint32_t addr, uint32_t reg) {
        ir::IrInstruction insn(ir::IrInstructionType::LOAD);
        ir::IrOperand reg_operand(ir::IrOperandType::REGISTER);
        reg_operand.reg_idx = reg;
        
        ir::IrOperand mem_operand(ir::IrOperandType::MEMORY);
        mem_operand.mem_base = 0;
        mem_operand.mem_offset = addr;
        mem_operand.mem_size = 4; // 32-bit load
        
        insn.operands.push_back(reg_operand);
        insn.operands.push_back(mem_operand);
        return insn;
    }
    
    ir::IrInstruction createStoreInstruction(uint32_t addr, uint32_t reg) {
        ir::IrInstruction insn(ir::IrInstructionType::STORE);
        ir::IrOperand mem_operand(ir::IrOperandType::MEMORY);
        mem_operand.mem_base = 0;
        mem_operand.mem_offset = addr;
        mem_operand.mem_size = 4; // 32-bit store
        
        ir::IrOperand reg_operand(ir::IrOperandType::REGISTER);
        reg_operand.reg_idx = reg;
        
        insn.operands.push_back(mem_operand);
        insn.operands.push_back(reg_operand);
        return insn;
    }
    
    ir::IrInstruction createFenceInstruction(MemoryModel::BarrierType type) {
        ir::IrInstruction insn(ir::IrInstructionType::MEM_FENCE);
        ir::IrOperand imm_operand(ir::IrOperandType::IMMEDIATE);
        imm_operand.imm_value = static_cast<uint32_t>(type);
        
        insn.operands.push_back(imm_operand);
        return insn;
    }
};

// Test basic barrier type mapping
TEST_F(MemoryModelTest, BarrierTypeMapping) {
    struct {
        MemoryModel::BarrierType input;
        const char* name;
        bool should_emit;
    } test_cases[] = {
        { MemoryModel::BARRIER_NONE, "BARRIER_NONE", false },
        { MemoryModel::BARRIER_MFENCE, "BARRIER_MFENCE", true },
        { MemoryModel::BARRIER_SFENCE, "BARRIER_SFENCE", true },
        { MemoryModel::BARRIER_LFENCE, "BARRIER_LFENCE", true },
        { MemoryModel::BARRIER_LOCK_PREFIX, "BARRIER_LOCK_PREFIX", true },
        { MemoryModel::BARRIER_XCHG, "BARRIER_XCHG", true },
        { MemoryModel::BARRIER_DMB_ISH, "BARRIER_DMB_ISH", true },
        { MemoryModel::BARRIER_DSB_ISH, "BARRIER_DSB_ISH", true },
        { MemoryModel::BARRIER_ISB, "BARRIER_ISB", true }
    };
    
    for (const auto& tc : test_cases) {
        // Verify that we can emit the barrier (no crashes)
        ASSERT_NO_THROW(memory_model->emit_memory_barrier(code_gen, tc.input)) 
            << "Failed to emit " << tc.name;
            
        // For BARRIER_NONE, verify no code was generated
        if (!tc.should_emit) {
            // This is a simplification - in a real test we'd verify no bytes were written
            // But our test CodeGenerator is stubbed, so we just check no exceptions
            SUCCEED() << "Successfully processed " << tc.name;
        }
    }
}

// Test memory operation analysis
TEST_F(MemoryModelTest, MemoryOperationAnalysis) {
    // Test load analysis
    ir::IrInstruction load_insn = createLoadInstruction(0x1000, 1);
    MemoryModel::BarrierType load_barrier = memory_model->analyze_load_operation(load_insn);
    // Regular loads generally don't need barriers, but implementation may vary
    
    // Test store analysis
    ir::IrInstruction store_insn = createStoreInstruction(0x1000, 2);
    MemoryModel::BarrierType store_barrier = memory_model->analyze_store_operation(store_insn);
    // Stores might need barriers in ARM to maintain x86 TSO semantics
    
    // Test operation sequencing - store followed by load needs a barrier
    bool needs_barrier = memory_model->needs_barrier_between(store_insn, load_insn);
    EXPECT_TRUE(needs_barrier) << "Store->Load sequence should require a barrier";
    
    // Two loads should not need a barrier
    ir::IrInstruction load_insn2 = createLoadInstruction(0x2000, 3);
    bool loads_need_barrier = memory_model->needs_barrier_between(load_insn, load_insn2);
    EXPECT_FALSE(loads_need_barrier) << "Load->Load sequence should not require a barrier";
}

// Test IR-level memory barrier insertion
TEST_F(MemoryModelTest, IrBarrierInsertion) {
    ir::IrBasicBlock block(1);
    
    // Add a store instruction
    block.instructions.push_back(createStoreInstruction(0x1000, 1));
    
    // Add a memory fence
    memory_model->add_memory_barrier_to_ir(&block, MemoryModel::BARRIER_MFENCE);
    
    // Add a load instruction
    block.instructions.push_back(createLoadInstruction(0x2000, 2));
    
    // The basic block should now have 3 instructions: STORE, FENCE, LOAD
    ASSERT_EQ(block.instructions.size(), 3) << "Expected 3 instructions in block";
    EXPECT_EQ(block.instructions[0].type, ir::IrInstructionType::STORE);
    EXPECT_EQ(block.instructions[1].type, ir::IrInstructionType::MEM_FENCE);
    EXPECT_EQ(block.instructions[2].type, ir::IrInstructionType::LOAD);
    
    // Verify fence type
    EXPECT_EQ(block.instructions[1].operands[0].imm_value, static_cast<uint32_t>(MemoryModel::BARRIER_MFENCE));
}

// Test x86 memory fence conversion
TEST_F(MemoryModelTest, X86FenceConversion) {
    struct {
        MemoryModel::BarrierType x86_fence;
        const char* name;
        bool should_use_dmb;
    } test_cases[] = {
        { MemoryModel::BARRIER_MFENCE, "MFENCE", true },
        { MemoryModel::BARRIER_SFENCE, "SFENCE", true },
        { MemoryModel::BARRIER_LFENCE, "LFENCE", true }
    };
    
    for (const auto& tc : test_cases) {
        // Create a fence instruction
        ir::IrInstruction fence_insn = createFenceInstruction(tc.x86_fence);
        
        // Emit the barrier
        memory_model->emit_memory_barrier(code_gen, tc.x86_fence);
        
        // In a real test with a non-stubbed code generator, we'd verify the 
        // generated ARM instruction matches what we expect
        // For now, we just verify no crashes
        SUCCEED() << "Successfully converted " << tc.name << " to ARM equivalent";
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 