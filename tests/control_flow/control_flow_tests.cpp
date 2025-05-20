#include <gtest/gtest.h>
#include "jit_core/jit_api.h"
#include "eflags/eflags_state.h"
#include <cstring>

// Reuse the MockMemory class from previous tests
class MockMemory {
public:
    static uint8_t* guestMemory;
    static constexpr size_t GUEST_MEMORY_SIZE = 16384; // Larger memory for control flow tests
    
    static bool initialize() {
        guestMemory = new uint8_t[GUEST_MEMORY_SIZE];
        if (!guestMemory) return false;
        memset(guestMemory, 0, GUEST_MEMORY_SIZE);
        return true;
    }
    
    static void cleanup() {
        delete[] guestMemory;
        guestMemory = nullptr;
    }
    
    static uint8_t read8(uint32_t address) {
        if (address < GUEST_MEMORY_SIZE)
            return guestMemory[address];
        return 0;
    }
    
    static void write8(uint32_t address, uint8_t value) {
        if (address < GUEST_MEMORY_SIZE)
            guestMemory[address] = value;
    }
    
    static uint32_t read32(uint32_t address) {
        if (address + 3 < GUEST_MEMORY_SIZE) {
            return (guestMemory[address]) |
                   (guestMemory[address + 1] << 8) |
                   (guestMemory[address + 2] << 16) |
                   (guestMemory[address + 3] << 24);
        }
        return 0;
    }
    
    static void write32(uint32_t address, uint32_t value) {
        if (address + 3 < GUEST_MEMORY_SIZE) {
            guestMemory[address] = value & 0xFF;
            guestMemory[address + 1] = (value >> 8) & 0xFF;
            guestMemory[address + 2] = (value >> 16) & 0xFF;
            guestMemory[address + 3] = (value >> 24) & 0xFF;
        }
    }
};

uint8_t* MockMemory::guestMemory = nullptr;

// Mock memory callbacks for JIT
uint8_t mockRead8(void* context, uint32_t address) {
    return MockMemory::read8(address);
}

uint32_t mockRead32(void* context, uint32_t address) {
    return MockMemory::read32(address);
}

void mockWrite8(void* context, uint32_t address, uint8_t value) {
    MockMemory::write8(address, value);
}

void mockWrite32(void* context, uint32_t address, uint32_t value) {
    MockMemory::write32(address, value);
}

// Test fixture for control flow tests
class ControlFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code that will be called before each test
        MockMemory::initialize();
        
        // Initialize the JIT
        JitInitParams params;
        params.hostContextPtr = nullptr;
        params.readMemory8Callback = mockRead8;
        params.readMemory32Callback = mockRead32;
        params.writeMemory8Callback = mockWrite8;
        params.writeMemory32Callback = mockWrite32;
        
        jit = jitCreate(&params);
        ASSERT_NE(jit, nullptr);
    }

    void TearDown() override {
        // Cleanup code that will be called after each test
        jitDestroy(jit);
        MockMemory::cleanup();
    }
    
    // Helper to set up test code in mock memory
    void setupTestCode(uint32_t address, const uint8_t* code, size_t size) {
        for (size_t i = 0; i < size; i++) {
            MockMemory::write8(address + i, code[i]);
        }
    }
    
    // Execute a block and return the final EIP value
    uint32_t executeAndGetEIP(uint32_t startAddress) {
        // Get the compiled host code
        void* hostCode = jitGetHostCodeForGuestAddress(jit, startAddress);
        EXPECT_NE(hostCode, nullptr);
        if (!hostCode) return 0;
        
        // Execute the block
        jitExecute(jit, hostCode);
        
        // Return the updated EIP
        return jitGetGuestEIP(jit);
    }

    // Test members
    JitInstance* jit;
};

// Test unconditional JMP (direct)
TEST_F(ControlFlowTest, UnconditionalDirectJump) {
    // Create a simple block with a direct jump
    // JMP 0x2000
    const uint8_t jmpCode[] = {
        0xE9, 0xFB, 0x0F, 0x00, 0x00  // jmp 0x2000 (relative offset from next instruction)
    };
    
    uint32_t jmpAddress = 0x1000;
    setupTestCode(jmpAddress, jmpCode, sizeof(jmpCode));
    
    // Target of the jump
    const uint8_t targetCode[] = {
        0x90  // nop
    };
    setupTestCode(0x2000, targetCode, sizeof(targetCode));
    
    // Set initial EIP
    jitSetGuestEIP(jit, jmpAddress);
    
    // Execute and get final EIP
    uint32_t finalEIP = executeAndGetEIP(jmpAddress);
    
    // EIP should be at the target address
    EXPECT_EQ(finalEIP, 0x2000);
}

// Test unconditional JMP (indirect)
TEST_F(ControlFlowTest, UnconditionalIndirectJump) {
    // Create a simple block with an indirect jump
    // MOV EAX, 0x3000
    // JMP EAX
    const uint8_t indirectJmpCode[] = {
        0xB8, 0x00, 0x30, 0x00, 0x00,  // mov eax, 0x3000
        0xFF, 0xE0                    // jmp eax
    };
    
    uint32_t jmpAddress = 0x1100;
    setupTestCode(jmpAddress, indirectJmpCode, sizeof(indirectJmpCode));
    
    // Target of the jump
    const uint8_t targetCode[] = {
        0x90  // nop
    };
    setupTestCode(0x3000, targetCode, sizeof(targetCode));
    
    // Set initial EIP
    jitSetGuestEIP(jit, jmpAddress);
    
    // Execute and get final EIP
    uint32_t finalEIP = executeAndGetEIP(jmpAddress);
    
    // EIP should be at the target address
    EXPECT_EQ(finalEIP, 0x3000);
}

// Test conditional jump (JZ/JE) - taken
TEST_F(ControlFlowTest, ConditionalJumpTaken) {
    // Create a block with a conditional jump that should be taken
    // XOR EAX, EAX   (sets ZF)
    // JZ target
    // MOV EBX, 1     (should be skipped)
    // target: NOP
    const uint8_t conditionalJmpCode[] = {
        0x31, 0xC0,                   // xor eax, eax
        0x74, 0x05,                   // jz target (skip 5 bytes)
        0xBB, 0x01, 0x00, 0x00, 0x00, // mov ebx, 1 (should be skipped)
        0x90                          // target: nop
    };
    
    uint32_t jmpAddress = 0x1200;
    setupTestCode(jmpAddress, conditionalJmpCode, sizeof(conditionalJmpCode));
    
    // Set initial EIP and registers
    jitSetGuestEIP(jit, jmpAddress);
    jitSetGuestRegister(jit, 3, 0); // EBX = 0
    
    // Execute and get final EIP
    uint32_t finalEIP = executeAndGetEIP(jmpAddress);
    
    // EIP should be at the target address (after the mov ebx, 1)
    EXPECT_EQ(finalEIP, jmpAddress + 9); // 0x1200 + 9 = 0x1209
    
    // EBX should still be 0 (mov ebx, 1 was skipped)
    EXPECT_EQ(jitGetGuestRegister(jit, 3), 0);
}

// Test conditional jump (JZ/JE) - not taken
TEST_F(ControlFlowTest, ConditionalJumpNotTaken) {
    // Create a block with a conditional jump that should not be taken
    // MOV EAX, 1     (clears ZF)
    // JZ target      (shouldn't be taken)
    // MOV EBX, 1     (should be executed)
    // target: NOP
    const uint8_t conditionalJmpCode[] = {
        0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1
        0x74, 0x05,                   // jz target (skip 5 bytes, shouldn't be taken)
        0xBB, 0x01, 0x00, 0x00, 0x00, // mov ebx, 1 (should be executed)
        0x90                          // target: nop
    };
    
    uint32_t jmpAddress = 0x1300;
    setupTestCode(jmpAddress, conditionalJmpCode, sizeof(conditionalJmpCode));
    
    // Set initial EIP and registers
    jitSetGuestEIP(jit, jmpAddress);
    jitSetGuestRegister(jit, 3, 0); // EBX = 0
    
    // Execute and get final EIP
    uint32_t finalEIP = executeAndGetEIP(jmpAddress);
    
    // EIP should be at the end of the code
    EXPECT_EQ(finalEIP, jmpAddress + 13); // 0x1300 + 13 = 0x130D
    
    // EBX should be 1 (mov ebx, 1 was executed)
    EXPECT_EQ(jitGetGuestRegister(jit, 3), 1);
}

// Test CALL and RET instructions
TEST_F(ControlFlowTest, CallAndReturn) {
    // Create a main block that calls a subroutine
    // CALL subroutine
    // MOV EAX, 1
    const uint8_t mainCode[] = {
        0xE8, 0x07, 0x00, 0x00, 0x00, // call subroutine (offset 7 from next instr)
        0xB8, 0x01, 0x00, 0x00, 0x00  // mov eax, 1
    };
    
    uint32_t mainAddress = 0x1400;
    setupTestCode(mainAddress, mainCode, sizeof(mainCode));
    
    // Create a subroutine that modifies EBX and returns
    // MOV EBX, 42
    // RET
    const uint8_t subroutineCode[] = {
        0xBB, 0x2A, 0x00, 0x00, 0x00, // mov ebx, 42
        0xC3                          // ret
    };
    
    uint32_t subroutineAddress = 0x1400 + 10; // After the main code
    setupTestCode(subroutineAddress, subroutineCode, sizeof(subroutineCode));
    
    // Set initial EIP and registers
    jitSetGuestEIP(jit, mainAddress);
    jitSetGuestRegister(jit, 0, 0); // EAX = 0
    jitSetGuestRegister(jit, 3, 0); // EBX = 0
    
    // Execute and get final EIP
    uint32_t finalEIP = executeAndGetEIP(mainAddress);
    
    // After completion, EIP should be after the main block
    EXPECT_EQ(finalEIP, mainAddress + 10);
    
    // EAX should be 1 (main block executed)
    EXPECT_EQ(jitGetGuestRegister(jit, 0), 1);
    
    // EBX should be 42 (subroutine executed)
    EXPECT_EQ(jitGetGuestRegister(jit, 3), 42);
}

// Test LOOP instruction
TEST_F(ControlFlowTest, LoopInstruction) {
    // Create a code block that uses LOOP to count down ECX
    // MOV ECX, 5     ; Loop counter
    // XOR EAX, EAX   ; Initialize accumulator
    // loop_start:
    // INC EAX        ; Increment accumulator
    // LOOP loop_start ; Decrement ECX and jump if not zero
    const uint8_t loopCode[] = {
        0xB9, 0x05, 0x00, 0x00, 0x00, // mov ecx, 5
        0x31, 0xC0,                   // xor eax, eax
        // loop_start:
        0x40,                         // inc eax
        0xE2, 0xFD                    // loop loop_start (-3 bytes)
    };
    
    uint32_t loopAddress = 0x1500;
    setupTestCode(loopAddress, loopCode, sizeof(loopCode));
    
    // Set initial EIP
    jitSetGuestEIP(jit, loopAddress);
    
    // Execute and get final EIP
    uint32_t finalEIP = executeAndGetEIP(loopAddress);
    
    // After completion, EIP should be after the loop
    EXPECT_EQ(finalEIP, loopAddress + 10);
    
    // ECX should be 0 (looped 5 times)
    EXPECT_EQ(jitGetGuestRegister(jit, 1), 0);
    
    // EAX should be 5 (incremented 5 times)
    EXPECT_EQ(jitGetGuestRegister(jit, 0), 5);
}

// Test complex branching structure
TEST_F(ControlFlowTest, ComplexBranching) {
    // Create a more complex code block with multiple branches
    // Implements: if (EAX > 10) { EBX = 1; } else { EBX = 2; }
    const uint8_t branchCode[] = {
        0x83, 0xF8, 0x0A,             // cmp eax, 10
        0x7E, 0x07,                   // jle else_branch
        // then_branch:
        0xBB, 0x01, 0x00, 0x00, 0x00, // mov ebx, 1
        0xEB, 0x05,                   // jmp end
        // else_branch:
        0xBB, 0x02, 0x00, 0x00, 0x00, // mov ebx, 2
        // end:
        0x90                          // nop
    };
    
    uint32_t branchAddress = 0x1600;
    setupTestCode(branchAddress, branchCode, sizeof(branchCode));
    
    // Test with EAX > 10 (then branch)
    jitSetGuestEIP(jit, branchAddress);
    jitSetGuestRegister(jit, 0, 15); // EAX = 15
    jitSetGuestRegister(jit, 3, 0);  // EBX = 0
    
    uint32_t finalEIP1 = executeAndGetEIP(branchAddress);
    
    // EIP should be at the end
    EXPECT_EQ(finalEIP1, branchAddress + 16);
    // EBX should be 1 (then branch)
    EXPECT_EQ(jitGetGuestRegister(jit, 3), 1);
    
    // Test with EAX <= 10 (else branch)
    jitSetGuestEIP(jit, branchAddress);
    jitSetGuestRegister(jit, 0, 5);  // EAX = 5
    jitSetGuestRegister(jit, 3, 0);  // EBX = 0
    
    uint32_t finalEIP2 = executeAndGetEIP(branchAddress);
    
    // EIP should be at the end
    EXPECT_EQ(finalEIP2, branchAddress + 16);
    // EBX should be 2 (else branch)
    EXPECT_EQ(jitGetGuestRegister(jit, 3), 2);
} 