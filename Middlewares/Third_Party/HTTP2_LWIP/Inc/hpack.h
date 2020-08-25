/**
 * @file hpack.h
 * @brief HPACK (Header Compression for HTTP/2)
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

#ifndef _HPACK_H
#define _HPACK_H

//Dependencies
#include "http2_client_config.h"
#include "http2_compiler_port.h"
#include "http2_status.h"

//HPACK buffer size
#ifndef HPACK_BUFFER_SIZE
   #define HPACK_BUFFER_SIZE 1024U
#elif (HPACK_BUFFER_SIZE < 128U)
   #error HPACK_BUFFER_SIZE parameter is not valid
#endif

//HPACK dynamic table size
#ifndef HPACK_DYNAMIC_TABLE_SIZE
   #define HPACK_DYNAMIC_TABLE_SIZE 4096U
#elif (HPACK_DYNAMIC_TABLE_SIZE < 128U)
   #error HPACK_DYNAMIC_TABLE_SIZE parameter is not valid
#endif

//HPACK static table size
#define HPACK_STATIC_TABLE_SIZE 61U
//HPACK entry overhead
#define HPACK_ENTRY_OVERHEAD 32U

//C++ guard
#ifdef __cplusplus
   extern "C" {
#endif


/**
 * @brief Header field format
 **/

typedef enum
{
   HPACK_INDEXED_HEADER_FIELD                   = 1U,
   HPACK_LITERAL_HEADER_FIELD_WITH_INC_INDEXING = 2U,
   HPACK_LITERAL_HEADER_FIELD_WITHOUT_INDEXING  = 3U,
   HPACK_LITERAL_HEADER_FIELD_NEVER_INDEXED     = 4U,
   HPACK_DYNAMIC_TABLE_UPDATE                   = 5U
} HpackHeaderFieldFormat;


/**
 * @brief Header field parsing callback
 **/

typedef Http2Status (*HpackParseCallback)(void *param,
   const char_t *name, const char_t *value);


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Dynamic table entry
 **/

typedef __start_packed struct
{
   size_t size;
   size_t nameLen;
   size_t valueLen;
   char_t name[];
} __end_packed HpackDynamicEntry;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief Static table entry
 **/

typedef struct
{
   const char_t *name;
   const char_t *value;
} HpackStaticEntry;


/**
 * @brief HPACK compression context
 **/

typedef struct
{
   HpackParseCallback parseCallback;
   void *parseCallbackParam;
   uint8_t dynamicTable[HPACK_DYNAMIC_TABLE_SIZE];
   size_t dynamicTableSize;
   size_t dynamicTableMaxSize;
   size_t dynamicTableAllowedMaxSize;
   uint_t dynamicTableEntryCount;
   bool_t newHeaderBlockEvent;
   bool_t resizeEvent;
   size_t resizeValue;
   size_t resizeMinValue;
   uint8_t buffer[HPACK_BUFFER_SIZE];
} HpackContext;


//HPACK related functions
void hpackInit(HpackContext *context);

Http2Status hpackResize(HpackContext *context, size_t newSize);

Http2Status hpackEncodeHeaderField(HpackContext *context, uint8_t *output,
   size_t outputSize, size_t *outputLen, const char_t *name, const char_t *value);

Http2Status hpackRegisterParseCallback(HpackContext *context,
   HpackParseCallback parseCallback, void *param);

Http2Status hpackDecodeHeaderBlock(HpackContext *context,
   const uint8_t *input, size_t inputLen, size_t *consumed);

Http2Status hpackDecodeIndexedHeaderField(HpackContext *context,
   const uint8_t *input, size_t inputLen, size_t *inputPos);

Http2Status hpackDecodeLiteralHeaderField(HpackContext *context, const uint8_t *input,
   size_t inputLen, size_t *inputPos, HpackHeaderFieldFormat format);

Http2Status hpackDecodeDynamicTableUpdate(HpackContext *context,
   const uint8_t *input, size_t inputLen, size_t *inputPos);

//C++ guard
#ifdef __cplusplus
   }
#endif

#endif
