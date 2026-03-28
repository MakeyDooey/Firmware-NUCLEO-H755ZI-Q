/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : FreeRTOS CLI Manager for CM7
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS_CLI.h"
#include "shared_logic.h"
#include "usart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId_t cliTaskHandle;
const osThreadAttr_t cliTask_attributes = {
    .name = "cliTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/* Internal Buffer for CLI */
static char pcInputString[128];
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartCLITask(void *argument);
void RegisterCLICommands(void);

// CLI Command implementations
BaseType_t prvPeekCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
BaseType_t prvAddStepCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString);
BaseType_t prvRunCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
BaseType_t prvClearCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                           const char *pcCommandString);

/* Helper for direct UART string output */
void CLI_UART_PutString(const char *s)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)s, strlen(s), 100);
}
/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void);

/**
 * @brief FreeRTOS initialization
 */
void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN Init */
    RegisterCLICommands();
    /* USER CODE END Init */

    /* Create the thread(s) */
    cliTaskHandle = osThreadNew(StartCLITask, NULL, &cliTask_attributes);
}

/* USER CODE BEGIN Header_StartCLITask */
/**
 * @brief Function implementing the cliTask thread.
 */
/* USER CODE END Header_StartCLITask */
void StartCLITask(void *argument)
{
    char cRxChar;
    int8_t cInputIndex = 0;
    char *pcOutputString;
    char *pcCommandToken;
    const char *pcDelimiter = ";";

    pcOutputString = FreeRTOS_CLIGetOutputBuffer();

    /* Clear screen and show welcome */
    CLI_UART_PutString("\033[2J\033[H"); // ANSI Clear Screen
    CLI_UART_PutString(" --- READY (Semicolon Support Active) --- \r\n");
    CLI_UART_PutString("> ");

    for (;;) {
        if (HAL_UART_Receive(&huart3, (uint8_t *)&cRxChar, 1, 10) == HAL_OK) {

            /* 1. Handle Carriage Return or Line Feed */
            if (cRxChar == '\r' || cRxChar == '\n') {
                CLI_UART_PutString("\r\n");
                pcInputString[cInputIndex] = '\0';

                if (cInputIndex > 0) {
                    /* --- MULTI-COMMAND LOGIC START --- */
                    // Get the first command before a semicolon
                    pcCommandToken = strtok(pcInputString, pcDelimiter);

                    while (pcCommandToken != NULL) {
                        BaseType_t xMore;

                        // Optional: Trim leading space if user typed "cmd1; cmd2"
                        while (*pcCommandToken == ' ')
                            pcCommandToken++;

                        if (strlen(pcCommandToken) > 0) {
                            do {
                                /* Process the individual command token */
                                xMore =
                                    FreeRTOS_CLIProcessCommand(pcCommandToken, pcOutputString,
                                                               configCOMMAND_INT_MAX_OUTPUT_SIZE);
                                /* Output the result */
                                CLI_UART_PutString(pcOutputString);
                            } while (xMore != pdFALSE);
                        }

                        // Get the next command in the sequence
                        pcCommandToken = strtok(NULL, pcDelimiter);
                    }
                    /* --- MULTI-COMMAND LOGIC END --- */
                }

                CLI_UART_PutString("\r\n> ");
                cInputIndex = 0;
                memset(pcInputString, 0, sizeof(pcInputString));
            }
            /* 2. Handle Backspace (ASCII 8 or 127) */
            else if (cRxChar == '\b' || cRxChar == 127) {
                if (cInputIndex > 0) {
                    cInputIndex--;
                    CLI_UART_PutString("\b \b");
                }
            }
            /* 3. Handle Normal Characters */
            else {
                if (cInputIndex < (sizeof(pcInputString) - 1)) {
                    HAL_UART_Transmit(&huart3, (uint8_t *)&cRxChar, 1, 10);
                    pcInputString[cInputIndex++] = cRxChar;
                }
            }
        }
        osDelay(1);
    }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void RegisterCLICommands(void)
{
    static const CLI_Command_Definition_t xPeek = {"peek", "peek: Show shared memory status\r\n",
                                                   prvPeekCommand, 0};
    static const CLI_Command_Definition_t xAdd = {"add", "add <op> <val> : Add step to M4\r\n",
                                                  prvAddStepCommand, 2};
    static const CLI_Command_Definition_t xRun = {"run", "run <mode>: 0=Stop, 1=Once, 2=Loop\r\n",
                                                  prvRunCommand, 1};
    static const CLI_Command_Definition_t xClear = {"clear", "clear: Clear the M4 program\r\n",
                                                    prvClearCommand, 0};

    FreeRTOS_CLIRegisterCommand(&xPeek);
    FreeRTOS_CLIRegisterCommand(&xAdd);
    FreeRTOS_CLIRegisterCommand(&xRun);
    FreeRTOS_CLIRegisterCommand(&xClear);
}

BaseType_t prvPeekCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    // Added Heartbeat to the output string
    snprintf(pcWriteBuffer, xWriteBufferLen,
             "M4 Status: %lu | Steps: %lu | Heartbeat: %lu | Button: %ld\r\n", SHARED_MEM->run_flag,
             SHARED_MEM->program_length,
             SHARED_MEM->m4_heartbeat, // <--- Add this
             SHARED_MEM->button_state);
    return pdFALSE;
}

BaseType_t prvAddStepCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString)
{
    const char *p1, *p2;
    BaseType_t l1, l2;

    p1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &l1);
    p2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &l2);

    uint32_t current_idx = SHARED_MEM->program_length; // Use a clear local name

    if (current_idx < 64) {
        SHARED_MEM->steps[current_idx].opcode = (uint32_t)atoi(p1);
        SHARED_MEM->steps[current_idx].pin_value = (uint32_t)atoi(p2);
        SHARED_MEM->program_length++;
        snprintf(pcWriteBuffer, xWriteBufferLen, "Added Step %lu\r\n", current_idx);
    } else {
        strncpy(pcWriteBuffer, "Error: Full\r\n", xWriteBufferLen);
    }
    return pdFALSE;
}

// And fix the memset warning in prvClearCommand:
BaseType_t prvClearCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    SHARED_MEM->run_flag = 0;
    SHARED_MEM->program_length = 0;
    // Cast to (void*) to ignore volatile for the duration of the wipe
    memset((void *)SHARED_MEM->steps, 0, sizeof(SHARED_MEM->steps));
    strncpy(pcWriteBuffer, "Cleared.\r\n", xWriteBufferLen);
    return pdFALSE;
}

BaseType_t prvRunCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *p1;
    BaseType_t l1;
    p1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &l1);
    SHARED_MEM->run_flag = (uint32_t)atoi(p1);
    snprintf(pcWriteBuffer, xWriteBufferLen, "M4 Run Flag -> %lu\r\n", SHARED_MEM->run_flag);
    return pdFALSE;
}

/* USER CODE END Application */
