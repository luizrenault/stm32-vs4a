/**
 * @file http2_client_stream.h
 * @brief HTTP/2 stream handling
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

#ifndef _HTTP2_CLIENT_STREAM_H
#define _HTTP2_CLIENT_STREAM_H

//Dependencies
#include "http2_client.h"

//C++ guard
#ifdef __cplusplus
   extern "C" {
#endif


/**
 * @brief Header field parsing callback
 **/

typedef Http2Status (*Http2ClientParseHeaderFieldCallback)(void *param,
   const char_t *name, const char_t *value);


/**
 * @brief HTTP/2 stream buffer
 **/

typedef struct
{
   char_t data[HTTP2_CLIENT_STREAM_BUFFER_SIZE]; ///<Data buffer
   size_t length;
   size_t threshold;
   size_t writePos;
   size_t readPos;
} Http2StreamBuffer;


/**
 * @brief HTTP request
 **/

typedef struct
{
#if (HTTP2_CLIENT_PUSH_PROMISE_SUPPORT == ENABLED)
   char_t path[HTTP2_CLIENT_PATH_MAX_LEN + 1];         ///<Path and query parts of the target URI
#endif
#if (HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT == ENABLED)
   char_t boundary[HTTP2_CLIENT_BOUNDARY_MAX_LEN + 1]; ///<Boundary string
   size_t boundaryLength;                              ///<Boundary string length
#endif
   uint_t dummy;                                       ///<Unused parameter
} Http2Request;


/**
 * @brief HTTP response
 **/

typedef struct
{
   uint_t status;                                               ///<HTTP status code
   char_t contentType[HTTP2_CLIENT_CONTENT_TYPE_MAX_LEN + 1];   ///<Content type
#if (HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT == ENABLED)
   char_t boundary[HTTP2_CLIENT_BOUNDARY_MAX_LEN + 1];          ///<Boundary string
   size_t boundaryLength;                                       ///<Boundary string length
   char_t multipartType[HTTP2_CLIENT_CONTENT_TYPE_MAX_LEN + 1]; ///<Multipart content type
#endif
} Http2Response;


/**
 * @brief HTTP/2 stream
 **/

struct _Http2ClientStream
{
   Http2StreamState state;           ///<Stream state
   bool_t orphan;                    ///Specifies whether the stream is an orphan
   uint32_t id;                      ///<Stream identifier
   uint32_t dependency;              ///<The stream that this stream depends on
   uint32_t weight;                  ///<Priority weight for the stream
   osSemaphoreId event;                    ///<Event object to receive stream-related notifications
   uint_t eventMask;                 ///<The events the application is interested in
   uint_t eventFlags;                ///<Returned events
   systime_t timeout;                ///<Timeout interval
   Http2ErrorCode errorCode;         ///<Error code
   Http2ClientContext *context;      ///<HTTP/2 client context

   Http2ExchangeState exchangeState; ///<HTTP request/response exchange state
   Http2Request request;             ///<HTTP request
   Http2Response response;           ///<HTTP response
   Http2StreamBuffer rxBuffer;       ///<Receive buffer
   Http2StreamBuffer txBuffer;       ///<Transmit buffer

   bool_t endStreamFlag;
   bool_t resetFlag;

   int32_t txWindow;                 ///<Stream flow-control window
   uint32_t rxWindowSizeInc;         ///<Window size increment value
#if (HTTP2_CLIENT_MULTIPART_TYPE_SUPPORT == ENABLED)
   uint_t txMultipartState;
   uint_t rxMultipartState;
#endif
};


//HTTP/2 client related functions
Http2ClientStream *http2ClientOpenStream(Http2ClientContext *context);

Http2Status http2ClientSetStreamTimeout(Http2ClientStream *stream,
   systime_t timeout);

Http2Status http2ClientSetHeaderField(Http2ClientStream *stream,
   const char_t *name, const char_t *value);

Http2Status http2ClientWriteHeader(Http2ClientStream *stream, uint_t flags);

Http2Status http2ClientReadHeader(Http2ClientStream *stream);

Http2Status http2ClientReadHeaderEx(Http2ClientStream *stream,
   Http2ClientParseHeaderFieldCallback parseCallback, void *param);

Http2Status http2ClientGetNextHeaderField(Http2ClientStream *stream,
   char_t **name, char_t **value);

const char_t *http2ClientGetPath(Http2ClientStream *stream);
uint_t http2ClientGetStatus(Http2ClientStream *stream);
const char_t *http2ClientGetContentType(Http2ClientStream *stream);

Http2Status http2ClientWriteStream(Http2ClientStream *stream,
   const void *data, size_t length, size_t *written, uint_t flags);

Http2Status http2ClientReadStream(Http2ClientStream *stream,
   void *data, size_t size, size_t *received, uint_t flags);

Http2Status http2ClientCancelStream(Http2ClientStream *stream);

void http2ClientCloseStream(Http2ClientStream *stream);

//C++ guard
#ifdef __cplusplus
   }
#endif

#endif
