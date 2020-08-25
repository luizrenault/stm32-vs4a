/**
 * @file http2_client_multipart.h
 * @brief Support for multipart content type
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

#ifndef _HTTP2_CLIENT_MULTIPART_H
#define _HTTP2_CLIENT_MULTIPART_H

//Dependencies
#include "http2_client.h"

//C++ guard
#ifdef __cplusplus
   extern "C" {
#endif


/**
 * @brief Multipart encoding/decoding states
 **/

typedef uint_t Http2MultipartState;

#define HTTP2_MULTIPART_STATE_PREAMBLE 0U
#define HTTP2_MULTIPART_STATE_HEADER   1U
#define HTTP2_MULTIPART_STATE_BODY     2U
#define HTTP2_MULTIPART_STATE_EPILOGUE 3U


//HTTP/2 client related functions
Http2Status http2ClientSetMultipartBoundary(Http2ClientStream *stream,
   const char_t *boundary);

Http2Status http2ClientSetMultipartHeaderField(Http2ClientStream *stream,
   const char_t *name, const char_t *value);

Http2Status http2ClientWriteMultipartHeader(Http2ClientStream *stream, uint_t flags);

Http2Status http2ClientReadMultipartHeader(Http2ClientStream *stream);

const char_t *http2ClientGetMultipartType(Http2ClientStream *stream);

Http2Status http2ClientWriteMultipartBody(Http2ClientStream *stream,
   const void *data, size_t length, size_t *written, uint_t flags);

Http2Status http2ClientReadMultipartBody(Http2ClientStream *stream,
   void *data, size_t size, size_t *received, uint_t flags);

Http2Status http2ClientSearchMultipartBoundary(Http2StreamBuffer *buffer,
   const char_t *boundary, size_t boundaryLen, size_t *pos);

//C++ guard
#ifdef __cplusplus
   }
#endif

#endif
