/**
  ******************************************************************************
  * @file    espDrvCore.h
  * @author  MCD Application Team
  * @brief   driver CORE  API
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
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
#ifndef _espDrvCore_
#define _espDrvCore_
#include "avs_misra.h"

MISRAC_DISABLE
#include "stdint.h"
#include "stddef.h"
#include "string.h"
#include "stdarg.h"
#include "stdio.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "netif/etharp.h"
MISRAC_ENABLE

#include "espDrvOS.h"
#include "espDrvProtocol.h"

#define ESP_TRACE_LEVEL                ((uint32_t )WIFI_TRACE_LVL_ERROR)
#define DRV_TRACE_LEVEL                ((uint32_t )WIFI_TRACE_LVL_ERROR)

#define NB_TRY_ESP_SYNC                 3
#define TX_TIMEOUT                      500
#define RX_TIMEOUT                      5000
#define SYNC_TIMEOUT                    5000
#define COM_CONFIG_DEFAULT              (115200)
#define NO_CHANGE_COM_HIGH_SPEED        921600
#define TIMEO_QUEUE                     (10*1000)
#define MAX_NOTIFIER                    10


#define     NO_DEBUG_TRACE_GPIO
#ifdef      DEBUG_TRACE_GPIO
#include <avs_board_f7.h>

#define DEBUG_TRACE_GPIO_1(state)        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6 , state? GPIO_PIN_SET  :  GPIO_PIN_RESET ); // cn14 A0 RX IT ( digiview pin 5)
#define DEBUG_TRACE_GPIO_2(state)        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4 , state? GPIO_PIN_SET  :  GPIO_PIN_RESET ); // cn14 A1 TX IT (digiview pin 7)
#define DEBUG_TRACE_GPIO_3(state)        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10 , state? GPIO_PIN_SET  :  GPIO_PIN_RESET ); // cn14 A3 Error (digiview pin 0)
#define DEBUG_TRACE_GPIO_4(state)        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8 , state? GPIO_PIN_SET  :  GPIO_PIN_RESET ); // cn14 A4  stop  (digiview pin 2)
#define DEBUG_TRACE_GPIO_5(state)        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9 , state? GPIO_PIN_SET  :  GPIO_PIN_RESET ); // cn14 A5 pin 8  (digiview pin 8)     

#else

#define     DEBUG_TRACE_GPIO_1(state)
#define     DEBUG_TRACE_GPIO_2(state)
#define     DEBUG_TRACE_GPIO_3(state)
#define     DEBUG_TRACE_GPIO_4(state)
#define     DEBUG_TRACE_GPIO_5(state)

#endif

#ifndef HAVE_TRUE
#define TRUE    1
#define HAVE_TRUE
#endif

#ifndef HAVE_FALSE
#define FALSE    0
#define HAVE_FALSE
#endif

#ifndef HAVE_CHAR_T
typedef   char char_t; /*!< Char overload @ingroup type*/
#define HAVE_CHAR_T
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif


#ifndef __ALWAYS_INLINE
#if defined ( __GNUC__  ) || defined ( __ICCARM__   ) /* GCC + IAR */
#define __ALWAYS_INLINE __STATIC_INLINE
#elif defined (__CC_ARM) /* Keil */
#define __ALWAYS_INLINE __attribute__((always_inline))
#else
#warning "__ALWAYS_INLINE undefined "
#define __ALWAYS_INLINE
#endif
#endif


#define         WIFI_OK         1
#define         WIFI_ERROR      0
#define         WIFI_TIMEOUT    2
#define         WIFI_NOT_AVAILABLE 3

typedef         uint32_t        WIFI_Result;


#if defined(AVS_USE_DEBUG)
#define WIFI_ASSERT(a)     if(((int32_t)( a ))== FALSE)   {WIFI_ERROR_FATAL("Fatal: (%s) %d:%s",#a,__LINE__,__FILE__);}
#define WIFI_VERIFY(a)     if(((int32_t)( a ))== FALSE)   {WIFI_ERROR_FATAL("Fatal: (%s) %d:%s",#a,__LINE__,__FILE__);}
#else
#define WIFI_ASSERT(a)
#define WIFI_VERIFY(a)     a
#endif


#if defined(AVS_USE_DEBUG)
#define WIFI_RING_CHECK_PEAK
#endif
#ifdef WIFI_RING_CHECK_PEAK

#define WIFI_RING_CHECK_PROD_PEAK(ring) \
    if((ring)->szConsumer > (ring)->prodPeak)     \
    {\
      (ring)->prodPeak  = (ring)->szConsumer;\
    }
#else
#define WIFI_RING_CHECK_PROD_PEAK(...) ((void)0)
#endif




/**
 * @brief ring buffer handle
 *
 */
typedef struct  t_ring_bytes_buffer
{
  uint8_t      *pBuffer;               /* Ring  buffer */
  uint32_t      szBuffer;              /* Size buffer */
  uint32_t      prodPeak;               /* Size Peak buffer */
  int32_t       szConsumer;           /* Nb element in available in the ring buffer */
  uint32_t      prodPos;              /* Write  position */
  uint32_t      consumPos;            /* Read position */
} ring_bytes_buffer;

/**
 * @brief Create a ring buffer
 *
 * @param pHandle Handle
 * @param pBuffer ring buffer
 * @param nbbytes ring buffer size
 */
__ALWAYS_INLINE uint32_t ring_bytes_buffer_Create(ring_bytes_buffer *pHandle, uint8_t *pBuffer, uint32_t nbbytes);
__ALWAYS_INLINE uint32_t ring_bytes_buffer_Create(ring_bytes_buffer *pHandle, uint8_t *pBuffer, uint32_t nbbytes)
{
  memset(pHandle, 0, sizeof(*pHandle));
  pHandle->szBuffer = nbbytes;
  pHandle->pBuffer = pBuffer;
  /* RAZ by default */
  memset(pHandle->pBuffer, 0, nbbytes);
  return 1;
}

/**
 * @brief move the consumer
 *
 * @param pHandle handle
 * @param offset offset to move
 */
__ALWAYS_INLINE void ring_bytes_buffer_move_consumer(ring_bytes_buffer *pHandle, uint32_t offset);
__ALWAYS_INLINE void ring_bytes_buffer_move_consumer(ring_bytes_buffer *pHandle, uint32_t offset)
{
  pHandle->consumPos = (pHandle->consumPos + offset) % pHandle->szBuffer;
  pHandle->szConsumer -= (int32_t)offset;
}

/**
 * @brief return the pointer on the consumer
 *
 * @param pHandle handle
 */
__ALWAYS_INLINE uint8_t *ring_bytes_buffer_get_consumer(ring_bytes_buffer *pHandle);
__ALWAYS_INLINE uint8_t *ring_bytes_buffer_get_consumer(ring_bytes_buffer *pHandle)
{
  return &pHandle->pBuffer[pHandle->consumPos % pHandle->szBuffer];
}

/**
 * @brief Add a char un the ring buffer
 *
 */
__ALWAYS_INLINE void ring_bytes_buffer_produce_add(ring_bytes_buffer *pHandle, uint8_t data);
__ALWAYS_INLINE void ring_bytes_buffer_produce_add(ring_bytes_buffer *pHandle, uint8_t data)
{
  pHandle->pBuffer[pHandle->prodPos] = data;
  pHandle->prodPos = (pHandle->prodPos + 1) % pHandle->szBuffer;
  pHandle->szConsumer++;
  WIFI_RING_CHECK_PROD_PEAK(pHandle);
};

/**
 * @brief return a char from the ring buffer
 */
__ALWAYS_INLINE uint8_t ring_bytes_buffer_consumer_get(ring_bytes_buffer *pHandle, uint32_t off);
__ALWAYS_INLINE uint8_t ring_bytes_buffer_consumer_get(ring_bytes_buffer *pHandle, uint32_t off)
{
  return pHandle->pBuffer[(pHandle->consumPos + off) % pHandle->szBuffer];
}

/**
 * @brief return the maximum byte aligned in the consummer
 *
 */
__ALWAYS_INLINE uint32_t ring_bytes_buffer_size_consumer_aligned(ring_bytes_buffer *pHandle);
__ALWAYS_INLINE uint32_t ring_bytes_buffer_size_consumer_aligned(ring_bytes_buffer *pHandle)
{
  if (pHandle->prodPos > pHandle->consumPos)
  {
    return pHandle->prodPos - pHandle->consumPos;
  }
  return pHandle->szBuffer - pHandle->consumPos;
}
/**
 * @brief Inject a payload in the producer
 */
__ALWAYS_INLINE uint32_t ring_bytes_buffer_produce(ring_bytes_buffer *pHandle, uint32_t nbSample, uint8_t *pBuffer);
__ALWAYS_INLINE uint32_t ring_bytes_buffer_produce(ring_bytes_buffer *pHandle, uint32_t nbSample, uint8_t *pBuffer)
{
  uint32_t curSize = 0;
  /* Check the remaining data in the ring buffer */
  if (pHandle->szConsumer + nbSample >= pHandle->szBuffer)
  {
    nbSample = pHandle->szBuffer - pHandle->szConsumer;
  }

  if (pHandle->szConsumer + nbSample <= pHandle->szBuffer) /* Check if the buffer is too large */
  {
    if (pHandle->prodPos + nbSample >= pHandle->szBuffer) /* Check if we will reach the buffer end */
    {
      /* Nb element before the end of the buffer */
      uint32_t nbElem = (pHandle->szBuffer - pHandle->prodPos);
      /* Fill the remaining space */
      if (pBuffer)
      {
        memcpy((char_t *)&pHandle->pBuffer[pHandle->prodPos], pBuffer, nbElem);
        pBuffer += nbElem;
      }

      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbElem;
      pHandle->prodPos = 0;
      nbSample -= nbElem;
      curSize += nbElem;
    }
    if (nbSample)
    {
      if (pBuffer)
      {
        memcpy((char_t *)&pHandle->pBuffer[pHandle->prodPos], pBuffer, nbSample);
      }
      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbSample;
      pHandle->prodPos += nbSample;
      curSize += nbSample;
    }
  }
  WIFI_RING_CHECK_PROD_PEAK(pHandle);
  return curSize;
}

/**
 * @brief struct returning a response  payload from the module
 *
 */
typedef struct t_return_msg
{
  uint8_t       *pMsg;
  uint32_t       maxSzMsg;
}return_msg_t;


/**
 * @brief Struct handling Event notification
 *
 */
typedef struct t_com_notifier
{
  uint32_t        message;
  uint32_t       sequence;
  osEvent_t       *eventNotifer;
  uint8_t        result;
  return_msg_t*  pRetMsg;
}com_notifier_t;

typedef uint32_t (*espDrv_rx_cb)(void *pPayload,uint32_t sz);

/**
 * @brief Struct handling Rx LIN Frame packets
 *
 */
typedef struct t_rx_packet
{
  uint8_t                rxBuffer[MAX_RX_BUFFER];
  uint8_t                ring_rxBuffer[4*1024];
  ring_bytes_buffer      buffer;
  osTask_t               *hRxTask;
  osEvent_t               hRxEvent;
  osEvent_t               hTxEvent;
  osEvent_t               hEvtParser;
  uint32_t                rxInitialized;
  uint32_t                bSync;
  espDrv_rx_cb            rxCb;
}rx_packet_t;

WIFI_Result espDrv_init(uint32_t config, rx_packet_t *pRxPaket);
uint32_t espDrv_change_baudrate(uint32_t baudrate);
void espDrv_term(void);
WIFI_Result espDrv_send_frame(uint8_t *pData, uint32_t Length);
void espDrv_notify_callers(uint8_t message, frame_packet_header_t *pPkt, uint32_t len);
uint32_t espDrv_notify_create(uint8_t message, uint8_t sequence, osEvent_t *pNotify, return_msg_t *pRetMsg);
void espDrv_notify_delete(uint32_t handle);
WIFI_Result espDrv_send_packet(uint8_t message, uint16_t sequence, uint8_t result, uint8_t *pPayload, uint32_t len);
WIFI_Result espDrv_send_packet_pbuff(uint8_t message, uint16_t sequence, uint8_t result, struct pbuf *p);
WIFI_Result espDrv_send_ack(uint8_t message, uint8_t *pPayload, uint32_t len, return_msg_t *pRetMsg);
WIFI_Result espDrv_send_ack_pbuff(uint8_t message, struct pbuf *p, return_msg_t *pRetMsg);
WIFI_Result espDrv_send_ack_word(uint8_t message, uint32_t word, return_msg_t *pRetMsg);
void espDrv_parse_event(frame_packet_header_t *pPkt, uint32_t len);
WIFI_Result espDrv_rx_wait(uint8_t ch);
void espDrv_get_quoted_string(char_t quote, char_t *pQuote, char_t *pString, uint32_t maxSize);
void espDrv_get_quoted_string(char_t quote, char_t *pQuote, char_t *pString, uint32_t maxSize);
uint8_t espDrv_send_packet_start(uint32_t message, uint32_t sequence, uint32_t result, uint32_t chkSum);
char_t *espDrv_get_message_string(uint32_t message);
uint32_t espDrv_create_sequence_number(void);
uint32_t espDrv_calc_chksum(unsigned char *data, uint32_t data_size);
uint32_t netif_inject_input_frame(uint8_t *pBuffer, uint32_t len);
struct netif *netif_get_instance(void);
err_t netif_espDrv_init(struct netif *netif);
uint32_t wifi_network_start_dhcp(void);
void espDrv_set_wifi_state(uint32_t state);
void espDrv_notify_wifi_connected(uint32_t state);
void espDrv_notify_scan_result(char_t *pResult);
void espDrv_mem_dump(uint8_t *pPayload,uint32_t szpPayload);
uint32_t  espDrv_calc_chksum(uint8_t*data, uint32_t data_size);
void wifi_set_mac_adress(uint8_t *pMac);

extern uint32_t iEthRxCount;
extern uint32_t iEthTxCount;
extern uint32_t bSimulateDisconnect;
extern uint32_t wifiTraceLevel;





#endif
