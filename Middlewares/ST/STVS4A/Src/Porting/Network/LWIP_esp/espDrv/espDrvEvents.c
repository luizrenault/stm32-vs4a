/**
******************************************************************************
* @file    espDrvEvents.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Manage events coming from the RX line and parse string then dispatch & process
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
  Implement the processing of message coming from the module

*/

#include <avs_private.h>
#include "espDrvCore.h"

/**
 * @brief Process message from the module
 *
 * @param pPkt  Packet header
 * @param len   Packet len
 */

void espDrv_parse_event(frame_packet_header_t *pPkt, uint32_t len)
{
  /* process the message */
  uint8_t *pPayload = (uint8_t *)&pPkt[1];
  int32_t szpPayload = pPkt->szPkt;
  WIFI_ASSERT(szpPayload >= 0);
  /* parse the packet from the message */
  uint8_t message = pPkt->message;
  uint32_t result = pPkt->result;

  /* a packet response has always the flag MSG_RESPONSE */
  if ((message & MSG_FROM_ESP) == 0)
  {
    TRACE_ERROR("Response corrupted");
    return;
  }
  /* Remove flags */
  message &= MSG_MSK;
  /* Parses the  messages */
  switch (message)
  {
  case MSG_TRACE:
  {
    /* Reflect a Trace from the module to the console  */
    wifi_trace((uint32_t)-1,FALSE,"Esp: %s", pPayload);
    break;
  }
  case MSG_WIFI_CONNECTED:
  {
    espDrv_notify_wifi_connected(TRUE);
    break;
  }

  case MSG_WIFI_DISCONNECTED:
  {
    espDrv_notify_wifi_connected(FALSE);
    break;
  }

    /* Message handled but do nothing at the event level */
  case MSG_FW_VERSION:
  case MSG_WIFI_CONNECTED_SPOT:
  case MSG_WIFI_CONNECT:
  case MSG_WIFI_DISCONNECT:
  case MSG_TRACE_ENABLE:
  case MSG_WIFI_SCAN:
  case MSG_COM_SYNC:
  case MSG_TRANSPORT_SPEED:
  case MSG_GET_MAC_ADDRESS:
  case MSG_PING:
  case MSG_LINKOUTPUT_FRAME_FROM_HOST:
    break;

  case MSG_SCAN_RESULT:
  {
    if (pPkt->result != MSG_RESULT_OK)
    {
      espDrv_notify_scan_result(NULL);
    }
    else
    {
      espDrv_notify_scan_result((char_t *)pPayload);
    }
    break;
  }
  case MSG_TX_STRESS:
  {
    static uint32_t cpt = 0;
    WIFI_TRACE_VERBOSE("STX:%d", cpt++);
  }
  break;

  case MSG_RX_STRESS:
  {
    if (result == MSG_RESULT_OK)
    {
      static uint32_t cpt = 0;
      WIFI_TRACE_VERBOSE("SRX:%d", cpt++);
    }
    espDrv_send_packet(message, pPkt->sequence, result, 0, 0);
    break;
  }

  case MSG_INPUT_FRAME_FROM_ESP:
  {
#ifndef DISABLE_NETWORK_COM
    netif_inject_input_frame(pPayload, szpPayload);
#endif
    //       espDrv_send_packet(message, pPkt->sequence, ret  == TRUE ? MSG_RESULT_OK : MSG_RESULT_ERROR , 0, 0);
  }
  break;

  default:
  {
    AVS_Dump(AVS_TRACE_LVL_NETWORK, "Packet corrupted", pPkt, sizeof(*pPkt));
    break;
  }
  }
  /* Ack listener*/
  espDrv_notify_callers(message, pPkt, len);
}
