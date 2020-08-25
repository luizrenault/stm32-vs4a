/**
******************************************************************************
* @file    platform_init.h
* @author  MCD Application Team
* @version V1.0.0
* @date    04-Aout-2017
* @brief   Board HW init
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
*
* This software component is licensed by ST under Ultimate Liberty license
* SLA0044, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0044
*
******************************************************************************
*/
#ifndef __Platform_Init__
#define __Platform_Init__
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"
//#include "stm32f769i_discovery.h"
//#include "stm32f769i_discovery_lcd.h"
//#include "stm32f769i_discovery_ts.h"
//#include "stm32f769i_discovery_qspi.h"
#include "stm32f7xx_hal_def.h"
#include "stm32f7xx_hal_flash.h"

typedef void (*uart_isr_t)(void);
void                 platform_Init(void);
uint32_t             platform_uart_add_weak_cb(uint32_t type,   void (*cb)(UART_HandleTypeDef *UartHandle));
void                 platform_uart_clear_weak_cb(void);
void                 platform_uart_config(uint32_t baudrate);
void                 platform_uart_term(void);
void                 platform_uart_set_isr(uint32_t num, uart_isr_t isr);
UART_HandleTypeDef*  platform_uart_get_console(void);
void                 platform_config_persistent_storage(uint32_t Offset,uint32_t Sector,uint32_t SectorSize);



void      LCD_Swap_buffer_Init(void);
uint32_t  LCD_SwapBuffer(void);
void      LCD_WaitVSync(void);
void      LCD_Vsync_Init(void);


#endif
