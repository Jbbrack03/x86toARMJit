# XenoARM JIT - Register Allocation Strategy

This document outlines the strategy for mapping guest x86 CPU registers to host ARM AArch64 registers within the XenoARM JIT compiler. Efficient register allocation is critical for performance.

## 1. Introduction & Goals

*   **Purpose:** To define how x86 architectural registers (GPRs, MMX, SSE, x87 FPU stack, EFLAGS) are represented and managed using the AArch64's register set.
*   **Goals:**
    *   Maximize the use of AArch64 physical registers to reduce memory access for guest register state.
    *   Provide a clear and efficient mapping scheme.
    *   Minimize the overhead of register spilling and filling.
    *   Integrate cleanly with the Intermediate Representation (IR) and code generation phases.

## 2. AArch64 Target Registers

*   **General Purpose Registers (GPRs):**
    *   `X0-X30` (64-bit)
    *   `W0-W30` (32-bit, lower halves of X0-X30)
    *   `XZR`/`WZR` (Zero Register)
    *   `SP` (Stack Pointer)
*   **SIMD/Floating-Point Registers (NEON):**
    *   `V0-V31` (128-bit)
    *   Can be accessed as `Bn` (8-bit), `Hn` (16-bit), `Sn` (32-bit), `Dn` (64-bit) sub-registers.
*   **Condition Flags:** `NZCV` register (Negative, Zero, Carry, Overflow).

## 3. Mapping Guest x86 Registers

A combination of direct mapping for frequently used registers and a "guest context" structure in memory for less frequently accessed or spilled registers will likely be used.

### 3.1. x86 General Purpose Registers (GPRs)

*   `EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP` (32-bit) and their 16-bit (`AX, BX, ...`) and 8-bit (`AL, AH, ...`) counterparts.
*   **Strategy:**
    *   Attempt to map the primary 32-bit GPRs directly to a subset of AArch64 `W` registers (e.g., `W0-W7`).
    *   The JIT will maintain a canonical guest context structure in memory. If an x86 GPR is not currently live in an ARM register, its value will be loaded from this context. Writes will update the ARM register and potentially the context (if the ARM register needs to be repurposed).
    *   Access to 8-bit/16-bit sub-registers will be emulated using ARM bit manipulation instructions on the 32-bit `W` registers.

### 3.2. x86 MMX Registers

*   `MM0-MM7` (64-bit, aliased with x87 FPU stack).
*   **Strategy:**
    *   Map `MM0-MM7` to the lower 64-bits (`Dn`) of AArch64 `V0-V7` registers.
    *   Careful state management is needed due to aliasing with x87 FPU registers. The JIT must track whether a register is in MMX or FPU state.

### 3.3. x86 SSE Registers

*   `XMM0-XMM7` (128-bit).
*   **Strategy:**
    *   Map `XMM0-XMM7` directly to AArch64 `V8-V15` registers (or another dedicated subset of V registers).

### 3.4. x86 x87 FPU Register Stack

*   `ST(0)-ST(7)` (80-bit floating-point registers, forming a stack).
*   **Strategy (Challenging):**
    *   **Option 1 (Memory-based stack):** Emulate the x87 stack entirely in a dedicated memory region within the guest context. All x87 operations load operands from this memory stack, compute, and store results back. ARM `V` registers would be used for temporary calculations.
        *   Pros: Simpler to implement stack semantics (TOP pointer, empty/full conditions).
        *   Cons: Higher overhead due to memory access.
    *   **Option 2 (Partial Register Mapping):** Attempt to keep `ST(0)` (top of stack) and perhaps `ST(1)` in dedicated ARM `V` registers (using 80-bit representation if feasible, or 64-bit with precision management). Other stack elements reside in memory.
        *   Pros: Faster access to top-of-stack.
        *   Cons: More complex management of stack operations (`FINCSTP`, `FDECSTP`, `FXCH`).
    *   **V1.0 Approach:** Likely start with Option 1 (memory-based stack) for simplicity and correctness, given "minimal x87 FPU support" goal. Performance can be revisited.
    *   The FPU Control Word (FPCW), Status Word (FPSW), and Tag Word (FPTW) will also be part of the guest context in memory.

### 3.5. x86 EFLAGS Register

*   32-bit register with individual flags (CF, PF, AF, ZF, SF, TF, IF, DF, OF, etc.).
*   **Strategy:**
    *   **Direct Mapping (Partial):**
        *   `ZF` (Zero Flag) -> ARM `Z` flag.
        *   `SF` (Sign Flag) -> ARM `N` flag.
        *   `CF` (Carry Flag) -> ARM `C` flag (with careful handling for subtract/compare differences).
        *   `OF` (Overflow Flag) -> ARM `V` flag (with careful handling).
    *   **Explicit Storage (in Guest Context or dedicated ARM GPRs):**
        *   `AF` (Auxiliary Carry Flag): No direct ARM equivalent; must be calculated and stored.
        *   `PF` (Parity Flag): No direct ARM equivalent; must be calculated and stored.
        *   Other less frequently used or system flags (IF, DF, TF, etc.) will reside in the guest context structure.
    *   The IR will likely have operations that explicitly define or use these flags, which the code generator will translate into ARM sequences that update/read the NZCV register or the explicitly stored flag values.

## 4. Register Spilling and Filling

*   When the JIT needs more ARM registers than available for live guest registers or temporaries, spilling occurs.
*   **Strategy:**
    *   Spill to the canonical guest context structure in memory.
    *   The JIT's IR will use virtual registers. The register allocator (even a simple one) will decide which virtual registers map to physical ARM registers for a given block and which are spilled.
    *   For V1.0, a simple scheme might involve reserving a few ARM GPRs for GPR mapping, a few V registers for XMM/MMX, and spilling others or reloading them on demand per block.

## 5. Caller-Saved vs. Callee-Saved Conventions

*   **JITted Code to JITted Code:** Within a single stream of JITted code, the JIT controls all register usage.
*   **JITted Code to Host Helper Functions:** When JITted code calls out to C/C++ helper functions (e.g., for complex memory operations, API calls), the AArch64 standard calling convention must be respected.
    *   Caller-saved registers (`X0-X18`, `V0-V7`, `V16-V31` typically) can be clobbered by the callee. The JIT must save any live guest state in these registers before calling a host helper if that state needs to be preserved.
    *   Callee-saved registers (`X19-X29`, `SP`, `V8-V15` typically) must be preserved by the callee (host helper).
*   **Host to JITted Code Entry:** The dispatcher will set up guest registers (from context) into their designated ARM registers before jumping to JITted code.

## 6. Interaction with the IR

*   The IR will operate on an abstraction of guest registers or its own set of virtual registers.
*   The register allocation phase (or a simpler mapping strategy for V1.0) occurs between IR generation/optimization and AArch64 code generation.
*   The code generator will then emit ARM instructions using the physical ARM registers assigned to IR virtual registers.

## 7. Initial Algorithm (V1.0)

*   **Basic Block Allocator / On-the-Fly Mapping:**
    *   For each x86 basic block being JITted:
        *   Identify x86 registers used.
        *   Attempt to map them to a pre-defined set of preferred ARM registers.
        *   If conflicts arise or more registers are needed, load from/store to the guest context structure in memory.
        *   This is less optimal than global allocators but simpler to implement initially.
*   No complex graph coloring or linear scan algorithms are planned for V1.0. Focus on correctness and a functional mapping.

---
This strategy will be refined as the JIT's IR and code generation capabilities mature.