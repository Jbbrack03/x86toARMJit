# External Technical References

This document lists key external technical references and summaries of information gathered for the XenoARM JIT project.

## 1. Original Xbox CPU (Custom Intel Pentium III Coppermine)

*   **Base Microarchitecture:** Intel Pentium III Coppermine.
*   **Clock Speed:** 733 MHz.
*   **Key Customization:** Reduced L2 Cache to **128KB** (standard desktop Coppermine CPUs had 256KB).
*   **Instruction Sets:** IA-32, MMX, SSE. x87 FPU support is required but described as "minimal" for compatibility.
*   **Sources:**
    *   AnandTech "Understanding the Hardware – The X-CPU": `https://www.anandtech.com/show/853/2`
    *   xboxdevwiki "CPU": `https://xboxdevwiki.net/CPU`
    *   Wikipedia "Xbox technical specifications": `https://en.wikipedia.org/wiki/Xbox_technical_specifications`

## 2. Intel Pentium III Instruction Set Architecture (ISA)

For detailed information on the IA-32, MMX, and SSE instruction sets as implemented on the Coppermine microarchitecture, the primary references are Intel's official Software Developer's Manuals.

*   **Intel® Pentium® III Processor Datasheet (References to Developer Manuals):**
    *   [`https://download.intel.com/design/PentiumIII/datashts/24526408.pdf`](https://download.intel.com/design/PentiumIII/datashts/24526408.pdf)
    *   This document, on page 10 (PDF page 12), lists the following crucial manuals:
        *   **Intel Architecture Software Developer's Manual, Volume 1: Basic Architecture** (Order Number 243190)
        *   **Intel Architecture Software Developer's Manual, Volume 2: Instruction Set Reference** (Order Number 243191) - *This is the key reference for instruction opcodes, operands, and behavior.*
        *   **Intel Architecture Software Developer's Manual, Volume 3: System Programming Guide** (Order Number 243192)

*   **Direct links to (potentially newer versions of) Intel® 64 and IA-32 Architectures Software Developer’s Manuals:**
    *   It's generally recommended to find these on Intel's official website (`https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html`) to ensure the latest versions are consulted, though for Pentium III Coppermine, older archived versions specific to that era would be most accurate if available.
    *   Volume 2 (Instruction Set Reference) is critical for opcode details, instruction descriptions, flag modifications, and exceptions.

*(Further sections will be added as more information is gathered, e.g., for ARM NEON, x87 FPU specifics, memory models, etc.)*

## 3. ARM NEON (Advanced SIMD) Architecture

NEON is ARM's Advanced SIMD (Single Instruction, Multiple Data) architecture extension, crucial for emulating x86 MMX and SSE instructions.

*   **Arm Developer - Neon Programmers' Guide (General):**
    *   [`https://developer.arm.com/documentation/den0018/latest/`](https://developer.arm.com/documentation/den0018/latest/)
    *   Provides a comprehensive overview of NEON technology.

*   **Arm Developer - Neon Programmer Guide for Armv8-A (Specific to AArch64):**
    *   [`https://documentation-service.arm.com/static/6530e5163f12c06bc0f740ef`](https://documentation-service.arm.com/static/6530e5163f12c06bc0f740ef)
    *   Focuses on NEON for the Armv8-A architecture, which includes AArch64.

*   **Arm Developer - NEON Instructions (Assembly Reference):**
    *   [`https://developer.arm.com/documentation/dui0473/latest/neon-instructions`](https://developer.arm.com/documentation/dui0473/latest/neon-instructions)
    *   Details on NEON assembly instructions.

*   **Arm Developer - NEON Intrinsics Introduction (for C/C++ programming):**
    *   [`https://developer.arm.com/documentation/den0018/latest/NEON-Intrinsics/Introduction`](https://developer.arm.com/documentation/den0018/latest/NEON-Intrinsics/Introduction)
    *   Information on using NEON via intrinsics in C/C++ code, which is highly relevant for the JIT implementation.

*   **Community Resources:**
    *   GitHub - thenifty/neon-guide: [`https://github.com/thenifty/neon-guide`](https://github.com/thenifty/neon-guide) (Practical examples for intrinsics)
    *   GCC ARM NEON Intrinsics: [`http://gcc.gnu.org/onlinedocs/gcc/ARM-NEON-Intrinsics.html`](http://gcc.gnu.org/onlinedocs/gcc/ARM-NEON-Intrinsics.html) (Compiler-specific documentation)

## 4. x87 Floating-Point Unit (FPU) Emulation

The Original Xbox CPU includes an x87 FPU, and the project requires "minimal x87 FPU support for compatibility." The exact subset of x87 instructions and behaviors critical for this minimal support is not yet fully defined and may require empirical testing or insights from existing Xbox emulator projects.

*   **Primary Reference:**
    *   **Intel Architecture Software Developer's Manual, Volume 2: Instruction Set Reference** (Order Number 243191, or newer versions from Intel's website) will contain the full x87 instruction set details.
*   **Key Considerations for "Minimal Support":**
    *   **Precision:** x87 FPU operates with 80-bit internal precision. Games relying on this specific precision for certain calculations might behave incorrectly if only 32-bit (SSE single-precision) or 64-bit (SSE double-precision) emulation is provided for x87 operations. Instructions like `FLD80` (load 80-bit float), `FSTP80` (store 80-bit float and pop) and the FPU control word settings related to precision will be important.
    *   **Transcendental Instructions:** x87 includes instructions for trigonometric functions (e.g., `FSIN`, `FCOS`, `FSINCOS`, `FPTAN`, `FPATAN`) and logarithmic/exponential functions (e.g., `FYL2X`, `FYL2XP1`, `F2XM1`, `FSCALE`). If games use these directly instead of library calls, they will need emulation.
    *   **Control Word and Status Word:** The x87 FPU has a control word (FPUCW) and status word (FPUSW) that affect its operation (rounding modes, exception masks, precision control) and report its state. Instructions like `FLDCW`, `FSTCW`, `FSTSW`, `FCLEX` will be important.
    *   **Basic Arithmetic and Data Movement:** Common instructions like `FLD` (load float), `FST`/`FSTP` (store float), `FADD`/`FSUB`/`FMUL`/`FDIV` (arithmetic), `FCHS` (change sign), `FABS` (absolute value), `FXCH` (exchange registers) are likely to be part of any "minimal" set.
*   **Further Research:**
    *   Consulting Original Xbox homebrew development resources.
    *   Analyzing source code or documentation of existing Xbox emulators (e.g., Xemu, Cxbx-R) for insights into their x87 FPU handling.

## 5. x86 EFLAGS Register Emulation on ARM AArch64

Emulating the x86 EFLAGS register (containing flags like CF, PF, AF, ZF, SF, OF) on ARM AArch64 is a significant challenge due to differences in how flags are set and their meanings. ARM's NZCV register (Negative, Zero, Carry, Overflow) doesn't directly map to all x86 flags, and x86 instructions modify flags more extensively.

*   **Key Challenges:**
    *   **More x86 Flags:** x86 has AF (Auxiliary Carry) and PF (Parity) flags with no direct ARM equivalents.
    *   **Implicit Flag Setting:** Many x86 integer and logical instructions implicitly modify EFLAGS. ARM instructions often require explicit flag-setting variants or separate compare instructions.
    *   **Different Semantics:** The Carry (C) and Overflow (V) flags can have different meanings for subtraction and comparison operations between x86 and ARM.
    *   **Reference:** The Rosetta 2 fast article provides a high-level comparison: [`https://dougallj.wordpress.com/2022/11/09/why-is-rosetta-2-fast/`](https://dougallj.wordpress.com/2022/11/09/why-is-rosetta-2-fast/)

*   **General Emulation Techniques (requiring further research for specific implementation):**
    *   **Lazy Flag Evaluation:** Calculate specific x86 flags only when an instruction explicitly reads them (e.g., conditional jumps, `LAHF`, `SAHF`, `PUSHF`, `POPF`).
    *   **Explicit Flag Calculation:** Generate ARM code sequences to compute each required x86 flag (SF, ZF, OF, CF, AF, PF) based on the operation's result and operands. This is often necessary for AF and PF.
    *   **Shadow Flags:** Store the state of x86 EFLAGS (or individual flags) in ARM general-purpose registers or a dedicated memory location, updated by emulated x86 instructions.
    *   **Leveraging ARM NZCV Flags:**
        *   Map x86 SF, ZF directly to ARM N, Z.
        *   Map x86 CF, OF to ARM C, V where semantics align (e.g., addition). Careful handling is needed for subtractions/comparisons due to differing C-flag logic (borrow vs. not-borrow).
        *   Use ARM conditional instructions (`CMP`, `CMN`, `TST`) to set NZCV flags, followed by ARM conditional select instructions (`CSEL`, `CSINC`, etc.) or conditional branches (`B.cond`) to emulate x86 conditional behavior.
    *   **Dedicated Helper Routines:** Implement optimized helper functions in ARM assembly or C to handle common or complex flag updates.

*   **Further Research:**
    *   Study the EFLAGS emulation strategies in open-source x86 emulators/dynamic translators like QEMU, Bochs, FEX-Emu, Box86/Box64.
    *   Look for academic papers on binary translation and CPU emulation focusing on flag handling.
    *   The Intel Architecture Software Developer's Manuals (Volume 1: Basic Architecture, and Volume 2: Instruction Set Reference) are essential for understanding how each x86 instruction affects EFLAGS.

## 6. Self-Modifying Code (SMC) Detection and Handling

Self-Modifying Code (SMC) occurs when a program alters its own instructions during execution. JIT emulators must detect SMC and invalidate any corresponding translated code in the Translation Cache (TC) to ensure correctness.

*   **Key Concepts & Challenges:**
    *   **Detection:** Identifying when guest code writes to memory regions that have been JITted.
    *   **Invalidation:** Efficiently removing or marking stale entries in the Translation Cache.
    *   **Host Instruction Cache Coherency:** Ensuring the host CPU (ARM) fetches the newly modified guest code, not stale instructions from its own i-cache.
    *   **Performance:** SMC handling can introduce overhead, especially if SMC is frequent.

*   **Common Techniques:**
    *   **Page Protection:**
        *   Mark memory pages containing JITted guest code as read-only.
        *   A write attempt by the guest triggers a page fault.
        *   The JIT's fault handler invalidates TC entries corresponding to that page, then allows the write (e.g., by temporarily making the page writable or emulating the write).
        *   The code is re-JITted upon next execution.
    *   **Write Tracking via Memory Interface:**
        *   The JIT's guest memory write functions check if the write target overlaps with any JITted code regions.
        *   If so, invalidate the corresponding TC entries. Requires efficient mapping from guest addresses to TC entries.
    *   **Host Instruction Cache Invalidation:**
        *   After SMC is detected and before re-executing or re-JITting the modified guest code, the host ARM processor's instruction cache for the affected memory range must be invalidated (e.g., using ARM's `IC IVAU` or similar cache maintenance instructions). This is critical for correctness.
    *   **Granularity:** Detection and invalidation can be at page-level or finer-grained (e.g., basic block level).

*   **References:**
    *   **Wikipedia - Self-modifying code:** [`https://en.wikipedia.org/wiki/Self-modifying_code`](https://en.wikipedia.org/wiki/Self-modifying_code) (General concepts, cache interaction)
    *   **ACM Paper - Handling SelfModifying Code Using Software Dynamic Translation:** [`https://src.acm.org/binaries/content/assets/src/2008/joy-w-kamunyori.pdf`](https://src.acm.org/binaries/content/assets/src/2008/joy-w-kamunyori.pdf) (Academic perspective on SMC in dynamic translation)
    *   **Stack Overflow - JIT emulation and tracking dirty memory blocks:** [`https://stackoverflow.com/questions/28700095/jit-emulation-and-tracking-dirty-memory-blocks`](https://stackoverflow.com/questions/28700095/jit-emulation-and-tracking-dirty-memory-blocks) (Practical discussion)

*   **Further Research:**
    *   Investigate SMC handling in mature emulators like QEMU, FEX-Emu, Box86/Box64.
    *   Details of ARM cache maintenance operations and memory protection mechanisms (`mprotect`).

## 7. Memory Consistency Models (x86 TSO vs. ARM Weakly-Ordered)

x86 processors generally follow a Total Store Order (TSO) memory consistency model, which is relatively strong. ARM processors, on the other hand, use a more weakly-ordered memory model, allowing more aggressive reordering of memory operations for performance. Emulating x86 code that might implicitly rely on TSO may require the JIT to insert ARM memory barrier instructions.

*   **x86 Total Store Order (TSO):**
    *   Stores are not reordered with other stores.
    *   Loads can be reordered with older stores to *different* addresses.
    *   Provides a fairly intuitive memory order for programmers.

*   **ARM Weakly-Ordered Model:**
    *   Allows more aggressive reordering of loads and stores by the hardware, provided single-thread data dependencies are met.
    *   Requires explicit memory barrier instructions to enforce stricter ordering when needed (e.g., for inter-core communication, device I/O).

*   **ARM Memory Barriers:**
    *   **`DMB` (Data Memory Barrier):** Ensures that all memory accesses appearing in program order before the DMB are observed before any memory accesses appearing after the DMB. Various scopes (e.g., `SY` for full system, `ISH` for inner-shareable).
    *   **`DSB` (Data Synchronization Barrier):** A stronger barrier. It blocks execution of further instructions (not just memory accesses) until all memory accesses before the DSB have completed. Various scopes.
    *   **`ISB` (Instruction Synchronization Barrier):** Flushes the processor pipeline and instruction prefetch buffers, ensuring that instructions fetched after the ISB reflect any prior changes to system state (e.g., MMU configuration, self-modified code after i-cache flush).

*   **Implications for JIT:**
    *   For single-threaded guest code, the primary concern is ensuring that the emulated memory operations for a sequence of x86 instructions appear in the correct order, especially if they involve atomic operations (if the guest uses them) or interaction with emulated memory-mapped I/O.
    *   If the JITted code needs to emulate x86 atomic instructions (e.g., `LOCK` prefix, `XCHG`) or specific memory ordering primitives (like `MFENCE`, `SFENCE`, `LFENCE`), ARM's load-exclusive/store-exclusive instructions (`LDXR`/`STXR`) and appropriate DMB/DSB barriers will be necessary.
    *   For initial V1.0 (likely focusing on single-threaded guest execution), explicit barriers might be less frequently needed within typical JITted code blocks unless emulating specific x86 synchronization instructions or certain types of memory-mapped I/O.

*   **References:**
    *   **Paper - A Comprehensive Study and Optimization of ARM Barriers:** [`https://ipads.se.sjtu.edu.cn/_media/publications/liuppopp20.pdf`](https://ipads.se.sjtu.edu.cn/_media/publications/liuppopp20.pdf) (Compares ARM WMM to x86 TSO)
    *   **ArangoDB Blog - C++ Memory Model: Migrating from x86 to ARM:** [`https://arangodb.com/2021/02/cpp-memory-model-migrating-from-x86-to-arm/`](https://arangodb.com/2021/02/cpp-memory-model-migrating-from-x86-to-arm/)
    *   **Wikipedia - Memory ordering:** [`https://en.wikipedia.org/wiki/Memory_ordering`](https://en.wikipedia.org/wiki/Memory_ordering)
    *   **Arm Developer - Barriers:** [`https://developer.arm.com/documentation/100941/latest/Barriers`](https://developer.arm.com/documentation/100941/latest/Barriers) (DMB, DSB, ISB details)
    *   **WUSTL Slides - Hardware Memory Consistency Model:** [`https://classes.engineering.wustl.edu/cse539/web/lectures/mem_model.pdf`](https://classes.engineering.wustl.edu/cse539/web/lectures/mem_model.pdf) (x86-TSO explanation)
    *   **Box86 GitHub Issue #519:** [`https://github.com/ptitSeb/box86/issues/519`](https://github.com/ptitSeb/box86/issues/519) (Emulator-specific discussion)

## 8. Executable Memory Management (mmap, mprotect, Cache Coherency) on ARM Linux

JIT compilers need to allocate memory, write translated machine code into it, and then mark that memory as executable. On ARM Linux, this typically involves `mmap` and `mprotect` system calls, along with crucial ARM-specific instruction cache maintenance.

*   **Typical Workflow:**
    1.  **Allocate Writable Memory:** Use `mmap()` to allocate a memory region. Initially, permissions are often `PROT_READ | PROT_WRITE` to allow the JIT to write code into it.
        *   Example: `ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);`
    2.  **Write JITted Code:** The JIT engine populates this memory region with the generated AArch64 instructions.
    3.  **Instruction Cache Invalidation (Pre-Execute):** Before making the code executable or immediately after writing and before first execution, the ARM CPU's instruction cache corresponding to this memory region **must** be invalidated. This ensures the CPU fetches the newly written instructions, not stale data from the i-cache.
        *   On ARMv8-A, this typically involves an instruction like `IC IVAU, Xn` (Invalidate Instruction Cache by MVA to PoU, where Xn holds the address) for the range of modified memory.
    4.  **Change Protection to Executable:** Use `mprotect()` to change the memory region's permissions to `PROT_READ | PROT_EXEC`.
        *   Example: `mprotect(ptr, size, PROT_READ | PROT_EXEC);`
    5.  **Instruction Synchronization Barrier:** After changing memory permissions to executable and after any i-cache maintenance, an `ISB` (Instruction Synchronization Barrier) instruction should be executed. This flushes the processor's pipeline and ensures that subsequent instruction fetches see the changes to memory permissions and i-cache state.
    6.  **Execute JITted Code:** The JIT can now branch/jump to the address `ptr`.

*   **Considerations for Self-Modifying Code (SMC):**
    *   If JITted code is marked `PROT_READ | PROT_EXEC` and the guest attempts to write to it, a page fault occurs.
    *   The fault handler would:
        1.  Invalidate the Translation Cache entry.
        2.  Use `mprotect()` to temporarily set the page to `PROT_READ | PROT_WRITE`.
        3.  Allow the guest write to proceed.
        4.  Perform i-cache invalidation for the modified region.
        5.  Execute an `ISB`.
        6.  The code will be re-JITted (and permissions set back to `PROT_READ | PROT_EXEC`) when next needed.

*   **Security Policies (e.g., SELinux):**
    *   Operating system security policies like SELinux can restrict the ability of processes to mark memory as executable (`execmem` permission). This is generally more of a concern on hardened server systems than typical embedded Linux environments like those on handhelds, but it's a potential factor.
    *   W^X (Write XOR Execute): Some systems enforce or allow policies where memory cannot be both writable and executable simultaneously. JITs adapt by toggling permissions with `mprotect`. Modern Linux kernels also support making pages executable-only (`PROT_EXEC` without `PROT_READ`).

*   **References:**
    *   **Stack Overflow - Mark segment executable:** [`https://stackoverflow.com/questions/57459867/how-can-you-mark-a-segment-of-memory-as-executable-in-c`](https://stackoverflow.com/questions/57459867/how-can-you-mark-a-segment-of-memory-as-executable-in-c)
    *   **nullprogram - JIT Compiler Skirmish with SELinux:** [`https://nullprogram.com/blog/2018/11/15/`](https://nullprogram.com/blog/2018/11/15/)
    *   **Michael Kohn - JIT Compiler Development:** [`https://www.mikekohn.net/software/jit_compiler_development.php`](https://www.mikekohn.net/software/jit_compiler_development.php)
    *   **ARM Architecture Reference Manual (for `IC IVAU`, `ISB` and other cache/barrier instructions):** The definitive source for ARM-specific instructions. (Search on Arm Developer website).

## 9. x86 Architectural Test Suites

To validate the correctness of the x86 instruction emulation, architectural test suites are crucial. These suites typically run low-level tests to verify CPU behavior for various instructions and scenarios.

*   **`XenoARM_JIT_PRD.md` Reference:**
    *   The PRD mentions integrating with "CPU architectural test suites (e.g., custom-made or adapted x86 test suites, [x86test](https://github.com/klauspost/x86test))" and "open-source x86 test suites."
    *   A direct repository for `x86test` by `klauspost` was not immediately found with the initial search. Further clarification or alternative search terms might be needed if this specific suite is a hard requirement.

*   **Example of a General x86 CPU Test Suite:**
    *   **`barotto/test386.asm`:** [`https://github.com/barotto/test386.asm`](https://github.com/barotto/test386.asm)
        *   This is a tester for 80386 and later CPUs, written for NASM. It runs as a BIOS replacement and is designed for testing emulators. This type of suite can be very valuable.

*   **Other Potential Sources/Types of Tests:**
    *   **Emulator-Specific Tests:** Many emulators develop their own targeted CPU tests.
    *   **DOS-based CPU Tests:** Older DOS-era CPU diagnostic tools and test suites.
    *   **Custom-Developed Tests:** For specific instruction groups or behaviors that are hard to test with generic suites.

*   **Further Research:**
    *   Clarify the exact `x86test` suite referenced in the PRD if it's a specific requirement.
    *   Explore other open-source x86 test suites suitable for emulator validation (e.g., from other emulator projects, academic research).