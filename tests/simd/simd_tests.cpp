#include "gtest/gtest.h"
#include "xenoarm_jit/api.h" // Use the new API header
#include <cstdint>           // For uint64_t, uint8_t etc.
#include <vector>
#include <cstring> // For memcpy
#include <array>
#include "logging/logger.h" // Make sure Logger is included for XenoARM_JIT::Logger

// Mock memory implementation (simplified, ensure it covers what tests need)
class MockMemory {
public:
    static constexpr size_t GUEST_MEMORY_SIZE = 16 * 1024 * 1024; // 16MB
    static uint8_t guest_memory[GUEST_MEMORY_SIZE];

    static bool initialize() {
        std::memset(guest_memory, 0, GUEST_MEMORY_SIZE);
        return true;
    }
    static void cleanup() {
        // No specific cleanup needed for a static array unless dynamically allocated
    }

    static uint8_t read8(uint32_t address) {
        if (address < GUEST_MEMORY_SIZE) return guest_memory[address];
        ADD_FAILURE() << "MockMemory: read8 out of bounds: " << address;
        return 0;
    }
    static void write8(uint32_t address, uint8_t value) {
        if (address < GUEST_MEMORY_SIZE) guest_memory[address] = value;
        else ADD_FAILURE() << "MockMemory: write8 out of bounds: " << address;
    }

    static uint16_t read16(uint32_t address) {
        if (address + 1 < GUEST_MEMORY_SIZE) return *reinterpret_cast<uint16_t*>(&guest_memory[address]);
        ADD_FAILURE() << "MockMemory: read16 out of bounds: " << address;
        return 0;
    }
    static void write16(uint32_t address, uint16_t value) {
        if (address + 1 < GUEST_MEMORY_SIZE) *reinterpret_cast<uint16_t*>(&guest_memory[address]) = value;
        else ADD_FAILURE() << "MockMemory: write16 out of bounds: " << address;
    }

    static uint32_t read32(uint32_t address) {
        if (address + 3 < GUEST_MEMORY_SIZE) return *reinterpret_cast<uint32_t*>(&guest_memory[address]);
        ADD_FAILURE() << "MockMemory: read32 out of bounds: " << address;
        return 0;
    }
    static void write32(uint32_t address, uint32_t value) {
        if (address + 3 < GUEST_MEMORY_SIZE) *reinterpret_cast<uint32_t*>(&guest_memory[address]) = value;
        else ADD_FAILURE() << "MockMemory: write32 out of bounds: " << address;
    }
    
    static uint64_t read64(uint32_t address) {
        if (address + 7 < GUEST_MEMORY_SIZE) return *reinterpret_cast<uint64_t*>(&guest_memory[address]);
        ADD_FAILURE() << "MockMemory: read64 out of bounds: " << address;
        return 0;
    }
    static void write64(uint32_t address, uint64_t value) {
        if (address + 7 < GUEST_MEMORY_SIZE) *reinterpret_cast<uint64_t*>(&guest_memory[address]) = value;
        else ADD_FAILURE() << "MockMemory: write64 out of bounds: " << address;
    }

    static void read_block(uint32_t address, void* buffer, uint32_t size) {
        if (address + size <= GUEST_MEMORY_SIZE) {
            std::memcpy(buffer, &guest_memory[address], size);
        } else {
            ADD_FAILURE() << "MockMemory: read_block out of bounds: " << address << " size " << size;
            std::memset(buffer, 0, size); // Zero out buffer on error
        }
    }
    static void write_block(uint32_t address, const void* buffer, uint32_t size) {
        if (address + size <= GUEST_MEMORY_SIZE) {
            std::memcpy(&guest_memory[address], buffer, size);
        } else {
            ADD_FAILURE() << "MockMemory: write_block out of bounds: " << address << " size " << size;
        }
    }
};
uint8_t MockMemory::guest_memory[MockMemory::GUEST_MEMORY_SIZE];


// JitConfig callback functions
uint8_t host_read_u8(uint32_t address, void* /*user_data*/) { return MockMemory::read8(address); }
uint16_t host_read_u16(uint32_t address, void* /*user_data*/) { return MockMemory::read16(address); }
uint32_t host_read_u32(uint32_t address, void* /*user_data*/) { return MockMemory::read32(address); }
uint64_t host_read_u64(uint32_t address, void* /*user_data*/) { return MockMemory::read64(address); }
void host_read_block(uint32_t address, void* buffer, uint32_t size, void* /*user_data*/) { MockMemory::read_block(address, buffer, size); }

void host_write_u8(uint32_t address, uint8_t value, void* /*user_data*/) { MockMemory::write8(address, value); }
void host_write_u16(uint32_t address, uint16_t value, void* /*user_data*/) { MockMemory::write16(address, value); }
void host_write_u32(uint32_t address, uint32_t value, void* /*user_data*/) { MockMemory::write32(address, value); }
void host_write_u64(uint32_t address, uint64_t value, void* /*user_data*/) { MockMemory::write64(address, value); }
void host_write_block(uint32_t address, const void* buffer, uint32_t size, void* /*user_data*/) { MockMemory::write_block(address, buffer, size); }

void host_log_callback(int level, const char* message) {
    // Basic logging, can be improved
    std::cout << "JIT_LOG level " << level << ": " << message << std::endl;
}

void host_exception_callback(const XenoARM_JIT::GuestException& exception, void* /*user_data*/) {
    ADD_FAILURE() << "Guest exception: type=" << exception.type << " code=" << exception.code << " address=" << std::hex << exception.address;
}


class SIMDTest : public ::testing::Test {
protected:
    XenoARM_JIT::JitContext* jit_ctx = nullptr;

    void SetUp() override {
        MockMemory::initialize();
        XenoARM_JIT::Logger::getInstance().setLogLevel(XenoARM_JIT::DEBUG);
        
        XenoARM_JIT::JitConfig config;
        config.user_data = nullptr; // Not used in these simple callbacks

        config.read_memory_u8 = host_read_u8;
        config.read_memory_u16 = host_read_u16;
        config.read_memory_u32 = host_read_u32;
        config.read_memory_u64 = host_read_u64;
        config.read_memory_block = host_read_block;

        config.write_memory_u8 = host_write_u8;
        config.write_memory_u16 = host_write_u16;
        config.write_memory_u32 = host_write_u32;
        config.write_memory_u64 = host_write_u64;
        config.write_memory_block = host_write_block;
        
        config.log_callback = host_log_callback;
        config.exception_callback = host_exception_callback;
        
        // Set reasonable defaults for other config values if necessary
        // config.code_cache_size = 16 * 1024 * 1024; // Default from api.h
        // config.page_size = 4096; // Default
        // config.enable_smc_detection = true; // Default
        // config.conservative_memory_model = true; // Default

        jit_ctx = XenoARM_JIT::Jit_Init(config);
        ASSERT_NE(jit_ctx, nullptr) << "JIT initialization failed.";
    }

    void TearDown() override {
        if (jit_ctx) {
            XenoARM_JIT::Jit_Shutdown(jit_ctx);
            jit_ctx = nullptr;
        }
        MockMemory::cleanup();
    }

    // Helper to copy code to mock memory and tell JIT about it
    void setupTestCode(uint32_t address, const uint8_t* code, size_t size) {
        MockMemory::write_block(address, code, size);
        // Optional: If JIT needs to be explicitly told about code regions for some reason
        // XenoARM_JIT::Jit_RegisterCodeMemory(jit_ctx, address, size);
    }
    
    // Helper to translate and execute a block of code
    void executeBlock(uint32_t address) {
        ASSERT_NE(jit_ctx, nullptr);
        void* host_code = XenoARM_JIT::Jit_TranslateBlock(jit_ctx, address);
        ASSERT_NE(host_code, nullptr) << "JIT translation failed for address 0x" << std::hex << address;
        XenoARM_JIT::Jit_ExecuteBlock(jit_ctx, host_code);
    }
    
    // Helper to set an MMX register
    void setMMXRegister(int regIdx, uint64_t value) {
        if (!jit_ctx) {
            ADD_FAILURE() << "jit_ctx is null in setMMXRegister for reg " << regIdx;
            return;
        }
        bool success = XenoARM_JIT::Jit_SetGuestMMXRegister(jit_ctx, regIdx, value);
        if (!success) {
            ADD_FAILURE() << "Jit_SetGuestMMXRegister failed for reg " << regIdx << " with value 0x" << std::hex << value;
        }
    }
    
    // Helper to get an MMX register
    uint64_t getMMXRegister(int regIdx) {
        if (!jit_ctx) {
            ADD_FAILURE() << "jit_ctx is null in getMMXRegister for reg " << regIdx;
            return 0; // Return a default/error value
        }
        uint64_t value = 0;
        bool success = XenoARM_JIT::Jit_GetGuestMMXRegister(jit_ctx, regIdx, &value);
        if (!success) {
            ADD_FAILURE() << "Jit_GetGuestMMXRegister failed for reg " << regIdx;
            // Optionally, could return a specific error sentinel value if 0 is a valid success result
        }
        return value;
    }
    
    // Helper to set an XMM register (value is 16 bytes)
    void setXMMRegister(int regIdx, const uint8_t* value) {
        if (!jit_ctx) {
            ADD_FAILURE() << "jit_ctx is null in setXMMRegister for reg " << regIdx;
            return;
        }
        bool success = XenoARM_JIT::Jit_SetGuestXMMRegister(jit_ctx, regIdx, value);
        if (!success) {
            ADD_FAILURE() << "Jit_SetGuestXMMRegister failed for reg " << regIdx;
        }
    }
    
    // Helper to get an XMM register (value is a 16-byte buffer)
    void getXMMRegister(int regIdx, uint8_t* value_out) {
        if (!jit_ctx) {
            ADD_FAILURE() << "jit_ctx is null in getXMMRegister for reg " << regIdx;
            // Zero out the buffer to ensure defined state on error
            if (value_out) std::memset(value_out, 0, 16);
            return;
        }
        bool success = XenoARM_JIT::Jit_GetGuestXMMRegister(jit_ctx, regIdx, value_out);
        if (!success) {
            ADD_FAILURE() << "Jit_GetGuestXMMRegister failed for reg " << regIdx;
            // Zero out the buffer on API failure as well
            if (value_out) std::memset(value_out, 0, 16);
        }
    }
};

// Test MMX basic operations
TEST_F(SIMDTest, MMXBasicOperations) {
    // Test MOVQ (move quadword) - register to register
    const uint8_t movqCode[] = {
        0x0F, 0x6F, 0xC1  // movq mm0, mm1
    };
    
    uint32_t movqAddress = 0x1000;
    setupTestCode(movqAddress, movqCode, sizeof(movqCode));
    
    // Set initial register values
    uint64_t mm1_value = 0x0123456789ABCDEF;
    setMMXRegister(1, mm1_value);
    setMMXRegister(0, 0); // Clear mm0
    
    // Execute
    executeBlock(movqAddress);
    
    // Verify MM0 now contains MM1's value
    EXPECT_EQ(getMMXRegister(0), mm1_value);
    
    // Test memory load
    const uint8_t movqMemLoadCode[] = { // Renamed for clarity
        0x0F, 0x6F, 0x05, 0x00, 0x20, 0x00, 0x00  // movq mm0, [0x2000]
    };
    
    uint32_t movqMemLoadAddress = 0x1100;
    setupTestCode(movqMemLoadAddress, movqMemLoadCode, sizeof(movqMemLoadCode));
    
    // Set up memory with a test value
    uint64_t memValue = 0xFEDCBA9876543210;
    MockMemory::write64(0x2000, memValue);
    
    // Clear MM0
    setMMXRegister(0, 0);
    
    // Execute
    executeBlock(movqMemLoadAddress);
    
    // Verify MM0 contains the memory value
    EXPECT_EQ(getMMXRegister(0), memValue);
    
    // Test memory store
    const uint8_t movqMemStoreCode[] = { // Renamed for clarity
        0x0F, 0x7F, 0x0D, 0x50, 0x20, 0x00, 0x00  // movq [0x2050], mm1
    };
    
    uint32_t movqMemStoreAddress = 0x1200;
    setupTestCode(movqMemStoreAddress, movqMemStoreCode, sizeof(movqMemStoreCode));
    
    // Set MM1 for storing
    uint64_t mm1_store_val = 0xAABBCCDDEEFF0011;
    setMMXRegister(1, mm1_store_val);
    
    // Clear the destination memory
    MockMemory::write64(0x2050, 0);
    
    // Execute
    executeBlock(movqMemStoreAddress);
    
    // Verify memory now contains MM1's value
    EXPECT_EQ(MockMemory::read64(0x2050), mm1_store_val);
}

// Test MMX arithmetic operations
TEST_F(SIMDTest, MMXArithmetic) {
    // PADDB (Packed Add Bytes) mm0, mm1
    const uint8_t paddbCode[] = { 0x0F, 0xFC, 0xC1 };
    setupTestCode(0x1300, paddbCode, sizeof(paddbCode));

    uint64_t mm0_in = 0x0102030405060708;
    uint64_t mm1_in = 0x1020304050607080;
    setMMXRegister(0, mm0_in);
    setMMXRegister(1, mm1_in);
    executeBlock(0x1300);
    
    // Expected: 0x01+0x10=0x11, 0x02+0x20=0x22, ..., 0x08+0x80=0x88
    uint64_t expected_paddb = 0x1122334455667788;
    EXPECT_EQ(getMMXRegister(0), expected_paddb) << "PADDB failed";

    // PSUBW (Packed Subtract Words) mm2, mm3
    const uint8_t psubwCode[] = { 0x0F, 0xF9, 0xD3 }; // mm2, mm3
    setupTestCode(0x1310, psubwCode, sizeof(psubwCode));

    uint64_t mm2_in = 0x0010002000300040;
    uint64_t mm3_in = 0x0001000200030004;
    setMMXRegister(2, mm2_in);
    setMMXRegister(3, mm3_in);
    executeBlock(0x1310);

    // Expected: 0x0010-0x0001=0x000F, ..., 0x0040-0x0004=0x003C
    uint64_t expected_psubw = 0x000F001E002D003C;
    EXPECT_EQ(getMMXRegister(2), expected_psubw) << "PSUBW failed";
    
    // PMULLW (Packed Multiply Low Words) mm4, mm5
    const uint8_t pmullwCode[] = { 0x0F, 0xD5, 0xE5 }; // mm4, mm5
    setupTestCode(0x1320, pmullwCode, sizeof(pmullwCode));
    
    uint64_t mm4_in = 0x0002000300040005; // Words: 2, 3, 4, 5
    uint64_t mm5_in = 0x0006000700080009; // Words: 6, 7, 8, 9
    setMMXRegister(4, mm4_in);
    setMMXRegister(5, mm5_in);
    executeBlock(0x1320);
    
    // Expected: 2*6=12 (0x000C), 3*7=21 (0x0015), 4*8=32 (0x0020), 5*9=45 (0x002D)
    uint64_t expected_pmullw = 0x000C00150020002D;
    EXPECT_EQ(getMMXRegister(4), expected_pmullw) << "PMULLW failed";
}

// Test MMX logical operations
TEST_F(SIMDTest, MMXLogical) {
    // PAND mm0, mm1
    const uint8_t pandCode[] = { 0x0F, 0xDB, 0xC1 };
    setupTestCode(0x1400, pandCode, sizeof(pandCode));
    uint64_t mm0_in_pand = 0xFF00FF00FF00FF00;
    uint64_t mm1_in_pand = 0xF0F0F0F0F0F0F0F0;
    setMMXRegister(0, mm0_in_pand);
    setMMXRegister(1, mm1_in_pand);
    executeBlock(0x1400);
    EXPECT_EQ(getMMXRegister(0), (mm0_in_pand & mm1_in_pand));

    // POR mm2, mm3
    const uint8_t porCode[] = { 0x0F, 0xEB, 0xD3 };
    setupTestCode(0x1410, porCode, sizeof(porCode));
    uint64_t mm2_in_por = 0x00FF00FF00FF00FF;
    uint64_t mm3_in_por = 0xF000F000F000F000;
    setMMXRegister(2, mm2_in_por);
    setMMXRegister(3, mm3_in_por);
    executeBlock(0x1410);
    EXPECT_EQ(getMMXRegister(2), (mm2_in_por | mm3_in_por));

    // PXOR mm4, mm5
    const uint8_t pxorCode[] = { 0x0F, 0xEF, 0xE5 };
    setupTestCode(0x1420, pxorCode, sizeof(pxorCode));
    uint64_t mm4_in_pxor = 0xAAFFAAFFFF0000AA;
    uint64_t mm5_in_pxor = 0xFF00FF00AAFFAA00;
    setMMXRegister(4, mm4_in_pxor);
    setMMXRegister(5, mm5_in_pxor);
    executeBlock(0x1420);
    EXPECT_EQ(getMMXRegister(4), (mm4_in_pxor ^ mm5_in_pxor));
}

// Test MMX comparison (example: PCMPEQB - Packed Compare for Equal Bytes)
TEST_F(SIMDTest, MMXComparison) {
    // PCMPEQB mm0, mm1
    const uint8_t pcmpeqbCode[] = { 0x0F, 0x74, 0xC1 };
    setupTestCode(0x1500, pcmpeqbCode, sizeof(pcmpeqbCode));

    uint64_t mm0_in = 0x010203FF01020300;
    uint64_t mm1_in = 0x01020300010203FF;
    setMMXRegister(0, mm0_in);
    setMMXRegister(1, mm1_in);
    executeBlock(0x1500);
    // Expected: FF FF FF 00 FF FF FF 00 (bytes are equal or not)
    uint64_t expected_pcmpeqb = 0xFFFFFF00FFFFFF00; 
    EXPECT_EQ(getMMXRegister(0), expected_pcmpeqb);
}


// Test MMX shifts (example: PSLLQ - Packed Shift Left Logical Quadword)
TEST_F(SIMDTest, MMXShifts) {
    // PSLLQ mm0, mm1 (mm1 contains count) - this is problematic as mm1 is a full reg
    // For an immediate shift count, it would be: 0F 73 /7 ib
    // For PSLLQ mm0, imm8: 0x66, 0x0F, 0x73, 0xF0, count (e.g., 0x08 for 8 bits)
    // Let's test PSLLQ mm0, 8
    const uint8_t psllqCode[] = { 0x66, 0x0F, 0x73, 0xF0, 0x08 };
    setupTestCode(0x1600, psllqCode, sizeof(psllqCode));

    uint64_t mm0_in_psllq = 0x0001020304050607;
    setMMXRegister(0, mm0_in_psllq);
    executeBlock(0x1600);
    EXPECT_EQ(getMMXRegister(0), (mm0_in_psllq << 8));

    // PSRLQ mm1, 16
    const uint8_t psrlqCode[] = { 0x66, 0x0F, 0x73, 0xD1, 0x10 }; // PSRLQ mm1, 16
    setupTestCode(0x1610, psrlqCode, sizeof(psrlqCode));
    uint64_t mm1_in_psrlq = 0xFFEEDDCCBBAA9988;
    setMMXRegister(1, mm1_in_psrlq);
    executeBlock(0x1610);
    EXPECT_EQ(getMMXRegister(1), (mm1_in_psrlq >> 16));
}


// Test SSE basic operations (e.g. MOVAPS - Move Aligned Packed Single-Precision)
TEST_F(SIMDTest, SSEBasicOperations) {
    // MOVAPS xmm0, xmm1
    const uint8_t movapsCode[] = { 0x0F, 0x28, 0xC1 }; // xmm0 from xmm1
    setupTestCode(0x2000, movapsCode, sizeof(movapsCode));

    alignas(16) float xmm1_in_f[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    alignas(16) float xmm0_out_f[4];

    setXMMRegister(1, reinterpret_cast<const uint8_t*>(xmm1_in_f));
    alignas(16) std::array<float, 4> xmm0_clear_val_basic = {0.0f, 0.0f, 0.0f, 0.0f};
    setXMMRegister(0, reinterpret_cast<const uint8_t*>(xmm0_clear_val_basic.data()));

    executeBlock(0x2000);

    getXMMRegister(0, reinterpret_cast<uint8_t*>(xmm0_out_f));
    for(int i=0; i<4; ++i) EXPECT_FLOAT_EQ(xmm0_out_f[i], xmm1_in_f[i]);
}

// Test SSE memory operations
TEST_F(SIMDTest, SSEMemoryOperations) {
    // MOVAPS xmm0, [mem_addr] (aligned)
    const uint8_t movapsLoadCode[] = { 0x0F, 0x28, 0x05, 0x00, 0x30, 0x00, 0x00 }; // movaps xmm0, [0x3000]
    uint32_t loadAddr = 0x2100;
    setupTestCode(loadAddr, movapsLoadCode, sizeof(movapsLoadCode));

    alignas(16) float mem_in_f[4] = {10.0f, 20.0f, 30.0f, 40.0f};
    MockMemory::write_block(0x3000, mem_in_f, sizeof(mem_in_f)); // Ensure memory is aligned if using aligned move

    alignas(16) float xmm0_out_f[4];
    alignas(16) std::array<float, 4> xmm0_clear_val_mem = {0.0f, 0.0f, 0.0f, 0.0f};
    setXMMRegister(0, reinterpret_cast<const uint8_t*>(xmm0_clear_val_mem.data()));

    executeBlock(loadAddr);
    getXMMRegister(0, reinterpret_cast<uint8_t*>(xmm0_out_f));
    for(int i=0; i<4; ++i) EXPECT_FLOAT_EQ(xmm0_out_f[i], mem_in_f[i]);

    // MOVAPS [mem_addr], xmm1 (aligned)
    const uint8_t movapsStoreCode[] = { 0x0F, 0x29, 0x0D, 0x00, 0x31, 0x00, 0x00 }; // movaps [0x3100], xmm1
    uint32_t storeAddr = 0x2200;
    setupTestCode(storeAddr, movapsStoreCode, sizeof(movapsStoreCode));

    alignas(16) float xmm1_in_f[4] = {5.5f, 6.6f, 7.7f, 8.8f};
    setXMMRegister(1, reinterpret_cast<const uint8_t*>(xmm1_in_f));

    alignas(16) float mem_out_f[4];
    alignas(16) std::array<float, 4> mem_clear_val = {0.0f, 0.0f, 0.0f, 0.0f};
    MockMemory::write_block(0x3100, mem_clear_val.data(), sizeof(mem_clear_val));

    executeBlock(storeAddr);
    MockMemory::read_block(0x3100, mem_out_f, sizeof(mem_out_f));
    for(int i=0; i<4; ++i) EXPECT_FLOAT_EQ(mem_out_f[i], xmm1_in_f[i]);
}


// Test SSE arithmetic (e.g. ADDPS - Add Packed Single-Precision)
TEST_F(SIMDTest, SSEArithmetic) {
    // ADDPS xmm0, xmm1
    const uint8_t addpsCode[] = { 0x0F, 0x58, 0xC1 }; // xmm0, xmm1
    setupTestCode(0x2300, addpsCode, sizeof(addpsCode));

    alignas(16) float xmm0_in_f[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    alignas(16) float xmm1_in_f[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    setXMMRegister(0, reinterpret_cast<const uint8_t*>(xmm0_in_f));
    setXMMRegister(1, reinterpret_cast<const uint8_t*>(xmm1_in_f));

    executeBlock(0x2300);

    alignas(16) float xmm0_out_f[4];
    getXMMRegister(0, reinterpret_cast<uint8_t*>(xmm0_out_f));
    for(int i=0; i<4; ++i) EXPECT_FLOAT_EQ(xmm0_out_f[i], xmm0_in_f[i] + xmm1_in_f[i]);
}

// Test SSE comparison (e.g. CMPPS EQ - Compare Packed Single-Precision Equal)
TEST_F(SIMDTest, SSEComparison) {
    // CMPPS xmm0, xmm1, 0 (Equal)
    const uint8_t cmppsCode[] = { 0x0F, 0xC2, 0xC1, 0x00 }; // xmm0, xmm1, imm8=0 (EQ)
    setupTestCode(0x2400, cmppsCode, sizeof(cmppsCode));

    alignas(16) float xmm0_in_f[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    alignas(16) float xmm1_in_f[4] = {1.0f, 0.0f, 3.0f, 5.0f}; // xmm1[1] and xmm1[3] differ
    setXMMRegister(0, reinterpret_cast<const uint8_t*>(xmm0_in_f));
    setXMMRegister(1, reinterpret_cast<const uint8_t*>(xmm1_in_f));

    executeBlock(0x2400);

    alignas(16) uint32_t xmm0_out_u32[4]; // Result is 0xFFFFFFFF or 0x00000000 per float
    getXMMRegister(0, reinterpret_cast<uint8_t*>(xmm0_out_u32));

    EXPECT_EQ(xmm0_out_u32[0], 0xFFFFFFFF); // 1.0f == 1.0f
    EXPECT_EQ(xmm0_out_u32[1], 0x00000000); // 2.0f != 0.0f
    EXPECT_EQ(xmm0_out_u32[2], 0xFFFFFFFF); // 3.0f == 3.0f
    EXPECT_EQ(xmm0_out_u32[3], 0x00000000); // 4.0f != 5.0f
}

// Example of an SSE2 test (Packed Double-Precision)
TEST_F(SIMDTest, SSE2DoubleArithmetic) {
    // ADDPD (Add Packed Double-Precision Floating-Point Values)
    // addpd xmm0, xmm1
    const uint8_t addpd_code[] = { 0x66, 0x0F, 0x58, 0xC1 }; // 0x66 prefix for SSE2 double
    setupTestCode(0x2500, addpd_code, sizeof(addpd_code));

    alignas(16) double xmm0_double_in[2] = {10.0, 20.0};
    alignas(16) double xmm1_double_in[2] = {30.0, 40.0};

    setXMMRegister(0, reinterpret_cast<uint8_t*>(xmm0_double_in));
    setXMMRegister(1, reinterpret_cast<uint8_t*>(xmm1_double_in));

    executeBlock(0x2500);

    alignas(16) double xmm0_double_out[2];
    getXMMRegister(0, reinterpret_cast<uint8_t*>(xmm0_double_out));

    EXPECT_DOUBLE_EQ(xmm0_double_out[0], 40.0);
    EXPECT_DOUBLE_EQ(xmm0_double_out[1], 60.0);
}

// Future tests could include:
// - MMX state transitions with FPU
// - SSE data conversion instructions
// - SSE shuffle and unpack instructions
// - More complex arithmetic and logical operations
// - Cache line boundary tests for memory operations
// - Denormals, NaNs, Infinities for floating point