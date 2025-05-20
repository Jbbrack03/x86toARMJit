#ifndef XENOARM_JIT_EXCEPTION_HANDLER_H
#define XENOARM_JIT_EXCEPTION_HANDLER_H

#include <cstdint>

namespace xenoarm_jit {

// Define the callback function type for exception reporting
// This will be called by the JIT when an x86 exception occurs
typedef void (*guest_exception_callback_t)(uint32_t exception_vector, uint32_t error_code);

/**
 * Exception Handler class for managing x86 exceptions in the JIT
 * 
 * This class provides a central mechanism for detecting, reporting, and
 * handling x86 exceptions that occur during JITted code execution.
 */
class ExceptionHandler {
public:
    /**
     * Initialize the exception handler system
     * 
     * @return true if initialization was successful, false otherwise
     */
    static bool init();
    
    /**
     * Set the callback function for reporting exceptions to the host
     * 
     * @param callback Function pointer to the host's exception handler
     */
    static void set_exception_callback(guest_exception_callback_t callback);
    
    /**
     * Report a generic exception to the host
     * 
     * @param exception_vector The x86 exception vector number (0-31)
     * @param error_code The error code associated with the exception, if any
     * @param faulting_address The address that caused the exception (typically guest EIP)
     * @return true if the exception was reported successfully, false otherwise
     */
    static bool report_exception(uint32_t exception_vector, uint32_t error_code, uint32_t faulting_address);
    
    /**
     * Report a divide-by-zero exception (#DE)
     * 
     * @param eip The guest EIP where the divide error occurred
     * @return true if the exception was reported successfully, false otherwise
     */
    static bool report_divide_by_zero(uint32_t eip);
    
    /**
     * Report an invalid opcode exception (#UD)
     * 
     * @param eip The guest EIP where the invalid opcode was encountered
     * @return true if the exception was reported successfully, false otherwise
     */
    static bool report_invalid_opcode(uint32_t eip);
    
    /**
     * Report a page fault exception (#PF)
     * 
     * @param eip The guest EIP where the page fault occurred
     * @param error_code The page fault error code (present bit, write bit, etc.)
     * @param fault_address The memory address that caused the page fault
     * @return true if the exception was reported successfully, false otherwise
     */
    static bool report_page_fault(uint32_t eip, uint32_t error_code, uint32_t fault_address);
    
    /**
     * Report an x87 FPU exception (#MF)
     * 
     * @param eip The guest EIP where the FPU exception occurred
     * @param fpu_status The FPU status word at the time of the exception
     * @return true if the exception was reported successfully, false otherwise
     */
    static bool report_fpu_exception(uint32_t eip, uint32_t fpu_status);
    
    /**
     * Report a SIMD floating-point exception (#XF)
     * 
     * @param eip The guest EIP where the SIMD exception occurred
     * @param mxcsr The MXCSR register value at the time of the exception
     * @return true if the exception was reported successfully, false otherwise
     */
    static bool report_simd_exception(uint32_t eip, uint32_t mxcsr);
    
    /**
     * Get the last faulting address (similar to x86 CR2 for page faults)
     * 
     * @return The memory address that caused the last fault
     */
    static uint32_t get_last_faulting_address();
    
private:
    // The registered callback function
    static guest_exception_callback_t exception_callback;
    
    // The last faulting address (like CR2 for page faults)
    static uint32_t last_faulting_address;
};

} // namespace xenoarm_jit

#endif // XENOARM_JIT_EXCEPTION_HANDLER_H 