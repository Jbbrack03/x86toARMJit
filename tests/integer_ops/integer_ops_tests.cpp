#include <gtest/gtest.h>
#include "jit_core/jit_api.h"
#include "eflags/eflags_state.h"
#include <cstring>

// Similar mock memory setup as in dispatcher_tests.cpp
class MockMemory {
public:
    static uint8_t* guestMemory;
    static constexpr size_t GUEST_MEMORY_SIZE = 4096;
    
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

// Test fixture for integer operations tests
class IntegerOpsTest : public ::testing::Test {
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
    
    // Helper to execute a code block
    void executeBlock(uint32_t address, uint32_t* regs) {
        // Get the compiled host code
        void* hostCode = jitGetHostCodeForGuestAddress(jit, address);
        ASSERT_NE(hostCode, nullptr);
        
        // Set registers in JIT instance
        for (int i = 0; i < 8; i++) {
            jitSetGuestRegister(jit, i, regs[i]);
        }
        
        // Execute the block
        jitExecute(jit, hostCode);
        
        // Get the updated register values
        for (int i = 0; i < 8; i++) {
            regs[i] = jitGetGuestRegister(jit, i);
        }
    }

    // Test members
    JitInstance* jit;
};

// Test basic integer arithmetic operations
TEST_F(IntegerOpsTest, BasicArithmetic) {
    // Test ADD
    const uint8_t addCode[] = {
        0x01, 0xD8  // add eax, ebx
    };
    
    uint32_t addAddress = 0x1000;
    setupTestCode(addAddress, addCode, sizeof(addCode));
    
    uint32_t regs[8] = {0};
    regs[0] = 5;    // EAX = 5
    regs[3] = 10;   // EBX = 10
    
    executeBlock(addAddress, regs);
    
    // EAX should now be 15
    EXPECT_EQ(regs[0], 15);
    
    // Test SUB
    const uint8_t subCode[] = {
        0x29, 0xC8  // sub eax, ecx
    };
    
    uint32_t subAddress = 0x1100;
    setupTestCode(subAddress, subCode, sizeof(subCode));
    
    regs[0] = 20;   // EAX = 20
    regs[1] = 7;    // ECX = 7
    
    executeBlock(subAddress, regs);
    
    // EAX should now be 13
    EXPECT_EQ(regs[0], 13);
    
    // Test INC
    const uint8_t incCode[] = {
        0x40  // inc eax
    };
    
    uint32_t incAddress = 0x1200;
    setupTestCode(incAddress, incCode, sizeof(incCode));
    
    regs[0] = 42;   // EAX = 42
    
    executeBlock(incAddress, regs);
    
    // EAX should now be 43
    EXPECT_EQ(regs[0], 43);
    
    // Test DEC
    const uint8_t decCode[] = {
        0x4B  // dec ebx
    };
    
    uint32_t decAddress = 0x1300;
    setupTestCode(decAddress, decCode, sizeof(decCode));
    
    regs[3] = 100;  // EBX = 100
    
    executeBlock(decAddress, regs);
    
    // EBX should now be 99
    EXPECT_EQ(regs[3], 99);
}

// Test logical operations
TEST_F(IntegerOpsTest, LogicalOperations) {
    // Test AND
    const uint8_t andCode[] = {
        0x21, 0xD8  // and eax, ebx
    };
    
    uint32_t andAddress = 0x1000;
    setupTestCode(andAddress, andCode, sizeof(andCode));
    
    uint32_t regs[8] = {0};
    regs[0] = 0xFF00FF00;  // EAX = 0xFF00FF00
    regs[3] = 0x0FF00FF0;  // EBX = 0x0FF00FF0
    
    executeBlock(andAddress, regs);
    
    // EAX should now be 0x0F000F00
    EXPECT_EQ(regs[0], 0x0F000F00);
    
    // Test OR
    const uint8_t orCode[] = {
        0x09, 0xC8  // or eax, ecx
    };
    
    uint32_t orAddress = 0x1100;
    setupTestCode(orAddress, orCode, sizeof(orCode));
    
    regs[0] = 0x0000FFFF;  // EAX = 0x0000FFFF
    regs[1] = 0xFFFF0000;  // ECX = 0xFFFF0000
    
    executeBlock(orAddress, regs);
    
    // EAX should now be 0xFFFFFFFF
    EXPECT_EQ(regs[0], 0xFFFFFFFF);
    
    // Test XOR
    const uint8_t xorCode[] = {
        0x31, 0xC0  // xor eax, eax
    };
    
    uint32_t xorAddress = 0x1200;
    setupTestCode(xorAddress, xorCode, sizeof(xorCode));
    
    regs[0] = 0xDEADBEEF;  // EAX = 0xDEADBEEF
    
    executeBlock(xorAddress, regs);
    
    // EAX should now be 0
    EXPECT_EQ(regs[0], 0);
    
    // Test NOT
    const uint8_t notCode[] = {
        0xF7, 0xD0  // not eax
    };
    
    uint32_t notAddress = 0x1300;
    setupTestCode(notAddress, notCode, sizeof(notCode));
    
    regs[0] = 0x0000FFFF;  // EAX = 0x0000FFFF
    
    executeBlock(notAddress, regs);
    
    // EAX should now be 0xFFFF0000
    EXPECT_EQ(regs[0], 0xFFFF0000);
}

// Test shifts and rotates
TEST_F(IntegerOpsTest, ShiftsAndRotates) {
    // Test SHL
    const uint8_t shlCode[] = {
        0xC1, 0xE0, 0x04  // shl eax, 4
    };
    
    uint32_t shlAddress = 0x1000;
    setupTestCode(shlAddress, shlCode, sizeof(shlCode));
    
    uint32_t regs[8] = {0};
    regs[0] = 0x0000000F;  // EAX = 0x0000000F
    
    executeBlock(shlAddress, regs);
    
    // EAX should now be 0x000000F0
    EXPECT_EQ(regs[0], 0x000000F0);
    
    // Test SHR
    const uint8_t shrCode[] = {
        0xC1, 0xE8, 0x08  // shr eax, 8
    };
    
    uint32_t shrAddress = 0x1100;
    setupTestCode(shrAddress, shrCode, sizeof(shrCode));
    
    regs[0] = 0x00FF0000;  // EAX = 0x00FF0000
    
    executeBlock(shrAddress, regs);
    
    // EAX should now be 0x0000FF00
    EXPECT_EQ(regs[0], 0x0000FF00);
    
    // Test SAR
    const uint8_t sarCode[] = {
        0xC1, 0xF8, 0x04  // sar eax, 4
    };
    
    uint32_t sarAddress = 0x1200;
    setupTestCode(sarAddress, sarCode, sizeof(sarCode));
    
    regs[0] = 0xF0000000;  // EAX = 0xF0000000 (negative number)
    
    executeBlock(sarAddress, regs);
    
    // EAX should now be 0xFF000000 (sign extended)
    EXPECT_EQ(regs[0], 0xFF000000);
    
    // Test ROL
    const uint8_t rolCode[] = {
        0xC1, 0xC0, 0x04  // rol eax, 4
    };
    
    uint32_t rolAddress = 0x1300;
    setupTestCode(rolAddress, rolCode, sizeof(rolCode));
    
    regs[0] = 0xF000000F;  // EAX = 0xF000000F
    
    executeBlock(rolAddress, regs);
    
    // EAX should now be 0x0000000FF
    EXPECT_EQ(regs[0], 0x0000000FF);
}

// Test flags computation
TEST_F(IntegerOpsTest, FlagsComputation) {
    // Test zero flag (ZF)
    const uint8_t zeroFlagCode[] = {
        0x29, 0xC0  // sub eax, eax
    };
    
    uint32_t zfAddress = 0x1000;
    setupTestCode(zfAddress, zeroFlagCode, sizeof(zeroFlagCode));
    
    uint32_t regs[8] = {0};
    regs[0] = 42;   // EAX = 42
    
    executeBlock(zfAddress, regs);
    
    // EAX should now be 0
    EXPECT_EQ(regs[0], 0);
    
    // ZF should be set
    uint32_t eflags = jitGetGuestEflags(jit);
    EXPECT_TRUE(eflags & EFLAGS_ZF);
    
    // Test sign flag (SF)
    const uint8_t signFlagCode[] = {
        0x83, 0xE8, 0x01  // sub eax, 1
    };
    
    uint32_t sfAddress = 0x1100;
    setupTestCode(sfAddress, signFlagCode, sizeof(signFlagCode));
    
    regs[0] = 0;   // EAX = 0
    
    executeBlock(sfAddress, regs);
    
    // EAX should now be -1 (0xFFFFFFFF)
    EXPECT_EQ(regs[0], 0xFFFFFFFF);
    
    // SF should be set
    eflags = jitGetGuestEflags(jit);
    EXPECT_TRUE(eflags & EFLAGS_SF);
    
    // Test carry flag (CF)
    const uint8_t carryFlagCode[] = {
        0x83, 0xD8, 0x01  // sbb eax, 1
    };
    
    uint32_t cfAddress = 0x1200;
    setupTestCode(cfAddress, carryFlagCode, sizeof(carryFlagCode));
    
    regs[0] = 0;   // EAX = 0
    
    // First set the carry flag
    jitSetGuestEflags(jit, EFLAGS_CF);
    
    executeBlock(cfAddress, regs);
    
    // EAX should now be -2 (0xFFFFFFFE)
    EXPECT_EQ(regs[0], 0xFFFFFFFE);
    
    // Test overflow flag (OF)
    const uint8_t overflowFlagCode[] = {
        0x05, 0x01, 0x00, 0x00, 0x80  // add eax, 0x80000001
    };
    
    uint32_t ofAddress = 0x1300;
    setupTestCode(ofAddress, overflowFlagCode, sizeof(overflowFlagCode));
    
    regs[0] = 0x7FFFFFFF;   // EAX = 0x7FFFFFFF (max positive int32)
    
    executeBlock(ofAddress, regs);
    
    // Result should overflow
    EXPECT_EQ(regs[0], 0x80000000);
    
    // OF should be set
    eflags = jitGetGuestEflags(jit);
    EXPECT_TRUE(eflags & EFLAGS_OF);
} 