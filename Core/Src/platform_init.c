/**
******************************************************************************
* @file    platform_init.c
* @author  MCD Application Team
* @version V1.0.0
* @date    04-Aout-2017
* @brief   Board HW initialization
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

#include "platform_init.h"
#include "cmsis_os.h"
#include "avs.h"


#define LTCD_LINE                   0


UART_HandleTypeDef UART_Handle;
static xSemaphoreHandle     vSyncEvent;
static uint32_t             cptFlip;
static uint32_t             gScreen[2];
extern  LTDC_HandleTypeDef hltdc_discovery;
extern  DSI_HandleTypeDef  hdsi_discovery;
static void     console_uart(void);


typedef enum
{
  UART_RXCPL,
  UART_TXCPL,
  UART_ERROR
}t_uart_weak_type;
typedef struct t_uart_weak_cb
{
  uint32_t type;
  void (*cb)(UART_HandleTypeDef *UartHandle);
}uart_weak_cb_t;





static uint32_t         iUartWeakCB;
static uart_weak_cb_t   tUartWeakCB[5];
static uart_isr_t       tUartITHandle[8];


#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */



/* extern variables for Patch to reset AFE back to good AVS buffer config */

void avs_Error_Handler(void)
{

  AVS_Signal_Exeception(NULL, AVS_SIGNAL_EXCEPTION_HARD_FAULT);
}





/**
* @brief  System Clock Configuration
*        The system Clock is configured as follow :
*           System Clock source           = PLL (HSE)
*           SYSCLK(Hz)                     = 216000000
*           HCLK(Hz)                       = 216000000
*           AHB Prescaler                 = 1
*           APB1 Prescaler                 = 4
*           APB2 Prescaler                 = 2
*           HSE Frequency(Hz)             = 25000000
*           PLL_M                         = 25
*           PLL_N                         = 432
*           PLL_P                         = 2
*           PLL_Q                         = 9
*           VDD(V)                         = 3.3
*           Main regulator output voltage = Scale1 mode
*           Flash Latency(WS)             = 7
* @param  None
* @retval None
*/

void SystemClock_Config(void)
{

  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  /*   __HAL_RCC_PWR_CLK_ENABLE(); */

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);


  /* Enable overdrive mode */
  HAL_PWREx_EnableOverDrive();

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 */
  /* Clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);

}

/**
 * @brief Set the flash in Dual bank  ( mandatory for flash storage)
 **/

static uint32_t FLASH_Config(void)
{
  uint32_t  flashMode = 1;
  FLASH_OBProgramInitTypeDef    OBInit;
  HAL_FLASH_OB_Unlock();
  HAL_FLASH_Unlock();
  memset(&OBInit, 0, sizeof(OBInit));
  HAL_FLASHEx_OBGetConfig(&OBInit);
  if((OBInit.USERConfig & OB_NDBANK_SINGLE_BANK) == OB_NDBANK_SINGLE_BANK)
  {
    flashMode = 0;
  }

  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
  return flashMode ;
}




/**
 * @brief Debug UART initialization
 * @param[in] baud rate UART baud rate
 **/

void platform_uart_config(uint32_t baudrate)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOA clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  /* Enable USART1 clock */
  __HAL_RCC_USART1_CLK_ENABLE();

  /* Configure USART1_TX (PA9) and USART1_RX (PA10) */
  GPIO_InitStructure.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
  GPIO_InitStructure.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 1);
  /* By default, USARTx_IRQn   as an higher priority than the kernel, we need to reduce it, in order to allow rtos syscall in the ISR */
//  NVIC_SetPriority(USART1_IRQn, (configMAX_SYSCALL_INTERRUPT_PRIORITY >> 4));
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  /* Configure USART1 */
  UART_Handle.Instance = USART1;
  UART_Handle.Init.BaudRate = baudrate;
  UART_Handle.Init.WordLength = UART_WORDLENGTH_8B;
  UART_Handle.Init.StopBits = UART_STOPBITS_1;
  UART_Handle.Init.Parity = UART_PARITY_NONE;
  UART_Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UART_Handle.Init.Mode = UART_MODE_TX_RX;
  HAL_UART_Init(&UART_Handle);
  platform_uart_set_isr(1,console_uart);

}

void  platform_uart_term()
{
  HAL_UART_DeInit(&UART_Handle);
  platform_uart_set_isr(1,0);
}


UART_HandleTypeDef * platform_uart_get_console()
{
  return &UART_Handle;
}


/**
* @brief MPU configuration
**/

/*
We use SDRAM and stm32f769xx_flash_SDRAM.icf
 we use a double buffering
 so on top of the normal configuration ( nocache for eth and sram, we add LCD_FRAME_BUFFER_LAYER0 and LCD_FRAME_BUFFER_LAYER1

*/

__weak void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable MPU */
  HAL_MPU_Disable();

  /* SRAM main ram */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x20020000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.SubRegionDisable = 0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* SRAM2 nocache */
  /* Configure the MPU attributes as Normal non-cacheable for Eth buffers the SRAM2 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x2007C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.SubRegionDisable = 0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1; /* Normal memory*/
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as Device for Ethernet Descriptors in the SRAM2 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x2007C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512B;
  MPU_InitStruct.SubRegionDisable = 0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0; /* Device memory*/
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);


  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.BaseAddress = 0xC0400000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4MB;
  MPU_InitStruct.SubRegionDisable = 0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as strongly ordred for QSPI (unused zone) */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as WT for QSPI (used 16Mbytes) */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER5;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  
  
  /* Configure Last sector of Flash as non cachable         */
  /*  This to avoid cache maintenance + no need performance */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER6;
  MPU_InitStruct.BaseAddress = 0x081E0000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
    

  
  /* Enable MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);


}

/* For debug, we initialize some GPIO to be able to use a DIGITVIEW */
static void   GPIO_Config()
{

  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOA clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Map CN14 A0 = PA6 , A1= PA4 */

  GPIO_InitStructure.Pin = GPIO_PIN_6 | GPIO_PIN_4;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
  GPIO_InitStructure.Alternate = 0;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);


  GPIO_InitStructure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_14  | GPIO_PIN_15     ;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
  GPIO_InitStructure.Alternate = 0;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);



  __HAL_RCC_GPIOF_CLK_ENABLE();
  /* Map CN14 A3 = PF10 , A4= PF8 A5= PF9 */

  GPIO_InitStructure.Pin = GPIO_PIN_10 | GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
  GPIO_InitStructure.Alternate = 0;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);

}


/**
  * @brief  Line Event callback.
  * @param  hltdc: pointer to a LTDC_HandleTypeDef structure that contains
  *                the configuration information for the LTDC.
  * @retval None
  */
void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *hltdc)
{
  portBASE_TYPE flag = (portBASE_TYPE)FALSE;
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    xSemaphoreGiveFromISR(vSyncEvent, &flag);
    portEND_SWITCHING_ISR(flag);
  }
  HAL_LTDC_ProgramLineEvent(&hltdc_discovery, LTCD_LINE);

}




__weak void LCD_Vsync_Init()
{
  /* VSync will be synchronized by an event from the LCD line ISR */
  vSemaphoreCreateBinary(vSyncEvent);
  xSemaphoreTake(vSyncEvent, 0);
  /* By default, LCD line as an higher priority than the kernel, we need to reduce it, in order to allow rtos syscall in the ISR */
  NVIC_SetPriority(LTDC_IRQn, (configMAX_SYSCALL_INTERRUPT_PRIORITY >> 4));
  HAL_LTDC_ProgramLineEvent(&hltdc_discovery, LTCD_LINE);

}

__weak void LCD_SetFBStarAdress(uint32_t layer, uint32_t offset)
{
  hltdc_discovery.LayerCfg[layer].FBStartAdress = offset;
}



/*

  Init the default LCD support
  This standard init could be overloaded to fix a tearing effect due to the use of a DSI screen

*/
__weak void GUI_LCD_Init()
{
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_Clear(0);

  BSP_LCD_SetLayerVisible(0, ENABLE);
  BSP_LCD_SelectLayer(0);
  BSP_LCD_DisplayOn();


  /* Compute screen placement */
  gScreen[0] = LCD_FB_START_ADDRESS;
  gScreen[1] = LCD_FB_START_ADDRESS + BSP_LCD_GetXSize() * BSP_LCD_GetYSize() * 4;
  LCD_Vsync_Init();
}
/*

  Set the screen buffer

*/

__weak uint32_t LCD_SwapBuffer()
{
  uint32_t iScreen  = gScreen[cptFlip & 1];

  LTDC_LAYER(&hltdc_discovery, 0)->CFBAR = iScreen;
  __HAL_LTDC_RELOAD_CONFIG(&hltdc_discovery);
  cptFlip++;
  return gScreen[cptFlip & 1];
}

/*

  Wait the next vertical blanking

*/
__weak void LCD_WaitVSync()
{
  xSemaphoreTake(vSyncEvent, 1000);

}




void platform_Init()
{
  MPU_Config();
  /* HAL library initialization */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

  /* Enable I-cache and D-cache */
  SCB_EnableICache();
  SCB_EnableDCache();

#ifdef  USE_TRACEALYZER
#if ( configUSE_TRACE_FACILITY == 1 )
  vTraceEnable(TRC_START);
#endif
#endif

  platform_uart_config(921600);

  /* LED configuration */
  BSP_LED_Init(LED_RED);
  BSP_LED_Init(LED_GREEN);
  /* Clear LEDs */
  BSP_LED_Off(LED_RED);
  
  /* Mandatory for spirit mp3 codec ??????? */
#ifdef STM32F10X_CL
  RCC_AHBPeriphClockCmd( RCC_AHBPeriph_CRC, ENABLE);
#else
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
#endif /* STM32F10X_CL */
  

  /* Initialize user button */
  BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_GPIO /*BUTTON_MODE_GPIO*/);

  /* Mandatory for assets mng */
  BSP_QSPI_Init();
  BSP_QSPI_EnableMemoryMappedMode();

  if(!FLASH_Config())
  {
    platform_config_persistent_storage(0x081C0000,FLASH_SECTOR_11,256);
  }
  else
  {
    platform_config_persistent_storage(0x081E0000,FLASH_SECTOR_23,128);
  }
  GUI_LCD_Init();
#if defined(AVS_USE_DEBUG)
  GPIO_Config();
#endif
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  for(int a = 0; a < iUartWeakCB ; a++)
  {
    if(tUartWeakCB[a].type == UART_RXCPL)
    {
      tUartWeakCB[a].cb(UartHandle);
    }
  }
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  for(int a = 0; a < iUartWeakCB ; a++)
  {
    if(tUartWeakCB[a].type == UART_TXCPL)
    {
      tUartWeakCB[a].cb(UartHandle);
    }
  }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  for(int a = 0; a < iUartWeakCB ; a++)
  {
    if(tUartWeakCB[a].type == UART_ERROR)
    {
      tUartWeakCB[a].cb(UartHandle);
}
  }
}

void platform_uart_clear_weak_cb()
{
  iUartWeakCB = 0;
}

void platform_uart_set_isr(uint32_t num, uart_isr_t isr)
{
  tUartITHandle[num]=isr;
}



uint32_t platform_uart_add_weak_cb(uint32_t type,   void (*cb)(UART_HandleTypeDef *UartHandle))
{
  if(iUartWeakCB >= 5) return AVS_ERROR;
  tUartWeakCB[iUartWeakCB].type = type;
  tUartWeakCB[iUartWeakCB].cb = cb;
  iUartWeakCB++;
  return AVS_OK;
}


/**
  * @brief  Re targets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&UART_Handle, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}


static void console_uart(void)
{
  HAL_UART_IRQHandler(&UART_Handle);
}



void USART1_IRQHandler(void)
{
  if(tUartITHandle[1]) (tUartITHandle[1])();
}

void UART5_IRQHandler(void)
{
  if(tUartITHandle[5])  (tUartITHandle[5])();
}



