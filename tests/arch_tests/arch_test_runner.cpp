#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <cstdint>
#include <cstring>

#include "jit_core/c_api.h"       
#include "jit_core/jit_api.h"       
#include "xenoarm_jit/memory_manager.h" 
#include "logging/logger.h"

// Use proper namespaces
using namespace xenoarm_jit;
using Logger = xenoarm_jit::logging::Logger;
using LogLevel = xenoarm_jit::logging::LogLevel;

// Architecture Test Runner for XenoARM JIT
// This program loads and runs architecture test suites to verify JIT correctness

#define TEST_REPORT_FILE "arch_test_results.txt"

// Test case structure
struct ArchTest {
    std::string name;
    std::vector<uint8_t> code;
    uint32_t entryPoint;
    std::function<bool(JitState*)> verifier;
};

// Function to load test386.asm tests
std::vector<ArchTest> loadTest386Tests(const std::string& filename) {
    std::vector<ArchTest> tests;
    
    // This will parse test386.asm formatted tests and load them
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to load test386.asm file: " << filename << std::endl;
        return tests;
    }
    
    // Test386 parsing logic will go here
    // For now, we'll create a simple test for demonstration
    
    ArchTest sampleTest;
    sampleTest.name = "Sample x86 Arithmetic Test";
    // Simple addition test: add eax, 42; ret
    sampleTest.code = {0x83, 0xC0, 0x2A, 0xC3};
    sampleTest.entryPoint = 0x1000;
    sampleTest.verifier = [](JitState* state) {
        uint32_t eaxValue = 0;
        jit_get_guest_register(state, JIT_REG_EAX, &eaxValue);
        // Expected: Initial EAX (0) + 42 = 42
        return eaxValue == 42;
    };
    
    tests.push_back(sampleTest);
    
    return tests;
}

// Memory access callbacks
static uint8_t readMemoryU8(uint32_t address, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    return memManager->read_u8(address);
}

static uint16_t readMemoryU16(uint32_t address, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    return memManager->read_u16(address);
}

static uint32_t readMemoryU32(uint32_t address, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    return memManager->read_u32(address);
}

static uint64_t readMemoryU64(uint32_t address, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    return memManager->read_u64(address);
}

static void writeMemoryU8(uint32_t address, uint8_t value, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    memManager->write_u8(address, value);
}

static void writeMemoryU16(uint32_t address, uint16_t value, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    memManager->write_u16(address, value);
}

static void writeMemoryU32(uint32_t address, uint32_t value, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    memManager->write_u32(address, value);
}

static void writeMemoryU64(uint32_t address, uint64_t value, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    memManager->write_u64(address, value);
}

static void readMemoryBlock(uint32_t address, void* buffer, uint32_t size, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    for (uint32_t i = 0; i < size; i++) {
        static_cast<uint8_t*>(buffer)[i] = memManager->read_u8(address + i);
    }
}

static void writeMemoryBlock(uint32_t address, const void* buffer, uint32_t size, void* userData) {
    auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userData);
    for (uint32_t i = 0; i < size; i++) {
        memManager->write_u8(address + i, static_cast<const uint8_t*>(buffer)[i]);
    }
}

// Run a single architectural test
bool runArchTest(const ArchTest& test) {
    std::cout << "Running test: " << test.name << std::endl;
    
    // Initialize memory manager
    auto memManager = std::make_unique<xenoarm_jit::MemoryManager>(nullptr, 4096); // Using 4096 as default page size
    
    // Load test code into memory
    for (size_t i = 0; i < test.code.size(); i++) {
        memManager->write_u8(test.entryPoint + i, test.code[i]);
    }
    
    // Initialize JIT
    JitConfig config;
    
    // Set up memory callbacks
    config.memoryReadCallback = [](void* userdata, uint64_t address, uint32_t size) -> uint64_t {
        auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userdata);
        switch (size) {
            case 1: return memManager->read_u8(address);
            case 2: return memManager->read_u16(address);
            case 4: return memManager->read_u32(address);
            case 8: return memManager->read_u64(address);
            default: return 0;
        }
    };
    
    config.memoryWriteCallback = [](void* userdata, uint64_t address, uint64_t value, uint32_t size) {
        auto memManager = static_cast<xenoarm_jit::MemoryManager*>(userdata);
        switch (size) {
            case 1: memManager->write_u8(address, static_cast<uint8_t>(value)); break;
            case 2: memManager->write_u16(address, static_cast<uint16_t>(value)); break;
            case 4: memManager->write_u32(address, static_cast<uint32_t>(value)); break;
            case 8: memManager->write_u64(address, value); break;
        }
    };
    
    config.userdata = memManager.get();
    
    // Initialize logging
    config.logCallback = [](void* userdata, int level, const char* message) {
        std::cout << "JIT [" << level << "]: " << message << std::endl;
    };
    
    // Initialize JIT
    JitState* state = nullptr;
    if (jit_init(&state, &config) != JIT_SUCCESS) {
        std::cerr << "Failed to initialize JIT" << std::endl;
        return false;
    }
    
    // Set initial register state (all zeros for simplicity)
    jit_set_guest_register(state, JIT_REG_EAX, 0);
    jit_set_guest_register(state, JIT_REG_EBX, 0);
    jit_set_guest_register(state, JIT_REG_ECX, 0);
    jit_set_guest_register(state, JIT_REG_EDX, 0);
    jit_set_guest_register(state, JIT_REG_ESI, 0);
    jit_set_guest_register(state, JIT_REG_EDI, 0);
    jit_set_guest_register(state, JIT_REG_ESP, 0x10000); // Initial stack pointer
    jit_set_guest_register(state, JIT_REG_EBP, 0);
    jit_set_guest_register(state, JIT_REG_EIP, test.entryPoint);
    
    // Enable SMC detection
    jit_enable_smc_detection(state, true);
    
    // Run the test
    bool result = jit_run(state) == JIT_SUCCESS;
    
    // Check the result
    bool testPassed = false;
    if (result) {
        testPassed = test.verifier(state);
    } else {
        std::cerr << "JIT execution failed" << std::endl;
    }
    
    // Clean up
    jit_cleanup(state);
    
    return testPassed;
}

// Main function
int main(int argc, char** argv) {
    std::cout << "XenoARM JIT Architecture Test Runner" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    
    // Initialize logging
    Logger::init(LogLevel::INFO);
    
    std::string test386File = "tests/arch_tests/test386.asm";
    if (argc > 1) {
        test386File = argv[1];
    }
    
    // Load tests
    auto tests = loadTest386Tests(test386File);
    if (tests.empty()) {
        std::cerr << "No tests loaded!" << std::endl;
        return 1;
    }
    
    // Open report file
    std::ofstream reportFile(TEST_REPORT_FILE);
    if (!reportFile) {
        std::cerr << "Failed to create test report file" << std::endl;
        return 1;
    }
    
    // Run tests and collect results
    size_t passedTests = 0;
    
    reportFile << "XenoARM JIT Architecture Test Results" << std::endl;
    reportFile << "=====================================\n" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (const auto& test : tests) {
        bool passed = runArchTest(test);
        
        if (passed) {
            passedTests++;
            std::cout << "[PASS] " << test.name << std::endl;
            reportFile << "[PASS] " << test.name << std::endl;
        } else {
            std::cout << "[FAIL] " << test.name << std::endl;
            reportFile << "[FAIL] " << test.name << std::endl;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Print summary
    reportFile << "\nSummary:" << std::endl;
    reportFile << "--------" << std::endl;
    reportFile << "Total tests: " << tests.size() << std::endl;
    reportFile << "Passed: " << passedTests << std::endl;
    reportFile << "Failed: " << (tests.size() - passedTests) << std::endl;
    reportFile << "Time: " << duration.count() << "ms" << std::endl;
    
    std::cout << "\nSummary:" << std::endl;
    std::cout << "--------" << std::endl;
    std::cout << "Total tests: " << tests.size() << std::endl;
    std::cout << "Passed: " << passedTests << std::endl;
    std::cout << "Failed: " << (tests.size() - passedTests) << std::endl;
    std::cout << "Time: " << duration.count() << "ms" << std::endl;
    
    reportFile.close();
    std::cout << "Test report saved to " << TEST_REPORT_FILE << std::endl;
    
    return (passedTests == tests.size()) ? 0 : 1;
} 