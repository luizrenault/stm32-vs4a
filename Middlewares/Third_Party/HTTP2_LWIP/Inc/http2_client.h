/**
 * @file http2_client.h
 * @brief HTTP/2 client
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

#ifndef _HTTP2_CLIENT_H
#define _HTTP2_CLIENT_H

//Dependencies
#include "cmsis_os.h"
#include "http2_client_config.h"
#include "http2_compiler_port.h"
#include "http2_common.h"
#include "http2_status.h"
#include "hpack.h"

//HTTP/2 client support
#ifndef HTTP2_CLIENT_SUPPORT
   #define HTTP2_CLIENT_SUPPORT ENABLED
#elif (HTTP2_CLIENT_SUPPORT != ENABLED && HTTP2_CLIENT_SUPPORT != DISABLED)
   #error HTTP2_CLIENT_SUPPORT parameter is not valid
#endif

//HTTP/2 over SSL/TLS
#ifndef HTTP2_CLIENT_TLS_SUPPORT
   #define HTTP2_CLIENT_TLS_SUPPORT ENABLED
#elif (HTTP2_CLIENT_TLS_SUPPORT != ENABLED && HTTP2_CLIENT_TLS_SUPPORT != DISABLED)
   #error HTTP2_CLIENT_TLS_SUPPORT parameter is not valid
#endif

//Server push support
#ifndef HTTP2_CLIENT_PUSH_PROMISE_SUPPORT
   #define HTTP2_CLIENT_PUSH_PROMISE_SUPPORT DISABLED
#elif (HTTP2_CLIENT_PUSH_PROMISE_SUPPORT != ENABLED && HTTP2_CLIENT_PUSH_PROMISE_SUPPORT != DISABLED)
   #error HTTP2_CLIENT_PUSH_PROMISE_SUPPORT parameter is not valid
#endif

//Multipart content type support
#ifndef HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT
   #define HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT ENABLED
#elif (HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT != ENABLED && HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT != DISABLED)
   #error HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT parameter is not valid
#endif

//Maximum number of concurrently open streams
#ifndef HTTP2_CLIENT_MAX_STREAMS
   #define HTTP2_CLIENT_MAX_STREAMS 4U
#elif (HTTP2_CLIENT_MAX_STREAMS < 1U)
   #error HTTP2_CLIENT_MAX_STREAMS parameter is not valid
#endif

//Default timeout
#ifndef HTTP2_CLIENT_DEFAULT_TIMEOUT
   #define HTTP2_CLIENT_DEFAULT_TIMEOUT 30000U
#elif (HTTP2_CLIENT_DEFAULT_TIMEOUT < 1000U)
   #error HTTP2_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Size of the HTTP/2 client buffer
#ifndef HTTP2_CLIENT_BUFFER_SIZE
   #define HTTP2_CLIENT_BUFFER_SIZE 1024U
#elif (HTTP2_CLIENT_BUFFER_SIZE < 1U)
   #error HTTP2_CLIENT_BUFFER_SIZE parameter is not valid
#endif

//Size of the HTTP/2 stream buffer
#ifndef HTTP2_CLIENT_STREAM_BUFFER_SIZE
   #define HTTP2_CLIENT_STREAM_BUFFER_SIZE 4096U
#elif (HTTP2_CLIENT_STREAM_BUFFER_SIZE < 1U)
   #error HTTP2_CLIENT_STREAM_BUFFER_SIZE parameter is not valid
#endif

//Maximum length for path
#ifndef HTTP2_CLIENT_PATH_MAX_LEN
   #define HTTP2_CLIENT_PATH_MAX_LEN 64U
#elif (HTTP2_CLIENT_PATH_MAX_LEN < 1U)
   #error HTTP2_CLIENT_PATH_MAX_LEN parameter is not valid
#endif

//Maximum length for boundary string
#ifndef HTTP2_CLIENT_BOUNDARY_MAX_LEN
   #define HTTP2_CLIENT_BOUNDARY_MAX_LEN 70U
#elif (HTTP2_CLIENT_BOUNDARY_MAX_LEN < 1U)
   #error HTTP2_CLIENT_BOUNDARY_MAX_LEN parameter is not valid
#endif

//Maximum length for content type
#ifndef HTTP2_CLIENT_CONTENT_TYPE_MAX_LEN
   #define HTTP2_CLIENT_CONTENT_TYPE_MAX_LEN 32U
#elif (HTTP2_CLIENT_CONTENT_TYPE_MAX_LEN < 1U)
   #error HTTP2_CLIENT_CONTENT_TYPE_MAX_LEN parameter is not valid
#endif

//SSL/TLS supported?
#if (HTTP2_CLIENT_TLS_SUPPORT == ENABLED)

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/ssl.h"

#endif

//Forward declaration of Http2ClientContext structure
struct _Http2ClientContext;
#define Http2ClientContext struct _Http2ClientContext

//Forward declaration of Http2ClientStream structure
struct _Http2ClientStream;
#define Http2ClientStream struct _Http2ClientStream

//Dependencies
#include "http2_client_stream.h"

//C++ guard
#ifdef __cplusplus
   extern "C" {
#endif


/**
 * @brief HTTP/2 client states
 **/

typedef enum
{
   HTTP2_CLIENT_STATE_DISCONNECTED  = 0U,
   HTTP2_CLIENT_STATE_CONNECTING    = 1U,
   HTTP2_CLIENT_STATE_CONNECTED     = 2U,
   HTTP2_CLIENT_STATE_DISCONNECTING = 3U
} Http2ClientState;


/**
 * @brief Flags used by read and write functions
 **/

typedef uint_t Http2ClientFlags;

#define HTTP2_CLIENT_FLAG_END_STREAM 0x0100U
#define HTTP2_CLIENT_FLAG_WAIT_ALL   0x0800U
#define HTTP2_CLIENT_FLAG_BREAK_CHAR 0x1000U
#define HTTP2_CLIENT_FLAG_BREAK_CRLF 0x100AU
#define HTTP2_CLIENT_FLAG_NO_DELAY   0x4000U
#define HTTP2_CLIENT_FLAG_DELAY      0x8000U


//The HTTP2_CLIENT_FLAG_BREAK macro causes the read function to stop reading
//data whenever the specified break character is encountered
#define HTTP2_CLIENT_FLAG_BREAK(c) (HTTP2_CLIENT_FLAG_BREAK_CHAR | LSB(c))


/**
 * @brief Generic network socket
 **/

typedef int_t Http2Socket;


/**
 * @brief Generic socket address
 **/

typedef void *Http2SocketAddr;


//SSL/TLS supported?
#if (HTTP2_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief SSL/TLS configuration parameters
 **/

typedef mbedtls_ssl_config Http2TlsConfig;


/**
 * @brief SSL/TLS context
 **/

typedef mbedtls_ssl_context Http2TlsContext;


/**
 * @brief SSL/TLS initialization callback function
 **/

typedef Http2Status (*Http2ClientTlsInitCallback)(Http2ClientContext *context,
   Http2TlsConfig *tlsConfig, Http2TlsContext *tlsContext);

#endif


/**
 * @brief Server push callback function
 **/

typedef Http2Status (*Http2ClientPushCallback)(Http2ClientContext *context,
   Http2ClientStream *stream);


/**
 * @brief HTTP/2 client context
 **/

struct _Http2ClientContext
{
   osMutexId mutex;
   osSemaphoreId event;
   Http2ClientState state;                             ///<Connection state
   Http2Settings localSettings;                        ///<Local configuration options
   Http2Settings peerSettings;                         ///<Peer configuration options
   systime_t timeout;                                  ///<connection timeout, in milliseconds
   systime_t keepAlive;                                ///<Keep-alive value, in milliseconds
   Http2Socket socket;                                 ///<Underlying TCP socket
   HpackContext hpackContext;                          ///<HPACK context
   Http2ErrorCode errorCode;                           ///<Error code
   uint32_t txStreamId;                                ///<Current stream identifier (client-initiated streams)
   uint32_t rxStreamId;                                ///<Current stream identifier (server-initiated streams)
   Http2ClientStream stream[HTTP2_CLIENT_MAX_STREAMS]; ///<Streams
   uint8_t buffer[HTTP2_CLIENT_BUFFER_SIZE];           ///<Internal buffer
   size_t bufferLen;                                   ///<Current length of the buffer, in bytes
#if (HTTP2_CLIENT_TLS_SUPPORT == ENABLED)
   Http2TlsConfig tlsConfig;                           ///<SSL/TLS configuration parameters
   Http2TlsContext tlsContext;                         ///<SSL/TLS context
   Http2ClientTlsInitCallback tlsInitCallback;         ///<SSL/TLS initialization callback function
#endif
#if (HTTP2_CLIENT_PUSH_PROMISE_SUPPORT == ENABLED)
   Http2ClientPushCallback pushCallback;               ///<Server push callback function
#endif
   int32_t txWindow;                                   ///<Connection flow-control window
   uint32_t rxWindowSizeInc;                           ///<Window size increment value
   systime_t timestamp;                                ///<Timestamp to manage keep-alive
   Http2FrameContext lastFrame;                        ///<Provides information about the last frame received
};


//HTTP/2 client related functions
Http2Status http2ClientInit(Http2ClientContext *context);

Http2Status http2ClientSetTimeout(Http2ClientContext *context, systime_t timeout);
Http2Status http2ClientSetKeepAlive(Http2ClientContext *context, systime_t keepAlive);

#if (HTTP2_CLIENT_TLS_SUPPORT == ENABLED)

Http2Status http2ClientRegisterTlsInitCallback(Http2ClientContext *context,
   Http2ClientTlsInitCallback callback);

#endif

Http2Status http2ClientRegisterPushCallback(Http2ClientContext *context,
   Http2ClientPushCallback callback);

Http2Status http2ClientConnect(Http2ClientContext *context,
   Http2SocketAddr serverAddr, size_t serverAddrLen);

Http2Status http2ClientProcessEvents(Http2ClientContext *context,
   systime_t timeout);

Http2Status http2ClientShutdown(Http2ClientContext *context,
   Http2ErrorCode errorCode);

void http2ClientClose(Http2ClientContext *context);

//C++ guard
#ifdef __cplusplus
   }
#endif

#endif
