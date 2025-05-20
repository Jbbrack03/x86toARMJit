#ifndef XENOARM_JIT_C_API_H
#define XENOARM_JIT_C_API_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of JitState structure
typedef struct JitState JitState;

// Error codes
#define JIT_SUCCESS 0
#define JIT_ERROR_INVALID_PARAMETER 1
#define JIT_ERROR_NOT_INITIALIZED 2
#define JIT_ERROR_MEMORY_ALLOCATION 3
#define JIT_ERROR_EXECUTION_FAILED 4

// Register constants
#define JIT_REG_EAX 0
#define JIT_REG_ECX 1
#define JIT_REG_EDX 2
#define JIT_REG_EBX 3
#define JIT_REG_ESP 4
#define JIT_REG_EBP 5
#define JIT_REG_ESI 6
#define JIT_REG_EDI 7
#define JIT_REG_EIP 8
#define JIT_REG_EFLAGS 9

// Memory read/write callback definitions
typedef uint64_t (*MemoryReadCallback)(void* userdata, uint64_t address, uint32_t size);
typedef void (*MemoryWriteCallback)(void* userdata, uint64_t address, uint64_t value, uint32_t size);
typedef void (*LoggingCallback)(void* userdata, int level, const char* message);

// JIT configuration structure
typedef struct {
    void* userdata;
    MemoryReadCallback memoryReadCallback;
    MemoryWriteCallback memoryWriteCallback;
    LoggingCallback logCallback;
} JitConfig;

// C API Functions
int jit_init(JitState** state, JitConfig* config);
void jit_cleanup(JitState* state);
int jit_run(JitState* state);
void jit_enable_smc_detection(JitState* state, bool enable);

// Translation cache control
void jit_clear_translation_cache(JitState* state);
void* jit_translate(JitState* state, uint32_t guest_address);

// Register access functions
int jit_get_guest_register(JitState* state, int reg_index, uint32_t* value);
int jit_set_guest_register(JitState* state, int reg_index, uint32_t value);

// EFLAGS access
int jit_get_guest_eflags(JitState* state, uint32_t* eflags);
int jit_set_guest_eflags(JitState* state, uint32_t eflags);

// MMX/XMM register access
int jit_get_guest_mmx_register(JitState* state, int reg_index, uint64_t* value);
int jit_set_guest_mmx_register(JitState* state, int reg_index, uint64_t value);
int jit_get_guest_xmm_register(JitState* state, int reg_index, uint8_t* value);
int jit_set_guest_xmm_register(JitState* state, int reg_index, const uint8_t* value);

#ifdef __cplusplus
}
#endif

// C++ namespace for the logging system used by the tests
#ifdef __cplusplus
namespace xenoarm_jit {
namespace logging {
    enum class LogLevel {
        ERROR = 0,
        WARNING = 1,
        INFO = 2,
        DEBUG = 3
    };

    class Logger {
    public:
        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }

        void log(LogLevel level, const char* message) {}
        void setLogLevel(LogLevel level) {}
        
        static void init(LogLevel level) {
            getInstance().setLogLevel(level);
        }
    };
}
}
#endif

#endif // XENOARM_JIT_C_API_H 