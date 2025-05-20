# XenoARM JIT API Reference (Initial Draft)

This document provides an initial overview of the public Application Programming Interface (API) for the XenoARM JIT. This API allows a host emulator or application to control and interact with the JIT engine.

*Note: This is an initial draft and is subject to change as development progresses.*

## Introduction

The XenoARM JIT API is designed to be integrated into a larger emulator project. It provides functions to manage the JIT's lifecycle, control CPU execution, access guest CPU state, and interface with the guest's memory space.

## Core API Functions

The following sections outline the planned categories of API functions. Specific function signatures, parameter types, and return values will be defined in the source code and updated here.

### 1. Initialization and Configuration

Functions to initialize the JIT engine, configure its parameters, and clean up resources.

*   **`jit_error_t xenoarm_jit_init(jit_config_t* config);`**
    *   Initializes the JIT engine.
    *   `config`: A pointer to a structure containing configuration parameters (e.g., translation cache size, logging options).
    *   Returns an error code indicating success or failure.

*   **`void xenoarm_jit_shutdown();`**
    *   Shuts down the JIT engine and releases all allocated resources.

*   **`jit_error_t xenoarm_jit_configure(jit_option_key_t key, void* value);`** (Example)
    *   Allows runtime configuration of specific JIT options after initialization.
    *   `key`: An enum or identifier for the option to configure.
    *   `value`: A pointer to the value for the specified option.

### 2. Execution Control

Functions to start, stop, and step through guest code execution.

*   **`jit_error_t xenoarm_jit_run();`**
    *   Starts or resumes execution of guest code from the current program counter.
    *   Execution continues until a stop condition is met (e.g., breakpoint, host request, guest OS halt).

*   **`jit_error_t xenoarm_jit_step(uint32_t num_instructions);`** (Example)
    *   Executes a specified number of guest instructions.
    *   `num_instructions`: The number of instructions to step.

*   **`void xenoarm_jit_stop();`**
    *   Requests the JIT to stop execution at the next convenient point.

### 3. Guest CPU Register Access

Functions to read from and write to the guest CPU's registers.

*   **`jit_error_t xenoarm_jit_get_guest_register(guest_reg_id_t reg_id, void* out_value);`**
    *   Reads the value of a specified guest CPU register.
    *   `reg_id`: Identifier for the guest register (e.g., EAX, EBX, XMM0).
    *   `out_value`: A pointer to store the register's value.

*   **`jit_error_t xenoarm_jit_set_guest_register(guest_reg_id_t reg_id, const void* in_value);`**
    *   Writes a value to a specified guest CPU register.
    *   `reg_id`: Identifier for the guest register.
    *   `in_value`: A pointer to the value to write.

### 4. Memory Access Hooks/Interface

The JIT needs to interact with the guest's memory space, which is typically managed by the host emulator. The primary mechanism for this will be a set of callback functions provided by the host emulator during JIT initialization. These callbacks allow the JIT to request memory operations without needing direct access to the host's memory management internals.

*   **Memory Callback Function Typedefs:**
    ```c
    // Read operations
    typedef uint8_t (*host_read_guest_memory_u8_cb_t)(uint32_t guest_address, void* user_data);
    typedef uint16_t (*host_read_guest_memory_u16_cb_t)(uint32_t guest_address, void* user_data);
    typedef uint32_t (*host_read_guest_memory_u32_cb_t)(uint32_t guest_address, void* user_data);
    typedef uint64_t (*host_read_guest_memory_u64_cb_t)(uint32_t guest_address, void* user_data); // For MMX/SSE or 64-bit general values
    typedef void (*host_read_guest_memory_block_cb_t)(uint32_t guest_address, void* host_buffer, uint32_t size, void* user_data);

    // Write operations
    typedef void (*host_write_guest_memory_u8_cb_t)(uint32_t guest_address, uint8_t value, void* user_data);
    typedef void (*host_write_guest_memory_u16_cb_t)(uint32_t guest_address, uint16_t value, void* user_data);
    typedef void (*host_write_guest_memory_u32_cb_t)(uint32_t guest_address, uint32_t value, void* user_data);
    typedef void (*host_write_guest_memory_u64_cb_t)(uint32_t guest_address, uint64_t value, void* user_data); // For MMX/SSE or 64-bit general values
    typedef void (*host_write_guest_memory_block_cb_t)(uint32_t guest_address, const void* host_buffer, uint32_t size, void* user_data);

    // Optional: For querying memory attributes (e.g., for SMC detection or advanced JIT decisions)
    // Bitmask of attributes like GUEST_MEM_ATTR_READABLE, GUEST_MEM_ATTR_WRITABLE, GUEST_MEM_ATTR_EXECUTABLE_GUEST
    typedef uint32_t guest_mem_attr_t;
    typedef guest_mem_attr_t (*host_get_guest_memory_attributes_cb_t)(uint32_t guest_address, void* user_data);

    // Optional: For host to notify JIT about memory becoming dirty (relevant for SMC if not using page protection faults)
    // typedef void (*host_notify_guest_memory_dirty_cb_t)(uint32_t guest_address, uint32_t size, void* user_data);
    ```

*   **Guest Memory Callbacks Structure:**
    ```c
    typedef struct {
        // User-defined data pointer, passed to every callback. Can be used by the host to store its context.
        void* user_data;

        // Read callbacks (all are required)
        host_read_guest_memory_u8_cb_t read_u8;
        host_read_guest_memory_u16_cb_t read_u16;
        host_read_guest_memory_u32_cb_t read_u32;
        host_read_guest_memory_u64_cb_t read_u64;
        host_read_guest_memory_block_cb_t read_block;

        // Write callbacks (all are required)
        host_write_guest_memory_u8_cb_t write_u8;
        host_write_guest_memory_u16_cb_t write_u16;
        host_write_guest_memory_u32_cb_t write_u32;
        host_write_guest_memory_u64_cb_t write_u64;
        host_write_guest_memory_block_cb_t write_block;

        // Optional attribute query callback
        host_get_guest_memory_attributes_cb_t get_attributes; // Can be NULL if not implemented by host

        // Optional memory dirty notification callback
        // host_notify_guest_memory_dirty_cb_t notify_dirty; // Can be NULL
    } guest_memory_callbacks_t;
    ```

*   **Callback Registration Function:**
    ```c
    // This function would be called by the host during or after xenoarm_jit_init().
    // The JIT will store the provided callback structure for its internal memory operations.
    jit_error_t xenoarm_jit_register_memory_callbacks(const guest_memory_callbacks_t* callbacks);
    ```
    *   The JIT must be initialized (`xenoarm_jit_init`) before these callbacks can be registered.
    *   The `callbacks` structure pointer must remain valid for the lifetime of the JIT, or the JIT must make a deep copy. It's recommended the JIT copies the structure.
    *   The `user_data` field in `guest_memory_callbacks_t` allows the host to pass its own context (e.g., a pointer to its main emulator object) to its memory access functions when they are called by the JIT.

*   **Memory Fault Handling:**
    *   If the JIT, during its execution of translated code, encounters a situation that would be a guest-level memory fault (e.g., an x86 page fault that the JIT's generated code cannot resolve directly via the host memory callbacks), it should use the "Exception Handling Interface" (Section 6) to report this to the host.
    *   SMC (Self-Modifying Code) detection might rely on the host OS's page fault mechanism if the JITted code pages are protected. In such cases, the OS fault would lead to a signal handler within the JIT or host, which then triggers TC invalidation. Alternatively, if the host's `write_*` callbacks detect a write to a JITted region, the host could directly call a JIT API function to invalidate specific code regions (API for this TBD).

### 5. Logging and Error Reporting

Interfaces to control JIT logging and retrieve detailed error information.

*   **`jit_error_t xenoarm_jit_set_log_level(log_level_t level);`**
    *   Sets the verbosity of the JIT's internal logging.

*   **`const char* xenoarm_jit_get_last_error_string();`**
    *   Retrieves a human-readable string describing the last error that occurred within the JIT.

*   **Callback for log messages:**
    ```c
    typedef void (*jit_log_callback_t)(log_level_t level, const char* message);
    jit_error_t xenoarm_jit_set_log_callback(jit_log_callback_t callback);
    ```

### 6. Exception Handling Interface

Functions or callbacks to report guest CPU exceptions (e.g., Page Fault, General Protection Fault) to the host emulator.

*   **Example Callback Registration:**
    ```c
    typedef void (*guest_exception_callback_t)(uint32_t exception_vector, uint32_t error_code);
    jit_error_t xenoarm_jit_set_exception_callback(guest_exception_callback_t callback);
    ```

This API is designed to be flexible and extensible to support the evolving needs of the XenoARM JIT project.