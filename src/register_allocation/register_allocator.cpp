#include "xenoarm_jit/register_allocation/register_allocator.h"
#include "logging/logger.h"
#include <algorithm>
#include <set>
#include <limits>
#include <cassert>
#include <map>

namespace xenoarm_jit {
namespace register_allocation {

// Structure to represent register usage statistics for smart allocation
struct RegisterUsageStats {
    uint32_t vreg_id;
    uint32_t use_count;         // How many times this register is used
    bool is_x86_reg_mapped;     // Whether this is a direct mapping of an x86 register
    bool involved_in_loop;      // Whether this register is used in a loop
};

// SpillAllocator implementation (Phase 8)
SpillAllocator::SpillAllocator() : current_offset_(0) {
}

int32_t SpillAllocator::allocate_spill_slot(uint32_t vreg_id, ir::IrDataType data_type) {
    // Determine size needed for this data type
    uint32_t size = 8; // Default to 64-bit
    
    // Adjust size based on data type
    switch (data_type) {
        case ir::IrDataType::I8:
        case ir::IrDataType::U8:
            size = 1;
            break;
        case ir::IrDataType::I16:
        case ir::IrDataType::U16:
            size = 2;
            break;
        case ir::IrDataType::I32:
        case ir::IrDataType::U32:
        case ir::IrDataType::F32:
            size = 4;
            break;
        case ir::IrDataType::I64:
        case ir::IrDataType::U64:
        case ir::IrDataType::F64:
        case ir::IrDataType::V64_B8:
        case ir::IrDataType::V64_W4:
        case ir::IrDataType::V64_D2:
            size = 8;
            break;
        case ir::IrDataType::F80:
            size = 16; // Align to 16 for 80-bit floats for simplicity
            break;
        case ir::IrDataType::V128_B16:
        case ir::IrDataType::V128_W8:
        case ir::IrDataType::V128_D4:
        case ir::IrDataType::V128_Q2:
            size = 16;
            break;
        case ir::IrDataType::PTR:
            size = 8; // 64-bit pointers on AArch64
            break;
        default:
            LOG_WARNING("Unknown data type for spill slot allocation. Defaulting to 8 bytes.");
            size = 8;
            break;
    }
    
    // Ensure proper alignment (align to size)
    int32_t aligned_offset = (current_offset_ + size - 1) & ~(size - 1);
    
    // Store allocation info
    allocated_slots_[aligned_offset] = size;
    
    // Update current offset for next allocation
    current_offset_ = aligned_offset + size;
    
    LOG_DEBUG("Allocated spill slot at offset " + std::to_string(aligned_offset) + " of size " + std::to_string(size) + " for vreg " + std::to_string(vreg_id));
    
    return aligned_offset;
}

void SpillAllocator::free_spill_slot(int32_t offset) {
    // Remove the slot from allocated slots
    if (allocated_slots_.find(offset) != allocated_slots_.end()) {
        LOG_DEBUG("Freed spill slot at offset " + std::to_string(offset));
        allocated_slots_.erase(offset);
    }
}

int32_t SpillAllocator::get_total_spill_size() const {
    return current_offset_;
}

void SpillAllocator::reset() {
    allocated_slots_.clear();
    current_offset_ = 0;
}

// Helper function to check if an instruction is likely to modify a register
bool isRegisterModifyingInstruction(const ir::IrInstruction& inst) {
    // Check instruction types that typically modify their first operand
    switch (inst.type) {
        case ir::IrInstructionType::MOV:
        case ir::IrInstructionType::ADD:
        case ir::IrInstructionType::SUB:
        case ir::IrInstructionType::MUL:
        case ir::IrInstructionType::DIV:
        case ir::IrInstructionType::AND:
        case ir::IrInstructionType::OR:
        case ir::IrInstructionType::XOR:
        case ir::IrInstructionType::LOAD:
        case ir::IrInstructionType::INC:
        case ir::IrInstructionType::DEC:
        case ir::IrInstructionType::POP:
        case ir::IrInstructionType::VEC_MOV:
        case ir::IrInstructionType::VEC_ADD_PS:
        case ir::IrInstructionType::VEC_SUB_PS:
        case ir::IrInstructionType::VEC_MUL_PS:
        case ir::IrInstructionType::VEC_DIV_PS:
        case ir::IrInstructionType::VEC_ADD_PD:
        case ir::IrInstructionType::VEC_SUB_PD:
        case ir::IrInstructionType::VEC_MUL_PD:
        case ir::IrInstructionType::VEC_DIV_PD:
        case ir::IrInstructionType::VEC_ADD_PI8:
        case ir::IrInstructionType::VEC_SUB_PI8:
        case ir::IrInstructionType::VEC_MUL_PI16:
        case ir::IrInstructionType::VEC_ADD_W:
            return true;
        default:
            return false;
    }
}

// Helper function to check if an instruction is a branch
bool isBranchInstruction(const ir::IrInstruction& inst) {
    switch (inst.type) {
        case ir::IrInstructionType::JMP:
        case ir::IrInstructionType::BR_EQ:
        case ir::IrInstructionType::BR_NE:
        case ir::IrInstructionType::BR_LT:
        case ir::IrInstructionType::BR_LE:
        case ir::IrInstructionType::BR_GT:
        case ir::IrInstructionType::BR_GE:
        case ir::IrInstructionType::BR_BL:
        case ir::IrInstructionType::BR_BE:
        case ir::IrInstructionType::BR_BH:
        case ir::IrInstructionType::BR_BHE:
        case ir::IrInstructionType::BR_ZERO:
        case ir::IrInstructionType::BR_NOT_ZERO:
        case ir::IrInstructionType::BR_SIGN:
        case ir::IrInstructionType::BR_NOT_SIGN:
        case ir::IrInstructionType::BR_OVERFLOW:
        case ir::IrInstructionType::BR_NOT_OVERFLOW:
        case ir::IrInstructionType::BR_PARITY:
        case ir::IrInstructionType::BR_NOT_PARITY:
        case ir::IrInstructionType::BR_CARRY:
        case ir::IrInstructionType::BR_NOT_CARRY:
        case ir::IrInstructionType::BR_COND:
            return true;
        default:
            return false;
    }
}

// Helper function to check if a data type requires a NEON register
bool requiresNeonRegister(ir::IrDataType data_type) {
    switch (data_type) {
        case ir::IrDataType::F32:
        case ir::IrDataType::F64:
        case ir::IrDataType::F80:
        case ir::IrDataType::V64_B8:
        case ir::IrDataType::V64_W4:
        case ir::IrDataType::V64_D2:
        case ir::IrDataType::V128_B16:
        case ir::IrDataType::V128_W8:
        case ir::IrDataType::V128_D4:
        case ir::IrDataType::V128_Q2:
            return true;
        default:
            return false;
    }
}

RegisterAllocator::RegisterAllocator() {
    LOG_DEBUG("RegisterAllocator created.");
    
    // Initialize free register lists
    // Reserve some GPRs for temporary usage during code generation
    // X28 is reserved as temp, X29 is FP, X30 is LR, X31 is SP/XZR
    for (int i = 0; i < 28; i++) {
        if (i != 16 && i != 17) { // Reserve x16, x17 for platform use
            free_gpr_registers_.push_back(i);
        }
    }
    
    // For NEON/Vector registers, use V0-V31
    // Reserve V0-V7 for temporary use
    for (int i = 8; i < 32; i++) {
        free_neon_registers_.push_back(i);
    }
}

RegisterAllocator::~RegisterAllocator() {
    LOG_DEBUG("RegisterAllocator destroyed.");
}

// Enhanced compute_lifetimes function for Phase 8
std::vector<VRegLifetime> RegisterAllocator::compute_lifetimes(const std::vector<ir::IrInstruction>& ir_instructions) {
    // Map from vreg_id to its lifetime
    std::unordered_map<uint32_t, VRegLifetime> lifetimes;
    
    // First pass: Compute usage statistics and find start/end points for each vreg
    for (size_t i = 0; i < ir_instructions.size(); i++) {
        const auto& inst = ir_instructions[i];
        
        // Check for registers in operands
        for (const auto& op : inst.operands) {
            if (op.type == ir::IrOperandType::REGISTER) {
                uint32_t vreg_id = op.reg_idx;
                
                // Initialize or update lifetime
                if (lifetimes.find(vreg_id) == lifetimes.end()) {
                    VRegLifetime lifetime;
                    lifetime.vreg_id = vreg_id;
                    lifetime.data_type = op.data_type;
                    lifetime.start = i;
                    lifetime.end = i;
                    lifetime.use_count = 1;
                    lifetime.is_active = false;
                    lifetime.is_loop_register = false;
                    lifetime.is_x86_mapped = (vreg_id < 8); // Assume vregs 0-7 are x86 mappings
                    lifetime.priority = 0.0f;
                    lifetimes[vreg_id] = lifetime;
                } else {
                    // Update end point if this is used later
                    lifetimes[vreg_id].end = i;
                    // Update usage count
                    lifetimes[vreg_id].use_count++;
                }
            }
        }
        
        // Special handling for instructions that modify registers (typically first operand)
        if (isRegisterModifyingInstruction(inst)) {
            // For instructions that modify their first operand
            if (!inst.operands.empty() && inst.operands[0].type == ir::IrOperandType::REGISTER) {
                uint32_t vreg_id = inst.operands[0].reg_idx;
                
                // Initialize or update lifetime
                if (lifetimes.find(vreg_id) == lifetimes.end()) {
                    VRegLifetime lifetime;
                    lifetime.vreg_id = vreg_id;
                    lifetime.data_type = inst.operands[0].data_type;
                    lifetime.start = i;
                    lifetime.end = i;
                    lifetime.use_count = 1;
                    lifetime.is_active = false;
                    lifetime.is_loop_register = false;
                    lifetime.is_x86_mapped = (vreg_id < 8); // Assume vregs 0-7 are x86 mappings
                    lifetime.priority = 0.0f;
                    lifetimes[vreg_id] = lifetime;
                } else {
                    // Update end point
                    lifetimes[vreg_id].end = i;
                    // Update usage count
                    lifetimes[vreg_id].use_count++;
                }
            }
        }
    }
    
    // Convert map to vector for sorting
    std::vector<VRegLifetime> result;
    for (const auto& [vreg_id, lifetime] : lifetimes) {
        result.push_back(lifetime);
        
        // Also update our lifetime map for later reference
        lifetime_map_[vreg_id] = lifetime;
    }
    
    // Sort by start point for linear scan algorithm
    std::sort(result.begin(), result.end(), 
              [](const VRegLifetime& a, const VRegLifetime& b) {
                  return a.start < b.start;
              });
    
    return result;
}

// New method to detect loops in the instruction stream (Phase 8)
void RegisterAllocator::detect_loops(const std::vector<ir::IrInstruction>& ir_instructions, std::vector<VRegLifetime>& lifetimes) {
    // This is a simplified loop detection algorithm that looks for backward branches
    std::vector<std::pair<uint32_t, uint32_t>> loops; // start, end pairs of loop regions
    
    for (size_t i = 0; i < ir_instructions.size(); i++) {
        const auto& inst = ir_instructions[i];
        
        if (isBranchInstruction(inst)) {
            // Look for backward branches which might indicate loops
            if (!inst.operands.empty() && inst.operands[0].type == ir::IrOperandType::IMMEDIATE) {
                uint32_t target = static_cast<uint32_t>(inst.operands[0].imm_value);
                
                // If target is before current instruction, it's a backward branch (potential loop)
                if (target < i) {
                    LOG_DEBUG("Detected potential loop from instruction " + 
                              std::to_string(target) + " to " + std::to_string(i));
                    
                    // Add this loop region
                    loops.emplace_back(target, i);
                }
            }
        }
    }
    
    // Mark registers that are used within loop regions
    for (auto& lifetime : lifetimes) {
        for (const auto& loop : loops) {
            // If the register's lifetime overlaps with a loop region, mark it as a loop register
            if (lifetime.start <= loop.second && lifetime.end >= loop.first) {
                lifetime.is_loop_register = true;
                
                // Update in our map as well
                if (lifetime_map_.find(lifetime.vreg_id) != lifetime_map_.end()) {
                    lifetime_map_[lifetime.vreg_id].is_loop_register = true;
                }
                
                LOG_DEBUG("Register v" + std::to_string(lifetime.vreg_id) + 
                          " marked as loop register (used in loop " + 
                          std::to_string(loop.first) + "-" + std::to_string(loop.second) + ")");
                break;
            }
        }
    }
}

// New method to compute register priorities for allocation decisions (Phase 8)
void RegisterAllocator::compute_register_priorities(std::vector<VRegLifetime>& lifetimes) {
    // Compute priority based on:
    // 1. Is it an x86 mapped register? (highest priority)
    // 2. Is it used in a loop? (high priority)
    // 3. How many times is it used? (medium priority)
    // 4. How long is its lifetime? (inverse - shorter lifetimes get higher priority)
    
    // Find maximum values for normalization
    uint32_t max_use_count = 1;
    uint32_t max_lifetime = 1;
    
    for (const auto& lifetime : lifetimes) {
        max_use_count = std::max(max_use_count, lifetime.use_count);
        max_lifetime = std::max(max_lifetime, lifetime.end - lifetime.start + 1);
    }
    
    // Compute priority for each register
    for (auto& lifetime : lifetimes) {
        float priority = 0.0f;
        
        // x86 mapped registers get highest priority
        if (lifetime.is_x86_mapped) {
            priority += 10000.0f; // Significantly increase priority for x86 mapped registers
        }
        
        // Loop registers get high priority
        if (lifetime.is_loop_register) {
            priority += 500.0f;
        }
        
        // Use count (normalized to 0-100)
        priority += 100.0f * static_cast<float>(lifetime.use_count) / static_cast<float>(max_use_count);
        
        // Inverse lifetime (shorter lifetimes get higher priority)
        float lifetime_length = static_cast<float>(lifetime.end - lifetime.start + 1);
        priority += 50.0f * (1.0f - lifetime_length / static_cast<float>(max_lifetime));
        
        lifetime.priority = priority;
        
        // Update in our map as well
        if (lifetime_map_.find(lifetime.vreg_id) != lifetime_map_.end()) {
            lifetime_map_[lifetime.vreg_id].priority = priority;
        }
        
        LOG_DEBUG("Register v" + std::to_string(lifetime.vreg_id) + 
                  " priority: " + std::to_string(priority) + 
                  " (x86_mapped=" + std::to_string(lifetime.is_x86_mapped) + 
                  ", loop=" + std::to_string(lifetime.is_loop_register) + 
                  ", uses=" + std::to_string(lifetime.use_count) + 
                  ", lifetime=" + std::to_string(lifetime.end - lifetime.start + 1) + ")");
    }
}

// New method to select a register to spill based on priorities (Phase 8)
uint32_t RegisterAllocator::select_register_to_spill(const std::vector<uint32_t>& candidates) {
    uint32_t spill_candidate = candidates[0];
    float lowest_priority = std::numeric_limits<float>::max();
    
    for (uint32_t vreg_id : candidates) {
        if (lifetime_map_.find(vreg_id) != lifetime_map_.end()) {
            float priority = lifetime_map_[vreg_id].priority;
            if (priority < lowest_priority) {
                lowest_priority = priority;
                spill_candidate = vreg_id;
            }
        }
    }
    
    LOG_DEBUG("Selected register v" + std::to_string(spill_candidate) + 
              " for spilling (priority=" + std::to_string(lowest_priority) + ")");
    
    return spill_candidate;
}

// New method to spill a register (Phase 8)
void RegisterAllocator::spill_register(uint32_t vreg_id, ir::IrDataType data_type) {
    if (register_mappings_.find(vreg_id) == register_mappings_.end()) {
        LOG_ERROR("Attempted to spill register v" + std::to_string(vreg_id) + 
                  " that doesn't have a mapping");
        return;
    }
    
    RegisterMapping& mapping = register_mappings_[vreg_id];
    
    // If already spilled, nothing to do
    if (mapping.is_spilled) {
        return;
    }
    
    // Allocate a spill slot
    int32_t offset = spill_allocator_.allocate_spill_slot(vreg_id, data_type);
    
    // Update the mapping
    mapping.is_spilled = true;
    mapping.stack_offset = offset;
    
    // Free the physical register
    if (mapping.type == PhysicalRegisterType::GPR) {
        free_gpr_registers_.push_back(mapping.gpr_physical_reg_idx);
    } else {
        free_neon_registers_.push_back(mapping.neon_physical_reg_idx);
    }
    
    LOG_DEBUG("Spilled register v" + std::to_string(vreg_id) + 
              " to stack offset " + std::to_string(offset));
}

// Linear scan register allocation implementation (Phase 8)
void RegisterAllocator::linear_scan_register_allocation(const std::vector<VRegLifetime>& lifetimes) {
    LOG_DEBUG("Performing linear scan register allocation with spilling support.");
    
    // Active list starts empty
    active_registers_.clear();
    
    // Process virtual registers in order of increasing start point
    for (const auto& lifetime : lifetimes) {
        uint32_t vreg_id = lifetime.vreg_id;
        ir::IrDataType data_type = lifetime.data_type;
        bool needs_neon = requiresNeonRegister(data_type);
        
        LOG_DEBUG("Processing register v" + std::to_string(vreg_id) + 
                  " (start=" + std::to_string(lifetime.start) + 
                  ", end=" + std::to_string(lifetime.end) + 
                  ", neon=" + std::to_string(needs_neon) + ")");
        
        // Expire old intervals
        for (auto it = active_registers_.begin(); it != active_registers_.end(); ) {
            uint32_t active_vreg = *it;
            if (lifetime_map_[active_vreg].end < lifetime.start) {
                LOG_DEBUG("Register v" + std::to_string(active_vreg) + " expired");
                
                // If not spilled, free the physical register
                if (!register_mappings_[active_vreg].is_spilled) {
                    if (register_mappings_[active_vreg].type == PhysicalRegisterType::GPR) {
                        free_gpr_registers_.push_back(register_mappings_[active_vreg].gpr_physical_reg_idx);
                    } else {
                        free_neon_registers_.push_back(register_mappings_[active_vreg].neon_physical_reg_idx);
                    }
                }
                
                it = active_registers_.erase(it);
            } else {
                ++it;
            }
        }
        
        // Try to allocate a register
        RegisterMapping mapping;
        mapping.is_spilled = false;
        mapping.stack_offset = 0;
        
        if (needs_neon) {
            if (!free_neon_registers_.empty()) {
                // Allocate a NEON register
                mapping.type = PhysicalRegisterType::NEON;
                mapping.neon_physical_reg_idx = free_neon_registers_.back();
                free_neon_registers_.pop_back();
                
                LOG_DEBUG("Allocated NEON register v" + std::to_string(mapping.neon_physical_reg_idx) + 
                          " for virtual register v" + std::to_string(vreg_id));
            } else {
                // Need to spill a register
                if (active_registers_.empty()) {
                    // Shouldn't happen if we have reasonable register allocation
                    LOG_ERROR("No active registers to spill but ran out of NEON registers");
                    
                    // Create a spill location for this register
                    mapping.is_spilled = true;
                    mapping.stack_offset = spill_allocator_.allocate_spill_slot(vreg_id, data_type);
                    
                    LOG_DEBUG("Spilled register v" + std::to_string(vreg_id) + 
                              " to stack offset " + std::to_string(mapping.stack_offset) + 
                              " because no free NEON registers");
                } else {
                    // Find candidates for spilling (must be NEON registers)
                    std::vector<uint32_t> spill_candidates;
                    
                    for (uint32_t active_vreg : active_registers_) {
                        if (register_mappings_[active_vreg].type == PhysicalRegisterType::NEON && 
                            !register_mappings_[active_vreg].is_spilled) {
                            spill_candidates.push_back(active_vreg);
                        }
                    }
                    
                    if (spill_candidates.empty()) {
                        // No NEON registers to spill
                        LOG_ERROR("No NEON registers to spill");
                        
                        // Create a spill location for this register
                        mapping.is_spilled = true;
                        mapping.stack_offset = spill_allocator_.allocate_spill_slot(vreg_id, data_type);
                        
                        LOG_DEBUG("Spilled register v" + std::to_string(vreg_id) + 
                                  " to stack offset " + std::to_string(mapping.stack_offset) + 
                                  " because no NEON registers to spill");
                    } else {
                        // Select a register to spill
                        uint32_t spill_vreg = select_register_to_spill(spill_candidates);
                        
                        // Spill the selected register
                        uint32_t neon_reg = register_mappings_[spill_vreg].neon_physical_reg_idx;
                        spill_register(spill_vreg, lifetime_map_[spill_vreg].data_type);
                        
                        // Allocate the freed register to this vreg
                        mapping.type = PhysicalRegisterType::NEON;
                        mapping.neon_physical_reg_idx = neon_reg;
                        
                        LOG_DEBUG("Allocated NEON register v" + std::to_string(mapping.neon_physical_reg_idx) + 
                                  " for virtual register v" + std::to_string(vreg_id) + 
                                  " after spilling v" + std::to_string(spill_vreg));
                    }
                }
            }
        } else {
            if (!free_gpr_registers_.empty()) {
                // Allocate a GPR register
                mapping.type = PhysicalRegisterType::GPR;
                mapping.gpr_physical_reg_idx = free_gpr_registers_.back();
                free_gpr_registers_.pop_back();
                
                LOG_DEBUG("Allocated GPR register x" + std::to_string(mapping.gpr_physical_reg_idx) + 
                          " for virtual register v" + std::to_string(vreg_id));
            } else {
                // Need to spill a register
                if (active_registers_.empty()) {
                    // Shouldn't happen if we have reasonable register allocation
                    LOG_ERROR("No active registers to spill but ran out of GPR registers");
                    
                    // Create a spill location for this register
                    mapping.is_spilled = true;
                    mapping.stack_offset = spill_allocator_.allocate_spill_slot(vreg_id, data_type);
                    
                    LOG_DEBUG("Spilled register v" + std::to_string(vreg_id) + 
                              " to stack offset " + std::to_string(mapping.stack_offset) + 
                              " because no free GPR registers");
                } else {
                    // Find candidates for spilling (must be GPR registers)
                    std::vector<uint32_t> spill_candidates;
                    
                    for (uint32_t active_vreg : active_registers_) {
                        if (register_mappings_[active_vreg].type == PhysicalRegisterType::GPR && 
                            !register_mappings_[active_vreg].is_spilled && 
                            lifetime_map_[active_vreg].end > lifetime.end) {
                            spill_candidates.push_back(active_vreg);
                        }
                    }
                    
                    if (spill_candidates.empty()) {
                        // No GPR registers to spill
                        LOG_ERROR("No GPR registers to spill");
                        
                        // Create a spill location for this register
                        mapping.is_spilled = true;
                        mapping.stack_offset = spill_allocator_.allocate_spill_slot(vreg_id, data_type);
                        
                        LOG_DEBUG("Spilled register v" + std::to_string(vreg_id) + 
                                  " to stack offset " + std::to_string(mapping.stack_offset) + 
                                  " because no GPR registers to spill");
                    } else {
                        // Select a register to spill
                        uint32_t spill_vreg = select_register_to_spill(spill_candidates);
                        
                        // Spill the selected register
                        uint32_t gpr_reg = register_mappings_[spill_vreg].gpr_physical_reg_idx;
                        spill_register(spill_vreg, lifetime_map_[spill_vreg].data_type);
                        
                        // Allocate the freed register to this vreg
                        mapping.type = PhysicalRegisterType::GPR;
                        mapping.gpr_physical_reg_idx = gpr_reg;
                        
                        LOG_DEBUG("Allocated GPR register x" + std::to_string(mapping.gpr_physical_reg_idx) + 
                                  " for virtual register v" + std::to_string(vreg_id) + 
                                  " after spilling v" + std::to_string(spill_vreg));
                    }
                }
            }
        }
        
        // Store the mapping
        register_mappings_[vreg_id] = mapping;
        
        // Add to active list
        active_registers_.push_back(vreg_id);
    }
}

// Main allocate function enhanced for Phase 8
std::unordered_map<uint32_t, RegisterMapping> RegisterAllocator::allocate(const std::vector<ir::IrInstruction>& instructions) {
    register_mappings_.clear();
    spill_allocator_.reset();
    
    // First, analyze register lifetimes
    std::map<uint32_t, RegisterLifetime> register_lifetimes;
    analyze_register_lifetimes(instructions, register_lifetimes);
    
    // Phase 8 enhancement: Perform loop detection for better register allocation
    std::unordered_set<uint32_t> loop_registers;
    detect_loops_and_hot_registers(instructions, loop_registers);
    
    // Prepare usage statistics for priority-based allocation
    std::vector<RegisterUsageStats> usage_stats;
    for (const auto& [vreg_id, lifetime] : register_lifetimes) {
        RegisterUsageStats stats;
        stats.vreg_id = vreg_id;
        stats.use_count = lifetime.uses;
        stats.is_x86_reg_mapped = (vreg_id < 8); // First 8 virtual registers are typically direct x86 mappings
        stats.involved_in_loop = loop_registers.count(vreg_id) > 0;
        usage_stats.push_back(stats);
    }
    
    // Sort registers by priority for allocation
    // Priority order: loop registers > x86 mapped > high use count
    std::sort(usage_stats.begin(), usage_stats.end(), [](const RegisterUsageStats& a, const RegisterUsageStats& b) {
        // First prioritize loop registers
        if (a.involved_in_loop != b.involved_in_loop) {
            return a.involved_in_loop > b.involved_in_loop;
        }
        
        // Then prioritize x86 mapped registers
        if (a.is_x86_reg_mapped != b.is_x86_reg_mapped) {
            return a.is_x86_reg_mapped > b.is_x86_reg_mapped;
        }
        
        // Finally prioritize by usage count
        return a.use_count > b.use_count;
    });
    
    // First pass: Create initial mappings for all registers
    for (const auto& stats : usage_stats) {
        uint32_t vreg_id = stats.vreg_id;
        const RegisterLifetime& lifetime = register_lifetimes[vreg_id];
        
        // Determine register type (GPR or NEON) based on data type
        bool needs_neon = false;
        ir::IrDataType data_type = ir::IrDataType::I32; // Default
        
        if (!lifetime.accesses.empty()) {
            const auto& first_access = lifetime.accesses.front();
            if (first_access.inst_idx < instructions.size()) {
                const auto& instr = instructions[first_access.inst_idx];
                if (!instr.operands.empty() && first_access.operand_idx < instr.operands.size()) {
                    data_type = instr.operands[first_access.operand_idx].data_type;
                    needs_neon = requiresNeonRegister(data_type);
                }
            }
        }
        
        // Create a mapping for each register (initially marked as not spilled)
        RegisterMapping mapping;
        mapping.is_spilled = false;
        mapping.stack_offset = 0;
        mapping.type = needs_neon ? PhysicalRegisterType::NEON : PhysicalRegisterType::GPR;
        mapping.gpr_physical_reg_idx = 0;  // Default values, will be set during allocation
        mapping.neon_physical_reg_idx = 0;
        
        register_mappings_[vreg_id] = mapping;
    }
    
    // Second pass: Allocate physical registers based on priority
    // Reset the free register lists
    free_gpr_registers_.clear();
    free_neon_registers_.clear();
    
    // Initialize free register lists
    for (int i = 0; i < 28; i++) {
        if (i != 16 && i != 17) { // Reserve x16, x17 for platform use
            free_gpr_registers_.push_back(i);
        }
    }
    
    for (int i = 8; i < 32; i++) {
        free_neon_registers_.push_back(i);
    }
    
    // Now allocate physical registers
    for (const auto& stats : usage_stats) {
        uint32_t vreg_id = stats.vreg_id;
        const RegisterLifetime& lifetime = register_lifetimes[vreg_id];
        RegisterMapping& mapping = register_mappings_[vreg_id];
        
        // Determine data type for proper spilling if needed
        ir::IrDataType data_type = ir::IrDataType::I32; // Default
        if (!lifetime.accesses.empty()) {
            const auto& first_access = lifetime.accesses.front();
            if (first_access.inst_idx < instructions.size()) {
                const auto& instr = instructions[first_access.inst_idx];
                if (!instr.operands.empty() && first_access.operand_idx < instr.operands.size()) {
                    data_type = instr.operands[first_access.operand_idx].data_type;
                }
            }
        }
        
        // Attempt register allocation
        if (mapping.type == PhysicalRegisterType::NEON) {
            if (!free_neon_registers_.empty()) {
                int hw_reg = free_neon_registers_.back();
                free_neon_registers_.pop_back();
                
                mapping.neon_physical_reg_idx = hw_reg;
                
                LOG_DEBUG("Allocated NEON register v" + std::to_string(hw_reg) + " for vreg " + std::to_string(vreg_id) +
                         (stats.involved_in_loop ? " (loop register)" : ""));
            } else {
                // Need to spill this register
                mapping.is_spilled = true;
                mapping.stack_offset = spill_allocator_.allocate_spill_slot(vreg_id, data_type);
                
                LOG_DEBUG("Spilled NEON vreg " + std::to_string(vreg_id) + " due to register pressure" +
                         (stats.involved_in_loop ? " (despite being in loop)" : ""));
            }
        } else {
            if (!free_gpr_registers_.empty()) {
                int hw_reg = free_gpr_registers_.back();
                free_gpr_registers_.pop_back();
                
                mapping.gpr_physical_reg_idx = hw_reg;
                
                LOG_DEBUG("Allocated GPR register x" + std::to_string(hw_reg) + " for vreg " + std::to_string(vreg_id) +
                         (stats.involved_in_loop ? " (loop register)" : ""));
            } else {
                // Need to spill this register
                mapping.is_spilled = true;
                mapping.stack_offset = spill_allocator_.allocate_spill_slot(vreg_id, data_type);
                
                LOG_DEBUG("Spilled GPR vreg " + std::to_string(vreg_id) + " due to register pressure" +
                         (stats.involved_in_loop ? " (despite being in loop)" : ""));
            }
        }
    }
    
    return register_mappings_;
}

// New functions for Phase 8

void RegisterAllocator::setup_function_prologue(const std::vector<ir::IrInstruction>& ir_instructions) {
    // Allocate registers first
    allocate(ir_instructions);
    
    // Get the total spill size needed
    int32_t spill_size = spill_allocator_.get_total_spill_size();
    
    // Align to 16 bytes
    spill_size = (spill_size + 15) & ~15;
    
    LOG_DEBUG("Function prologue: spill size = " + std::to_string(spill_size) + " bytes");
}

void RegisterAllocator::generate_function_epilogue() {
    // Nothing specific to do here for now
    LOG_DEBUG("Function epilogue generated");
}

bool RegisterAllocator::is_register_spilled(uint32_t vreg_id) const {
    if (register_mappings_.find(vreg_id) == register_mappings_.end()) {
        return false;
    }
    
    return register_mappings_.at(vreg_id).is_spilled;
}

int32_t RegisterAllocator::get_spill_offset(uint32_t vreg_id) const {
    if (register_mappings_.find(vreg_id) == register_mappings_.end() || 
        !register_mappings_.at(vreg_id).is_spilled) {
        return -1;
    }
    
    return register_mappings_.at(vreg_id).stack_offset;
}

int32_t RegisterAllocator::get_total_spill_size() const {
    // Get the raw spill size
    int32_t spill_size = spill_allocator_.get_total_spill_size();
    
    // Align to 16 bytes
    return (spill_size + 15) & ~15;
}

// New Phase 8 method to detect loops and identify registers used within loops
void RegisterAllocator::detect_loops_and_hot_registers(
    const std::vector<ir::IrInstruction>& instructions,
    std::unordered_set<uint32_t>& loop_registers) {
    
    // Step 1: Find potential backward branches (indicators of loops)
    std::vector<std::pair<size_t, size_t>> potential_loops; // pairs of (target, branch_instruction)
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        const auto& instr = instructions[i];
        
        // Check if this is a branch instruction with an immediate target
        if (isBranchInstruction(instr) && !instr.operands.empty() && 
            instr.operands[0].type == ir::IrOperandType::IMMEDIATE) {
            
            // Get the branch target
            size_t target = static_cast<size_t>(instr.operands[0].imm_value);
            
            // Check if this is a backward branch (target < current)
            if (target < i) {
                potential_loops.push_back({target, i});
                LOG_DEBUG("Detected potential loop: Branch at " + std::to_string(i) + 
                          " to target " + std::to_string(target));
            }
        }
    }
    
    // Step 2: For each detected loop, find registers used within the loop body
    for (const auto& [loop_start, loop_end] : potential_loops) {
        for (size_t i = loop_start; i <= loop_end; ++i) {
            const auto& instr = instructions[i];
            
            // Collect all registers used in this instruction
            for (size_t op_idx = 0; op_idx < instr.operands.size(); ++op_idx) {
                const auto& operand = instr.operands[op_idx];
                if (operand.type == ir::IrOperandType::REGISTER) {
                    loop_registers.insert(operand.reg_idx);
                    LOG_DEBUG("Register " + std::to_string(operand.reg_idx) + 
                             " marked as used in loop");
                }
            }
        }
    }
}

// Analyze register lifetimes for allocation (Phase 8)
void RegisterAllocator::analyze_register_lifetimes(
    const std::vector<ir::IrInstruction>& instructions,
    std::map<uint32_t, RegisterLifetime>& register_lifetimes) {
    
    LOG_DEBUG("Analyzing register lifetimes for allocation");
    
    // Process each instruction
    for (size_t inst_idx = 0; inst_idx < instructions.size(); ++inst_idx) {
        const auto& instr = instructions[inst_idx];
        
        // Process each operand in the instruction
        for (size_t op_idx = 0; op_idx < instr.operands.size(); ++op_idx) {
            const auto& operand = instr.operands[op_idx];
            
            // Only process register operands
            if (operand.type != ir::IrOperandType::REGISTER) {
                continue;
            }
            
            uint32_t vreg_id = operand.reg_idx;
            
            // Create lifetime entry if it doesn't exist
            if (register_lifetimes.find(vreg_id) == register_lifetimes.end()) {
                RegisterLifetime lifetime;
                lifetime.start = inst_idx;
                lifetime.end = inst_idx;
                lifetime.uses = 1;
                register_lifetimes[vreg_id] = lifetime;
            } else {
                // Update existing lifetime
                RegisterLifetime& lifetime = register_lifetimes[vreg_id];
                lifetime.end = inst_idx;  // Extend end point
                lifetime.uses++;          // Increment usage count
                
                // Record this access
                RegisterAccess access;
                access.inst_idx = inst_idx;
                access.operand_idx = op_idx;
                lifetime.accesses.push_back(access);
            }
        }
    }
    
    LOG_DEBUG("Found " + std::to_string(register_lifetimes.size()) + " virtual registers");
}

} // namespace register_allocation
} // namespace xenoarm_jit