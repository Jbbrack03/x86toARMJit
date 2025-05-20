# XenoARM JIT - Testing Strategy

## 1. Introduction

This document outlines the testing strategy for the XenoARM JIT project. The primary goals of this strategy are to ensure the JIT's correctness in emulating x86 instructions, its stability under various conditions, and its performance characteristics, particularly on target ARM hardware. A comprehensive testing approach is crucial for the success of an emulator JIT.

## 2. Types of Testing

A multi-layered testing approach will be employed, covering different aspects of the JIT.

### 2.1. Unit Testing

*   **Objective:** To test individual components and modules of the JIT in isolation.
*   **Scope:**
    *   **x86 Decoder:**
        *   Verify correct decoding of individual x86 instructions (opcodes, operands, addressing modes).
        *   Test handling of instruction prefixes (operand size, address size, segment overrides, REX if ever applicable).
        *   Test boundary conditions and invalid instruction sequences.
    *   **Intermediate Representation (IR) Generation:**
        *   Verify that the decoder correctly translates x86 instructions into the expected IR sequences as per [`docs/IR_DESIGN.md`](IR_DESIGN.md:1).
        *   Test with various x86 instruction combinations.
    *   **AArch64 Code Generation:**
        *   Verify correct translation of individual IR instructions/constructs into AArch64 machine code.
        *   Test register mapping and usage for specific IR operations.
    *   **JIT Core Components:**
        *   Translation Cache (TC) management: Test lookup, insertion, (later) invalidation, and eviction logic.
        *   Dispatcher logic: Test transitions between JIT entry, TC execution, and fallback (if any).
        *   Register Allocator (basic mapping): Test correctness of guest-to-host register mapping.
        *   Memory Interface Wrappers: Test the JIT's internal handling of memory callback results.
*   **Framework:** A C++ unit testing framework such as Google Test or Catch2 will be used (as indicated in [`docs/BUILD_AND_TEST.md`](BUILD_AND_TEST.md:13)).
*   **Methodology:** Each module will have a corresponding set of unit tests. Tests will be data-driven where possible (e.g., input x86 byte stream, expected decoded structure or IR output).

### 2.2. Integration Testing

*   **Objective:** To test the interaction between different components of the JIT.
*   **Scope:**
    *   Test the full pipeline: x86 Decoder -> IR Generator -> (Optimizer) -> AArch64 Code Generator -> TC -> Execution.
    *   Utilize the minimal host emulator stub to:
        *   Load small, targeted x86 test programs/snippets.
        *   Invoke the JIT to compile and run these snippets.
        *   Verify final guest register states, memory outputs, and EFLAGS correctness against expected values.
    *   Test API interactions between the host stub and the JIT.
*   **Methodology:** Develop a suite of small x86 assembly programs designed to test specific JIT functionalities and component interactions.

### 2.3. Architectural / Compatibility Testing

*   **Objective:** To validate the JIT's emulation accuracy against known x86 CPU behavior across a wide range of instructions and scenarios.
*   **Scope:**
    *   Comprehensive testing of IA-32 integer instructions.
    *   MMX instruction set.
    *   SSE instruction set (floating-point and integer).
    *   Minimal x87 FPU instruction subset.
    *   EFLAGS behavior for all supported instructions.
    *   Control flow instructions (jumps, calls, returns, loops).
    *   Exception generation and reporting for various faulting conditions.
*   **Test Suites & Tools:**
    *   **`barotto/test386.asm`** (from [`docs/EXTERNAL_REFERENCES.md`](EXTERNAL_REFERENCES.md:233)): Plan for adapting this suite to run via the host stub, or extracting relevant tests. This suite is designed for low-level CPU testing.
    *   **Other Open-Source x86 Test Suites:** Research and identify other suitable suites (e.g., from projects like Bochs, QEMU, or specific CPU test collections like YACAS if adaptable).
    *   **Custom-Developed Tests:** Create specific, fine-grained tests for instructions or behaviors that are difficult to isolate with generic suites, or for known tricky edge cases in x86 architecture.
*   **Methodology:** Execute these test suites within the JIT environment (via the host stub). Compare execution results (register states, memory, flags) against known correct outputs or the behavior of a reference x86 CPU/emulator.

### 2.4. Performance Testing & Benchmarking

*   **Objective:** To measure and analyze the performance of the JIT and the code it generates.
*   **Scope:**
    *   JIT compilation overhead (time to JIT a block of code).
    *   Execution speed of JITted code.
    *   Translation Cache (TC) effectiveness (hit/miss rates).
    *   Overall CPU emulation performance.
*   **Methodology & Benchmarks:**
    *   Define a clear methodology for running benchmarks within the emulated environment provided by the host stub.
    *   **Synthetic CPU Benchmarks:**
        *   Dhrystone, CoreMark (as mentioned in [`XenoARM_JIT_PRD.md`](../XenoARM_JIT_PRD.md:110)) to get a general idea of CPU throughput.
    *   **Microbenchmarks:**
        *   Measure time taken for JIT compilation of code blocks of varying sizes/complexity.
        *   Measure TC lookup speed.
        *   Measure execution time of specific, frequently used JITted code patterns.
    *   **Simple Original Xbox Homebrew:** Later in development, if the host stub evolves sufficiently, test with simple homebrew applications that are CPU-bound to get more realistic performance feedback.
*   **Target Hardware for Profiling:**
    *   Primary: Retroid Pocket 2 Flip (or similar ARM gaming handheld).
    *   Secondary: Emulated ARM environment (e.g., QEMU user-mode or system-mode configured with a comparable ARM core like Cortex-A75/A55) if physical hardware access is limited during early development.
*   **Metrics:**
    *   Guest instructions per second (GIPS).
    *   Host execution time for benchmark completion.
    *   TC hit/miss ratio.
    *   JIT compilation time per block/function.

### 2.5. Self-Modifying Code (SMC) Testing

*   **Objective:** To ensure the JIT correctly detects and handles self-modifying guest code.
*   **Scope:**
    *   Test cases where guest code overwrites its own instructions that have already been JITted.
    *   Test cases where guest code generates new code in a data area and then jumps to it.
*   **Methodology:**
    *   Develop specific x86 assembly test cases that exhibit various forms of SMC.
    *   Verify that the JIT's SMC detection mechanism (e.g., page protection faults) triggers correctly.
    *   Verify that the Translation Cache invalidates the stale JITted code.
    *   Verify that the modified or newly generated code is correctly re-JITted and executed.
    *   Ensure host instruction cache coherency is maintained.

## 3. Test Harnesses and Infrastructure

*   **Minimal Host Emulator Stub:** This will be a key tool. It will need to:
    *   Load the JIT library.
    *   Provide a simulated guest memory space.
    *   Implement the JIT API callbacks (memory access, exceptions, logging).
    *   Load x86 binary code (test programs) into guest memory.
    *   Set initial guest CPU state.
    *   Invoke the JIT to run the guest code.
    *   Allow inspection of guest CPU state and memory after execution for verification.
*   **Test Automation Scripts:** Scripts (e.g., Python, shell scripts) will be developed to automate:
    *   Building the JIT and host stub.
    *   Running unit tests.
    *   Executing integration and architectural test suites.
    *   Collecting and summarizing test results.
*   **Continuous Integration (CI):**
    *   Consider setting up CI (e.g., using GitHub Actions) to automatically build the project and run all tests on every commit or pull request. This helps catch regressions early.

## 4. Code Coverage

*   **Objective:** To measure the extent to which the JIT's source code is exercised by the test suites.
*   **Tools:** Standard C++ code coverage tools like `gcov` (with `lcov` for report generation) or Clang's source-based code coverage.
*   **Target:** Aim for a high code coverage percentage (e.g., >80-90%) for critical JIT modules like the decoder, IR manipulation, code generator, and TC logic. While 100% coverage is often impractical, focusing on critical paths is essential.

## 5. Debugging Aids for Testing

Effective testing relies on good debugging capabilities.
*   **Enhanced JIT Logging:** Implement verbose logging options within the JIT that can be enabled during tests to trace execution flow, register allocation decisions, TC activity, etc.
*   **IR and AArch64 Code Dumping:** Provide mechanisms (e.g., environment variables, JIT configuration options) to dump:
    *   The IR generated for specific x86 code blocks.
    *   The AArch64 machine code generated from the IR.
    *   This is invaluable for debugging translation errors found by tests.
*   **Debugger Integration:** Ensure the JIT and host stub can be easily debugged with standard debuggers like GDB (or LLDB). This includes:
    *   Building with debug symbols (`-g`).
    *   Techniques for setting breakpoints in JITted code (can be challenging but sometimes possible via host debugger and knowledge of TC layout).
    *   Debugging the JIT's internal state while it's processing guest code.

---
This testing strategy will evolve with the project, with new tests and methodologies added as different JIT features are implemented.