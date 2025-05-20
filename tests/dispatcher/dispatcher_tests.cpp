#include <gtest/gtest.h>
#include "jit_core/jit_api.h"
#include "translation_cache/translation_cache.h"
#include "decoder/decoder.h"
#include "aarch64/code_generator.h"

// Mock class for memory access
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

// Test fixture for dispatcher tests
class DispatcherTest : public ::testing::Test {
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
    
    // Helper to set up simple test code in mock memory
    void setupTestCode(uint32_t address, const uint8_t* code, size_t size) {
        for (size_t i = 0; i < size; i++) {
            MockMemory::write8(address + i, code[i]);
        }
    }

    // Test members
    JitInstance* jit;
};

// Test basic dispatcher functionality (lookup and code generation)
TEST_F(DispatcherTest, BasicDispatcherFunctionality) {
    // Set up test code in guest memory (mov eax, 42)
    const uint8_t testCode[] = {0xB8, 0x2A, 0x00, 0x00, 0x00};
    uint32_t codeAddress = 0x1000;
    setupTestCode(codeAddress, testCode, sizeof(testCode));
    
    // Get the compiled host code address
    void* hostCode = jitGetHostCodeForGuestAddress(jit, codeAddress);
    
    // Should return a valid pointer (not nullptr) when compilation succeeds
    EXPECT_NE(hostCode, nullptr);
    
    // Lookup again - should return the same pointer (cached)
    void* cachedHostCode = jitGetHostCodeForGuestAddress(jit, codeAddress);
    EXPECT_EQ(hostCode, cachedHostCode);
}

// Test translation cache management
TEST_F(DispatcherTest, TranslationCacheManagement) {
    // Set up multiple code blocks in guest memory
    const uint8_t block1[] = {0xB8, 0x01, 0x00, 0x00, 0x00}; // mov eax, 1
    const uint8_t block2[] = {0xB9, 0x02, 0x00, 0x00, 0x00}; // mov ecx, 2
    
    uint32_t address1 = 0x1000;
    uint32_t address2 = 0x2000;
    
    setupTestCode(address1, block1, sizeof(block1));
    setupTestCode(address2, block2, sizeof(block2));
    
    // Get host code for both blocks
    void* hostCode1 = jitGetHostCodeForGuestAddress(jit, address1);
    void* hostCode2 = jitGetHostCodeForGuestAddress(jit, address2);
    
    // Both should be valid and different
    EXPECT_NE(hostCode1, nullptr);
    EXPECT_NE(hostCode2, nullptr);
    EXPECT_NE(hostCode1, hostCode2);
    
    // Invalidate translation for block1
    jitInvalidateCache(jit, address1, sizeof(block1));
    
    // Lookup block1 again - should return a new pointer
    void* newHostCode1 = jitGetHostCodeForGuestAddress(jit, address1);
    EXPECT_NE(newHostCode1, nullptr);
    EXPECT_NE(newHostCode1, hostCode1); // Should be different from the original
    
    // Block2 should be unaffected
    void* cachedHostCode2 = jitGetHostCodeForGuestAddress(jit, address2);
    EXPECT_EQ(hostCode2, cachedHostCode2);
}

// Test cache invalidation
TEST_F(DispatcherTest, CacheInvalidation) {
    // Set up test code
    const uint8_t testCode[] = {0xB8, 0x2A, 0x00, 0x00, 0x00}; // mov eax, 42
    uint32_t codeAddress = 0x1000;
    setupTestCode(codeAddress, testCode, sizeof(testCode));
    
    // Get host code
    void* hostCode = jitGetHostCodeForGuestAddress(jit, codeAddress);
    EXPECT_NE(hostCode, nullptr);
    
    // Invalidate entire cache
    jitInvalidateAllCache(jit);
    
    // Lookup again - should return a different pointer
    void* newHostCode = jitGetHostCodeForGuestAddress(jit, codeAddress);
    EXPECT_NE(newHostCode, nullptr);
    EXPECT_NE(hostCode, newHostCode);
}

// Test basic block chaining
TEST_F(DispatcherTest, BasicBlockChaining) {
    // Set up two consecutive blocks in memory
    // Block 1: mov eax, 1
    // Block 2: mov eax, 2
    const uint8_t block1[] = {0xB8, 0x01, 0x00, 0x00, 0x00};
    const uint8_t block2[] = {0xB8, 0x02, 0x00, 0x00, 0x00};
    
    uint32_t address1 = 0x1000;
    uint32_t address2 = 0x1005; // Right after block1
    
    setupTestCode(address1, block1, sizeof(block1));
    setupTestCode(address2, block2, sizeof(block2));
    
    // Get host code for block1
    void* hostCode1 = jitGetHostCodeForGuestAddress(jit, address1);
    EXPECT_NE(hostCode1, nullptr);
    
    // At this point, translation cache might have already attempted to chain block1->block2
    // by recognizing they're consecutive
    
    // Get host code for block2
    void* hostCode2 = jitGetHostCodeForGuestAddress(jit, address2);
    EXPECT_NE(hostCode2, nullptr);
    
    // Now explicitly request block chaining (if API supports it)
    // Note: This is an example - actual API might differ
    bool chainingResult = jitChainBlocks(jit, address1, address2);
    
    // Chaining should succeed
    EXPECT_TRUE(chainingResult);
} 