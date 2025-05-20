# XenoARM JIT: Project Overview

## 1. Core Purpose

XenoARM JIT is a high-performance Just-In-Time (JIT) compiler designed to translate **Original Xbox CPU instructions (custom Intel Pentium III Coppermine-based, 32-bit x86 IA-32)** into native **ARM (AArch64) machine code**. This enables the emulation of the Original Xbox CPU on modern ARM-based devices.

## 2. Primary Target Audience & Devices

*   **Primary Audience:** Developers of Xbox emulation software for ARM platforms.
*   **Primary Target Devices:** ARM-based gaming handhelds (e.g., Retroid Pocket 2 Flip), high-end smartphones, tablets, and ARM-based PCs. The **Retroid Pocket 2 Flip** and similar hardware are key focus points for performance and compatibility.

## 3. Key Project Goals

*   **Accuracy:** Faithfully emulate the Original Xbox CPU's instruction set (IA-32, MMX, SSE, minimal x87 FPU) and behavior.
*   **Performance:** Achieve high execution speed on ARM AArch64 by dynamically recompiling code, with a focus on optimizing for target handheld devices.
*   **Modularity:** Develop a JIT core that is extensible and easily integrated into larger emulator projects.
*   **Stability & Reliability:** Ensure the JIT and its generated code are robust.
*   **Maintainability:** Create a well-documented and clean codebase.

## 4. High-Level Architecture

The JIT operates through a pipeline of core components:

*   **Decoder:** Parses x86 instruction streams.
*   **Intermediate Representation (IR):** A lightweight, platform-agnostic representation of decoded instructions, facilitating analysis and optimization.
*   **Optimizer:** Performs basic optimization passes (e.g., constant folding, dead code elimination).
*   **ARM AArch64 Code Generator:** Translates IR into ARMv8-A/ARMv9-A machine code, utilizing NEON SIMD for x86 SSE/MMX.
*   **Translation Cache (TC):** Stores and retrieves compiled ARM code blocks to avoid re-translation.
*   **Dispatcher:** Manages execution flow between JIT-compiled code and the JIT engine.
*   **Self-Modifying Code (SMC) Handling:** Detects and manages guest code that modifies itself, invalidating TC entries as needed.
*   **Memory Management Interface:** Allows the JIT to interact with an external guest memory system.

## 5. Scope (V1.0 - Original Xbox Focus)

*   **In Scope:** Full IA-32 user-mode instruction set (including MMX, SSE, minimal x87 FPU), CPU flags, register states, basic segmentation, basic exception handling, JIT engine components (decoder, IR, optimizer, AArch64 backend, TC, dispatcher, SMC handling), debugging/testing infrastructure, and a C++ API for integration.
*   **Out of Scope (for V1.0):** Full system emulation (GPU, APU, OS HLE), comprehensive interpreter fallback, advanced JIT optimizations, UI, specific game hacks, other Xbox console generations (360, One/Series), SMM, AArch32 target.

## 6. Technical Stack & Key Challenges

*   **Development Language:** C++ (C++17 or newer recommended).
*   **Build System:** CMake.
*   **Version Control:** Git.
*   **Key Challenges:**
    *   Accurate and performant SSE emulation using ARM NEON.
    *   Efficient emulation of complex x86 EFLAGS register.
    *   Robust handling of Self-Modifying Code (SMC).
    *   Managing memory ordering differences between x86 (TSO) and ARM.
    *   Achieving good floating-point precision (especially for x87 FPU).
    *   Optimizing performance bottlenecks on target ARM hardware.
    *   Working within ARM Linux security constraints (e.g., `mprotect`/`PROT_EXEC`).

## 7. Current Project Documents

For more detailed information, refer to:

*   [`XenoARM_JIT_PRD.md`](../XenoARM_JIT_PRD.md:1) (Product Requirements Document)
*   [`docs/README.md`](README.md:1) (Project README in docs)
*   [`docs/ARCHITECTURE.md`](ARCHITECTURE.md:1) (Detailed Architecture)
*   [`docs/BUILD_AND_TEST.md`](BUILD_AND_TEST.md:1)
*   [`docs/CONTRIBUTING.md`](CONTRIBUTING.md:1)
*   [`docs/API_REFERENCE.md`](API_REFERENCE.md:1)

This overview should provide a foundational understanding of the XenoARM JIT project.