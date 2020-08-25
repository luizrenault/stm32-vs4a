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
* This software component is licensed by ST under Ultimate Liberty license
* SLA0044, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0044
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
#include "lwip/apps/sntp.h"
#include "ethernetif.h"
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
  struct avs_tls_context  tlsContext;           /* Authentification tls contextx */
  struct netif            netContext;           /* Lwip context */
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

