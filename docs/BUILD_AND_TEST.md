# Building and Testing XenoARM JIT

This document provides instructions on how to build the XenoARM JIT project and run its tests.

## Prerequisites

Before you begin, ensure you have the following installed:

*   **C++ Compiler:** A modern C++ compiler (C++17 or newer recommended) with good AArch64 support.
    *   Examples: Clang, GCC.
*   **CMake:** Version 3.15 or higher is recommended. CMake is used as the build system.
*   **Git:** For cloning the repository and managing versions.
*   **Testing Framework:** The project uses a C++ testing framework (e.g., Google Test or Catch2). This will typically be fetched by CMake or included as a submodule.

## Building the Project

1.  **Clone the Repository:**
    ```bash
    git clone <repository_url> XenoARM_JIT
    cd XenoARM_JIT
    ```
    (Replace `<repository_url>` with the actual URL of the Git repository)

2.  **Configure with CMake:**
    Create a build directory and run CMake from there:
    ```bash
    mkdir build
    cd build
    cmake .. 
    ```
    *   You can specify a generator if needed (e.g., `cmake .. -G "Ninja"` or `cmake .. -G "Unix Makefiles"`).
    *   For cross-compilation to AArch64, you might need to provide a CMake toolchain file. (Details to be added once cross-compilation setup is defined).
    *   Common CMake build types:
        *   `cmake .. -DCMAKE_BUILD_TYPE=Debug` (for development, includes debug symbols)
        *   `cmake .. -DCMAKE_BUILD_TYPE=Release` (for performance, enables optimizations)
        *   `cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo` (Release with debug symbols)

3.  **Compile the Project:**
    After CMake configuration is successful, build the project using your chosen build tool:
    ```bash
    # If using Makefiles (default on many Linux systems)
    make -j$(nproc) 

    # If using Ninja
    ninja
    ```
    The compiled binaries (executables, libraries, tests) will typically be found in the `build` directory or a subdirectory within it.

## Running Tests

The project includes several types of tests to ensure correctness and stability:

*   **Unit Tests:** Test individual components and functions of the JIT in isolation.
*   **Architectural Tests:** Validate the JIT's emulation accuracy against known x86 CPU behavior, potentially using suites like `x86test` or custom-developed tests.

To run the tests (assuming they are built as part of the default build process):

```bash
# Navigate to the build directory if not already there
cd /path/to/XenoARM_JIT/build

# Run tests (the exact command might depend on the testing framework and CMake setup)
ctest
# or directly execute the test binary, e.g., ./xenoarm_tests
```

Refer to the output of the test runner for pass/fail status and any detailed error messages.

## Debugging

*   **Logging:** The JIT incorporates internal logging capabilities. Check the project's configuration options or runtime flags to control log verbosity and output.
*   **Dumping Generated ARM Code:** For specific x86 blocks, the JIT may provide an option to dump the translated AArch64 code. This is invaluable for debugging translation errors.
*   **Debuggers:**
    *   Use **GDB** (GNU Debugger) or **LLDB** for debugging the JIT compiler itself when running on the host machine.
    *   When running on target ARM hardware, ARM-specific debuggers might be necessary.
    *   **Valgrind** can be useful for detecting memory issues (e.g., leaks, invalid memory access) when testing on a host Linux system.
*   **Build Type:** Ensure you are using a `Debug` or `RelWithDebInfo` build type in CMake to have necessary debug symbols.