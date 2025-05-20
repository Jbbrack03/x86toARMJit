#ifndef XENOARM_JIT_EFLAGS_STATE_H
#define XENOARM_JIT_EFLAGS_STATE_H

#include <cstdint>

// Bit positions for EFLAGS
#define EFLAGS_CF      0x00000001  // Carry Flag
#define EFLAGS_PF      0x00000004  // Parity Flag
#define EFLAGS_AF      0x00000010  // Auxiliary Flag
#define EFLAGS_ZF      0x00000040  // Zero Flag
#define EFLAGS_SF      0x00000080  // Sign Flag
#define EFLAGS_TF      0x00000100  // Trap Flag
#define EFLAGS_IF      0x00000200  // Interrupt Enable Flag
#define EFLAGS_DF      0x00000400  // Direction Flag
#define EFLAGS_OF      0x00000800  // Overflow Flag
#define EFLAGS_IOPL    0x00003000  // I/O Privilege Level
#define EFLAGS_NT      0x00004000  // Nested Task
#define EFLAGS_RF      0x00010000  // Resume Flag
#define EFLAGS_VM      0x00020000  // Virtual 8086 Mode
#define EFLAGS_AC      0x00040000  // Alignment Check
#define EFLAGS_VIF     0x00080000  // Virtual Interrupt Flag
#define EFLAGS_VIP     0x00100000  // Virtual Interrupt Pending
#define EFLAGS_ID      0x00200000  // ID Flag

// For arithmetic operations
#define EFLAGS_ARITH_MASK (EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF)
// For logical operations (doesn't affect CF or OF, clears them)
#define EFLAGS_LOGIC_MASK (EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF)
// For shift operations
#define EFLAGS_SHIFT_MASK (EFLAGS_CF | EFLAGS_PF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF)
// For compare operations (like logical but doesn't modify the flags)
#define EFLAGS_CMP_MASK   (EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_OF)

// Class to track EFLAGS state
class EFlagsState {
public:
    EFlagsState();
    ~EFlagsState();

    // Get and set EFLAGS
    uint32_t getEflags() const;
    void setEflags(uint32_t eflags);

    // Get and set individual flags
    bool getFlag(uint32_t flag) const;
    void setFlag(uint32_t flag, bool value);

    // Update flags after arithmetic operations
    void updateAfterAdd(uint32_t result, uint32_t operand1, uint32_t operand2);
    void updateAfterSub(uint32_t result, uint32_t operand1, uint32_t operand2);
    void updateAfterLogic(uint32_t result);
    void updateAfterShift(uint32_t result, uint32_t original, int count, bool left);

private:
    uint32_t eflags_;  // Current EFLAGS value
};

#endif // XENOARM_JIT_EFLAGS_STATE_H 