/* USER CODE BEGIN Header */
/*
  @file           : main.c
  @brief          : Main program - ColorMFA clone for VW Golf 4
*/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "dma2d.h"
#include "fdcan.h"
#include "quadspi.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9481.h"
#include "img_logo.h"
#include <stdio.h>
#include <stdlib.h>
#include "buttons.h"
#include "ui_graphics.h"
#include "can_dual.h"
#include "kline.h"
#include "settings.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t screen_id;
    uint32_t last_update;
} AppState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_FPS_TARGET      60
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* FDCAN1 RX Callback */

uint8_t qspi_ok = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FMC_Init();
  MX_DMA_Init();
  MX_QUADSPI_Init();
  MX_DMA2D_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

      /* ==================== MPU Configuration ==================== */
      MPU_Region_InitTypeDef MPU_InitStruct = {0};
      HAL_MPU_Disable();

      MPU_InitStruct.Enable = MPU_REGION_ENABLE;
      MPU_InitStruct.Number = MPU_REGION_NUMBER0;
      MPU_InitStruct.BaseAddress = 0x60000000UL;
      MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
      MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
      MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
      MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
      MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE; //  Важно для DMA2D
      MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
      HAL_MPU_ConfigRegion(&MPU_InitStruct);

      MPU_InitStruct.Number = MPU_REGION_NUMBER1;
      MPU_InitStruct.BaseAddress = 0x90000000UL;
      MPU_InitStruct.Size = MPU_REGION_SIZE_16MB;
      MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
      MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
      HAL_MPU_ConfigRegion(&MPU_InitStruct);

      HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
      SCB_EnableDCache();
      SCB_EnableICache();

      Buttons_Init();           //  Инициализация кнопок
      UI_Init();
      ILI9481_Init();
      CAN_Dual_Init();
      Settings_Load();
      KLine_Init();

      // Включение прерываний FDCAN
      HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 1, 0);
      HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
      HAL_NVIC_SetPriority(FDCAN2_IT0_IRQn, 1, 0);
      HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);

      ILI9481_InitBuffer();
      HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);

      HAL_QSPI_Abort(&hqspi);

      QSPI_CommandTypeDef sCmd = {0};
      sCmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCmd.Instruction     = 0x35;
      HAL_QSPI_Command(&hqspi, &sCmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
      ILI9481_Delay(20);

      QSPI_CommandTypeDef sCommand = {0};
      QSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

      sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
      sCommand.Instruction       = 0x6B;
      sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
      sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DataMode          = QSPI_DATA_4_LINES;
      sCommand.DummyCycles       = 8;
      sCommand.NbData            = 0;
      sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

      sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

      if (HAL_QSPI_MemoryMapped(&hqspi, &sCommand, &sMemMappedCfg) == HAL_OK)
      {
          SCB_CleanDCache();
          SCB_InvalidateICache();
          qspi_ok = 1;
      }
      else
      {
          ILI9481_DrawString(10, 60, "QSPI: FAIL", COLOR_RED, COLOR_BLACK, 2);
      }

      ILI9481_Clear(COLOR_BLACK);          // Очищаем RAM
      ILI9481_DrawImage_QSPI(0, 0, 320, 480, LOGO_ADDR);
      ILI9481_FlushBuffer();
      ILI9481_Delay(1500);



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /* Асинхронный запуск K-Line диалога */
	      if (g_kline_dialog_running) {
	          uint8_t k1=0, k2=0;
	          uint8_t res = KLine_KWP1281_Dialog(0x01, &k1, &k2);
	          g_kline_kb1 = k1;
	          g_kline_kb2 = k2;
	          g_kline_dialog = (res < 3) ? 1 : 0;
	          g_kline_ackmode = res;
	          g_kline_dialog_running = 0;
	      }

	      /* ==================== ОБРАБОТЧИКИ ДИАГНОСТИКИ ==================== */
	      /* Чтение блоков измерений */
	      if (g_diag_req_meas) {
	          g_diag_req_meas = 0;
	          KLine_GetMeasGroup(g_diag_addr, g_diag_group, &g_diag_meas);
	      }

	      /* Чтение ошибок */
	      if (g_diag_req_dtc) {
	          g_diag_req_dtc = 0;
	          uint8_t ok = KLine_GetDtcList(g_diag_addr, &g_diag_dtcs);
	          g_diag_dtc_result = ok ? 1 : 2; // 1 = успех, 2 = ошибка/молчит
	          g_diag_dtc_busy = 0;            // Сбрасываем флаг занятости
	      }

	      /* Стирание ошибок */
	      if (g_diag_req_clr) {
	          g_diag_req_clr = 0;
	          g_diag_clr_result = KLine_ClearDtcs(g_diag_addr) ? 1 : 2;
	          g_diag_dtc_busy = 0;
	          if (g_diag_clr_result == 1) {
	              g_diag_dtcs.count   = 0;
	              g_diag_dtc_result   = 0;   // список недействителен -> экран снова предложит "ОК = прочитать"
	          }
	      }
	      /* =============================================================== */

	      /* Update UI logic */
	      UI_Update();

	      /* Render UI */
	      UI_Render();

	      /* Maintain ~60 FPS */
	      HAL_Delay(1000 / APP_FPS_TARGET);
	      Settings_CheckSave();
	      CAN_UpdateGearEstimate();
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
