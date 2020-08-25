/**
******************************************************************************
* @file    avs_network_private.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This file provides the network abstraction and porting glue
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2018 STMicroelectronics International N.V.
* All rights reserved.</center></h2>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted, provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. Neither the name of STMicroelectronics nor the names of other
*    contributors to this software may be used to endorse or promote products
*    derived from this software without specific written permission.
* 4. This software, including modifications and/or derivative works of this
*    software, must execute solely and exclusively on microcontroller or
*    microprocessor devices manufactured by or for STMicroelectronics.
* 5. Redistribution and use of this software other than as permitted under
*    this license is void and will automatically terminate your rights under
*    this license.
*
* THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
* RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
* SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/
#ifndef _avs_network_private_
#define _avs_network_private_


/* LwIP dependencies */

MISRAC_DISABLE          /* Disable all C-State for external dependancies */

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/netdb.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/apps/sntp.h"
#include "threading_alt.h"

/* MbedTLS dependencies */
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/platform.h"
#include "netif/ethernet.h"

#include "http2_client.h"
#include "http2_debug.h"
#include "http2_compiler_port.h"
#include "http2_client.h"
#include "http2_client_multipart.h"
MISRAC_ENABLE          /* Enable all  C-State */




/* Normaly, all gloabal data used for the instance */
typedef struct avs_tls_context
{
  mbedtls_net_context      server_fd;
  mbedtls_entropy_context  entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context      sslContext;
  mbedtls_ssl_config       conf;
  mbedtls_x509_crt         cacert;
  int16_t                  refCount;
} Avs_tls_context;



typedef struct t_avs_network_porting
{
  struct avs_tls_context  tlsContext;             /* Authentification tls contextx */
  struct netif            netContext;             /* Lwip context */
  uint8_t                 esp8266Online;           /* true if we have detected the esp8266 */
} avs_network_porting;


typedef struct t_avs_host_name
{
  struct sockaddr_in  ip;
} avs_host_name;



Http2Status avs_porting_http2_client_tls_init_Callback(Http2ClientContext *context, Http2TlsConfig *tlsConfig, Http2TlsContext *tlsContext);
AVS_Result  avs_porting_tls_init(void);
AVS_Result  avs_get_host_name(char *pUrl, avs_host_name *host);
AVS_Result  avs_tls_instance_init(void);
void        avs_tls_instance_deinit(void);



#endif /* _avs_network_private_ */

