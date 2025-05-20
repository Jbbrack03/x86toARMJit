#ifndef XENOARM_JIT_SIGNAL_HANDLER_H
#define XENOARM_JIT_SIGNAL_HANDLER_H

#include <cstdint>
#include <functional>
#include <signal.h>

namespace xenoarm_jit {

// Forward declaration
class MemoryManager;

// Class for handling signals related to memory protection
class SignalHandler {
public:
    // Initialize the signal handler with a memory manager
    static bool initialize(MemoryManager* memory_manager);
    
    // Cleanup the signal handler
    static void cleanup();
    
    // Get the instance of the signal handler
    static SignalHandler* get_instance();
    
    // Called by the signal handler to handle memory access faults
    void handle_segv(int signum, siginfo_t* info, void* context);
    
private:
    // Private constructor - use initialize() instead
    SignalHandler(MemoryManager* memory_manager);
    ~SignalHandler();
    
    // Previous signal handlers
    struct sigaction prev_segv_action_;
    
    // Memory manager reference
    MemoryManager* memory_manager_;
    
    // Singleton instance
    static SignalHandler* instance_;
};

} // namespace xenoarm_jit

#endif // XENOARM_JIT_SIGNAL_HANDLER_H 