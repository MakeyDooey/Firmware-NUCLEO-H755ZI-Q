#include "FreeRTOS_CLI.h"
#include "shared_logic.h"
#include <stdio.h>
#include <string.h>

// Command: peek
// Usage: Shows current shared memory status
BaseType_t prvPeekCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    snprintf(pcWriteBuffer, xWriteBufferLen, "M4 Status: %s | Length: %lu | Button: %d\r\n",
             (SHARED_MEM->run_flag > 0) ? "RUNNING" : "STOPPED", SHARED_MEM->program_length,
             SHARED_MEM->button_state);
    return pdFALSE;
}

// Command: add <opcode> <val> <ms>
// Usage: add 1 1 500 (Sets GPIO high for 500ms)
BaseType_t prvAddStepCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString)
{
    const char *p1, *p2;
    BaseType_t l1, l2;

    p1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &l1);
    p2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &l2);

    if (SHARED_MEM->program_length < 64) {
        uint32_t idx = SHARED_MEM->program_length;
        SHARED_MEM->steps[idx].opcode = atoi(p1);
        SHARED_MEM->steps[idx].pin_value = atoi(p2);
        SHARED_MEM->program_length++;

        snprintf(pcWriteBuffer, xWriteBufferLen, "Step %lu added.\r\n", idx);
    } else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Buffer full!\r\n");
    }
    return pdFALSE;
}

// Command: run <mode>
// Usage: run 1 (Run once), run 2 (Looping), run 0 (Stop)
BaseType_t prvRunCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *p1;
    BaseType_t l1;
    p1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &l1);

    SHARED_MEM->run_flag = atoi(p1);
    snprintf(pcWriteBuffer, xWriteBufferLen, "M4 Run Flag set to %d\r\n",
             (int)SHARED_MEM->run_flag);
    return pdFALSE;
}
