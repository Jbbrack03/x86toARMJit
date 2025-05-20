#ifndef XENOARM_JIT_FPU_DECODER_H
#define XENOARM_JIT_FPU_DECODER_H

#include "xenoarm_jit/decoder/decoder.h"
#include "xenoarm_jit/ir.h"

namespace xenoarm_jit {
namespace decoder {

/**
 * Decode an x87 FPU instruction and generate corresponding IR
 * 
 * @param decoder Pointer to the Decoder instance
 * @param insn Pointer to the x86 instruction being decoded
 * @param ir_func Pointer to the IR function being built
 * @param curr_block Pointer to the current IR basic block
 * @return true if instruction was successfully decoded, false otherwise
 */
bool decode_fpu_instruction(Decoder* decoder, x86_insn* insn, ir::IrFunction* ir_func, ir::IrBasicBlock* curr_block);

} // namespace decoder
} // namespace xenoarm_jit

#endif // XENOARM_JIT_FPU_DECODER_H 