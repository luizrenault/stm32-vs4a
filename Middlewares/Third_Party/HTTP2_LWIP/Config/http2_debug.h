/**
 * @file http2_debug.h
 * @brief Debugging facilities
 *
 * @section License
 *
 * Copyright (C) 2010-2017 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP HTTP/2.
 *
 * This software is provided under the license terms of the following agreements:
 *   - MASTER DEVELOPMENT and LICENSE AGREEMENT (February 24, 2017)
 *   - STATEMENT OF WORK 1 (February 24, 2017)
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 1.2.0
 **/

#ifndef _HTTP2_DEBUG_H
#define _HTTP2_DEBUG_H

/* Dependencies */
#include <stdio.h>
#include "cmsis_os.h"
#include "http2_compiler_port.h"

/* Trace level definitions */
#define TRACE_LEVEL_OFF      0
#define TRACE_LEVEL_FATAL    1
#define TRACE_LEVEL_ERROR    2
#define TRACE_LEVEL_WARNING  3
#define TRACE_LEVEL_INFO     4
#define TRACE_LEVEL_DEBUG    5

/* Default trace level */
#ifndef TRACE_LEVEL
#define TRACE_LEVEL TRACE_LEVEL_INFO
#endif

#ifndef TRACE_PRINTF
#define TRACE_PRINTF(...) osThreadSuspendAll(), debugfprintf(stderr, __VA_ARGS__), osThreadResumeAll()
#endif

#ifndef TRACE_ARRAY
#define TRACE_ARRAY(p, a, n) osThreadSuspendAll(), debugDisplayArray(stderr, p, a, n), osThreadResumeAll() 
#endif


#ifndef TRACE_MPI
#define TRACE_MPI(p, a) osThreadSuspendAll(), mpiDump(stderr, p, a), osThreadResumeAll()
#endif


/* Debugging macros */
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

/* Debug related functions */
void debugInit(uint32_t baudrate);
void debugfprintf(FILE *stream, ...);
void debugDisplayArray(FILE *stream,
                       const char_t *prepend, const void *data, size_t length);

#endif
