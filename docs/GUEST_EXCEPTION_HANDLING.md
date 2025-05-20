# XenoARM JIT - Guest Exception Handling Flow

This document describes how the XenoARM JIT detects guest CPU exceptions (originating from the emulated x86 code) and reports them to the host emulator.

## 1. Introduction

The Original Xbox CPU (IA-32) can generate various exceptions due to programming errors, hardware conditions, or specific instructions. The JIT's role is not to handle these exceptions in the guest's context (that's the job of the emulated guest OS or application, facilitated by the host emulator), but to:
1.  Accurately detect conditions that would cause an x86 exception.
2.  Preserve the guest CPU state as it would be at the point of exception.
3.  Report the exception details to the host emulator via the defined API ([`API_REFERENCE.md#6-exception-handling-interface`](API_REFERENCE.md:90)).
4.  Allow the host emulator to manage the guest's response to the exception and potentially resume execution.

## 2. Types of x86 Exceptions (User-Mode Focus)

The JIT will primarily be concerned with exceptions that can occur in user-mode execution on an IA-32 processor. These include (but are not limited to):

*   **`#DE` (Divide Error / Vector 0):** e.g., `DIV` or `IDIV` with divisor 0 or quotient overflow.
*   **`#DB` (Debug Exception / Vector 1):** Caused by various debug conditions (breakpoints, watchpoints). The JIT itself might not generate these directly unless specifically emulating debug registers and traps. Host debugger interaction is more likely.
*   **`#BP` (Breakpoint / Vector 3):** Caused by `INT 3` instruction.
*   **`#OF` (Overflow / Vector 4):** Caused by `INTO` instruction if Overflow Flag (OF) is set.
*   **`#BR` (BOUND Range Exceeded / Vector 5):** Caused by `BOUND` instruction if test fails.
*   **`#UD` (Invalid Opcode / Undefined Opcode / Vector 6):** Caused by executing an invalid or privileged instruction in user mode.
*   **`#NM` (Device Not Available / No Math Coprocessor / Vector 7):** Related to FPU/MMX/SSE instructions if the corresponding unit is disabled or in a specific state (e.g., via `CR0` bits).
*   **`#DF` (Double Fault / Vector 8):** Typically occurs when trying to invoke an exception handler for a previous exception, and another exception occurs during that attempt. More of a system-level concern, but the JIT must report the initial fault correctly.
*   **`#TS` (Invalid TSS / Vector 10):** Task State Segment related, less common in typical game execution but possible.
*   **`#NP` (Segment Not Present / Vector 11):** Accessing a segment marked not present.
*   **`#SS` (Stack Segment Fault / Vector 12):** Stack-related errors (e.g., limit exceeded).
*   **`#GP` (General Protection Fault / Vector 13):** A catch-all for various protection violations (e.g., writing to read-only segment, limit violations, privileged instruction execution). This is a common one.
*   **`#PF` (Page Fault / Vector 14):** Occurs if a memory access violates page-level protection (e.g., page not present, write to read-only page, privilege violation) and the guest MMU (emulated by host) signals this.
*   **`#MF` (x87 FPU Floating-Point Error / Math Fault / Vector 16):** x87 FPU exceptions (e.g., denormal operand, divide by zero, overflow, underflow, precision).
*   **`#AC` (Alignment Check / Vector 17):** If alignment checking is enabled (`EFLAGS.AC=1` and `CR0.AM=1`) and an unaligned memory reference is made to user-mode data.
*   **`#MC` (Machine Check / Vector 18):** Hardware-level errors, less likely to be *generated* by JIT but could be reported if host detects them.
*   **`#XF` (SIMD Floating-Point Exception / Vector 19):** SSE/SSE2/SSE3 floating-point exceptions.

## 3. JIT Detection Mechanisms

The JIT can detect exception conditions through several means:

*   **Explicit Checks in JITted Code:**
    *   For instructions like `DIV`/`IDIV`, the JITted ARM code will explicitly check for a zero divisor before performing the division. If zero, it will trigger a call to the exception reporting mechanism.
    *   For `BOUND`, the JITted code will perform the bounds check.
    *   For `INT3` or `INTO`.
*   **Decoder Phase:**
    *   The x86 decoder can identify invalid or privileged instructions. If encountered in a context where they'd cause `#UD` or `#GP`, this can be flagged.
*   **Host MMU / OS Signals (via Memory Interface):**
    *   When the JITted code attempts a memory access (via the host memory callbacks), the host's emulated MMU is responsible for checking page permissions, presence, etc.
    *   If the host MMU determines a page fault (`#PF`) should occur, it will not perform the memory access and instead, the memory callback function should signal an error back to the JIT. The JIT's internal wrapper around the host callback would then initiate the exception reporting flow.
    *   Similarly, if the host OS (running the emulator) detects an FPU exception (e.g., from an ARM VFP instruction used to emulate x87/SSE) that needs to be reflected to the guest as `#MF` or `#XF`, this needs a pathway. Often, FPU exceptions are masked in the JITted code, and status flags are checked explicitly.
*   **Self-Modifying Code (SMC) Faults:** If page protection is used for SMC detection, an OS-level page fault (e.g., SIGSEGV on Linux) will occur when JITted code (or guest code) writes to a read-only code page. The JIT's signal handler must distinguish this from a guest-level `#PF`. This is primarily for JIT maintenance, not direct guest exception reporting, though it involves similar fault handling logic.

## 4. Information Provided to Host

When an exception is reported via the `guest_exception_callback_t` (see [`API_REFERENCE.md`](API_REFERENCE.md:1)):

*   `uint32_t exception_vector`: The x86 exception vector number (0-31, or up to 255).
*   `uint32_t error_code`: The error code associated with the exception, if any (e.g., for `#PF`, `#GP`, `#SS`, `#TS`, `#NP`). If no error code, this should be a defined value (e.g., 0 or -1).
*   **Implicit State:**
    *   The guest's Program Counter (`EIP` or `RIP` for x86-64) should point to the instruction that *caused* the exception. The JIT must ensure this is correctly set before calling the host callback.
    *   For page faults (`#PF`), the faulting memory address (typically found in `CR2` on x86) needs to be communicated. This might be an additional parameter to the callback or retrieved by the host via a specific JIT API function.
    *   Other guest registers (`EAX`, `EBX`, etc.) should reflect their state *before* the faulting instruction executed (or as per x86 specification for that specific exception). The host can retrieve these using `xenoarm_jit_get_guest_register()`.

## 5. JIT State Upon Exception

*   The JIT must immediately cease execution of the current translated block.
*   It must ensure that the guest's architectural state (especially `EIP` and any registers modified by partially executed instructions, though ideally exceptions are precise) is correctly rolled back or set to reflect the state at the point of the fault, as defined by IA-32 architecture.
*   It then calls the registered `guest_exception_callback_t`.

## 6. Host Emulator Responsibilities

*   Receive the exception details from the JIT.
*   Consult the `error_code` and faulting address (if applicable).
*   Emulate the guest OS's response to this exception. This might involve:
    *   Invoking a guest OS exception handler by setting guest `EIP` to the handler's address and modifying the guest stack.
    *   Terminating the guest application.
    *   Performing some corrective action and attempting to resume.
*   The host is responsible for all guest-level Interrupt Descriptor Table (IDT) lookups and privilege-level changes associated with exception handling.

## 7. Resuming Execution from Host

*   If the host determines that guest execution can resume (e.g., after a page fault is handled by loading a page, or a guest handler is set up), it will typically call `xenoarm_jit_run()` again.
*   The JIT will then start execution from the current guest `EIP` (which might have been modified by the host or the emulated guest handler).

## 8. Specific Exception Considerations

*   **`#PF` (Page Fault):** The JIT relies on the host's memory interface to signal page faults. The host provides the "ground truth" for memory accessibility. The JIT's generated code will make calls like `read_u32(address)`. If `address` is invalid, the host's implementation of `read_u32` should not crash but rather inform the JIT (e.g., via a special return value or by setting a JIT context flag) that a fault occurred, allowing the JIT to trigger the `#PF` callback to the host.
*   **`#UD` (Invalid Opcode):** The JIT's decoder should identify invalid/unsupported opcodes and can trigger `#UD` directly.
*   **FPU Exceptions (`#MF`, `#XF`):** The JITted ARM NEON/VFP code should ideally be generated to use "non-stop" mode (masking exceptions) and then explicitly check ARM FPU status registers after operations to determine if an x86 FPU exception should be raised. This avoids host OS signal handling for FPU issues where possible.

---
This flow ensures a clean separation of concerns: the JIT handles detection and reporting based on its view of guest execution, and the host handles the guest OS/application-level response.