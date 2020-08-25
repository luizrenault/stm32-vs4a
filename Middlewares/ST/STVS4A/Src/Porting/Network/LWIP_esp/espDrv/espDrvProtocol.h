/*******************************************************************************
* @file    espDrvProtocol.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Communication protocol between host and module
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


#ifndef _espDrvProtocol_h
#define _espDrvProtocol_h

#define NETIF_MTU         1500
#define MAC_SIZE          6
#define FCS_SIZE          4
#define MAX_PAYLOAD_FRAME       (NETIF_MTU +2*MAC_SIZE + FCS_SIZE)
#define PKT_HEADER_SIZE 4
#define MAX_RX_BUFFER		(MAX_PAYLOAD_FRAME + sizeof(frame_packet_header_t)+PKT_HEADER_SIZE) 
#define MAX_TX_BUFFER		(MAX_RX_BUFFER)
#define SZ_RX_RING_BUFFER	(8*MAX_RX_BUFFER)
#define MAX_SSID_NAME       		30
#define MAX_PASS_NAME       		30

#define NO_DISABLE_NETWORK_COM


#define MSG_MSK 			    0x3F	/* isolate the primary message */
#define MSG_FROM_ESP			0x80	/* Send a response to the corresponding message */
#define MSG_ACK		        	0x40	/* the message Ack must processed immediatly*/

typedef enum
{
	MSG_FW_VERSION=1,		     /* Return the firmware version, returns a string */
	MSG_TRACE,					/* Trace */
	MSG_PING,					/* Test the com between the host and the module, must return  MSG_RESULT_OK*/
	MSG_WIFI_CONNECTED,			/* Station connected */
	MSG_WIFI_DISCONNECTED,		/* Station disconnected */
	MSG_TRACE_ENABLE,			/* Enable/disable Trace */
	MSG_TRANSPORT_SPEED,		/* Change the transport speed UART/I2C/etc...*/
	MSG_WIFI_SCAN,				/* scan the Wifi and return the result */
	MSG_WIFI_CONNECTED_SPOT,	/* return the name of the connected spot*/
	MSG_WIFI_CONNECT,			/* connect the station */
	MSG_WIFI_DISCONNECT,		/* disconnect the station */
	MSG_LINKOUTPUT_FRAME_FROM_HOST, /* IP output from the host */
	MSG_INPUT_FRAME_FROM_ESP,		/* IP input  from the host */
	MSG_COM_SYNC,	                /* Sync esp and host */
	MSG_GET_MAC_ADDRESS,			/* Get Mac address */
	MSG_TX_STRESS,
	MSG_RX_STRESS,
	MSG_RX_STRESS_START,
	MSG_SCAN_RESULT,
	MSG_MEM_DUMP,
	MSG_END
}espmodule_message_t;

#define WIFI_TRACE_LVL_ERROR	1U
#define WIFI_TRACE_LVL_WARNING	2U
#define WIFI_TRACE_LVL_VERBOSE	4U
#define WIFI_TRACE_LVL_FRAME    8U
#define WIFI_TRACE_LVL_DEFAULT   (WIFI_TRACE_LVL_ERROR | WIFI_TRACE_LVL_WARNING )
#define USE_DEBUG
#if defined(USE_DEBUG)
#define WIFI_TRACE_WARNING(...)        wifi_trace(WIFI_TRACE_LVL_WARNING,TRUE,__VA_ARGS__)
#define WIFI_TRACE_ERROR(...)          wifi_trace(WIFI_TRACE_LVL_ERROR,TRUE,__VA_ARGS__)
#define WIFI_TRACE_VERBOSE(...)        wifi_trace(WIFI_TRACE_LVL_VERBOSE,TRUE,__VA_ARGS__)
#define WIFI_TRACE_FRAME(...)          wifi_trace(WIFI_TRACE_LVL_FRAME,TRUE,__VA_ARGS__)
#define WIFI_ERROR_FATAL(...)          wifi_trace(WIFI_TRACE_LVL_ERROR,TRUE,__VA_ARGS__);while(1);
#else
#define WIFI_TRACE_ERROR(...)           wifi_trace(WIFI_TRACE_LVL_ERROR,TRUE,__VA_ARGS__)
#define WIFI_TRACE_WARNING(...)         ((void)0)
#define WIFI_TRACE_VERBOSE(...)		((void)0)
#define WIFI_ERROR_FATAL(...)            wifi_trace(WIFI_TRACE_LVL_ERROR,TRUE,__VA_ARGS__);while(1);
#endif

void		wifi_trace(uint32_t msg, uint32_t cr, const char  *pFormat, ...);
void		wifi_set_trace_level(uint32_t lvl);

#define 	MSG_RESULT_OK				0		/* Response OK code */
#define 	MSG_RESULT_ERROR			1		/* Response ERROR code */
#define 	MSG_RESULT_CHKSUM			2		/* Response ERROR code */


#define  SIZE_IP_ADDR				4                      /* 4 digits for IP V4 */
#define  PKT_START_CODE            ((uint16_t)0x5354)      /* Signature ST */

typedef struct    t_frame_packet_header
{
        uint8_t 	 message;	/* packet message */
        uint16_t	 szPkt;		/* packet size */
        uint8_t 	 result;	/* result error, sum etc.. */
        uint16_t 	 sequence;  /* sequence number */
        uint32_t	 chk_sum; 	/* packet checksum  */
}frame_packet_header_t;




#endif
