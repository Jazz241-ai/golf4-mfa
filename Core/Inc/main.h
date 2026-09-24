/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* ============================================================================
   QSPI Flash Memory Map
   ============================================================================ */
extern uint8_t qspi_ok;
#define QSPI_PTR(addr)      ((volatile const uint8_t*)(addr))
/* ============================================================================
   QSPI Flash Memory Map - ISOCPEUR Fonts
   ============================================================================ */


#define FONT_16_ADDR            0x90000000U
#define FONT_20_ADDR            0x900017C0U
#define FONT_22_ADDR            0x900033A0U
#define FONT_24_ADDR            0x900053B0U
#define FONT_40_ADDR            0x900085D0U
#define FONT_60_ADDR            0x9000EF90U
#define FONT_70_ADDR            0x9001D940U
#define FONT_100_ADDR           0x90031A90U
#define FONT_120_ADDR           0x900560A0U


#define FONT_16_CHAR_WIDTH      13
#define FONT_16_CHAR_HEIGHT     17
#define FONT_16_BYTES_PER_CHAR  34

#define FONT_20_CHAR_WIDTH      16
#define FONT_20_CHAR_HEIGHT     20
#define FONT_20_BYTES_PER_CHAR  40

#define FONT_22_CHAR_WIDTH      16
#define FONT_22_CHAR_HEIGHT     23
#define FONT_22_BYTES_PER_CHAR  46

#define FONT_24_CHAR_WIDTH      17
#define FONT_24_CHAR_HEIGHT     24
#define FONT_24_BYTES_PER_CHAR  72

#define FONT_40_CHAR_WIDTH      29
#define FONT_40_CHAR_HEIGHT     38
#define FONT_40_BYTES_PER_CHAR  152

#define FONT_60_CHAR_WIDTH      43
#define FONT_60_CHAR_HEIGHT     56
#define FONT_60_BYTES_PER_CHAR  336

#define FONT_70_CHAR_WIDTH      49
#define FONT_70_CHAR_HEIGHT     66
#define FONT_70_BYTES_PER_CHAR  462

#define FONT_100_CHAR_WIDTH      70
#define FONT_100_CHAR_HEIGHT     93
#define FONT_100_BYTES_PER_CHAR  837

#define FONT_120_CHAR_WIDTH      83
#define FONT_120_CHAR_HEIGHT     111
#define FONT_120_BYTES_PER_CHAR  1221

#define LOGO_ADDR               0x9008B1B0U
#define LOGO_BG1                0x900D61C0U
#define LOGO_BG2                0x901211D0U
#define LOGO_BG3                0x9016C1E0U
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
