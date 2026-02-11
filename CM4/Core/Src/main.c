/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body (Cortex-M4 Worker)
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"
#include "tim.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "shared_logic.h"
#include <stdint.h>
/* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    /* Note: M7 handles the main PLLs; M4 syncs to the bus clocks. */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM1_Init();

    /* USER CODE BEGIN 2 */
    // 1. Start PWM on Timer 1
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    // 2. Clear any local states and ensure we start with LED OFF
    HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, GPIO_PIN_RESET);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* 1. Heartbeat & Button Polling
         * We do this every loop iteration so the M7 'peek' command stays updated.
         */
        SHARED_MEM->m4_heartbeat++;

        // PC13 is the Blue Button on most H7 Nucleos
        SHARED_MEM->button_state = (uint8_t)HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

        /* 2. Check Mailbox for Run Flag */
        if (SHARED_MEM->run_flag > 0) {
            // Iterate through the program steps provided by M7
            for (uint32_t i = 0; i < SHARED_MEM->program_length; i++) {
                // Check for "Emergency Stop" - if M7 clears run_flag mid-sequence, exit immediately
                if (SHARED_MEM->run_flag == 0)
                    break;

                // Use a local copy for execution to avoid multiple volatile reads in the switch
                Step_t step = SHARED_MEM->steps[i];

                switch (step.opcode) {
                case OP_SET_GPIO: // Opcode 1
                    HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin,
                                      (step.pin_value > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    break;

                case OP_SET_PWM: // Opcode 2
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, step.pin_value);
                    break;

                case OP_WAIT: // Opcode 3
                    HAL_Delay(step.duration_ms);
                    break;

                default:
                    // Unknown opcode, do nothing
                    break;
                }
            }

            /* 3. Handle Run Modes */
            if (SHARED_MEM->run_flag == 1) {
                // "Run Once" mode: Clear the flag after the sequence finishes
                SHARED_MEM->run_flag = 0;
            }
            // If run_flag == 2 ("Loop"), we don't clear it, so the 'while' starts the 'for' again.
        } else {
            /* 4. Idle State
             * If nothing is running, we wait for an interrupt (like SysTick) to save power
             * and reduce bus contention with the M7.
             */
            __WFI();
        }
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    /* * On H7 dual-core, the M7 core usually configures the RCC and PLLs.
     * If CubeMX generated code here, keep it; otherwise, this can remain empty
     * as the M4 will inherit the clock settings from the D2 domain.
     */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}
