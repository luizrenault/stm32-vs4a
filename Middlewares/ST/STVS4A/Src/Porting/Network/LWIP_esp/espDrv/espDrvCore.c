
/**
******************************************************************************
* @file    espDrvCore.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   driver CORE  API
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
  Implement API to format packets and  send receive message
  Implements also API to send command and wait response from the module
  Implement the Frame server to hook LWIP
  Implement TRACE message

*/


#include "espDrvCore.h"
#include "avs_wifi_private.h"
#include "espDrvGlueAvs.h"

#define AVS_SEND_CMD_TIMEOUT    (2000)  
#define MAX_COMMAND_RETRY       5

static com_notifier_t   tStackNotifier[MAX_NOTIFIER];
static uint16_t         gGlobalSequence;
static rx_packet_t      hRxPacket;
static uint8_t          tTxpacket[MAX_TX_BUFFER];
static osMutex_t        hPacketLock;
       uint32_t         wifiTraceLevel;


/* Just to make Keil happy  with empty sections  */

#if defined ( __GNUC__   ) ||  defined ( __CC_ARM   )
uint32_t  dummy_blk1[5] __attribute__((section(".sram2_non_cached_device")));/* Ethernet Rx MA Descriptor */
uint8_t   dummy_blk2[5][5] __attribute__((section(".sram2_non_cached_normal")));/* Ethernet Receive Buffer */
#endif



/**
 * @brief Set the espWifi level of trace
 *
 * @param lvl  combination of bits
 */
void  wifi_set_trace_level(uint32_t lvl)
{
  wifiTraceLevel = lvl;
}

/*
  Debug returns the message string
*/
#define TEXT_DEFINE(a) {#a,a}
struct {
  char_t*	 pmsg;
  uint32_t msg;
}
tTextMessage[] =
{
  TEXT_DEFINE(MSG_FW_VERSION),
  TEXT_DEFINE(MSG_FW_VERSION),
  TEXT_DEFINE(MSG_TRACE),
  TEXT_DEFINE(MSG_PING),
  TEXT_DEFINE(MSG_WIFI_CONNECTED),
  TEXT_DEFINE(MSG_WIFI_DISCONNECTED),
  TEXT_DEFINE(MSG_TRACE_ENABLE),
  TEXT_DEFINE(MSG_TRANSPORT_SPEED),
  TEXT_DEFINE(MSG_WIFI_SCAN),
  TEXT_DEFINE(MSG_WIFI_CONNECTED_SPOT),
  TEXT_DEFINE(MSG_WIFI_CONNECT),
  TEXT_DEFINE(MSG_WIFI_DISCONNECT),
  TEXT_DEFINE(MSG_LINKOUTPUT_FRAME_FROM_HOST),
  TEXT_DEFINE(MSG_INPUT_FRAME_FROM_ESP),
  TEXT_DEFINE(MSG_COM_SYNC),
  TEXT_DEFINE(MSG_GET_MAC_ADDRESS),
  { 0,0 }
};

/**
 * @brief Lock for a message string
 *
 * @param message  Message ID
 * @return char_t* String message
 */
char_t * espDrv_get_message_string(uint32_t message)
{
  uint32_t index = 0;
  for (index = 0; tTextMessage[index].pmsg; index++)
  {
    if (tTextMessage[index].msg == (message & MSG_MSK))
    {
      return tTextMessage[index].pmsg;
    }

  }
  return "Unknown";
}

/**
 * @brief notify a caller an pass the result
 *
 * @param message message
 * @param pPkt    packet
 * @param len     packet size
 */
void  espDrv_notify_callers(uint8_t message,frame_packet_header_t* pPkt,uint32_t len)
{
  for(uint32_t a = 0; a < MAX_NOTIFIER ; a++)
  {
    /* If the message and the sequence matches, we have to notify the caller than the message is returned from the module */
    uint32_t msg=  osAtomicRead(&tStackNotifier[a].message);
    
    if(msg != (uint32_t)-1)
    {
      if((msg ==  message) && (tStackNotifier[a].sequence == pPkt->sequence))
      {
        /* copy the return code */
        tStackNotifier[a].result = pPkt->result;
        if((tStackNotifier[a].pRetMsg != 0) && (tStackNotifier[a].pRetMsg->pMsg != 0))
        {
          /* copy the payload in the return handle */
          uint32_t szBlk = MIN(len - sizeof(frame_packet_header_t),tStackNotifier[a].pRetMsg->maxSzMsg);
          memcpy(tStackNotifier[a].pRetMsg->pMsg,(uint8_t *)&pPkt[1],szBlk );
        }
        if(tStackNotifier[a].eventNotifer)
        {
          /* signal the message is arrived */
          osEvent_Set(tStackNotifier[a].eventNotifer);
        }
        break;
      }
    }
  }
}

/**
 * @brief Create a notifier
 * notice the notification could  be used in an ISR, so we can't use a mutex, so we use an atomic to enable the notificiation entry that make sure the entry will be valide without issue
 * @param message   message
 * @param sequence  sequence
 * @param pNotify   RTOS object to signal when the message is notified
 * @param pRetMsg   option return payload
 * @return uint32_t the notifier index in the table
 */
uint32_t espDrv_notify_create(uint8_t message,uint8_t sequence,osEvent_t *pNotify,return_msg_t*  pRetMsg)
{
  int32_t index = -1;
  for(uint32_t a = 0; a < MAX_NOTIFIER ; a++)
  {
    /* look for a room */
    if(tStackNotifier[a].message == (uint32_t)-1)
    {
      tStackNotifier[a].sequence = sequence;
      tStackNotifier[a].result = 0;
      tStackNotifier[a].eventNotifer = pNotify;
      tStackNotifier[a].pRetMsg = pRetMsg;
      osAtomicWrite(&tStackNotifier[a].message,message);
      index  = a;
      break;
    }
  }
  WIFI_ASSERT(index != -1);
  return index;
}

/**
 * @brief Delete a notifier
 *
 * @param handle Index
 */

void     espDrv_notify_delete(uint32_t handle)
{
  osAtomicWrite(&tStackNotifier[handle].message,(uint32_t)-1);

}

/**
 * @brief Generate an sequence number. A sequence number is used to distinguish to message of the same value
 *
 * @return uint32_t the sequence number
 */
uint32_t espDrv_create_sequence_number(void);
uint32_t espDrv_create_sequence_number(void)
{
  /* The header is not encoded, so remove the possibility to have an ESC code */
  gGlobalSequence++;
  return   gGlobalSequence;
}

/**
 * @brief Compute the packet checksum
 *
 * @param data  Frame
 * @param data_size Frame size
 * @return uint32_t check sum computed
 */
uint32_t  espDrv_calc_chksum(uint8_t*data, uint32_t data_size)
{
  uint16_t cnt;
  uint32_t result;
  result = 0xEF;
  for (cnt = 0; cnt < data_size; cnt++)
  {
    result += data[cnt];
  }
  return result;
}
/**
 * @brief Lock the send  TX Packet
 *
 */
void espDrv_packet_lock(void)
{
  osMutex_Lock(&hPacketLock);
}
/**
 * @brief Unlock the send  Tx Packet
 *
 */
void espDrv_packet_unlock(void)
{
  osMutex_Unlock(&hPacketLock);
}

/**
 * @brief Send a packet, Create an header , compute the checksum
 *
 * @param message   Message type
 * @param sequence  Sequence num
 * @param result    Result error code
 * @param pPayload  Payload to send
 * @param len       Payload size to send
 * @return WIFI_Result Wifi error code
 */

WIFI_Result espDrv_send_packet(uint8_t message,uint16_t sequence,uint8_t  result, uint8_t *pPayload,uint32_t len)
{
  frame_packet_header_t *pPkt = (frame_packet_header_t *)&tTxpacket[4];
  espDrv_packet_lock();
  /* Create the header */
  pPkt->message = message;
  pPkt->sequence = sequence;
  pPkt->result   = result;
  pPkt->szPkt = len;
  WIFI_ASSERT(len < 1500);
  memcpy((uint8_t *)&pPkt[1],pPayload,len);
  uint32_t pktLen = len +  sizeof(frame_packet_header_t);
  /* compute the check sum */
  pPkt->chk_sum  = espDrv_calc_chksum((uint8_t *)&pPkt[1],pPkt->szPkt);
  /* Create the Level 1 header */
  tTxpacket[0] = (uint8_t )(PKT_START_CODE >> 8);
  tTxpacket[1] = (uint8_t )(PKT_START_CODE & 0xFF);
  tTxpacket[2] = (uint8_t )(pktLen >> 8);
  tTxpacket[3] = (uint8_t )(pktLen  & 0xFF);
  /* send the raw frame */
  espDrv_send_frame(tTxpacket, pktLen+4);
  espDrv_packet_unlock();
  return WIFI_OK;
}


/**
 * @brief Send a packet, Create an header , compute the checksum
 *
 * @param message   Message type
 * @param sequence  Sequence num
 * @param result    Result error code
 * @param p         LWIP buffer
 * @return WIFI_Result Wifi error code
 */

WIFI_Result espDrv_send_packet_pbuff(uint8_t message,uint16_t sequence,uint8_t  result, struct pbuf *p)
{
  espDrv_packet_lock();
  frame_packet_header_t *pPkt = (frame_packet_header_t *)&tTxpacket[4];
  /* Create the header */
  pPkt->message = message;
  pPkt->sequence = sequence;
  pPkt->result   = result;
  pPkt->szPkt = p->tot_len;
  WIFI_ASSERT(p->tot_len < MAX_PAYLOAD_FRAME);
  /* copy the buffer */
  struct pbuf *q=p;
  uint8_t *pPayload = (uint8_t *)&pPkt[1];
  while(q)
  {
    memcpy(pPayload ,q->payload,q->len);
    pPayload += q->len;
    q = q->next;
  }
  uint32_t pktLen = pPkt->szPkt +  sizeof(frame_packet_header_t);
  /* Create the Level 1 header */
  tTxpacket[0] = (uint8_t )(PKT_START_CODE >> 8);
  tTxpacket[1] = (uint8_t )(PKT_START_CODE & 0xFF);
  tTxpacket[2] = (uint8_t )(pktLen >> 8);
  tTxpacket[3] = (uint8_t )(pktLen  & 0xFF);
  /* Compute the checksum */
  pPkt->chk_sum  = espDrv_calc_chksum((uint8_t *)&pPkt[1],pPkt->szPkt);
  /* send the raw frame */
  espDrv_send_frame(tTxpacket, pktLen+4);
  espDrv_packet_unlock();
  return WIFI_OK;
}

/**
 * @brief Send a command to the esp8266 and wait for the response
 *
 * @param message message type
 * @param pPayload  payload
 * @param len       payload size
 * @param pRetMsg   optional return results
 * @return WIFI_Result Wifi error code
 */

WIFI_Result espDrv_send_ack(uint8_t message,uint8_t *pPayload,uint32_t len,return_msg_t*  pRetMsg)
{
  WIFI_Result ret=WIFI_NOT_AVAILABLE;
  uint32_t   sequence;
  uint32_t   retry = MAX_COMMAND_RETRY;
  osEvent_t  notifyMe;
  /* Create a notifier */
  sequence = espDrv_create_sequence_number();
  WIFI_VERIFY(osEvent_Create(&notifyMe));
  uint32_t hHandle = espDrv_notify_create(message,sequence,&notifyMe,pRetMsg);
  /* Send the message and wait the  response */

  while(retry-- && ret == WIFI_NOT_AVAILABLE)
  {
  /* Send the message */
     espDrv_send_packet(message,sequence,0,pPayload,len);

  /* Wait the response */
  if(osEvent_Wait(&notifyMe,AVS_SEND_CMD_TIMEOUT) == WIFI_ERROR)
  {
    WIFI_TRACE_ERROR("Notifier :  time-out %s",espDrv_get_message_string(message));
    ret = WIFI_TIMEOUT;
    goto exit;
  }
    if(tStackNotifier[hHandle].result == MSG_RESULT_OK)
  {
      /* exit next turn  */
      ret = WIFI_OK;
    }
    if(tStackNotifier[hHandle].result == MSG_RESULT_ERROR)
    {
      /* exit next turn  */
    ret = WIFI_ERROR;
  }

    if(tStackNotifier[hHandle].result == MSG_RESULT_CHKSUM)
    {
      WIFI_TRACE_WARNING("Packet checksum error: re-send the packet");
    }

  }
exit:
  espDrv_notify_delete(hHandle);
  osEvent_Delete(&notifyMe);
  return ret;
}

/**
 * @brief Send a command to the esp8266 and wait for the response
 *
 * @param message Message type
 * @param p   LWIP buffer
 * @param pRetMsg     optional return results
 * @return WIFI_Result  Wifi error code
 */

WIFI_Result espDrv_send_ack_pbuff(uint8_t message,struct pbuf *p,return_msg_t*  pRetMsg)
{
  WIFI_Result ret=WIFI_NOT_AVAILABLE;
  uint32_t   sequence;
  uint32_t   retry = MAX_COMMAND_RETRY;
  osEvent_t  notifyMe;

  /* Create a notifier */
  sequence = espDrv_create_sequence_number();
  WIFI_VERIFY(osEvent_Create(&notifyMe));
  uint32_t hHandle = espDrv_notify_create(message,sequence,&notifyMe,pRetMsg);

  while(retry-- && ret == WIFI_NOT_AVAILABLE)
  {
  /* send the message */
     espDrv_send_packet_pbuff(message,sequence,0,p);

  /* Wait the response */
  if(osEvent_Wait(&notifyMe,AVS_SEND_CMD_TIMEOUT) == WIFI_ERROR)
  {
    WIFI_TRACE_ERROR("Notifier :  time-out %s",espDrv_get_message_string(message));
    ret = WIFI_TIMEOUT;
    goto exit;
  }
    if(tStackNotifier[hHandle].result == MSG_RESULT_OK)
  {
      /* exit next turn  */
      ret = WIFI_OK;
    }
    if(tStackNotifier[hHandle].result == MSG_RESULT_ERROR)
    {
      /* exit next turn  */
    ret = WIFI_ERROR;
  }

    if(tStackNotifier[hHandle].result == MSG_RESULT_CHKSUM)
    {
      WIFI_TRACE_WARNING("Packet checksum error: re-send the packet");
    }

  }
exit:
  espDrv_notify_delete(hHandle);
  osEvent_Delete(&notifyMe);
  return ret;
}

/**
 * @brief Send a sync frame to check if the Wifi module is online
 *
 * @return WIFI_Result
 */
WIFI_Result espDrv_send_sync()
{

  WIFI_Result ret=WIFI_OK;
  osEvent_t  notifyMe;
  static const uint8_t sync_frame[] =
  { 0x07, 0x07, 0x12,0x20,
  0x55, 0x55, 0x55, 0x55, 0x55,
  0x55, 0x55, 0x55, 0x55, 0x55,
  0x55, 0x55, 0x55, 0x55, 0x55,
  0x55, 0x55, 0x55, 0x55, 0x55,
  0x55, 0x55, 0x55, 0x55, 0x55,
  0x55, 0x55, 0x55, 0x55, 0x55,
  0x55, 0x55
  };
  static uint8_t sync_frameR[sizeof(sync_frame)];
  return_msg_t hMsg ;
  hMsg.pMsg = sync_frameR;
  hMsg.maxSzMsg = sizeof(sync_frameR);;
   
  WIFI_VERIFY(osEvent_Create(&notifyMe));
  uint32_t hHandle = espDrv_notify_create(MSG_COM_SYNC,0,&notifyMe,&hMsg);

  /* Wait the response */
  if(osEvent_Wait(&notifyMe,SYNC_TIMEOUT) == WIFI_ERROR)
  {
    ret = WIFI_TIMEOUT;
    goto exit;
  }
  if(tStackNotifier[hHandle].result != 0)
  {
    ret = WIFI_ERROR;
  }
  
  if(memcmp(sync_frameR,sync_frame,sizeof(sync_frame))== 0)
  {
    ret = WIFI_OK;
  }
  
exit:
  espDrv_notify_delete(hHandle);
  osEvent_Delete(&notifyMe);
  return ret;
}


/**
 * @brief Send a command with only a uint32 as a payload
 *
 * @param message message type
 * @param word the word
 * @param pRetMsg optional return results
 * @return WIFI_Result Wifi error code
 */
WIFI_Result espDrv_send_ack_word(uint8_t message,uint32_t word,return_msg_t*  pRetMsg)
{
  return espDrv_send_ack(message,(uint8_t *)&word,sizeof(word),pRetMsg);
}

/**
 * @brief RX frame parser
 *
 * @param pPayload  Raw payload
 * @param szPayload Raw payload size
 * @return uint32_t Wifi error code
 */

uint32_t com_rx_pkt_parser(void *pPayload, uint32_t szPayload)
{
  frame_packet_header_t *pPkt = pPayload;
  /* Check the checksum */
  uint32_t chSum = espDrv_calc_chksum((uint8_t *)&pPkt[1],pPkt->szPkt);
  if(pPkt->chk_sum != chSum )
  {
    pPkt->result = MSG_RESULT_CHKSUM;
    WIFI_TRACE_ERROR("RX Packet Sum  error");
  }
  /* Send the packet to  the event handler */
  espDrv_parse_event(pPkt,szPayload);
  return TRUE;
}

/**
 * @brief Scan hot spots
 *
 * @param pString buffer that will receive the list of spots
 * @param maxSize buffer size
 * @return WIFI_Result Wifi error code
 */
WIFI_Result wifi_post_scan(char_t *pString, uint32_t maxSize)
{
  return_msg_t  rcvMsg;
  rcvMsg.maxSzMsg =maxSize-1;
  rcvMsg.pMsg =(uint8_t *)pString;
  return espDrv_send_ack(MSG_WIFI_SCAN,0,0,&rcvMsg);
}

/**
 * @brief Return the connected spot or an empty string
 *
 * @param pString  buffer string
 * @param maxSize  buffer string size
 * @return WIFI_Result Wifi error code
 */
WIFI_Result wifi_get_connected_spot(char_t *pString, uint32_t maxSize)
{
  return_msg_t  rcvMsg;
  rcvMsg.maxSzMsg =maxSize-1;
  rcvMsg.pMsg =(uint8_t *)pString;
  return espDrv_send_ack(MSG_WIFI_CONNECTED_SPOT,0,0,&rcvMsg);
}


/**
 * @brief Task of debug test ( stress the com)
 *
 * @param argument unused
 */
static  void      espDrv_Tx_stress_task(void  const * argument)
{
  while(1)
  {
    espDrv_send_ack(MSG_TX_STRESS,(uint8_t *)"Stress Packet",sizeof("Stress Packet"),NULL);
  }
}

/**
 * @brief Start the debug test stress the com
 *
 */

void  espDrv_Tx_stress_start()
{
  osTask_Create("stress",espDrv_Tx_stress_task,NULL,500,osTaskPriorityNormal);
}

/**
 * @brief Connect the hot spot according to the connection mode
 * 1) Get the Mac address (AP or STA )
 * 2) Send the command
 * 3) Inform LWIP about the mac address change
 *
 * @param state
 * @param mode
 * @return WIFI_Result
 */

WIFI_Result wifi_connect(uint32_t state,wifi_mode_e mode)
{
  WIFI_Result ret;
  return_msg_t param;
  uint8_t     tMac[6];
  param.pMsg      = tMac;
  param.maxSzMsg = sizeof(tMac);

  char_t *pInfo = osMalloc(1000);
  WIFI_ASSERT(pInfo);
  char_t *pCtx= wifi_get_connection_info();
  if(state && pInfo && pCtx)
  {
    /* format the message */
    snprintf(pInfo,1000,"mode:'%s' %s", mode == ESP_WIFI_STA ? "STA" : "AP",pCtx);
    ret = espDrv_send_ack(MSG_WIFI_CONNECT,(uint8_t *)pInfo ,strlen(pInfo)+1,NULL);
    if(ret == WIFI_OK)
    {
      ret = espDrv_send_ack_word(MSG_GET_MAC_ADDRESS,mode,&param);
      if(ret == WIFI_OK)
      {
        wifi_set_mac_adress(tMac);
      }
    }
  }
  else
  {
    ret = espDrv_send_ack(MSG_WIFI_DISCONNECT,0,0,NULL);
  }
  osFree(pInfo);
  return ret;
}

/**
 * @brief Init the Network Stack
 *
 * @return uint32_t
 */
uint32_t     wifi_network_init()
{
	/* Just to make Keil happy  with empty sections  */

#if defined ( __GNUC__   ) ||  defined ( __CC_ARM   )
		memset(dummy_blk1,0,sizeof(dummy_blk1));
		memset(dummy_blk2,0,sizeof(dummy_blk2));
#endif
	
	
  /* set the internal Debug level */
  wifi_set_trace_level(DRV_TRACE_LEVEL);

  /* Init the RX parser */
  memset(&hRxPacket,0,sizeof(hRxPacket));
  hRxPacket.rxCb = com_rx_pkt_parser;
  WIFI_VERIFY(osMutex_Create(&hPacketLock));

  /* Configuration espDrv  */
  if (espDrv_init(COM_CONFIG_DEFAULT,&hRxPacket) ==WIFI_ERROR)
  {
    return WIFI_ERROR;
  }
  /* RAZ notifier */
  memset(&tStackNotifier,-1,sizeof(tStackNotifier));

  /* Check the module presence */
  uint32_t  wifiState = FALSE;

  WIFI_TRACE_VERBOSE("Start Wifi COM Syncing .... ");
  uint32_t tryCount = 0;
  for(tryCount  = 0;   tryCount  < NB_TRY_ESP_SYNC &&   wifiState ==FALSE;tryCount ++)
  {
    if(espDrv_send_sync() == WIFI_OK)
    {
      wifiState  = TRUE;
      /* signal the sync is OK */
      hRxPacket.bSync = FALSE;
      espDrv_send_packet(MSG_COM_SYNC,0,MSG_RESULT_OK, 0,0);
      break;
    }
  }
  if(wifiState ==TRUE)
  {

    /*  Enable Module Traces level*/
    if(espDrv_send_ack_word(MSG_TRACE_ENABLE,ESP_TRACE_LEVEL,0) != WIFI_OK)
    {
      WIFI_TRACE_ERROR("Change Level Trace");
    }

    /*  Get FW version */
    char_t tResponse[100];
    return_msg_t ret;
    ret.pMsg = (uint8_t *)tResponse;
    ret.maxSzMsg = sizeof(tResponse);
    if(espDrv_send_ack(MSG_FW_VERSION,NULL,0,&ret) == WIFI_OK)
    {
      WIFI_TRACE_VERBOSE("Firmware revision: %s",tResponse);
    }

#ifdef CHANGE_COM_HIGH_SPEED
    /* Change the transport rate   */
    uint32_t newBaudRate = CHANGE_COM_HIGH_SPEED;
    if(espDrv_send_ack_word(MSG_TRANSPORT_SPEED,newBaudRate,NULL) == WIFI_OK)
    {
      /* Wait the execution message */
      osTaskDelay(100);
      /* Change the rate */
      if (espDrv_change_baudrate(newBaudRate) ==WIFI_ERROR)
      {
        WIFI_TRACE_VERBOSE("Transport HS error");
        return WIFI_ERROR;
      }
      /* Wait to prevent garbage packet during the transition */
      osTaskDelay(1000);
    /*  Re-Check if the module is on-line  and the COM is OK*/
    espDrv_send_ack(MSG_PING,NULL,0,0);
    }

#endif
    /* Start the connection using the current spot registered */
    wifi_connect(TRUE,ESP_WIFI_STA);
  }
  /* Init LWIP using the Uart NetIF */
  return wifi_network_lwip_init(wifiState);
}
/**
 * @brief Terminate network Wifi session
 *
 * @return uint32_t Wifi error code
 */

uint32_t wifi_network_term()
{
  /* Term com */
  espDrv_term();
  osMutex_Delete(&hPacketLock);
  wifi_network_lwip_term();
  return WIFI_OK;
}
