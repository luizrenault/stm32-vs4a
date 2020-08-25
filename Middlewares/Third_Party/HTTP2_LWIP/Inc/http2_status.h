/**
 * @file http2_error.h
 * @brief HTTP/2 status codes
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

#ifndef _HTTP2_STATUS_H
#define _HTTP2_STATUS_H


/**
 * @brief HTTP/2 status codes
 **/

typedef enum
{
   HTTP2_STATUS_OK = 0,
   HTTP2_STATUS_FAILURE,
   HTTP2_STATUS_OUT_OF_RESOURCES,
   HTTP2_STATUS_INVALID_PARAMETER,
   HTTP2_STATUS_INVALID_SYNTAX,
   HTTP2_STATUS_INVALID_LENGTH,
   HTTP2_STATUS_BUFFER_OVERFLOW,
   HTTP2_STATUS_BUFFER_UNDERFLOW,
   HTTP2_STATUS_WRONG_STATE,
   HTTP2_STATUS_OPEN_FAILED,
   HTTP2_STATUS_CONNECTION_FAILED,
   HTTP2_STATUS_CONNECTION_RESET,
   HTTP2_STATUS_WRITE_FAILED,
   HTTP2_STATUS_READ_FAILED,
   HTTP2_STATUS_DECODING_FAILED,
   HTTP2_STATUS_TIMEOUT,
   HTTP2_STATUS_WOULD_BLOCK,
   HTTP2_STATUS_END_OF_STREAM,
   HTTP2_STATUS_STREAM_REFUSED,
   HTTP2_STATUS_MORE_DATA_REQUIRED,
   HTTP2_STATUS_NO_MATCH,
   HTTP2_STATUS_PARTIAL_MATCH,
   HTTP2_STATUS_NOT_IMPLEMENTED
} Http2Status;

#endif
