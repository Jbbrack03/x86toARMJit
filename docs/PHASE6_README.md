# Phase 6: SMC, Memory Model, & API Finalization

This document describes the implementation of Phase 6 of the XenoARM JIT project, which focuses on:

1. Self-Modifying Code (SMC) detection and handling
2. Memory model consistency (x86 TSO vs ARM weakly-ordered)
3. API finalization

## 1. Overview

Phase 6 adds critical functionality to handle self-modifying code scenarios that are common in Xbox games and demos. It also addresses the memory model differences between x86 and ARM architectures to ensure correct emulation of memory operations. Finally, it completes and finalizes the public API for easy integration with host emulators.

## 2. Self-Modifying Code (SMC) Detection

### 2.1 Implementation Strategy

The SMC detection implementation uses a combination of:

1. **Page Protection**: Marking pages containing JITted code as read-only to detect writes
2. **Signal Handler**: Using a SIGSEGV handler to intercept protection faults when guest code writes to protected pages
3. **Translation Cache Invalidation**: Automatically removing affected translations when SMC is detected

This approach is efficient as it only introduces overhead when SMC actually occurs, which is relatively rare in most guest code.

### 2.2 Key Components

- **MemoryManager**: Tracks which guest memory pages contain JITted code and manages page protection
- **SignalHandler**: Intercepts and processes SIGSEGV signals caused by writes to protected code pages
- **Translation Cache Extensions**: Additional methods to efficiently invalidate affected code blocks

### 2.3 How It Works

1. When code is JITted, the corresponding guest memory pages are marked as containing code via `register_code_page()`
2. These pages are protected as read-only (from the guest perspective)
3. When guest code tries to write to a protected page (SMC), a page fault occurs
4. The signal handler intercepts the fault and calls `handle_protection_fault()`
5. The memory manager temporarily unprotects the page, invalidates affected translations, and allows the write to proceed
6. The page is reprotected after the write completes
7. The next time the code is executed, it will be re-JITted since the old translation was invalidated

### 2.4 Testing

The SMC detection test (`smc_test.cpp`) verifies:

1. Basic translation cache invalidation when code is modified
2. Explicit registration of code memory
3. Notification of memory modification via the API
4. Handling of protection faults

## 3. Memory Model Consistency

### 3.1 Implementation Strategy

The memory model handler addresses the differences between x86's Total Store Order (TSO) model and ARM's weaker memory ordering by inserting appropriate ARM memory barriers when needed.

### 3.2 Key Components

- **MemoryModel**: Analyzes memory operations and inserts appropriate barriers
- **Barrier Types**: Various ARM barriers (DMB, DSB, ISB) implemented to match x86 semantics
- **Memory Barrier IR Instructions**: IR-level representation of memory barriers

### 3.3 How It Works

1. The MemoryModel component analyzes memory operations during IR generation
2. For x86 instructions with implicit ordering semantics (like XCHG, LOCK prefix), it adds appropriate barrier IR instructions
3. For x86 explicit barrier instructions (MFENCE, SFENCE, LFENCE), it generates corresponding ARM barriers
4. During code generation, these barriers are emitted as ARM instructions (DMB, DSB, ISB)

### 3.4 Barrier Mapping

| x86 Operation | ARM Implementation |
|---------------|-------------------|
| MFENCE | DMB ISH (full barrier) |
| SFENCE | DMB ISHST (store barrier) |
| LFENCE | DMB ISHLD (load barrier) |
| LOCK prefix | DMB ISH before and after operation |
| XCHG | DMB ISH (implicit lock on x86) |

### 3.5 Testing

The memory model test (`memory_model_test.cpp`) verifies:

1. Proper translation of x86 memory barriers to ARM equivalents
2. Correct analysis of memory operations to determine barrier requirements
3. Insertion of barriers in the IR for memory operations
4. Handling of different memory ordering scenarios (load->load, store->load, etc.)

## 4. API Finalization

### 4.1 Key API Additions

The finalized API now includes:

- **SMC Handling**: Methods to register code memory and handle protection faults
- **Memory Model Control**: Configuration options for memory model strictness
- **Memory Access Callbacks**: Enhanced callbacks for guest memory access (8/16/32/64-bit)
- **Exception Handling**: Comprehensive guest exception reporting

### 4.2 Error Handling

The API now includes robust error handling:

- Consistent error codes returned by all functions
- Error messages available through logging callbacks
- Parameter validation for all API functions
- Graceful handling of edge cases (null pointers, invalid parameters, etc.)

### 4.3 Testing

The API test suite (`api_tests.cpp`) verifies:

1. Basic initialization and shutdown
2. Translation API functionality
3. SMC detection and invalidation via API
4. Memory barrier insertion
5. Error reporting and handling
6. Parameter validation
7. Full API coverage to ensure all required functions are implemented

## 5. Status Summary

The following table summarizes the current status of Phase 6 components:

| Component | Status |
|-----------|--------|
| SMC Detection | Complete |
| Memory Model | Complete |
| API Finalization | Complete |
| Error Handling | Complete |

## 6. Transition to Phase 7

With Phase 6 now complete, the project is ready to move on to Phase 7: Testing, Optimization & Documentation. The focus will shift to:

1. Comprehensive testing with architectural test suites
2. Performance profiling and optimization
3. Completing all project documentation
4. Preparing for the V1.0 release

All the necessary infrastructure for SMC detection, memory model consistency, and API integration is now in place, providing a solid foundation for the final phase of the project. 