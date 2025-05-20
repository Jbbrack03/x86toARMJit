#ifndef XENOARM_JIT_H
#define XENOARM_JIT_H

#include <cstdint>
#include "xenoarm_jit/exception_handler.h"

namespace xenoarm_jit {

// Error codes returned by API functions
enum jit_error_t {
    JIT_ERROR_SUCCESS = 0,           // Operation completed successfully
    JIT_ERROR_INVALID_PARAMETER = 1, // Invalid parameter passed to function
    JIT_ERROR_NOT_INITIALIZED = 2,   // JIT engine not initialized
    JIT_ERROR_ALREADY_INITIALIZED = 3, // JIT engine already initialized
    JIT_ERROR_MEMORY_ALLOCATION = 4, // Memory allocation failed
    JIT_ERROR_DECODING = 5,          // Error during instruction decoding
    JIT_ERROR_TRANSLATION = 6,       // Error during translation
    JIT_ERROR_EXECUTION = 7,         // Error during execution
    JIT_ERROR_UNSUPPORTED = 8,       // Unsupported feature or operation
    JIT_ERROR_INTERNAL = 9           // Internal error
};

// Set the callback function for handling guest exceptions
// This function should be called after initializing the JIT
// Returns JIT_ERROR_SUCCESS on success, or an error code on failure
jit_error_t xenoarm_jit_set_exception_callback(guest_exception_callback_t callback);

// More API function declarations would go here...

} // namespace xenoarm_jit

#endif // XENOARM_JIT_H 