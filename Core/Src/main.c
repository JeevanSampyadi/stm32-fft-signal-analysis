/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Round 3 - Fixed FFT Signal Analysis
  * @team           : Team 4
  * @board          : NUCLEO-G071RB
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define ARM_MATH_CM0PLUS          // Required for STM32G0 Cortex-M0+
#include "arm_math.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* FFT Configuration: Task 4 & 5 [cite: 47, 56] */
#define FFT_SIZE        256       // Processing size
#define INPUT_SAMPLES   128       // Data points from CSV
#define SAMPLING_RATE   1000.0f   // 1kHz based on 0.001s CSV time steps

/* Buffers */
float32_t fftInput[FFT_SIZE];
float32_t fftOutput[FFT_SIZE];
float32_t magnitudes[FFT_SIZE/2];

arm_rfft_fast_instance_f32 fftHandler;

/* Results */
float32_t maxEnergy = 0;
uint32_t maxIndex = 0;
float32_t detectedFreq = 0;

/* Task 1: Load Provided Signal Array [cite: 50]
   Extracted from your CSV "Sum" column */
const float32_t providedSignalArray[INPUT_SAMPLES] = {
0.00, 1.71, 2.49, 2.07, 0.95, 0.00, -0.22, 0.17, 0.59, 0.53, 0.00, -0.53, -0.59, -0.17, 0.22, 0.00,
-0.95, -2.07, -2.49, -1.71, 0.00, 1.71, 2.49, 2.07, 0.95, 0.00, -0.22, 0.17, 0.59, 0.53, 0.00, -0.53,
-0.59, -0.17, 0.22, 0.00, -0.95, -2.07, -2.49, -1.71, 0.00, 1.71, 2.49, 2.07, 0.95, 0.00, -0.22, 0.17,
0.59, 0.53, 0.00, -0.53, -0.59, -0.17, 0.22, 0.00, -0.95, -2.07, -2.49, -1.71, 0.00, 1.71, 2.49, 2.07,
0.95, 0.00, -0.22, 0.17, 0.59, 0.53, 0.00, -0.53, -0.59, -0.17, 0.22, 0.00, -0.95, -2.07, -2.49, -1.71,
0.00, 1.71, 2.49, 2.07, 0.95, 0.00, -0.22, 0.17, 0.59, 0.53, 0.00, -0.53, -0.59, -0.17, 0.22, 0.00,
-0.95, -2.07, -2.49, -1.71, 0.00, 1.71, 2.49, 2.07, 0.95, 0.00, -0.22, 0.17, 0.59, 0.53, 0.00, -0.53,
-0.59, -0.17, 0.22, 0.00, -0.95, -2.07, -2.49, -1.71, 0.00, 1.71, 2.49, 2.07, 0.95, 0.00, -0.22, 0.17
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void Process_FFT(void);

/* USER CODE BEGIN PFP */
/* Task 5: UART Output Reporting [cite: 57, 58] */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
/* USER CODE END PFP */

/**
  * @brief  Main program entry point
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* Initialize Peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  printf("\r\n--- Nucleo Signal Quest Round 3 ---\r\n");

  /* Task 2: Initialize CMSIS-DSP FFT [cite: 51] */
  arm_rfft_fast_init_f32(&fftHandler, FFT_SIZE);

  /* Task 1: Prepare Input Signal with Zero Padding [cite: 50] */
  memset(fftInput, 0, sizeof(fftInput)); // Clear buffer
  for (int i = 0; i < INPUT_SAMPLES; i++) {
      fftInput[i] = providedSignalArray[i]; // Load CSV data
  }

  /* Execute Analysis Once */
  Process_FFT();
  /* USER CODE END 2 */

  while (1)
  {
    /* Task 5: Report dominant frequency via UART [cite: 54, 57] */


    /* Optional: Visual Feedback [cite: 59] */
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

    // Blink delay varies with frequency
    uint32_t delay = (detectedFreq > 0) ? (uint32_t)(10000 / detectedFreq) : 1000;
    HAL_Delay(delay > 2000 ? 2000 : delay);
  }
}

/**
  * @brief  Task 2, 3 & 4: FFT Analysis Logic [cite: 51, 53, 54]
  */
void Process_FFT(void)
{
    /* Perform FFT */
    arm_rfft_fast_f32(&fftHandler, fftInput, fftOutput, 0);

    /* Compute Magnitude Spectrum */
    for (uint32_t i = 0; i < FFT_SIZE / 2; i++) {
        float32_t real = fftOutput[2*i];
        float32_t imag = fftOutput[2*i + 1];
        magnitudes[i] = sqrtf(real * real + imag * imag);
    }

    /* Remove DC component */
    magnitudes[0] = 0;

    /* Find dominant frequency bin */
    arm_max_f32(magnitudes, FFT_SIZE/2, &maxEnergy, &maxIndex);
    detectedFreq = (float32_t)maxIndex * (SAMPLING_RATE / (float32_t)FFT_SIZE);

    /* --- ONE-TIME DETAILED REPORT --- */
    printf("\r\n==========================================\r\n");
    printf("        SIGNAL ANALYSIS COMPLETE         \r\n");
    printf("==========================================\r\n");
    printf(" Bin | Frequency (Hz) | Magnitude\r\n");
    printf("------------------------------------------\r\n");

    for (uint32_t i = 1; i < FFT_SIZE / 2; i++) {
        // Only print peaks above 5% of the maximum energy to filter noise
        if (magnitudes[i] > (maxEnergy * 0.05f)) {
            float32_t binFreq = (float32_t)i * (SAMPLING_RATE / (float32_t)FFT_SIZE);
            printf(" %3lu | %10.2f Hz | %10.2f %s\r\n",
                   i, binFreq, magnitudes[i],
                   (i == maxIndex) ? "[DOMINANT]" : "");
        }
    }

    printf("------------------------------------------\r\n");
    printf(" FINAL RESULT: %.2f Hz detected.\r\n", detectedFreq);
    printf("==========================================\r\n\r\n");
}

/**
  * @brief USART2 Initialization (115200, 8N1) [cite: 58]
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USART2 TX/RX on PA2/PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* User LED on PA5 [cite: 30] */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

void Error_Handler(void) { __disable_irq(); while (1) {} }

