# XenoARM JIT - Coding Standards

This document outlines the coding standards and conventions to be followed for the XenoARM JIT project. Adhering to these standards will help ensure code consistency, readability, and maintainability.

## 1. Language Version

*   **C++17:** The project will be written in C++17. Utilize C++17 features where they improve clarity, safety, or performance.

## 2. Formatting Style

*   **Base Style:** We will adopt the **Google C++ Style Guide** as the foundational style.
    *   Reference: `https://google.github.io/styleguide/cppguide.html`
*   **Automated Formatting:** Code formatting will be enforced using `clang-format`.
    *   A `.clang-format` file will be maintained in the root of the repository.
    *   This file will be configured to be based on the `Google` style, with potential minor adjustments if deemed necessary by the team.
    *   **Action:** Create an initial `.clang-format` file based on `BasedOnStyle: Google`.
*   **Key Aspects (derived from Google Style, to be enforced by clang-format):**
    *   **Indentation:** 2 spaces. No tabs.
    *   **Line Length:** 80 characters (Google default). This can be discussed if a slightly longer limit (e.g., 100) is preferred.
    *   **Braces:**
        *   Allman style (braces on new lines) is generally *not* Google style. Google style typically uses K&R variant (opening brace on the same line for functions, classes, etc., but often on new lines for control statement bodies if multi-line). `clang-format` will handle this.
        *   Always use braces for control statements (`if`, `else`, `for`, `while`, `do-while`), even for single-line bodies.
    *   **Spaces vs. Tabs:** Spaces for indentation.
    *   **Pointer and Reference Alignment:** `int* p;` or `int& r;` (asterisk/ampersand next to the type or variable name can be configured, Google style is `int* p;`).

## 3. Naming Conventions

*   **General Rule:** Choose descriptive names. Avoid abbreviations unless very common and well-understood.
*   **File Names:**
    *   Lowercase with underscores: `my_useful_class.h`, `my_useful_class.cpp`.
*   **Type Names (Classes, Structs, Enums, Typedefs, `using` aliases):**
    *   `PascalCase` (e.g., `MyClass`, `CpuState`, `JitError`).
*   **Function Names:**
    *   `PascalCase` (e.g., `DecodeInstruction()`, `GenerateCodeBlock()`). (Google style for functions).
*   **Variable Names:**
    *   **Local Variables:** Lowercase with underscores: `local_variable`, `instruction_count`. (Google style).
    *   **Struct/Class Member Variables:** Lowercase with underscores, appended with a trailing underscore: `member_variable_`, `current_eip_`. (Google style).
    *   **Global Variables:** Prefix with `g_` followed by lowercase with underscores: `g_jit_config`. **Avoid global variables whenever possible.**
*   **Constant Names (`const`, `constexpr`, Enum Values):**
    *   `kPascalCase` for `constexpr` or `const` variables that are part of an API or fixed values (e.g., `kMaxCacheEntries`). (Google style).
    *   `ALL_CAPS_SNAKE_CASE` for macros (avoid macros for constants if `constexpr` can be used) and enum values if not using `enum class` with `kPascalCase` values. For `enum class`, `kPascalCase` for enumerators is also acceptable.
*   **Namespace Names:**
    *   Lowercase, usually short: `xenoarm_jit`. All project code should reside within this namespace or sub-namespaces.
*   **Macro Names (use sparingly):**
    *   `ALL_CAPS_SNAKE_CASE`. Prefer inline functions, `constexpr`, or templates over macros.

## 4. Comments

*   **Purpose:** Explain non-obvious code, document interfaces, and provide context.
*   **Style:**
    *   Use `//` for single-line comments.
    *   Use `/* ... */` for multi-line comments (less common than multiple `//`).
    *   **Doxygen-style comments** for public APIs in header files:
        ```cpp
        /**
         * @brief Brief description of the function or class.
         *
         * More detailed description if necessary.
         * @param param_name Description of the parameter.
         * @return Description of the return value.
         * @note Any important notes.
         */
        ```
        or
        ```cpp
        /// @brief Brief description.
        /// More details.
        ```
*   Comment complex algorithms or tricky sections of code.
*   Avoid commenting obvious code.

## 5. C++ Language Features & Best Practices

*   **Include Guards:** Use `#pragma once` in all header files.
*   **`nullptr`:** Use `nullptr` instead of `NULL` or `0` for pointers.
*   **`enum class`:** Prefer `enum class` over plain `enum` for improved type safety and scoping.
*   **Initialization:** Initialize variables upon declaration. Use `{}` (uniform initialization) where appropriate.
*   **`const` Correctness:** Use `const` extensively for variables, member functions, and parameters that should not be modified.
*   **Smart Pointers:**
    *   Consider `std::unique_ptr` for exclusive ownership where appropriate.
    *   Use `std::shared_ptr` if shared ownership is truly necessary (be mindful of potential performance implications in critical paths).
    *   For a low-level JIT, raw pointers will be common for performance-critical data structures and direct memory manipulation, but require careful lifetime management. Smart pointers can be used for higher-level management objects.
*   **Error Handling:**
    *   **Public API:** Use clear return codes (e.g., an enum like `XenoJitError`) to indicate success or failure. Avoid exceptions across the API boundary.
    *   **Internal JIT Logic:**
        *   Use `assert()` liberally for preconditions, postconditions, and invariants during development (these are typically compiled out in release builds).
        *   For recoverable internal errors, consider returning error codes or using a specific error state.
        *   Avoid using C++ exceptions for normal control flow. They may be considered for truly exceptional, unrecoverable internal states if it simplifies error propagation from deep call stacks, but this should be decided carefully due to potential performance impact and complexity in a JIT.
*   **Standard Library:** Use the C++ Standard Library (STL) containers, algorithms, and utilities where they provide clear benefits and do not impose unacceptable performance overhead in critical paths.
*   **Avoid `using namespace std;`** in header files. In source files, it's generally better to qualify names with `std::` or use specific `using std::vector;` declarations.

## 6. Code Structure

*   Keep functions short and focused on a single task.
*   Organize code into logical classes and namespaces.
*   Header files should be self-contained and include only what is necessary. Minimize include dependencies.

## 7. Evolution

This document is a starting point. These standards may evolve as the project progresses and the team (if it grows) provides feedback. Changes will be discussed and documented.

---
**Action Item:** Create a `.clang-format` file in the repository root with `BasedOnStyle: Google` and any agreed-upon minor deviations.