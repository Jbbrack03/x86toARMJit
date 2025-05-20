#include "xenoarm_jit/xenoarm_jit.h"
#include "xenoarm_jit/exception_handler.h"
#include "logging/logger.h"

namespace xenoarm_jit {

// Set the exception callback function that will be called when a guest exception occurs
jit_error_t xenoarm_jit_set_exception_callback(guest_exception_callback_t callback) {
    if (!callback) {
        XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::ERROR, "Attempt to register null exception callback");
        return JIT_ERROR_INVALID_PARAMETER;
    }
    
    ExceptionHandler::set_exception_callback(callback);
    XenoARM_JIT::Logger::getInstance().log(XenoARM_JIT::INFO, "Exception callback registered");
    
    return JIT_ERROR_SUCCESS;
}

// Existing API implementations would go here...

} // namespace xenoarm_jit 