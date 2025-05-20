#ifndef XENOARM_JIT_REGISTER_ALLOCATION_REGISTER_ALLOCATOR_H
#define XENOARM_JIT_REGISTER_ALLOCATION_REGISTER_ALLOCATOR_H

#include "xenoarm_jit/ir.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>

namespace xenoarm_jit {
namespace register_allocation {

// Structure to represent a register access in an instruction (Phase 8)
struct RegisterAccess {
    size_t inst_idx;      // Instruction index
    size_t operand_idx;   // Operand index within the instruction
};

// Structure to represent the lifetime of a register (Phase 8)
struct RegisterLifetime {
    uint32_t start;       // First instruction where register is used
    uint32_t end;         // Last instruction where register is used
    uint32_t uses;        // Number of times the register is used
    std::vector<RegisterAccess> accesses;  // List of all accesses to this register
};

// Represents a mapping from IR virtual register to AArch64 physical register
// Define the types of physical registers
enum class PhysicalRegisterType {
    GPR,  // General-Purpose Register (W/X registers)
    NEON  // NEON/SIMD Register (V registers)
};

// Represents a mapping from IR virtual register to AArch64 physical register
struct RegisterMapping {
    bool is_spilled;                        // Whether this register is spilled to memory
    PhysicalRegisterType type;              // Type of physical register (GPR or NEON)
    int gpr_physical_reg_idx;               // Physical GPR register number (if type is GPR)
    int neon_physical_reg_idx;              // Physical NEON register number (if type is NEON)
    int32_t stack_offset;                   // Offset from the stack pointer if spilled
};

// Structure to represent the lifetime of a virtual register (Phase 8)
struct VRegLifetime {
    uint32_t vreg_id;           // Virtual register ID
    ir::IrDataType data_type;   // Data type of the register
    uint32_t start;             // First instruction index where this register is used
    uint32_t end;               // Last instruction index where this register is used
    uint32_t use_count;         // How many times the register is used
    bool is_active;             // Whether this register is currently active in the allocation
    bool is_loop_register;      // Whether this register is used in a loop (Phase 8)
    bool is_x86_mapped;         // Whether this is a direct mapping of an x86 register
    float priority;             // Priority score for allocation (higher = more important) (Phase 8)
};

// Class for managing spill locations (Phase 8)
class SpillAllocator {
public:
    SpillAllocator();
    
    // Allocate a new spill slot for a register
    int32_t allocate_spill_slot(uint32_t vreg_id, ir::IrDataType data_type);
    
    // Free a spill slot
    void free_spill_slot(int32_t offset);
    
    // Get the total size of spill area needed
    int32_t get_total_spill_size() const;
    
    // Reset the spill allocator for a new function
    void reset();
    
private:
    // Map from stack offset to size of the allocation
    std::map<int32_t, uint32_t> allocated_slots_;
    int32_t current_offset_;
};

class RegisterAllocator {
public:
    RegisterAllocator();
    ~RegisterAllocator();

    // Performs register allocation for a sequence of IR instructions
    // Returns a mapping of IR virtual registers to AArch64 physical registers
    std::unordered_map<uint32_t, RegisterMapping> allocate(const std::vector<ir::IrInstruction>& ir_instructions);

    // Sets up initial ARM registers for function prologue (Phase 8)
    void setup_function_prologue(const std::vector<ir::IrInstruction>& ir_instructions);

    // Generate function epilogue code (Phase 8)
    void generate_function_epilogue();

    // Get information about a spill (Phase 8)
    bool is_register_spilled(uint32_t vreg_id) const;
    int32_t get_spill_offset(uint32_t vreg_id) const;
    int32_t get_total_spill_size() const;

private:
    // Helper method to analyze register lifetimes (Phase 8)
    void analyze_register_lifetimes(
        const std::vector<ir::IrInstruction>& instructions,
        std::map<uint32_t, RegisterLifetime>& register_lifetimes);
    
    // Helper method to compute virtual register lifetimes (Phase 8)
    std::vector<VRegLifetime> compute_lifetimes(const std::vector<ir::IrInstruction>& ir_instructions);
    
    // Helper method to compute register priorities based on usage patterns (Phase 8)
    void compute_register_priorities(std::vector<VRegLifetime>& lifetimes);
    
    // Helper method to detect loops in the instruction stream (Phase 8)
    void detect_loops(const std::vector<ir::IrInstruction>& ir_instructions, std::vector<VRegLifetime>& lifetimes);
    
    // Helper method to detect loops and identify hot registers (Phase 8 enhancement)
    void detect_loops_and_hot_registers(
        const std::vector<ir::IrInstruction>& instructions,
        std::unordered_set<uint32_t>& loop_registers);
    
    // Linear scan register allocation implementation (Phase 8)
    void linear_scan_register_allocation(const std::vector<VRegLifetime>& lifetimes);
    
    // Helper method to spill a register (Phase 8)
    void spill_register(uint32_t vreg_id, ir::IrDataType data_type);
    
    // Helper method to choose a register to spill based on priority (Phase 8)
    uint32_t select_register_to_spill(const std::vector<uint32_t>& candidates);
    
    // Maps from virtual register ID to physical register mapping
    std::unordered_map<uint32_t, RegisterMapping> register_mappings_;
    
    // List of available physical GPR registers (initialized in constructor)
    std::vector<uint32_t> free_gpr_registers_;
    
    // List of available NEON registers (initialized in constructor)
    std::vector<uint32_t> free_neon_registers_;
    
    // List of active virtual registers during allocation
    std::vector<uint32_t> active_registers_;
    
    // Spill allocator for managing stack slots (Phase 8)
    SpillAllocator spill_allocator_;
    
    // Map from vreg ID to its lifetime
    std::unordered_map<uint32_t, VRegLifetime> lifetime_map_;
};

} // namespace register_allocation
} // namespace xenoarm_jit

#endif // XENOARM_JIT_REGISTER_ALLOCATION_REGISTER_ALLOCATOR_H