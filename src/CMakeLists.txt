# Build the core XenoARM JIT library
add_library(xenoarm_jit STATIC
    logging/logger.cpp
    jit_core/jit_api.cpp
    jit_core/exception_handler.cpp
    jit_core/c_api_stubs.cpp
    decoder/x86_decoder.cpp
    decoder/fpu_decoder.cpp
    decoder/decoder.cpp
    ir/ir_dumper.cpp
    aarch64/code_generator.cpp
    aarch64/arm_assembler.cpp
    # Phase 6 components
    memory_manager.cpp
    signal_handler.cpp
    memory_model.cpp
    # FPU support
    simd/floating_point_conversion.cpp
    simd/simd_state.cpp
    # Phase 8 completion - ARM FPU optimizations
    simd/simd_helpers.cpp
    simd/fpu_transcendental_helpers.cpp
    aarch64/fpu_code_gen.cpp
    # Translation cache and register allocator
    translation_cache/translation_cache.cpp
    register_allocation/register_allocator.cpp
    # Add other core JIT source files here as they are created in later phases
)

# Link include directories to the library
target_include_directories(xenoarm_jit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/jit_core
    ${CMAKE_CURRENT_SOURCE_DIR}/translation_cache
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Set compile definitions
target_compile_definitions(xenoarm_jit PRIVATE
    # Phase 6 flags
    ENABLE_SMC_DETECTION=1
    ENABLE_MEMORY_MODEL=1
    # Phase 8 completion - ARM FPU optimizations
    # HOST_PROVIDES_MEMORY_ACCESS=1  # Disabled for test compatibility
)

# Modify this line to comment out the host_stub executable
# add_executable(host_stub
#    host_stub/main.cpp
# )
# 
# target_link_libraries(host_stub
#    PRIVATE xenoarm_jit
# )

# Set output directory for executables and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)