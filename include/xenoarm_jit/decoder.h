#ifndef XENOARM_JIT_DECODER_H
#define XENOARM_JIT_DECODER_H

#include <cstdint>
#include <vector>
#include "xenoarm_jit/ir.h"

namespace xenoarm_jit {
namespace decoder {

// A basic class for decoding x86 instructions into IR
class X86Decoder {
public:
    X86Decoder();
    ~X86Decoder();

    // Decodes a block of x86 code starting at guest_address
    // and returns the translated IR function.
    // For Phase 1, this will be a very basic implementation
    // handling a subset of instructions.
    ir::IrFunction decode_block(const uint8_t* guest_code, uint64_t guest_address, size_t max_bytes);

private:
    // Helper function to decode a single instruction
    // and return the corresponding IR instructions.
    std::vector<ir::IrInstruction> decode_instruction(const uint8_t* instruction_bytes, size_t& bytes_read, size_t max_bytes_for_instruction);

    // TODO: Add internal state and helper methods for decoding prefixes,
    // operands, etc. in later stages of Phase 1.
};

} // namespace decoder
} // namespace xenoarm_jit

#endif // XENOARM_JIT_DECODER_H