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
#include "shared_logic.h" // Shared memory mailbox
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
    /* Note: M7 usually handles the main system clock config; M4 just syncs. */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM1_Init();

    /* USER CODE BEGIN 2 */

    // 1. Start PWM on Timer 1 (M4 Owned)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    // 2. DEFAULT STATE: Turn on Yellow LED to signal successful M4 boot
    HAL_GPIO_WritePin(yellow_led_GPIO_Port, yellow_led_Pin, GPIO_PIN_SET);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        // The "Hard Loop": Checking the mailbox in SRAM4
        if (SHARED_MEM->run_flag > 0) {
            for (uint32_t i = 0; i < SHARED_MEM->program_length; i++) {
                Step_t step = SHARED_MEM->steps[i];

                switch (step.opcode) {
                case OP_SET_GPIO:
                    // Using your "green_led" user label
                    HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin,
                                      (step.pin_value > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    break;

                case OP_SET_PWM:
                    // Update TIM1 Duty Cycle (0-65535 based on your MX config)
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, step.pin_value);
                    break;

                case OP_WAIT:
                    // Deterministic delay using the M4's TIM4-based HAL_Delay
                    HAL_Delay(step.duration_ms);
                    break;

                default:
                    break;
                }

                // High-priority exit: If M7 clears the flag mid-sequence
                if (SHARED_MEM->run_flag == 0)
                    break;
            }

            // Mode 1: Run Once. Mode 2: Loop (don't clear flag)
            if (SHARED_MEM->run_flag == 1) {
                SHARED_MEM->run_flag = 0;
            }
        } else {
            // Idle state: keep jitter low by not doing much
            __WFI(); // Wait For Interrupt (saves power until a timer tick occurs)
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
    /* Clock configuration is typically inherited from the M7 Core */
    /* If CubeMX generated code here, keep it to ensure M4-side peripherals have correct bus clocks
     */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

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
