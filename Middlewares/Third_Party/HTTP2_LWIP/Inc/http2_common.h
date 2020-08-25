/**
 * @file http2_common.h
 * @brief Functions common to HTTP/2 client and server
 *
 * Copyright (C) 2010-2017 Oryx Embedded SARL. All rights reserved. 
 * 
 * @author Oryx Embedded SARL (www.oryx-embedded.com) 
 * @version 1.3.1 
 **/

/**
  ******************************************************************************
  * @attention
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#ifndef _HTTP2_COMMON_H
#define _HTTP2_COMMON_H

//Dependencies
#include "http2_client_config.h"
#include "http2_compiler_port.h"

//Initial value of SETTINGS_HEADER_TABLE_SIZE parameter
#define HTTP2_HEADER_TABLE_SIZE_DEF_VALUE 4096U

//Initial value of SETTINGS_ENABLE_PUSH parameter
#define HTTP2_ENABLE_PUSH_DEF_VALUE 1U

//Initial value of SETTINGS_MAX_CONCURRENT_STREAMS parameter
#define HTTP2_MAX_CONCURRENT_STREAMS_DEF_VALUE 4294967295U

//Initial value of SETTINGS_INITIAL_WINDOW_SIZE parameter
#define HTTP2_INITIAL_WINDOW_SIZE_DEF_VALUE 65535U
//Maximum value of SETTINGS_MAX_HEADER_LIST_SIZE parameter
#define HTTP2_INITIAL_WINDOW_SIZE_MAX_VALUE 2147483647U

//Initial value of SETTINGS_MAX_FRAME_SIZE parameter
#define HTTP2_MAX_FRAME_SIZE_DEF_VALUE 16384U
//Minimum value of SETTINGS_MAX_FRAME_SIZE parameter
#define HTTP2_MAX_FRAME_SIZE_MIN_VALUE 16384U
//Maximum value of SETTINGS_MAX_FRAME_SIZE parameter
#define HTTP2_MAX_FRAME_SIZE_MAX_VALUE 16777215U

//Initial value of SETTINGS_MAX_HEADER_LIST_SIZE parameter
#define HTTP2_MAX_HEADER_LIST_SIZE_DEF_VALUE 4294967295U

//Maximum window size
#define HTTP2_WINDOW_SIZE_MAX_VALUE 2147483647U

//Stream ID mask
#define HTTP2_STREAM_ID_MASK 0x7FFFFFFFU
//Window size increment mask
#define HTTP2_WINDOW_SIZE_INC_MASK 0x7FFFFFFFU

//Payload size of PING frames
#define HTTP2_PING_PAYLOAD_SIZE 8U

//C++ guard
#ifdef __cplusplus
   extern "C" {
#endif


/**
 * @brief Stream states
 **/

typedef enum
{
   HTTP2_STREAM_STATE_UNUSED             = 0U,
   HTTP2_STREAM_STATE_IDLE               = 1U,
   HTTP2_STREAM_STATE_RESERVED_LOCAL     = 2U,
   HTTP2_STREAM_STATE_RESERVED_REMOTE    = 3U,
   HTTP2_STREAM_STATE_OPEN               = 4U,
   HTTP2_STREAM_STATE_HALF_CLOSED_LOCAL  = 5U,
   HTTP2_STREAM_STATE_HALF_CLOSED_REMOTE = 6U,
   HTTP2_STREAM_STATE_CLOSED             = 7U,
   HTTP2_STREAM_STATE_ERROR              = 8U
} Http2StreamState;


/**
 * @brief HTTP request/response exchange states
 **/

typedef enum
{
   HTTP2_EXCHANGE_STATE_PP_HEADER        = 0U,
   HTTP2_EXCHANGE_STATE_REQ_HEADER       = 1U,
   HTTP2_EXCHANGE_STATE_REQ_BODY         = 2U,
   HTTP2_EXCHANGE_STATE_REQ_TRAILER      = 3U,
   HTTP2_EXCHANGE_STATE_RESP_HEADER      = 4U,
   HTTP2_EXCHANGE_STATE_RESP_BODY        = 5U,
   HTTP2_EXCHANGE_STATE_RESP_TRAILER     = 6U
} Http2ExchangeState;


/**
 * @brief HTTP/2 frame types
 **/

typedef uint_t Http2FrameType;

#define HTTP2_FRAME_TYPE_DATA          0x00U
#define HTTP2_FRAME_TYPE_HEADERS       0x01U
#define HTTP2_FRAME_TYPE_PRIORITY      0x02U
#define HTTP2_FRAME_TYPE_RST_STREAM    0x03U
#define HTTP2_FRAME_TYPE_SETTINGS      0x04U
#define HTTP2_FRAME_TYPE_PUSH_PROMISE  0x05U
#define HTTP2_FRAME_TYPE_PING          0x06U
#define HTTP2_FRAME_TYPE_GOAWAY        0x07U
#define HTTP2_FRAME_TYPE_WINDOW_UPDATE 0x08U
#define HTTP2_FRAME_TYPE_CONTINUATION  0x09U


/**
 * @brief HTTP/2 frame flags
 **/

typedef uint_t Http2Flags;

#define HTTP2_FRAME_FLAG_ACK         0x01U
#define HTTP2_FRAME_FLAG_END_STREAM  0x01U
#define HTTP2_FRAME_FLAG_END_HEADERS 0x04U
#define HTTP2_FRAME_FLAG_PADDED      0x08U
#define HTTP2_FRAME_FLAG_PRIORITY    0x20U


/**
 * @brief HTTP/2 setting identifiers
 **/

typedef uint_t Http2SettingId;

#define HTTP2_SETTINGS_HEADER_TABLE_SIZE      0x01U
#define HTTP2_SETTINGS_ENABLE_PUSH            0x02U
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS 0x03U
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE    0x04U
#define HTTP2_SETTINGS_MAX_FRAME_SIZE         0x05U
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE   0x06U


/**
 * @brief HTTP/2 error codes
 **/

typedef enum
{
   HTTP2_NO_ERROR                  = 0x00U, ///<Graceful shutdown
   HTTP2_ERROR_PROTOCOL            = 0x01U, ///<Protocol error detected
   HTTP2_ERROR_INTERNAL            = 0x02U, ///<Implementation fault
   HTTP2_ERROR_FLOW_CONTROL        = 0x03U, ///<Flow-control limits exceeded
   HTTP2_ERROR_SETTINGS_TIMEOUT    = 0x04U, ///<Settings not acknowledged
   HTTP2_ERROR_STREAM_CLOSED       = 0x05U, ///<Frame received for closed stream
   HTTP2_ERROR_FRAME_SIZE          = 0x06U, ///<Frame size incorrect
   HTTP2_ERROR_REFUSED_STREAM      = 0x07U, ///<Stream not processed
   HTTP2_ERROR_CANCEL              = 0x08U, ///<Stream cancelled
   HTTP2_ERROR_COMPRESSION         = 0x09U, ///<Compression state not updated
   HTTP2_ERROR_CONNECT             = 0x0AU, ///<TCP connection error for CONNECT method
   HTTP2_ERROR_ENHANCE_YOUR_CALM   = 0x0BU, ///<Processing capacity exceeded
   HTTP2_ERROR_INADEQUATE_SECURITY = 0x0CU, ///<Negotiated TLS parameters not acceptable
   HTTP2_ERROR_HTTP_1_1_REQUIRED   = 0x0DU  ///<Use HTTP/1.1 for the request
} Http2ErrorCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief HTTP/2 frame format
 **/

typedef __start_packed struct
{
   uint8_t length[3]; //0-2
   uint8_t type;      //3
   uint8_t flags;     //4
   uint32_t streamId; //5-8
   uint8_t payload[]; //9
} __end_packed Http2Frame;


/**
 * @brief HTTP/2 setting format
 **/

typedef __start_packed struct
{
   uint16_t id;    //0-1
   uint32_t value; //2-5
} __end_packed Http2Setting;


/**
 * @brief PRIORITY payload format
 **/

typedef __start_packed struct
{
   uint32_t streamDependency; //0-3
   uint8_t weight;            //4
} __end_packed Http2PriorityPayload;


/**
 * @brief GOWAWAY payload format
 **/

typedef __start_packed struct
{
   uint32_t lastStreamId;     //0-3
   uint32_t errorCode;        //4-7
   uint8_t additionnalData[]; //8
} __end_packed Http2GoAwayPayload;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief HTTP/2 settings
 **/

typedef struct
{
   uint32_t headerTableSize;
   uint32_t enablePush;
   uint32_t maxConcurrentStreams;
   uint32_t initialWindowSize;
   uint32_t maxFrameSize;
   uint32_t maxHeaderListSize;
} Http2Settings;


/**
 * @brief HTTP/2 frame decoding context
 **/

typedef struct
{
   uint8_t type;
   uint8_t flags;
   uint32_t streamId;
} Http2FrameContext;


//C++ guard
#ifdef __cplusplus
   }
#endif

#endif
