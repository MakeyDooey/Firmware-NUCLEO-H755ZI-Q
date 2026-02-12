#ifndef SHARED_LOGIC_H
#define SHARED_LOGIC_H

#include <stdint.h>

/**
 * @brief Shared Memory Address (SRAM4)
 * This region is accessible by both D1 (M7) and D2 (M4) domains.
 */
#define SHARED_RAM_ADDR 0x38000000

/* Bitmask flags for step.pin_value */
#define FLAG_GPIO_TYPE (1 << 0) // Bit 0
#define FLAG_LED_TYPE (1 << 1)  // Bit 1

// Actions (Mutually exclusive for a single step)
#define FLAG_ACTION_ON (1 << 2)  // Bit 2
#define FLAG_ACTION_OFF (1 << 3) // Bit 3
#define FLAG_ACTION_TOG (1 << 4) // Bit 4

// LED Targets (Can be combined)
#define MASK_RED (1 << 5)    // Bit 5
#define MASK_YELLOW (1 << 6) // Bit 6
#define MASK_GREEN (1 << 7)  // Bit 7
/**
 * @brief Command Opcodes for the M4 Worker
 */
typedef enum {
    OP_NOP = 0,
    OP_LED_CONTROL = 1, // pin_value: 0 (Low), 1 (High)
    OP_SET_PWM = 2,     // pin_value: Duty Cycle (0-100 or raw CCR)
} OpCode_t;

/**
 * @brief A single execution step for the M4 program
 */
typedef struct __attribute__((packed))
{
    uint32_t opcode;    // Use OpCode_t
    uint32_t pin_value; // Payload for GPIO/PWM
    //    uint32_t duration_ms; // Delay after this action
} Step_t;

/**
 * @brief The Master Mailbox Structure
 */
typedef struct __attribute__((packed))
{
    volatile uint32_t program_length; // Number of valid steps in the array
    volatile uint32_t run_flag;       // 0: Stop, 1: Run Once, 2: Loop
    volatile uint32_t button_state;
    volatile uint32_t m4_heartbeat;

    /* * Buffer for up to 64 commands.
     * Total size: 64 * 12 bytes = 768 bytes.
     */
    volatile Step_t steps[64];
} SharedMemory_t;

/**
 * @brief Helper Macro to access the shared memory
 */
#define SHARED_MEM ((SharedMemory_t *)SHARED_RAM_ADDR)

#endif /* SHARED_LOGIC_H */
