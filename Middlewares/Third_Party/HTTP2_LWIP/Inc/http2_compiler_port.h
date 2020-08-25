/**
 * @file http2_compiler_port.h
 * @brief Compiler specific definitions
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

#ifndef _HTTP2_COMPILER_PORT_H
#define _HTTP2_COMPILER_PORT_H

//Dependencies
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

//Types
#ifndef HAVE_CHAR_T
typedef   char char_t;
#define HAVE_CHAR_T
#endif

typedef signed int int_t;
typedef unsigned int uint_t;
typedef uint32_t systime_t;
typedef int bool_t;

//GCC compiler?
#if defined(__GNUC__)
   #undef __static_inline
   #define __static_inline static inline
   #undef __start_packed
   #define __start_packed
   #undef __end_packed
   #define __end_packed __attribute__((__packed__))
//Keil MDK-ARM compiler?
#elif defined(__CC_ARM)
   #undef __static_inline
   #define __static_inline static __inline
   #pragma anon_unions
   #undef __start_packed
   #define __start_packed __packed
   #undef __end_packed
   #define __end_packed
//IAR C compiler?
#elif defined(__IAR_SYSTEMS_ICC__)
   #undef __static_inline
   #define __static_inline static inline
   #undef __start_packed
   #define __start_packed __packed
   #undef __end_packed
   #define __end_packed
#endif

//Printf formatters
#define PRIuSIZE "u"
#define PRIuTIME "lu"

//Load unaligned 24-bit integer (big-endian encoding)
#define LOAD24BE(p) ( \
   ((uint32_t)(((uint8_t *)(p))[0]) << 16) | \
   ((uint32_t)(((uint8_t *)(p))[1]) << 8) | \
   ((uint32_t)(((uint8_t *)(p))[2]) << 0))

//Load unaligned 32-bit integer (big-endian encoding)
#define LOAD32BE(p) ( \
   ((uint32_t)(((uint8_t *)(p))[0]) << 24) | \
   ((uint32_t)(((uint8_t *)(p))[1]) << 16) | \
   ((uint32_t)(((uint8_t *)(p))[2]) << 8) | \
   ((uint32_t)(((uint8_t *)(p))[3]) << 0))

//Store unaligned 24-bit integer (big-endian encoding)
#define STORE24BE(a, p) \
   ((uint8_t *)(p))[0] = ((uint32_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint32_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint32_t)(a) >> 0) & 0xFFU

//Store unaligned 32-bit integer (big-endian encoding)
#define STORE32BE(a, p) \
   ((uint8_t *)(p))[0] = ((uint32_t)(a) >> 24) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint32_t)(a) >> 16) & 0xFFU, \
   ((uint8_t *)(p))[2] = ((uint32_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[3] = ((uint32_t)(a) >> 0) & 0xFFU

//Swap a 16-bit integer
#define SWAPINT16(x) ( \
   (((uint16_t)(x) & 0x00FFU) << 8) | \
   (((uint16_t)(x) & 0xFF00U) >> 8))

//Swap a 32-bit integer
#define SWAPINT32(x) ( \
   (((uint32_t)(x) & 0x000000FFUL) << 24) | \
   (((uint32_t)(x) & 0x0000FF00UL) << 8) | \
   (((uint32_t)(x) & 0x00FF0000UL) >> 8) | \
   (((uint32_t)(x) & 0xFF000000UL) >> 24))

//Byte order conversion function (16-bit integer)
__static_inline uint16_t swapInt16(uint16_t value)
{
   return SWAPINT16(value);
}

//Byte order conversion function (32-bit integer)
__static_inline uint32_t swapInt32(uint32_t value)
{
   return SWAPINT32(value);
}

//Undefine conflicting definitions
#ifdef HTONS
   #undef HTONS
#endif

#ifdef HTONL
   #undef HTONL
#endif

#ifdef htons
   #undef htons
#endif

#ifdef htonl
   #undef htonl
#endif

#ifdef NTOHS
   #undef NTOHS
#endif

#ifdef NTOHL
   #undef NTOHL
#endif

#ifdef ntohs
   #undef ntohs
#endif

#ifdef ntohl
   #undef ntohl
#endif

//Big-endian machine?
#ifdef _CPU_BIG_ENDIAN

//Host byte order to network byte order
#define HTONS(value) (value)
#define HTONL(value) (value)
#define htons(value) ((uint16_t) (value))
#define htonl(value) ((uint32_t) (value))

//Network byte order to host byte order
#define NTOHS(value) (value)
#define NTOHL(value) (value)
#define ntohs(value) ((uint16_t) (value))
#define ntohl(value) ((uint32_t) (value))

//Little-endian machine?
#else

//Host byte order to network byte order
#define HTONS(value) SWAPINT16(value)
#define HTONL(value) SWAPINT32(value)
#define htons(value) swapInt16((uint16_t) (value))
#define htonl(value) swapInt32((uint32_t) (value))

//Network byte order to host byte order
#define NTOHS(value) SWAPINT16(value)
#define NTOHL(value) SWAPINT32(value)
#define ntohs(value) swapInt16((uint16_t) (value))
#define ntohl(value) swapInt32((uint32_t) (value))

#endif

//Compilation flags used to enable/disable features
#define ENABLED  1
#define DISABLED 0

//Miscellaneous macros
#ifndef FALSE
   #define FALSE 0
#endif

#ifndef TRUE
   #define TRUE 1
#endif

#ifndef LSB
   #define LSB(x) ((x) & 0xFF)
#endif

#ifndef MSB
   #define MSB(x) (((x) >> 8) & 0xFF)
#endif

#ifndef MIN
   #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
   #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef arraysize
   #define arraysize(a) (sizeof(a) / sizeof(a[0]))
#endif

//Infinite delay
#define INFINITE_DELAY ((uint_t) -1)

//Time comparison
#define timeCompare(t1, t2) ((int32_t) ((t1) - (t2)))

#endif
