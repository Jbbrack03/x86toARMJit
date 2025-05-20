#ifndef XENOARM_JIT_AARCH64_CODE_GENERATOR_H
#define XENOARM_JIT_AARCH64_CODE_GENERATOR_H

#include "xenoarm_jit/ir.h"
#include "xenoarm_jit/register_allocation/register_allocator.h" // Include for RegisterMapping
#include "xenoarm_jit/translation_cache/translation_cache.h" // Include for TranslatedBlock
#include <vector>
#include <cstdint>
#include <unordered_map> // Include for std::unordered_map

namespace xenoarm_jit {
namespace aarch64 {

class CodeGenerator {
public:
    CodeGenerator();
    ~CodeGenerator();

    // Generates AArch64 code from a sequence of IR instructions and a register map
    std::vector<uint8_t> generate(
        const std::vector<ir::IrInstruction>& ir_instructions,
        const std::unordered_map<uint32_t, register_allocation::RegisterMapping>& register_map
    );

private:
    // Internal helper methods for generating specific AArch64 instructions
    void emit_instruction(std::vector<uint8_t>& code, uint32_t instruction);
    
    uint32_t get_physical_reg(uint32_t virtual_reg,
        const std::unordered_map<uint32_t, register_allocation::RegisterMapping>& register_map) const;
    uint32_t get_eflags_reg() const;
    
    // Using the complete TranslatedBlock definition from translation_cache.h
    void patch_branch(translation_cache::TranslatedBlock* source_block, 
                     const translation_cache::TranslatedBlock::ControlFlowExit& exit, 
                     translation_cache::TranslatedBlock* target_block);
                     
    void patch_branch_false(translation_cache::TranslatedBlock* source_block, 
                          const translation_cache::TranslatedBlock::ControlFlowExit& exit, 
                          translation_cache::TranslatedBlock* target_block);
};

} // namespace aarch64
} // namespace xenoarm_jit

#endif // XENOARM_JIT_AARCH64_CODE_GENERATOR_H