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
/* USER CODE BEGIN PD */
#ifndef B1_Pin
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#endif
/* USER CODE END PD */
osThreadId_t cliTaskHandle;
const osThreadAttr_t cliTask_attributes = {
    .name = "cliTask",
    .stack_size = 512 * 4, // CLI needs a bit more stack for string processing
    .priority = (osPriority_t)osPriorityNormal,
};
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
/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void);

/**
 * @brief  FreeRTOS initialization
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
    static char pcInputString[128];
    int8_t cInputIndex = 0;
    char *pcOutputString;

    pcOutputString = FreeRTOS_CLIGetOutputBuffer();

    printf("\r\n--- M7 CLI Ready ---\r\n> ");

    for (;;) {
        // Poll USART3 (VCOM) for input
        if (HAL_UART_Receive(&huart3, (uint8_t *)&cRxChar, 1, portMAX_DELAY) == HAL_OK) {

            if (cRxChar == '\r' || cRxChar == '\n') {
                printf("\r\n");
                pcInputString[cInputIndex] = '\0';

                if (strlen(pcInputString) > 0) {
                    BaseType_t xMore;
                    do {
                        xMore = FreeRTOS_CLIProcessCommand(pcInputString, pcOutputString,
                                                           configCOMMAND_INT_MAX_OUTPUT_SIZE);
                        printf("%s", pcOutputString);
                    } while (xMore != pdFALSE);
                }

                printf("\r\n> ");
                cInputIndex = 0;
                memset(pcInputString, 0, sizeof(pcInputString));
            } else if (cRxChar == '\b' || cRxChar == 127) { // Backspace
                if (cInputIndex > 0) {
                    cInputIndex--;
                    printf("\b \b");
                }
            } else {
                if (cInputIndex < 127) {
                    printf("%c", cRxChar);
                    pcInputString[cInputIndex++] = cRxChar;
                }
            }
        }

        // Update shared button state for M4 visibility
        // Force use of PC13 (Blue Button) regardless of CubeMX labeling
        osDelay(100); // Poll every 100ms
        SHARED_MEM->button_state = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET);
    }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void RegisterCLICommands(void)
{
    static const CLI_Command_Definition_t xPeek = {"peek", "peek: Show shared memory status\r\n",
                                                   prvPeekCommand, 0};
    static const CLI_Command_Definition_t xAdd = {
        "add", "add <op> <val> <ms>: Add step to M4 program\r\n", prvAddStepCommand, 3};
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
    snprintf(pcWriteBuffer, xWriteBufferLen, "M4 Status: %lu | Steps: %lu | Button: %d\r\n",
             SHARED_MEM->run_flag, SHARED_MEM->program_length, SHARED_MEM->button_state);
    return pdFALSE;
}

BaseType_t prvAddStepCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString)
{
    const char *p1, *p2, *p3;
    BaseType_t l1, l2, l3;

    p1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &l1);
    p2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &l2);
    p3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &l3);

    if (SHARED_MEM->program_length < 64) {
        uint32_t idx = SHARED_MEM->program_length;
        SHARED_MEM->steps[idx].opcode = (uint32_t)atoi(p1);
        SHARED_MEM->steps[idx].pin_value = (uint32_t)atoi(p2);
        SHARED_MEM->steps[idx].duration_ms = (uint32_t)atoi(p3);
        SHARED_MEM->program_length++;
        snprintf(pcWriteBuffer, xWriteBufferLen, "Added Step %lu [Op:%s Val:%s Ms:%s]\r\n", idx, p1,
                 p2, p3);
    } else {
        strncpy(pcWriteBuffer, "Error: M4 Buffer Full\r\n", xWriteBufferLen);
    }
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

BaseType_t prvClearCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    SHARED_MEM->run_flag = 0;
    SHARED_MEM->program_length = 0;
    memset(SHARED_MEM->steps, 0, sizeof(SHARED_MEM->steps));
    strncpy(pcWriteBuffer, "Program cleared.\r\n", xWriteBufferLen);
    return pdFALSE;
}

/* USER CODE END Application */
