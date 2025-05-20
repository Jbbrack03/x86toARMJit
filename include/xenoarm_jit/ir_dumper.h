#ifndef XENOARM_JIT_IR_DUMPER_H
#define XENOARM_JIT_IR_DUMPER_H

#include "xenoarm_jit/ir.h"
#include <ostream>

namespace xenoarm_jit {
namespace ir {

// Helper to convert IrInstructionType to string
const char* ir_instruction_type_to_string(IrInstructionType type);

// Helper to dump a single IrOperand
void dump_ir_operand(std::ostream& os, const IrOperand& operand);

// Helper to dump a single IrInstruction
void dump_ir_instruction(std::ostream& os, const IrInstruction& instruction);

// Helper to dump a single IrBasicBlock
void dump_ir_basic_block(std::ostream& os, const IrBasicBlock& block);

// Dumps the given IR function to the output stream.
void dump_ir_function(std::ostream& os, const IrFunction& ir_func);

} // namespace ir
} // namespace xenoarm_jit

#endif // XENOARM_JIT_IR_DUMPER_H