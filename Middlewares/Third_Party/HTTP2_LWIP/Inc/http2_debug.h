/**
 * @file http2_debug.h
 * @brief Debugging facilities
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

#ifndef _HTTP2_DEBUG_H
#define _HTTP2_DEBUG_H

//Dependencies
#include <stdio.h>
#include "cmsis_os.h"
#include "http2_compiler_port.h"

//Trace level definitions
#define TRACE_LEVEL_OFF      0
#define TRACE_LEVEL_FATAL    1
#define TRACE_LEVEL_ERROR    2
#define TRACE_LEVEL_WARNING  3
#define TRACE_LEVEL_INFO     4
#define TRACE_LEVEL_DEBUG    5

//Default trace level
#ifndef TRACE_LEVEL
   #define TRACE_LEVEL TRACE_LEVEL_INFO
#endif

//Trace output redirection
#ifndef TRACE_PRINTF
   #define TRACE_PRINTF(...) osThreadSuspendAll(), debugfprintf(stderr, __VA_ARGS__), osThreadResumeAll()
#endif

#ifndef TRACE_ARRAY
   #define TRACE_ARRAY(p, a, n) osThreadSuspendAll(), debugDisplayArray(stderr, p, a, n), osThreadResumeAll()
#endif

#ifndef TRACE_MPI
   #define TRACE_MPI(p, a) osThreadSuspendAll(), mpiDump(stderr, p, a), osThreadResumeAll()
#endif

//Debugging macros
#if (TRACE_LEVEL >= TRACE_LEVEL_FATAL)
   #define TRACE_FATAL(...) TRACE_PRINTF(__VA_ARGS__)
   #define TRACE_FATAL_ARRAY(p, a, n) TRACE_ARRAY(p, a, n)
#else
   #define TRACE_FATAL(...)
   #define TRACE_FATAL_ARRAY(p, a, n)
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_ERROR)
   #define TRACE_ERROR(...) TRACE_PRINTF(__VA_ARGS__)
   #define TRACE_ERROR_ARRAY(p, a, n) TRACE_ARRAY(p, a, n)
#else
   #define TRACE_ERROR(...)
   #define TRACE_ERROR_ARRAY(p, a, n)
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_WARNING)
   #define TRACE_WARNING(...) TRACE_PRINTF(__VA_ARGS__)
   #define TRACE_WARNING_ARRAY(p, a, n) TRACE_ARRAY(p, a, n)
#else
   #define TRACE_WARNING(...)
   #define TRACE_WARNING_ARRAY(p, a, n)
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_INFO)
   #define TRACE_INFO(...) TRACE_PRINTF(__VA_ARGS__)
   #define TRACE_INFO_ARRAY(p, a, n) TRACE_ARRAY(p, a, n)
   #define TRACE_INFO_NET_BUFFER(p, b, o, n)
   #define TRACE_INFO_MPI(p, a) TRACE_MPI(p, a)
#else
   #define TRACE_INFO(...)
   #define TRACE_INFO_ARRAY(p, a, n)
   #define TRACE_INFO_NET_BUFFER(p, b, o, n)
   #define TRACE_INFO_MPI(p, a)
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   #define TRACE_DEBUG(...) TRACE_PRINTF(__VA_ARGS__)
   #define TRACE_DEBUG_ARRAY(p, a, n) TRACE_ARRAY(p, a, n)
   #define TRACE_DEBUG_NET_BUFFER(p, b, o, n)
   #define TRACE_DEBUG_MPI(p, a) TRACE_MPI(p, a)
#else
   #define TRACE_DEBUG(...)
   #define TRACE_DEBUG_ARRAY(p, a, n)
   #define TRACE_DEBUG_NET_BUFFER(p, b, o, n)
   #define TRACE_DEBUG_MPI(p, a)
#endif

//Debug related functions
void debugInit(uint32_t baudrate);
void debugfprintf(FILE *stream, ...);
void debugDisplayArray(FILE *stream,
   const char_t *prepend, const void *data, size_t length);

#endif
