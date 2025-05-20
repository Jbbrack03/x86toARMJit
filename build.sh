#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake, setting appropriate options
cmake .. \
    -DSKIP_FPU_TESTS=OFF \
    -DSKIP_MEMORY_MODEL_TEST=ON \
    -DSKIP_CODE_GEN_TESTS=ON \
    -DSKIP_DISPATCHER_TESTS=ON \
    -DSKIP_INTEGER_OPS_TESTS=ON \
    -DSKIP_CONTROL_FLOW_TESTS=ON \
    -DSKIP_SIMD_TESTS=OFF \
    -DSKIP_ARCH_TESTS=ON \
    -DSKIP_BENCHMARKS=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build the project
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

echo ""
echo "Build complete."

# Run the tests if requested
if [ "$1" = "test" ]; then
    echo "Running FPU and SIMD tests..."
    ctest -V
fi 