# XenoARM JIT

## Project Overview

XenoARM JIT is a project to develop a high-performance Just-In-Time (JIT) compiler that translates Xbox CPU instructions into native ARM (AArch64) machine code. This JIT will serve as the core CPU emulation component for a potential future Xbox emulator on ARM-based devices (e.g., high-end smartphones, tablets, single-board computers, ARM-based PCs, and especially the Retroid Pocket 2 Flip).

## Target Platform

The initial focus is on the **Original Xbox** CPU (Custom Intel Pentium III Coppermine-based, 32-bit x86 IA-32 architecture, 733 MHz).

## Core Goals

*   To accurately emulate the target Xbox CPU's instruction set and behavior.
*   To achieve high performance by dynamically recompiling frequently executed code blocks into optimized ARMv8/ARMv9 (AArch64) code.
*   To provide a modular and extensible JIT core that can be integrated into a larger emulator project.
*   To ensure stability and reliability of the JIT compiler and the code it produces.
*   To create a maintainable and well-documented codebase.
*   To maximize compatibility and performance for the Flip 2 hardware and similar ARM gaming handhelds.

## High-Level Scope for V1.0

*   **CPU Core:** Implementation of the full IA-32 (Pentium III compatible) user-mode instruction set, including General Purpose Instructions, MMX, SSE, and minimal x87 FPU support. Accurate emulation of CPU flags and register states. Handling of basic x86 segmentation and basic exception handling.
*   **JIT Engine:** Frontend (x86 decoder), Intermediate Representation (IR), Basic Optimizer, Backend (AArch64 code generator leveraging NEON), Translation Cache (TC) management, Dispatcher, Self-Modifying Code (SMC) detection and handling, Memory Management Interface.
*   **Debugging & Testing:** Internal JIT logging, ability to dump generated ARM code, unit tests, CPU architectural test suites, integration with open-source x86 test suites.
*   **API:** Initialization and configuration, execution control, register access, memory access hooks, detailed logging and error reporting interfaces.

## Getting Started

*   [Building and Testing](BUILD_AND_TEST.md)
*   [Contributing](CONTRIBUTING.md)

## License

[Placeholder for License Information]