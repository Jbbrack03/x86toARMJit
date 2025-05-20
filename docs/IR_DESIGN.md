# XenoARM JIT - Intermediate Representation (IR) Design

This document details the design of the Intermediate Representation (IR) used within the XenoARM JIT compiler. The IR serves as a crucial layer between the x86 instruction decoder and the AArch64 code generator, facilitating analysis, optimization, and platform-agnostic representation.

## 1. Introduction & Goals

*   **Purpose:** To provide a lightweight, efficient, and extensible representation of x86 code blocks.
*   **Goals:**
    *   Simplify the translation process from x86 to AArch64.
    *   Enable basic optimization passes.
    *   Facilitate debugging and verification of the translation process.
    *   Maintain a level of abstraction from both source (x86) and target (AArch64) architectures where beneficial.

## 2. IR Instruction Set Overview

The IR will consist of a set of simplified, RISC-like instructions. Categories will include:

*   **Arithmetic Operations:** (e.g., `IR_ADD`, `IR_SUB`, `IR_MUL`, `IR_DIV`, `IR_MOD`, `IR_NEG`, `IR_INC`, `IR_DEC`)
*   **Logical Operations:** (e.g., `IR_AND`, `IR_OR`, `IR_XOR`, `IR_NOT`, `IR_SHL`, `IR_SHR`, `IR_SAR`, `IR_ROL`, `IR_ROR`)
*   **Comparison Operations:** (e.g., `IR_CMP_EQ`, `IR_CMP_NE`, `IR_CMP_LT`, `IR_CMP_GT`, `IR_CMP_LE`, `IR_CMP_GE` - signed/unsigned variants)
*   **Memory Access Operations:**
    *   Load: (e.g., `IR_LOAD_8`, `IR_LOAD_16`, `IR_LOAD_32`, `IR_LOAD_64`, `IR_LOAD_128` - for SIMD)
    *   Store: (e.g., `IR_STORE_8`, `IR_STORE_16`, `IR_STORE_32`, `IR_STORE_64`, `IR_STORE_128` - for SIMD)
    *   Addressing Modes: (e.g., Register indirect, Register + Offset)
*   **Control Flow Operations:**
    *   Unconditional Branch: `IR_JMP` (to IR block label or direct address)
    *   Conditional Branch: `IR_BR_COND` (based on IR condition code or operand, to IR block label)
    *   Call: `IR_CALL` (to guest address)
    *   Return: `IR_RET`
    *   Label: `IR_LABEL` (defines an IR block start)
*   **SIMD Operations (Mapping MMX/SSE):**
    *   Vector Arithmetic: (e.g., `IR_VEC_ADD_PS`, `IR_VEC_MUL_PI16`)
    *   Vector Logical: (e.g., `IR_VEC_AND_B`)
    *   Vector Pack/Unpack: (e.g., `IR_VEC_PACKSSDW`)
    *   Vector Shuffle/Permute: (e.g., `IR_VEC_SHUF_PS`)
*   **FPU Operations (Mapping x87 subset):**
    *   Load/Store FPU Register: (e.g., `IR_FLD`, `IR_FSTP`)
    *   FPU Arithmetic: (e.g., `IR_FADD`, `IR_FSUB`, `IR_FMUL`, `IR_FDIV`)
    *   FPU Control: (e.g., `IR_FLDCW`, `IR_FSTCW`)
*   **Flag Operations (Explicit):**
    *   Set/Get individual x86 flags if not directly handled by preceding IR ops. (e.g., `IR_SET_CF`, `IR_GET_ZF`)
    *   This might be more integrated into comparison/arithmetic ops that define flags.
*   **Host Calls/System Interface:**
    *   `IR_HOST_CALL` (to call specific helper functions in the JIT/emulator for complex tasks, e.g., memory access, exception reporting)
*   **Miscellaneous:**
    *   `IR_NOP`
    *   `IR_DEBUG_BREAK`

## 3. IR Operand Types

*   **Virtual Registers:** A large set of typed virtual registers (e.g., `vR0_32`, `vR1_64`, `vXMM0_128`).
*   **Immediates:** Constant values of various sizes (8-bit, 16-bit, 32-bit, 64-bit).
*   **Memory Operands:**
    *   Base Register + Displacement
    *   [Base Register + Index Register * Scale] + Displacement (if complex addressing is directly represented, or broken down)
*   **Labels/Block References:** For control flow.
*   **Condition Codes:** For conditional branches (may map closely to x86 or ARM conditions).

## 4. IR Data Types

The IR will explicitly type its virtual registers and immediate operands.

*   **Integer Types:** `i8`, `u8`, `i16`, `u16`, `i32`, `u32`, `i64`, `u64`.
*   **Floating-Point Types:** `f32` (single-precision), `f64` (double-precision), `f80` (extended-precision for x87 if directly handled).
*   **Vector Types (for SIMD):**
    *   `v64_b8` (8 x 8-bit bytes in 64-bit MMX-like vector)
    *   `v64_w4` (4 x 16-bit words in 64-bit MMX-like vector)
    *   `v64_d2` (2 x 32-bit dwords in 64-bit MMX-like vector)
    *   `v128_b16` (16 x 8-bit bytes in 128-bit SSE-like vector)
    *   `v128_w8` (8 x 16-bit words in 128-bit SSE-like vector)
    *   `v128_d4` (4 x 32-bit dwords/floats in 128-bit SSE-like vector)
    *   `v128_q2` (2 x 64-bit qwords/doubles in 128-bit SSE-like vector)
*   **Pointer Type:** `ptr` (host pointer size, for internal JIT use if needed).

## 5. Structure of IR Instructions

*   Likely a form of **Three-Address Code (TAC)** or **Two-Address Code** where appropriate.
    *   Example (TAC): `vR3_32 = IR_ADD_32 vR1_32, vR2_32`
    *   Example (TAC): `vR2_64 = IR_LOAD_64 [vR0_ptr + 0x10]`
*   **Explicit Flag Handling:** Decisions needed on whether flags are implicit outputs of some IR ops or always explicitly set/read by dedicated IR flag ops. For x86, explicit handling of AF/PF might be cleaner in IR.
*   **Basic Blocks:** IR will be organized into basic blocks, each ending with a control flow instruction.

## 6. Mapping x86 Instructions to IR Sequences (Examples)

*   **`ADD EAX, EBX` (x86)**
    *   `vEAX_32 = IR_ADD_32 vEAX_32, vEBX_32`
    *   `IR_UPDATE_FLAGS_FROM_ADD_32 vEAX_32, vEBX_32, result_vEAX_32` (or flags are implicit)
*   **`MOV ECX, [ESI + 0x10]` (x86)**
    *   `vTEMP_ptr = IR_ADD_PTR vESI_ptr, 0x10`
    *   `vECX_32 = IR_LOAD_32 [vTEMP_ptr]`
*   **`CMP AL, 0x5` (x86)**
    *   `IR_COMPARE_U8 vAL_u8, 0x5` (sets internal IR condition flags)
*   **`JZ short_label` (x86)**
    *   `IR_BR_COND ZERO, label_short_label`

## 7. Planned IR-Level Optimizations (V1.0 Focus - Basic)

*   **Constant Folding:** Evaluate constant expressions at JIT time.
    *   Example: `IR_ADD_32 5, 10` -> `IR_MOV_IMM_32 15`
*   **Dead Code Elimination (within a basic block):** Remove IR instructions whose results are not used.
*   **Peephole Optimizations:** Simple local transformations on IR sequences.
*   *(More advanced optimizations like common subexpression elimination, loop optimizations, etc., are for future versions.)*

## 8. Serialization/Deserialization

*   Consider a textual or binary format for dumping/loading IR.
*   **Purpose:** Invaluable for debugging the decoder, optimizer, and code generator. Allows inspection of the IR for specific x86 code blocks.

---
This initial design will evolve as development progresses.