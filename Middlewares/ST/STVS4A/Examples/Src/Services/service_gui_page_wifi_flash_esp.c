/**
******************************************************************************
* @file    service_gui_wifi_flash.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   flash esp8266 user interface
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2018 STMicroelectronics International N.V.
* All rights reserved.</center></h2>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted, provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. Neither the name of STMicroelectronics nor the names of other
*    contributors to this software may be used to endorse or promote products
*    derived from this software without specific written permission.
* 4. This software, including modifications and/or derivative works of this
*    software, must execute solely and exclusively on microcontroller or
*    microor devices manufactured by or for STMicroelectronics.
* 5. Redistribution and use of this software other than as permitted under
*    this license is void and will automatically terminate your rights under
*    this license.
*
* THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
* RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
* SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/



#include "service.h"

#ifdef AVS_USE_NETWORK_WIFI

#include "platform_init.h"
#include "avs_private.h"
#include "espDrv/espDrvCore.h"

#define  FLASH_SPEED    115200 
#define  RING_BUF_SIZE  (10*1024)

uint32_t             esp_hw_init(void);
uint32_t             esp_hw_enable_GPIO0(uint32_t state);
uint32_t             esp_hw_reset(uint32_t state );



static Avs_bytes_buffer espRing;        /* Ring buffer esp8266 delegation */
static Avs_bytes_buffer hostRing;       /* Ring buffer host delegation */
static uint32_t         bReInit=1;      /* True if Re-ini the com */
static uint8_t          hostRxChar;     /* Rx cuurent char */
static uint8_t          espRxChar;      /* Rx cuurent char */
static uint32_t         txTransfert;    /* Transfert size  */
static uint32_t         rxTransfert;    /* Transfert size  */

static  UART_HandleTypeDef esp_uart_handle;     /* UART handle for esp8266 */
static  UART_HandleTypeDef host_uart_handle;    /* UART handle for St-link*/



/*

    Init Esp8266 UART
*/


void esp_uart_isr(void);
void esp_uart_isr(void)
{
  
  HAL_UART_IRQHandler(&esp_uart_handle);
  
  
}

AVS_Result esp_uart_init(uint32_t bautRate);
AVS_Result esp_uart_init(uint32_t bautRate)
{
  
  HAL_UART_DeInit(&esp_uart_handle);
  
  /* Set the WiFi USART configuration parameters */
  esp_uart_handle.Instance        = UART5;
  esp_uart_handle.Init.BaudRate   = bautRate;
  esp_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
  esp_uart_handle.Init.StopBits   = UART_STOPBITS_1;
  esp_uart_handle.Init.Parity     = UART_PARITY_NONE;
  esp_uart_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  esp_uart_handle.Init.Mode       = UART_MODE_TX_RX;
  esp_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;
  
  /* Configure the USART IP */
  if(HAL_UART_Init(&esp_uart_handle) != HAL_OK)
  {
    return AVS_ERROR;
  }
  platform_uart_set_isr(5,&esp_uart_isr);
  HAL_NVIC_SetPriority(UART5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(UART5_IRQn);
  
  return AVS_OK;
  
}

/*

    Init St-link UART
*/

void host_uart_isr(void);
void host_uart_isr(void)
{
  
  HAL_UART_IRQHandler(&host_uart_handle);
  
  
}


static AVS_Result host_uart_init(uint32_t bautRate);
static AVS_Result host_uart_init(uint32_t bautRate)
{
  HAL_UART_DeInit(&host_uart_handle);
  
  /* Set the WiFi USART configuration parameters */
  host_uart_handle.Instance        = USART1;
  host_uart_handle.Init.BaudRate   = bautRate;
  host_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
  host_uart_handle.Init.StopBits   = UART_STOPBITS_1;
  host_uart_handle.Init.Parity     = UART_PARITY_NONE;
  host_uart_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  host_uart_handle.Init.Mode       = UART_MODE_TX_RX;
  host_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;
  
  /* Configure the USART IP */
  if(HAL_UART_Init(&host_uart_handle) != HAL_OK)
  {
    return AVS_ERROR;
  }
  platform_uart_set_isr(1,&host_uart_isr);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
  return AVS_OK;
}



/*

    Manage Uart Error ( overrun etc...)
*/

void my_ErrorCallback(UART_HandleTypeDef *huart);
void my_ErrorCallback(UART_HandleTypeDef *huart)
{
  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* If an error occurs , we need to re-lauchthe IT */
    if(huart == &host_uart_handle)
    {
      HAL_UART_Receive_IT(huart, (uint8_t *)&hostRxChar, 1);
      
    }
    
    if(huart == &esp_uart_handle)
    {
      HAL_UART_Receive_IT(huart, (uint8_t *)&espRxChar, 1);
    }
  }
}


/*

  overload the HAL weak RxCpltCallback  to manage Rx IT


*/
void my_RxCpltCallback(UART_HandleTypeDef *uart);
void my_RxCpltCallback(UART_HandleTypeDef *uart)
{
  if(uart == &host_uart_handle)
  {
    if(hostRing.pBuffer)
    {
      avs_bytes_buffer_produce_add(&hostRing,hostRxChar);
      txTransfert++;
    }
    HAL_UART_Receive_IT(uart, (uint8_t *)&hostRxChar, 1);
  }
  if(uart == &esp_uart_handle)
  {
    if(espRing.pBuffer)
    {
      avs_bytes_buffer_produce_add(&espRing,espRxChar);
      rxTransfert++;
    }
    HAL_UART_Receive_IT(uart, (uint8_t *)&espRxChar, 1);
  }
}


/*
      Task managing the uart relay from RX to TX

*/
static void service_flash_esp_task(const void *pCookie);
static void service_flash_esp_task(const void *pCookie)
{
  while(1)
  {
    uint8_t bSleep=1;
    if(bReInit)
    {
      /* Reset th ring buffer */
      avs_byte_buffer_reset(&hostRing);
      avs_byte_buffer_reset(&espRing);
      /* Init both uarts */
      esp_uart_init(FLASH_SPEED);
      host_uart_init(FLASH_SPEED);
      /* Reset the esp and change its boot mode as FW download*/
      /* Enable GPIO0 */
      esp_hw_enable_GPIO0(TRUE);
      
      /* Enable the reset  */
      esp_hw_reset(FALSE);
      avs_core_task_delay(500);
      /* Set the FW download mode */
      esp_hw_enable_GPIO0(FALSE);
      /* Reboot the esp */
      esp_hw_reset(TRUE);
      /* Hook HAL weak functions for the IT RX */
      platform_uart_clear_weak_cb();
      platform_uart_add_weak_cb(0,my_RxCpltCallback);
      platform_uart_add_weak_cb(2,my_ErrorCallback);
      
      /* Starts the RX IT pump */
      HAL_UART_Receive_IT(&esp_uart_handle, (uint8_t *)&espRxChar, 1);
      HAL_UART_Receive_IT(&host_uart_handle, (uint8_t *)&hostRxChar, 1);
      bReInit = 0;
      rxTransfert = 0;
      txTransfert = 0;
      
    }
    
    if(hostRing.szConsumer)
    {
      uint32_t size = hostRing.szConsumer ;
      /* When the host gets data, it send it the esp */
      for( uint32_t a = 0; a < size ; a++ )
      {
        uint8_t data = avs_bytes_buffer_consumer_get(&hostRing,a );
        HAL_UART_Transmit(&esp_uart_handle, (uint8_t *)&data, 1, 0xFFFF);
      }
      avs_bytes_buffer_move_consumer(&hostRing,size);
      
    }
    if(espRing.szConsumer)
    {
      uint32_t size = espRing.szConsumer ;
      /* When the esp gets data, it send it the host */
      for( uint32_t a = 0; a < size ; a++ )
      {
        uint8_t data = avs_bytes_buffer_consumer_get(&espRing,a );
        HAL_UART_Transmit(&host_uart_handle, (uint8_t *)&data, 1, 0xFFFF);
      }
      avs_bytes_buffer_move_consumer(&espRing,size);
      
    }
    
    if(BSP_PB_GetState(BUTTON_WAKEUP) == GPIO_PIN_SET)
    {
      while(BSP_PB_GetState(BUTTON_WAKEUP) == GPIO_PIN_SET)
      {
      }
      bReInit=1;
    }
    if(bSleep)
    {
      osDelay(1);
    }
    
  }
  
}
static void start_flashing(void);
static void start_flashing(void)
{
  /* Create ring buffers */
  avs_bytes_buffer_Create(&hostRing,RING_BUF_SIZE);
  avs_bytes_buffer_Create(&espRing,RING_BUF_SIZE);
  /* Term uart usage for all other tasks*/
  espDrv_term();
  platform_uart_term();
  /* Remove all message on the console */
  AVS_Set_Debug_Level( AVS_TRACE_LVL_MUTE);
  
  /* start the nerver ending task */
  
  if(task_Create("AVS:Flash esp8266",service_flash_esp_task,NULL,500,osPriorityNormal)  == 0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_GUI);
  }
  
}



/*

    Flash Page management 

*/

static uint32_t  gui_proc_wifi_flash_esp(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t  gui_proc_wifi_flash_esp(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{

  static gui_item_t* pButtonDone;
  static gui_item_t* pMessage;
  static gui_item_t* pPacket;
  static gui_item_t* pImage;

  /* Init the page */
  
  if(message == GUI_MSG_INIT)
  {
    pItem->format |= DRT_RENDER_ITEM;
    pItem->page   = PAGE_WIFI_FLASH_ESP;

    pImage = gui_add_item_res(GUI_ITEM_WIFI_FLASH_ESP_IMAGE, gui_proc_default, "esp8266_bmp");
    pImage->page   = PAGE_WIFI_FLASH_ESP;
    pImage->format   |= DRT_IMAGE;

   /* button Reboot*/
    pButtonDone =  gui_add_item_res(GUI_ITEM_WIFI_FLASH_ESP_REBOOT, gui_proc_default, "Reboot");
    pButtonDone->format |= DRT_FRAME | DRT_TS_ITEM;
    pButtonDone->page   = PAGE_WIFI_FLASH_ESP;
    pButtonDone->parent =     pItem;

    pPacket =  gui_add_item_res(GUI_ITEM_WIFI_FLASH_ESP_STAT_STATIC, gui_proc_default, "x/x");
    pPacket->page   = PAGE_WIFI_FLASH_ESP;
    pPacket->parent =     pItem;


    pMessage  = gui_add_item_res(GUI_ITEM_WIFI_FLASH_ESP_HELP_STATIC, gui_proc_default,
                             "1) Close the serial console\r"
                             "2) Use flash-espFw.bat to flash the new FW\r"
                             "3) Reboot when finished\r"
                             "\r"
                             "Read the Ref manual for details "
#ifdef  STM32F769xx
                               "\resp-GPIO0 connected to CN9:D8"
#endif
                              );
    pMessage->page   = PAGE_WIFI_FLASH_ESP;
    pMessage->format = (DRT_FRAME | DRT_HCENTER | DRT_RENDER_ITEM);
    pMessage->offX = 10;
  }

  
  if(message == GUI_MSG_PAGE_ACTIVATE)
  {
    start_flashing();
  }

  if(message == GUI_MSG_RENDER)
  {
    /* Render statistics */
    static char_t  tmpText[100];
    snprintf(tmpText,sizeof(tmpText),"Rx %3.2fK  Tx %3.2fK",rxTransfert/1024.0F,txTransfert/1024.0F );
    pPacket->pText =    tmpText;
  }

  if(message == GUI_MSG_BUTTON_CLICK_NOTIFY)
  {
    if((gui_item_t*)lparam == pButtonDone)
    {
      NVIC_SystemReset();
    }
  }

  return 0;
}

/*
  Wifi pages initialization ( weak function )

*/
gui_item_t*  gui_add_page_wifi_flash_esp(void);
 gui_item_t*  gui_add_page_wifi_flash_esp(void)
{
  return gui_add_item(0, 0, 1, 1, gui_proc_wifi_flash_esp,NULL);
}

#endif
