#ifndef SHARED_LOGIC_H
#define SHARED_LOGIC_H

#include <stdint.h>

/**
 * @brief Shared Memory Address (SRAM4)
 * This region is accessible by both D1 (M7) and D2 (M4) domains.
 */
#define SHARED_RAM_ADDR 0x38000000

/**
 * @brief Command Opcodes for the M4 Worker
 */
typedef enum {
    OP_NOP = 0,
    OP_SET_GPIO = 1, // pin_value: 0 (Low), 1 (High)
    OP_SET_PWM = 2,  // pin_value: Duty Cycle (0-100 or raw CCR)
    OP_WAIT = 3      // duration_ms: Delay in milliseconds
} OpCode_t;

/**
 * @brief A single execution step for the M4 program
 */
typedef struct __attribute__((packed))
{
    uint32_t opcode;      // Use OpCode_t
    uint32_t pin_value;   // Payload for GPIO/PWM
    uint32_t duration_ms; // Delay after this action
} Step_t;

/**
 * @brief The Master Mailbox Structure
 */
typedef struct __attribute__((packed))
{
    uint32_t program_length; // Number of valid steps in the array
    uint32_t run_flag;       // 0: Stop, 1: Run Once, 2: Loop
    uint8_t button_state;    // M7 writes, M4 (or CLI) reads

    /* * Buffer for up to 64 commands.
     * Total size: 64 * 12 bytes = 768 bytes.
     */
    Step_t steps[64];
} SharedMemory_t;

/**
 * Declare the mailbox variable and force it into the .shared_data section.
 * 'extern' is used here so you can define it in a .c file once.
 */
extern SharedMemory_t mailbox;

/**
 * @brief Helper Macro to access the shared memory
 */
#define SHARED_MEM ((SharedMemory_t *)SHARED_RAM_ADDR)

#endif /* SHARED_LOGIC_H */
