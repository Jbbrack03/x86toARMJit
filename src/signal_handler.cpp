#include "xenoarm_jit/signal_handler.h"
#include "xenoarm_jit/memory_manager.h"
#include "logging/logger.h"
#include <cstring>
#include <unistd.h>

namespace xenoarm_jit {

// Static instance
SignalHandler* SignalHandler::instance_ = nullptr;

// Signal handler function that redirects to the instance
static void segv_handler(int signum, siginfo_t* info, void* context) {
    // Get the instance and delegate to it
    SignalHandler* handler = SignalHandler::get_instance();
    if (handler) {
        handler->handle_segv(signum, info, context);
    } else {
        // If no handler, use default behavior (likely terminate)
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = 0;
        sigaction(SIGSEGV, &sa, nullptr);
        
        // Re-raise the signal
        raise(signum);
    }
}

SignalHandler::SignalHandler(MemoryManager* memory_manager)
    : memory_manager_(memory_manager) {
    LOG_DEBUG("SignalHandler created");
}

SignalHandler::~SignalHandler() {
    LOG_DEBUG("SignalHandler destroyed");
}

bool SignalHandler::initialize(MemoryManager* memory_manager) {
    if (instance_) {
        LOG_WARNING("SignalHandler already initialized");
        return false;
    }
    
    // Create the instance
    instance_ = new SignalHandler(memory_manager);
    
    // Set up the signal handler for SIGSEGV
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    
    // Save previous handler and install new one
    if (sigaction(SIGSEGV, &sa, &instance_->prev_segv_action_) != 0) {
        LOG_ERROR("Failed to install SIGSEGV handler");
        delete instance_;
        instance_ = nullptr;
        return false;
    }
    
    LOG_INFO("SIGSEGV handler installed for SMC detection");
    return true;
}

void SignalHandler::cleanup() {
    if (!instance_) {
        return;
    }
    
    // Restore previous signal handler
    sigaction(SIGSEGV, &instance_->prev_segv_action_, nullptr);
    
    // Delete the instance
    delete instance_;
    instance_ = nullptr;
    
    LOG_INFO("SIGSEGV handler uninstalled");
}

SignalHandler* SignalHandler::get_instance() {
    return instance_;
}

void SignalHandler::handle_segv(int signum, siginfo_t* info, void* context) {
    // Get the fault address
    uintptr_t fault_addr = reinterpret_cast<uintptr_t>(info->si_addr);
    
    LOG_DEBUG("SIGSEGV received at address 0x" + std::to_string(fault_addr));
    
    // Delegate to memory manager if this is a protection fault for SMC
    if (memory_manager_ && info->si_code == SEGV_ACCERR) {
        // Try to handle it as a code page write (SMC)
        memory_manager_->handle_protection_fault(static_cast<uint32_t>(fault_addr));
        
        // Success - we handled it, so return
        return;
    }
    
    // If we couldn't handle it, chain to previous handler
    if (prev_segv_action_.sa_flags & SA_SIGINFO) {
        prev_segv_action_.sa_sigaction(signum, info, context);
    } else if (prev_segv_action_.sa_handler != SIG_IGN && 
               prev_segv_action_.sa_handler != SIG_DFL) {
        prev_segv_action_.sa_handler(signum);
    } else {
        // No previous handler or default handler - terminate
        LOG_ERROR("Unhandled SIGSEGV at address 0x" + std::to_string(fault_addr));
        
        // Report to stderr
        const char* msg = "XenoARM JIT: Unhandled SIGSEGV\n";
        write(STDERR_FILENO, msg, strlen(msg));
        
        // Re-raise with default action
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = 0;
        sigaction(SIGSEGV, &sa, nullptr);
        
        // Re-raise the signal
        raise(signum);
    }
}

} // namespace xenoarm_jit 