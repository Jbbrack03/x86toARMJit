#include <gtest/gtest.h>
#include "xenoarm_jit/register_allocation/register_allocator.h"
#include "xenoarm_jit/ir.h"
#include <vector>
#include <unordered_map>

namespace xenoarm_jit {
namespace tests {

class RegisterAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        allocator = new register_allocation::RegisterAllocator();
    }
    
    void TearDown() override {
        delete allocator;
    }

    // Helper to create a simple sequence of IR instructions with register pressure
    std::vector<ir::IrInstruction> create_test_ir_sequence(int num_registers, bool create_loop = false) {
        std::vector<ir::IrInstruction> instructions;
        
        // Create a sequence of MOV instructions to define registers
        for (int i = 0; i < num_registers; i++) {
            ir::IrInstruction mov(ir::IrInstructionType::MOV);
            mov.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
            mov.operands.push_back(ir::IrOperand::make_imm(i, ir::IrDataType::I32));  // Source (immediate value)
            instructions.push_back(mov);
        }
        
        // Create some ADD instructions to use the registers
        for (int i = 0; i < num_registers; i++) {
            ir::IrInstruction add(ir::IrInstructionType::ADD);
            add.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
            add.operands.push_back(ir::IrOperand::make_reg((i + 1) % num_registers, ir::IrDataType::I32));  // Source
            instructions.push_back(add);
        }
        
        // If creating a loop, add a backward branch
        if (create_loop) {
            ir::IrInstruction jmp(ir::IrInstructionType::JMP);
            jmp.operands.push_back(ir::IrOperand::make_imm(0, ir::IrDataType::U32));  // Jump target (instruction 0)
            instructions.push_back(jmp);
        }
        
        return instructions;
    }
    
    // Helper to create a sequence with mixed GPR and NEON registers
    std::vector<ir::IrInstruction> create_mixed_register_test() {
        std::vector<ir::IrInstruction> instructions;
        
        // Create some GPR registers (I32)
        for (int i = 0; i < 5; i++) {
            ir::IrInstruction mov(ir::IrInstructionType::MOV);
            mov.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
            mov.operands.push_back(ir::IrOperand::make_imm(i, ir::IrDataType::I32));  // Source
            instructions.push_back(mov);
        }
        
        // Create some NEON registers (F32)
        for (int i = 5; i < 10; i++) {
            ir::IrInstruction mov(ir::IrInstructionType::VEC_MOV);
            mov.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::F32));  // Destination
            mov.operands.push_back(ir::IrOperand::make_imm(i, ir::IrDataType::F32));  // Source
            instructions.push_back(mov);
        }
        
        // Use the registers
        for (int i = 0; i < 5; i++) {
            ir::IrInstruction add(ir::IrInstructionType::ADD);
            add.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
            add.operands.push_back(ir::IrOperand::make_reg((i + 1) % 5, ir::IrDataType::I32));  // Source
            instructions.push_back(add);
        }
        
        // Use the NEON registers
        for (int i = 5; i < 10; i++) {
            ir::IrInstruction add(ir::IrInstructionType::VEC_ADD_PS);
            add.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::F32));  // Destination
            add.operands.push_back(ir::IrOperand::make_reg(((i + 1) % 5) + 5, ir::IrDataType::F32));  // Source
            instructions.push_back(add);
        }
        
        return instructions;
    }
    
    // Helper to create a sequence with very high register pressure to force spilling
    std::vector<ir::IrInstruction> create_high_pressure_sequence() {
        std::vector<ir::IrInstruction> instructions;
        
        // Create more GPR registers than available (e.g., 32 regs)
        for (int i = 0; i < 32; i++) {
            ir::IrInstruction mov(ir::IrInstructionType::MOV);
            mov.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
            mov.operands.push_back(ir::IrOperand::make_imm(i, ir::IrDataType::I32));  // Source
            instructions.push_back(mov);
        }
        
        // Create many NEON registers (more than available)
        for (int i = 32; i < 64; i++) {
            ir::IrInstruction mov(ir::IrInstructionType::VEC_MOV);
            mov.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::F32));  // Destination
            mov.operands.push_back(ir::IrOperand::make_imm(i, ir::IrDataType::F32));  // Source
            instructions.push_back(mov);
        }
        
        // Use all registers to ensure they stay live
        for (int i = 0; i < 64; i++) {
            if (i < 32) {
                ir::IrInstruction add(ir::IrInstructionType::ADD);
                add.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
                add.operands.push_back(ir::IrOperand::make_reg((i + 1) % 32, ir::IrDataType::I32));  // Source
                instructions.push_back(add);
            } else {
                ir::IrInstruction add(ir::IrInstructionType::VEC_ADD_PS);
                add.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::F32));  // Destination
                add.operands.push_back(ir::IrOperand::make_reg(((i + 1) % 32) + 32, ir::IrDataType::F32));  // Source
                instructions.push_back(add);
            }
        }
        
        return instructions;
    }
    
    register_allocation::RegisterAllocator* allocator;
};

// Test basic register allocation without spilling
TEST_F(RegisterAllocatorTest, BasicAllocation) {
    // Create a simple IR sequence with a small number of registers
    std::vector<ir::IrInstruction> ir_instructions = create_test_ir_sequence(5);
    
    // Run register allocation
    std::unordered_map<uint32_t, register_allocation::RegisterMapping> mapping = 
        allocator->allocate(ir_instructions);
    
    // Check that all virtual registers have been allocated
    EXPECT_EQ(mapping.size(), 5);
    
    // Check that no registers have been spilled
    for (const auto& [vreg, reg_map] : mapping) {
        EXPECT_FALSE(reg_map.is_spilled);
    }
}

// Test register allocation with spilling due to high register pressure
TEST_F(RegisterAllocatorTest, RegisterSpilling) {
    // Create an IR sequence with very high register pressure
    std::vector<ir::IrInstruction> ir_instructions = create_high_pressure_sequence();
    
    // Run register allocation
    std::unordered_map<uint32_t, register_allocation::RegisterMapping> mapping = 
        allocator->allocate(ir_instructions);
    
    // Check that all virtual registers have been allocated
    EXPECT_EQ(mapping.size(), 64);
    
    // Check that some registers have been spilled
    bool found_spilled = false;
    for (const auto& [vreg, reg_map] : mapping) {
        if (reg_map.is_spilled) {
            found_spilled = true;
            break;
        }
    }
    EXPECT_TRUE(found_spilled);
    
    // Check that spilled registers have valid stack offsets
    for (const auto& [vreg, reg_map] : mapping) {
        if (reg_map.is_spilled) {
            EXPECT_GE(reg_map.stack_offset, 0);
        }
    }
    
    // Check that the total spill size is greater than 0
    EXPECT_GT(allocator->get_total_spill_size(), 0);
}

// Test priority-based spilling (loop registers should be less likely to be spilled)
TEST_F(RegisterAllocatorTest, PriorityBasedSpilling) {
    // Create an IR sequence with a loop
    std::vector<ir::IrInstruction> ir_instructions = create_test_ir_sequence(32, true);
    
    // Run register allocation
    std::unordered_map<uint32_t, register_allocation::RegisterMapping> mapping = 
        allocator->allocate(ir_instructions);
    
    // The first few registers (0-7) are typically x86 mapped and should be higher priority
    // Check if any of them are spilled
    bool x86_mapped_spilled = false;
    for (int i = 0; i < 8; i++) {
        if (mapping.find(i) != mapping.end() && mapping[i].is_spilled) {
            x86_mapped_spilled = true;
            break;
        }
    }
    
    // Since priority should favor x86 mapped registers, they should be less likely to spill
    // This isn't guaranteed, but is a reasonable expectation for our test
    EXPECT_FALSE(x86_mapped_spilled);
}

// Test mixed GPR and NEON register allocation
TEST_F(RegisterAllocatorTest, MixedRegisterTypes) {
    // Create an IR sequence with both GPR and NEON registers
    std::vector<ir::IrInstruction> ir_instructions = create_mixed_register_test();
    
    // Run register allocation
    std::unordered_map<uint32_t, register_allocation::RegisterMapping> mapping = 
        allocator->allocate(ir_instructions);
    
    // Check that GPR registers got GPR physical registers
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(mapping[i].type, register_allocation::PhysicalRegisterType::GPR);
    }
    
    // Check that NEON registers got NEON physical registers
    for (int i = 5; i < 10; i++) {
        EXPECT_EQ(mapping[i].type, register_allocation::PhysicalRegisterType::NEON);
    }
}

// Test function prologue and epilogue generation
TEST_F(RegisterAllocatorTest, FunctionPrologueEpilogue) {
    // Create an IR sequence with some register pressure
    std::vector<ir::IrInstruction> ir_instructions = create_high_pressure_sequence();
    
    // Set up function prologue
    allocator->setup_function_prologue(ir_instructions);
    
    // Check that the total spill size is valid
    int32_t spill_size = allocator->get_total_spill_size();
    EXPECT_GT(spill_size, 0);
    
    // Check that the spill size is properly aligned (should be aligned to 16 bytes)
    EXPECT_EQ(spill_size & 0xF, 0);
    
    // Generate epilogue (mostly for coverage in this test)
    allocator->generate_function_epilogue();
}

// Test specific register spill query functions
TEST_F(RegisterAllocatorTest, SpillQueries) {
    // Create an IR sequence with very high register pressure
    std::vector<ir::IrInstruction> ir_instructions = create_high_pressure_sequence();
    
    // Run register allocation
    std::unordered_map<uint32_t, register_allocation::RegisterMapping> mapping = 
        allocator->allocate(ir_instructions);
    
    // Find a spilled register
    uint32_t spilled_vreg = 0;
    bool found_spilled = false;
    for (const auto& [vreg, reg_map] : mapping) {
        if (reg_map.is_spilled) {
            spilled_vreg = vreg;
            found_spilled = true;
            break;
        }
    }
    
    if (found_spilled) {
        // Check is_register_spilled
        EXPECT_TRUE(allocator->is_register_spilled(spilled_vreg));
        
        // Check get_spill_offset
        int32_t offset = allocator->get_spill_offset(spilled_vreg);
        EXPECT_GE(offset, 0);
        
        // Check that non-spilled register returns false
        uint32_t non_spilled_vreg = 0;
        for (const auto& [vreg, reg_map] : mapping) {
            if (!reg_map.is_spilled) {
                non_spilled_vreg = vreg;
                break;
            }
        }
        
        EXPECT_FALSE(allocator->is_register_spilled(non_spilled_vreg));
        EXPECT_EQ(allocator->get_spill_offset(non_spilled_vreg), -1);
    }
}

// Phase 8 test: Verify loop-aware register allocation prioritization
TEST_F(RegisterAllocatorTest, LoopAwareRegisterAllocation) {
    // Create a sequence with high register pressure including a loop
    // Structure will be:
    // - Define many registers (beyond hardware capacity)
    // - Create a loop that heavily uses a subset of registers
    // - After allocation, check that loop registers are less likely to be spilled
    
    std::vector<ir::IrInstruction> instructions;
    
    // Define 40 registers (well beyond hardware capacity)
    const int total_regs = 40;
    const int loop_regs = 8;  // Registers to use heavily in a loop
    
    // First, define all registers with initial values
    for (int i = 0; i < total_regs; i++) {
        ir::IrInstruction mov(ir::IrInstructionType::MOV);
        mov.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
        mov.operands.push_back(ir::IrOperand::make_imm(i, ir::IrDataType::I32));  // Source
        instructions.push_back(mov);
    }
    
    // Start of loop marker
    size_t loop_start_idx = instructions.size();
    
    // Create loop body that heavily uses a specific subset of registers (0-7)
    for (int iter = 0; iter < 5; iter++) {     // Create multiple operations in the loop
        for (int i = 0; i < loop_regs; i++) {  // Loop registers (0-7)
            // ADD reg[i], reg[(i+1) % loop_regs]
            ir::IrInstruction add(ir::IrInstructionType::ADD);
            add.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
            add.operands.push_back(ir::IrOperand::make_reg((i + 1) % loop_regs, ir::IrDataType::I32));  // Source
            instructions.push_back(add);
            
            // Also use some non-loop registers occasionally
            if (iter % 2 == 0) {
                ir::IrInstruction mov(ir::IrInstructionType::MOV);
                mov.operands.push_back(ir::IrOperand::make_reg(total_regs - i - 1, ir::IrDataType::I32));  // Destination
                mov.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Source
                instructions.push_back(mov);
            }
        }
    }
    
    // Add backward branch to form a loop
    ir::IrInstruction jmp(ir::IrInstructionType::JMP);
    jmp.operands.push_back(ir::IrOperand::make_imm(loop_start_idx, ir::IrDataType::U32));  // Jump target
    instructions.push_back(jmp);
    
    // Some additional operations after the loop to ensure all registers have some usage
    for (int i = loop_regs; i < total_regs; i++) {
        ir::IrInstruction add(ir::IrInstructionType::ADD);
        add.operands.push_back(ir::IrOperand::make_reg(i, ir::IrDataType::I32));  // Destination
        add.operands.push_back(ir::IrOperand::make_imm(1, ir::IrDataType::I32));  // Source
        instructions.push_back(add);
    }
    
    // Run register allocation with the loop-aware allocator
    std::unordered_map<uint32_t, register_allocation::RegisterMapping> mapping = 
        allocator->allocate(instructions);
    
    // Verify allocations - count how many loop registers vs non-loop registers are spilled
    int spilled_loop_regs = 0;
    int spilled_non_loop_regs = 0;
    
    for (int i = 0; i < total_regs; i++) {
        if (mapping.find(i) != mapping.end() && mapping[i].is_spilled) {
            if (i < loop_regs) {
                spilled_loop_regs++;
            } else {
                spilled_non_loop_regs++;
            }
        }
    }
    
    // With proper loop-aware allocation, loop registers should have lower spill rate
    // This is a probabilistic test, but with good implementation, the loop registers
    // should be prioritized enough that few or none are spilled
    EXPECT_LT(spilled_loop_regs, loop_regs / 2)
        << "Too many loop registers were spilled: " << spilled_loop_regs 
        << " out of " << loop_regs;
    
    // Calculate spill rates for comparison
    float loop_spill_rate = static_cast<float>(spilled_loop_regs) / loop_regs;
    float non_loop_spill_rate = static_cast<float>(spilled_non_loop_regs) / (total_regs - loop_regs);
    
    // Loop registers should have a significantly lower spill rate than non-loop registers
    EXPECT_LT(loop_spill_rate, non_loop_spill_rate)
        << "Loop registers have spill rate " << loop_spill_rate
        << " which is not less than non-loop registers' rate " << non_loop_spill_rate;
    
    std::cout << "Loop register spill rate: " << (loop_spill_rate * 100.0f) << "%"
              << " vs Non-loop register spill rate: " << (non_loop_spill_rate * 100.0f) << "%"
              << std::endl;
}

} // namespace tests
} // namespace xenoarm_jit 