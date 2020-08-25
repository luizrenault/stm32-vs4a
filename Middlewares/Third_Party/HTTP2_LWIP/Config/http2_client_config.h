/**
 * @file http2_client_config.h
 * @brief HTTP/2 client configuration file
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

#ifndef _HTTP2_CLIENT_CONFIG_H
#define _HTTP2_CLIENT_CONFIG_H

/* Trace level for TCP/IP stack debugging */
#define HTTP2_TRACE_LEVEL TRACE_LEVEL_DEBUG

/* HTTP/2 client support */
#define HTTP2_CLIENT_SUPPORT ENABLED

/* Define __weak keyword */
#if  defined ( __GNUC__ )
#ifndef __weak
#define __weak   __attribute__((weak))
#endif /* __weak */
#endif /* __GNUC__ */

#endif
