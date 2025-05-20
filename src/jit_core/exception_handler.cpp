#include "xenoarm_jit/exception_handler.h"
#include "logging/logger.h"
#include <sstream>
#include <iomanip>

namespace xenoarm_jit {

// Static class variable to hold the callback function
guest_exception_callback_t ExceptionHandler::exception_callback = nullptr;

bool ExceptionHandler::init() {
    // Any initialization needed for the exception handler
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::DEBUG, "Initializing exception handler");
    return true;
}

void ExceptionHandler::set_exception_callback(guest_exception_callback_t callback) {
    exception_callback = callback;
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, "Exception callback function registered");
}

bool ExceptionHandler::report_exception(uint32_t exception_vector, uint32_t error_code, uint32_t faulting_address) {
    if (!exception_callback) {
        std::stringstream ss;
        ss << "No exception callback registered but an exception occurred: Vector=" 
           << exception_vector << ", Error code=" << error_code;
        XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::ERROR, ss.str());
        return false;
    }
    
    std::stringstream ss;
    ss << "Reporting guest exception: Vector=" << exception_vector 
       << ", Error code=" << error_code << ", Address=0x" 
       << std::hex << std::setw(8) << std::setfill('0') << faulting_address;
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, ss.str());
    
    // Store the faulting address for possible retrieval by the host
    // This would typically go into a guest CR2-like register for page faults
    last_faulting_address = faulting_address;
    
    // Call the host's exception handler
    exception_callback(exception_vector, error_code);
    
    return true;
}

bool ExceptionHandler::report_divide_by_zero(uint32_t eip) {
    std::stringstream ss;
    ss << "Detected divide by zero at EIP=0x" << std::hex << std::setw(8) << std::setfill('0') << eip;
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, ss.str());
    
    // x86 #DE (Divide Error) is vector 0, no error code
    return report_exception(0, 0, eip);
}

bool ExceptionHandler::report_invalid_opcode(uint32_t eip) {
    std::stringstream ss;
    ss << "Detected invalid opcode at EIP=0x" << std::hex << std::setw(8) << std::setfill('0') << eip;
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, ss.str());
    
    // x86 #UD (Invalid Opcode) is vector 6, no error code
    return report_exception(6, 0, eip);
}

bool ExceptionHandler::report_page_fault(uint32_t eip, uint32_t error_code, uint32_t fault_address) {
    std::stringstream ss;
    ss << "Detected page fault at EIP=0x" << std::hex << std::setw(8) << std::setfill('0') << eip
       << ", Fault address=0x" << std::hex << std::setw(8) << std::setfill('0') << fault_address
       << ", Error code=" << std::dec << error_code;
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, ss.str());
    
    // x86 #PF (Page Fault) is vector 14, has error code and fault address
    return report_exception(14, error_code, fault_address);
}

bool ExceptionHandler::report_fpu_exception(uint32_t eip, uint32_t fpu_status) {
    std::stringstream ss;
    ss << "Detected FPU exception at EIP=0x" << std::hex << std::setw(8) << std::setfill('0') << eip
       << ", FPU status=0x" << std::hex << std::setw(8) << std::setfill('0') << fpu_status;
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, ss.str());
    
    // x86 #MF (Math Fault / FPU error) is vector 16, no error code
    // We pass the FPU status word as the "error code" for debugging purposes
    return report_exception(16, fpu_status, eip);
}

bool ExceptionHandler::report_simd_exception(uint32_t eip, uint32_t mxcsr) {
    std::stringstream ss;
    ss << "Detected SIMD exception at EIP=0x" << std::hex << std::setw(8) << std::setfill('0') << eip
       << ", MXCSR=0x" << std::hex << std::setw(8) << std::setfill('0') << mxcsr;
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, ss.str());
    
    // x86 #XF (SIMD Floating-Point Exception) is vector 19, no error code
    // We pass the MXCSR as the "error code" for debugging purposes
    return report_exception(19, mxcsr, eip);
}

uint32_t ExceptionHandler::get_last_faulting_address() {
    return last_faulting_address;
}

// Initialize static member
uint32_t ExceptionHandler::last_faulting_address = 0;

} // namespace xenoarm_jit 