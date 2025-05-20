# XenoARM JIT - V1.0

A high-performance JIT compiler for emulating the Original Xbox CPU (Intel Pentium III) on ARM AArch64 platforms.

## 🎮 Overview

XenoARM JIT is a dynamic recompiler that translates x86 instructions from Original Xbox games to native ARM machine code in real-time, enabling efficient emulation on devices like the Retroid Pocket 2 Flip, smartphones, tablets, and ARM-based PCs.

## ✨ Key Features

- **Complete x86 Emulation:** Full support for the Original Xbox CPU instruction set
- **SIMD Support:** Efficient translation of MMX and SSE to ARM NEON
- **Dynamic Recompilation:** Just-in-time compilation with translation caching
- **Self-Modifying Code Detection:** Reliable handling of code that modifies itself
- **Memory Model Consistency:** Proper handling of architecture memory model differences
- **Clean API:** Well-defined interface for host emulator integration

## 📚 Documentation

Detailed documentation is available in the `docs/` directory:

- [Development Plan](docs/DEVELOPMENT_PLAN.md): The phased development approach
- [Release Notes](docs/RELEASE_NOTES_V1.0.md): V1.0 features and capabilities
- [Future Development](docs/FUTURE_DEVELOPMENT.md): Roadmap for future enhancements
- [API Reference](docs/API_REFERENCE.md): Complete API documentation

## 🔧 Building from Source

```bash
# Clone the repository
git clone https://github.com/your-username/xenoarm-jit.git
cd xenoarm-jit

# Build using the provided script
./build.sh

# Or build manually
mkdir -p build
cd build
cmake ..
make
```

## 🧪 Testing

The project includes a comprehensive test suite:

```bash
cd build
ctest               # Run all tests
ctest -R api_tests  # Run specific tests
```

## 🚀 Integration

XenoARM JIT is designed to be integrated into host emulators. See the [API Reference](docs/API_REFERENCE.md) for details on how to integrate the JIT into your emulation project.

## 📈 Performance

XenoARM JIT delivers high-performance x86 emulation on ARM platforms:

- Efficient register allocation using linear scan algorithm
- Translation Cache with block linking optimization
- Minimal overhead for safety features like SMC detection
- SIMD acceleration using ARM NEON

## 📋 Requirements

- AArch64 (ARM 64-bit) platform
- C++17 compatible compiler
- CMake 3.10 or higher

## 📝 License

[Add your license information here]

## 🙏 Acknowledgments

This project builds upon research and techniques from the broader emulation community. Special thanks to all contributors who have helped make this project possible.

---

**XenoARM JIT V1.0** - Bringing Original Xbox games to ARM devices with high performance 