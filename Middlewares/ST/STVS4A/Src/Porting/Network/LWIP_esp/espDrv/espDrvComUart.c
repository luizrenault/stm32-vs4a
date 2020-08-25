/**
******************************************************************************
* @file    espDrvComUart.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   UART communication support for esp wifi module
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
*    microprocessor devices manufactured by or for STMicroelectronics.
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
/*
    This source handle Frame packet transmission over the UART, the code implement an LIN protocol and a DMA
    The Local Interconnection network(LIN) protocol allow to send and receive frames of variable lengths
    Each frame must be terminated by a 'break' character
    The stm32 fire an interruption when the break is detected, the HAL notify the break has an error.
    When the break is detected we stop the DMA, inspect the buffer contents and re-arm the DMA for the next packet
*/


#include <avs_board_f7.h>
#include "stm32f7xx_ll_usart.h"
#include "espDrvCore.h"

#define PARSE_RX_TASK_STACK_SIZE 1000                       /* Task  size  stack */
#define PARSE_RX_TASK_PRIORITY   osTaskPriorityAboveNormal  /* Task  priority   */
#define PARSE_RX_TASK_NAME       "parse:rx:esp"             /* Task  name       */

typedef enum
{
  UART_RXCPL,
  UART_TXCPL,
  UART_ERROR
} t_uart_weak_type;

/* UART & DMA config for LIN COM */

#define DMA_RX            DMA1_Stream0
#define DMA_RX_CH         DMA_CHANNEL_4
#define DMA_RX_IRQHandler DMA1_Stream0_IRQHandler
#define DMA_RX_ENABLE     __HAL_RCC_DMA1_CLK_ENABLE
#define DMA_RX_IRQn       DMA1_Stream0_IRQn
#define DMA_TX            DMA1_Stream7
#define DMA_TX_CH         DMA_CHANNEL_4
#define DMA_TX_IRQHandler DMA1_Stream7_IRQHandler
#define DMA_TX_ENABLE     __HAL_RCC_DMA1_CLK_ENABLE
#define DMA_TX_IRQn       DMA1_Stream7_IRQn
#define USART_IRQn        UART5_IRQn
#define COM_INSTANCE      UART5

/*  Function HW init as fall back , those Fn are overloaded in the platform adaptation source code   */

__weak uint32_t esp_hw_term()
{
  WIFI_TRACE_VERBOSE("espDrv_hw_term not overloaded");
  return 0;
}

__weak uint32_t esp_hw_init()
{
  WIFI_TRACE_VERBOSE("esp_hw_init not overloaded");
  return 0;
}

__weak uint32_t esp_hw_reset(uint32_t state)
{
  WIFI_TRACE_VERBOSE("espDrv_hw_reset not overloaded");
  return 0;
}

__weak uint32_t esp_hw_enable_GPIO0(uint32_t state)
{
  WIFI_TRACE_VERBOSE("espDrv_hw_enable_GPIO0 not overloaded");
  return 0;
}

__weak void platform_uart_set_isr(uint32_t num, void (*isr)(void))
{
  WIFI_TRACE_ERROR("platform_uart_set_isr should be overloaded in the init platform");
}
__weak void platform_uart_clear_weak_cb()
{

  WIFI_TRACE_ERROR("platform_uart_clear_weak_cb should be overloaded in the init platform");
}

__weak uint32_t platform_uart_add_weak_cb(uint32_t type, void (*cb)(UART_HandleTypeDef *UartHandle))
{
  WIFI_TRACE_ERROR("platform_uart_add_weak_cb should be overloaded in the init platform");
  return FALSE;
}


static void espDrv_isr(void);
static void RxCpltCallback_cb(UART_HandleTypeDef *UartHandle);
static void TxCpltCallback_cb(UART_HandleTypeDef *UartHandle);
static void ErrorCallback_cb(UART_HandleTypeDef *UartHandle);
static void espDrv_parse_pkt_task(void const *argument);
static uint32_t espDrv_parse_ack_priority_msg(void);

static DMA_HandleTypeDef  hdma_uart_rx;
static DMA_HandleTypeDef  hdma_uart_tx;
static UART_HandleTypeDef uartHandle;
static rx_packet_t        *gRxPaket;





/**
 * @brief  Start the DMA receive DMA. This function is the copy of the original  HAL_UART_Receive_DMA. But we have removed the Driver Lock 
 * since the HAL_UART_Receive_DMA may occur during the HAL_UART_Transmit_DMA , we are in situation of lock we cannot solve by the of mutex
 * there is no possibility to lock a mutex from the ISR. We can remove the protection because  DMA RX is only used by the ISR and there is no conflict 
 */



HAL_StatusTypeDef HAL_UART_Receive_DMA_NOLOCK(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
  uint32_t *tmp;
    if((pData == NULL ) || (Size == 0U))
    {
      return HAL_ERROR;
    }
    huart->pRxBuffPtr = pData;
    huart->RxXferSize = Size;
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxState = HAL_UART_STATE_BUSY_RX;
    /* Enable the DMA channel */
    tmp = (uint32_t*)&pData;
    HAL_DMA_Start_IT(huart->hdmarx, (uint32_t)&huart->Instance->RDR, *(uint32_t*)tmp, Size);
    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    SET_BIT(huart->Instance->CR3, USART_CR3_EIE);
    /* Enable the DMA transfer for the receiver request by setting the DMAR bit
    in the UART CR3 register */
    SET_BIT(huart->Instance->CR3, USART_CR3_DMAR);
    return HAL_OK;
}


/**
 * @brief Starts the DMA  for a  frame TX
 *
 * @param pBuffer The buffer
 * @param size  Size buffer, the size must be lower than 1500
 */
static void espDrv_dma_start_tx(void *pBuffer, uint32_t size)
{
  /* Start and wait the action is OK, the ISR may lock the driver if there is a collision  */
  while (HAL_UART_Transmit_DMA(&uartHandle, pBuffer, size) != HAL_OK)
  {
    /* Yield */
    osTaskDelay(0);
  }
}

/**
 * @brief Start the DMA for a frame receive 
 * notice the function is called only from the ISR
 */
static void espDrv_dma_start_rx()
{
  HAL_UART_Receive_DMA_NOLOCK(&uartHandle, gRxPaket->rxBuffer, sizeof(gRxPaket->rxBuffer));
}

/**
 * @brief Init the DMA
 *
 * @return uint32_t Wifi error code
 */
static uint32_t espDrv_dma_init()
{

  /* DMA controller clock enable */
  DMA_RX_ENABLE();
  DMA_TX_ENABLE();

  /* We use Free RTOS and the priority must be lower than the Free RTOS max priority */

  HAL_NVIC_SetPriority(DMA_RX_IRQn, 5, 0);
  HAL_NVIC_SetPriority(DMA_TX_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA_TX_IRQn);
  HAL_NVIC_EnableIRQ(DMA_RX_IRQn);

  hdma_uart_rx.Instance                 = DMA_RX;
  hdma_uart_rx.Init.Channel             = DMA_RX_CH;
  hdma_uart_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_uart_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_uart_rx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_uart_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_uart_rx.Init.Mode                = DMA_NORMAL;
  hdma_uart_rx.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_uart_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_uart_rx) != HAL_OK)
  {
    return FALSE;
  }
  __HAL_LINKDMA(&uartHandle, hdmarx, hdma_uart_rx);

  hdma_uart_tx.Instance                 = DMA_TX;
  hdma_uart_tx.Init.Channel             = DMA_TX_CH;
  hdma_uart_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_uart_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_uart_tx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_uart_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_uart_tx.Init.Mode                = DMA_NORMAL;
  hdma_uart_tx.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_uart_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&hdma_uart_tx) != HAL_OK)
  {
    return FALSE;
  }
  __HAL_LINKDMA(&uartHandle, hdmatx, hdma_uart_tx);
  return TRUE;
}

static uint32_t espDrv_uart_init(uint32_t baudrate)
{

  /* Hook the UART HAL Weak function and ISR IT callback */
  platform_uart_clear_weak_cb();
  platform_uart_add_weak_cb(UART_RXCPL, RxCpltCallback_cb);
  platform_uart_add_weak_cb(UART_TXCPL, TxCpltCallback_cb);
  platform_uart_add_weak_cb(UART_ERROR, ErrorCallback_cb);
  platform_uart_set_isr(5, espDrv_isr);

  /* Init the UART in LIN mode */
  uartHandle.Instance            = COM_INSTANCE;
  uartHandle.Init.BaudRate       = baudrate;
  uartHandle.Init.WordLength     = UART_WORDLENGTH_8B;
  uartHandle.Init.StopBits       = UART_STOPBITS_1;
  uartHandle.Init.Parity         = UART_PARITY_NONE;
  uartHandle.Init.Mode           = UART_MODE_TX_RX;
  uartHandle.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
  uartHandle.Init.OverSampling   = UART_OVERSAMPLING_16;
  uartHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  
  uartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_AUTOBAUDRATE_INIT;
  uartHandle.AdvancedInit.AutoBaudRateEnable = UART_ADVFEATURE_AUTOBAUDRATE_ENABLE;
  uartHandle.AdvancedInit.AutoBaudRateMode = UART_ADVFEATURE_AUTOBAUDRATE_ONSTARTBIT;
  

  
  if (HAL_LIN_Init(&uartHandle, UART_LINBREAKDETECTLENGTH_11B) != HAL_OK)
  {
    return FALSE;
  }

  /* Disable unexpected IT */
  __HAL_UART_DISABLE_IT(&uartHandle, UART_IT_TC);
  __HAL_UART_DISABLE_IT(&uartHandle, UART_IT_RXNE);
  __HAL_UART_DISABLE_IT(&uartHandle, UART_IT_IDLE);
  __HAL_UART_DISABLE_IT(&uartHandle, UART_IT_PE);
  __HAL_UART_DISABLE_IT(&uartHandle, UART_IT_CM);
  __HAL_UART_DISABLE_IT(&uartHandle, UART_IT_CTS);
  __HAL_UART_DISABLE_IT(&uartHandle, UART_IT_TXE);

  /* We use Free RTOS and the priority must be lower than the Free RTOS max priority */
  HAL_NVIC_SetPriority(USART_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART_IRQn);
  return WIFI_OK;
}

/**
 * @brief Re-init the UART with a new baud rate
 *
 * @param baudrate  New baud rate
 * @return uint32_t Wifi error code
 */
uint32_t espDrv_change_baudrate(uint32_t baudrate)
{
  gRxPaket->rxInitialized = FALSE;

  if (espDrv_uart_init(baudrate) == FALSE)
  {
    return WIFI_ERROR;
  }
  gRxPaket->rxInitialized = TRUE;
  espDrv_dma_start_rx();
  return WIFI_OK;
}



/**
 * @brief Auto detect the baud rate 
 *
 */

void  espDrv_auto_baud_rate_detection(void )
{
  uint32_t nbTry = 4;
  while(nbTry--)
  {
    uint32_t timeout=1000;
    /* Wait until auto detection is done*/
    __HAL_UART_SEND_REQ(&uartHandle,UART_AUTOBAUD_REQUEST);
    
    while(__HAL_UART_GET_FLAG(&uartHandle,UART_FLAG_ABRF) == RESET && __HAL_UART_GET_FLAG(&uartHandle,UART_FLAG_ABRE) == RESET && timeout--)
    {
      osTaskDelay(1);
    }
    
    if(__HAL_UART_GET_FLAG(&uartHandle,UART_FLAG_ABRF) == SET)
    {
      timeout=100;
      while(__HAL_UART_GET_FLAG(&uartHandle,UART_FLAG_RXNE) == RESET)
      {
        osTaskDelay(1);
        timeout--;
      }
      if(timeout != 0)
      {
        break;
      }
    }
  }
  if(nbTry ==0)
  {
    WIFI_TRACE_ERROR("UART baud rate detection failed");
  }
  else
  {
    uint32_t br = LL_USART_GetBaudRate(COM_INSTANCE,HAL_RCC_GetPCLK1Freq(),LL_USART_OVERSAMPLING_16);
    WIFI_TRACE_VERBOSE("UART baud rate detected at %d",br);
  }
}




/**
 * @brief main transport frame UART initialization
 *
 * @param baudRate  UART Baud rate
 * @param pRxPaket  Rx Packet handle
 * @return WIFI_Result Wifi Error code
 */

WIFI_Result espDrv_init(uint32_t baudRate, rx_packet_t *pRxPaket)
{

  /* First init the HW GPIO etc.  according to the board ... */
  if (esp_hw_init() == 0)
  {
    return WIFI_ERROR;
  }
  /* Reset the esp and change its boot mode as normal boot ( in case of flashing HW configuration ) */
  /* Enable the reset  */
  esp_hw_reset(FALSE);
  HAL_Delay(500);
  /* Set the normal  mode */
  esp_hw_enable_GPIO0(TRUE);
  /* Reboot the module */

  esp_hw_reset(TRUE);
  /* Wait 3 sec because the Esp FW send a status log at 78500 baud right after the bootup*/
  osTaskDelay(3000);

  gRxPaket = pRxPaket;
  /* All trace GPIO at 0 */
  DEBUG_TRACE_GPIO_1(0);
  DEBUG_TRACE_GPIO_2(0);
  DEBUG_TRACE_GPIO_3(0);
  DEBUG_TRACE_GPIO_4(0);
  DEBUG_TRACE_GPIO_5(0);

  /* Init the DMA first */
  if (espDrv_dma_init() == FALSE)
  {
    return WIFI_ERROR;
  }
  /* Init the UART */
  if (espDrv_uart_init(baudRate) == FALSE)
  {
    return WIFI_ERROR;
  }
  gRxPaket->bSync = TRUE;

  /* Create the Ring buffer Free RTOS object to manage the packet parsing */
  
  ring_bytes_buffer_Create(&gRxPaket->buffer, gRxPaket->ring_rxBuffer, sizeof(gRxPaket->ring_rxBuffer));
  WIFI_VERIFY(osEvent_Create(&gRxPaket->hRxEvent));
  WIFI_VERIFY(osEvent_Create(&gRxPaket->hTxEvent));
  WIFI_VERIFY(osEvent_Create(&gRxPaket->hEvtParser));
  gRxPaket->hRxTask = osTask_Create(PARSE_RX_TASK_NAME, espDrv_parse_pkt_task, NULL, PARSE_RX_TASK_STACK_SIZE, PARSE_RX_TASK_PRIORITY);
  WIFI_ASSERT(gRxPaket->hRxTask );
  
  espDrv_auto_baud_rate_detection();
 
  
  /* We are ready to start the pump */
  gRxPaket->rxInitialized = TRUE;
  /* Set  the  DMA ARMED on RX, all other will be re-armed in the ISR */
  espDrv_dma_start_rx();

  return WIFI_OK;
}

/**
 * @brief Terminate the driver instance
 *
 */
void espDrv_term(void)
{

  gRxPaket->rxInitialized = FALSE;
  /* Remove the ISR callback */
  platform_uart_set_isr(5, 0);

  /* delete the Parsing task */
  if (gRxPaket->hRxTask)
  {
    osTask_Delete(gRxPaket->hRxTask);
    gRxPaket->hRxTask = 0;
  }
  /* delete RTos objects */
  osEvent_Delete(&gRxPaket->hRxEvent);
  osEvent_Delete(&gRxPaket->hTxEvent);
  osEvent_Delete(&gRxPaket->hEvtParser);


  HAL_NVIC_DisableIRQ(DMA_RX_IRQn);
  HAL_NVIC_DisableIRQ(DMA_TX_IRQn);
  HAL_NVIC_DisableIRQ(USART_IRQn);
  HAL_UART_DeInit(&uartHandle);

}

/**
 * @brief Send a frame to the esp  module, the frame must be lower than 1500
 *
 * @param pData
 * @param Length
 * @return WIFI_Result
 */
WIFI_Result espDrv_send_frame(uint8_t *pData, uint32_t Length)
{
  WIFI_Result err = WIFI_ERROR;
  if (gRxPaket->rxInitialized == FALSE)
  {
    return err;
  }
  if(Length < sizeof(frame_packet_header_t))
  {
    return err;
  }
  if(Length >= MAX_PAYLOAD_FRAME)
  {
    return err;
  }
  DEBUG_TRACE_GPIO_2(1);
  /* Start the DMA */
  espDrv_dma_start_tx((uint8_t *)pData, Length);
  /* Wait for the end */
  if (osEvent_Wait(&gRxPaket->hTxEvent, TX_TIMEOUT) != WIFI_OK)
  {
    goto send_error;
  }
  /* Add a Break at the end of the frame */
  HAL_LIN_SendBreak(&uartHandle);
  /* Trace end TX */
  err = WIFI_OK;
send_error:
  DEBUG_TRACE_GPIO_2(0);
  return err;
}

/**
 * @brief Signal a packet complete
 *
 */
static void packetComplete()
{
  DEBUG_TRACE_GPIO_2(cptPkt++ & 1);
  /* Stop the DMA in case of overrun */
  HAL_DMA_Abort(&hdma_uart_rx);
  
  if (gRxPaket->rxInitialized)
  {
    /* Check the signature of the packet and make sure it is a good packet */
    if (gRxPaket->rxBuffer[0] == (PKT_START_CODE >> 8) &&
        gRxPaket->rxBuffer[1] == (PKT_START_CODE & 0xFF))
    {
      /* Extract the length and check it */
      uint32_t szPkt = (gRxPaket->rxBuffer[2] << 8) | gRxPaket->rxBuffer[3];
      if (szPkt <= MAX_RX_BUFFER)
      {
		/* check first the ACK */
        if(espDrv_parse_ack_priority_msg() == FALSE)
        {
          /* We can't stay in an ISR to prevent overrun, so we delegate the parsing and process of the packet in a thread */
          /* We copy the frame in a ring buffer and signal to the thread a incoming packet */
          uint32_t bytesInjected = ring_bytes_buffer_produce(&gRxPaket->buffer, szPkt + 4, gRxPaket->rxBuffer);
          if (bytesInjected  == szPkt + 4)
          {
            osEvent_Set(&gRxPaket->hEvtParser);
          }
          else
          {
            WIFI_TRACE_VERBOSE("Ring buffer overrun");
          }
        }
      }
      else
      {
        WIFI_TRACE_VERBOSE("Packet size too large");
      }
    }
    else
    {
      if(gRxPaket->bSync == FALSE )
      {
        WIFI_TRACE_VERBOSE("Bad packet signature");
      }
    }
  }
  /* Re-arm the DMA transfer */
    espDrv_dma_start_rx();
}

/**
 * @brief Signal a block Tx  complete
 *
 * @param UartHandle
 */
static void TxCpltCallback_cb(UART_HandleTypeDef *UartHandle)
{

  if (UartHandle == &uartHandle)
  {
    osEvent_Set(&gRxPaket->hTxEvent);
  }
}
/**
 * @brief Signal a full DMA transfer complete, normally it is an error because a break should be signalled before the end of the maximum transfer
 *
 * @param UartHandle
 */
static void RxCpltCallback_cb(UART_HandleTypeDef *UartHandle)
{

  if (UartHandle == &uartHandle)
  {
    WIFI_TRACE_ERROR("Miss the break character");
    packetComplete();
  }
}

/**
 * @brief Callback error, in the LIN implementation, an error means a Break is detected
 *
 * @param UartHandle  the UART handle
 */
static void ErrorCallback_cb(UART_HandleTypeDef *UartHandle)
{

  if (UartHandle == &uartHandle)
  {
   __HAL_UART_CLEAR_IT(UartHandle,UART_CLEAR_LBDF);
    packetComplete();
  }
}

/**
 * @brief Copy a partial buffer aligned from the ring buffer
 *
 * @param rBuffer ring buffer handle
 * @param pBuffer aligned buffer
 * @param len     aligned buffer size
 */
static void espDrv_copy_partial_ring_buffer(ring_bytes_buffer *rBuffer, uint8_t *pBuffer, uint32_t len)
{

  uint8_t *pAligned = pBuffer;
  uint8_t *pSrc;
  uint8_t *pSrc1 = ring_bytes_buffer_get_consumer(rBuffer);

  if (len >= MAX_RX_BUFFER)
  {
    WIFI_TRACE_ERROR("Error len");
    return;
  }
  /* Loop until the buffer is complete */
  while (len)
  {
    /* Get the maximum aligned size available in the ring buffer */
    uint32_t aLen = ring_bytes_buffer_size_consumer_aligned(rBuffer);
    /* clamp it to the maximum expected */
    if (aLen > len)
    {
      aLen = len;
    }
    /* Copy the payload in the partial buffer */
    pSrc = ring_bytes_buffer_get_consumer(rBuffer);
    memcpy(pAligned, pSrc, aLen);
    if (rBuffer->szConsumer < aLen)
    {
      WIFI_ASSERT(rBuffer->szConsumer >= aLen);
    }
    /* consume the block in the ring buffer */
    ring_bytes_buffer_move_consumer(rBuffer, aLen);
    len -= aLen;
    pAligned += aLen;
  }
}

/**
 * @brief return true if the message is parsed as a priority message 
 * ACK message are sent immediately because the parse thread could be locked by LWIP or task taking a while to be done 
 * and other thread could be lock and get a time-out because the ack  waiting to be dispatched due to the long task
 */

static uint32_t espDrv_parse_ack_priority_msg()
{
  frame_packet_header_t *pPkt = (frame_packet_header_t *)&gRxPaket->rxBuffer[PKT_HEADER_SIZE];
  if(pPkt->message & MSG_ACK)
  {
      if (gRxPaket)
      {
        /* Call the event handler */
        gRxPaket->rxCb(pPkt, pPkt->szPkt + sizeof(frame_packet_header_t));
        return TRUE;
      }
  }
  return FALSE;
}



/**
 * @brief loop paring the frame ring buffer and call the handler for event processing
 *
 */
void espDrv_parse_ring_buffer()
{
  static uint8_t tRawPkt[MAX_RX_BUFFER];
  while (1)
  {
    if (gRxPaket->buffer.szConsumer)
    {
      WIFI_ASSERT(gRxPaket->buffer.szConsumer >= sizeof(frame_packet_header_t));
      frame_packet_header_t *pPkt = (frame_packet_header_t *)&tRawPkt[4];
      /* copy the frame header */
      espDrv_copy_partial_ring_buffer(&gRxPaket->buffer, (void *)tRawPkt, sizeof(frame_packet_header_t) + 4);
      /* copy the  header in local to respect the alignment */
      uint8_t *pPayload = (uint8_t *)&pPkt[1];
      uint32_t szPayload = pPkt->szPkt;
      WIFI_ASSERT(gRxPaket->buffer.szConsumer >= szPayload);
      /* copy the frame payload */
      espDrv_copy_partial_ring_buffer(&gRxPaket->buffer, pPayload, szPayload);
      if (gRxPaket)
      {
        /* Call the event handler */
        gRxPaket->rxCb(pPkt, pPkt->szPkt + sizeof(frame_packet_header_t));
      }
    }
    else
    {
      break;
    }
  }
}
/**
 * @brief Event parsing task
 *
 * @param argument unused
 */
static void espDrv_parse_pkt_task(void const *argument)
{
  while (1)
  {
    /* Wait an event from the ISR */
    if (osEvent_Wait(&gRxPaket->hEvtParser, (uint32_t)-1) == WIFI_OK)
    {
      /* Parse the ring buffer until it is empty */
      espDrv_parse_ring_buffer();
    }
  }
}

/**
 * @brief DMA ISR handler
 *
 */
void DMA_TX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_uart_tx);
}
/**
 * @brief DMA ISR handler
 *
 */
void DMA_RX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_uart_rx);
}

/**
 * @brief UART ISR handler
 *
 */
static void espDrv_isr()
{

  HAL_UART_IRQHandler(&uartHandle);
}
