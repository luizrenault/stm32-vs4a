/**
******************************************************************************
* @file    avs_lwip_tls_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This module implements a basic tls streaming mainly used to stream audio
*          and ip
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
#include "avs_private.h"
#include "avs_network_private.h"


#if !defined(MBEDTLS_BIGNUM_C)
#warning "MBEDTLS_BIGNUM_C undefined"
#endif
#if !defined(MBEDTLS_ENTROPY_C)
#warning "MBEDTLS_ENTROPY_C undefined"
#endif
#if !defined(MBEDTLS_SSL_TLS_C)
#warning "MBEDTLS_SSL_TLS_C undefined"
#endif
#if !defined(MBEDTLS_SSL_CLI_C)
#warning "MBEDTLS_SSL_CLI_C undefined"
#endif
#if !defined(MBEDTLS_BIGNUM_C)
#warning "MBEDTLS_BIGNUM_C undefined"
#endif
#if !defined(MBEDTLS_NET_C)
#warning "MBEDTLS_NET_C undefined"
#endif
#if !defined(MBEDTLS_RSA_C)
#warning "MBEDTLS_RSA_C undefined"
#endif
#if !defined(MBEDTLS_CERTS_C)
#warning "MBEDTLS_CERTS_C undefined"
#endif
#if !defined(MBEDTLS_PEM_PARSE_C)
#warning "MBEDTLS_PEM_PARSE_C undefined"
#endif
#if !defined(MBEDTLS_CTR_DRBG_C)
#warning "MBEDTLS_CTR_DRBG_C undefined"
#endif
#if !defined(MBEDTLS_X509_CRT_PARSE_C)
#warning "MBEDTLS_X509_CRT_PARSE_C undefined"
#endif

/* Called from the TLS API  using a callback and a specific prototype

Notice : Generate MISRA error MISRA C 2004 rule 6.3
Called from the TLS API  using a callback and a specific prototype
*/


static void avs_tls_debug(void *ctx, int level, const char *file, int line, const char *str );
static void avs_tls_debug(void *ctx, int level, const char *file, int line, const char *str )
{
  AVS_Trace(AVS_TRACE_LVL_NETWORK, file, line, str);
}

typedef struct t_tls_instance
{
  mbedtls_entropy_context  entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_config       conf;
  mbedtls_x509_crt         cacert;
  mbedtls_x509_crt         client_cert;
  mbedtls_pk_context       client_key;
  uint32_t                 needCertificate;
  
  int16_t                  refCount;
  uint32_t                 cert_option;
} tls_instance_t;




typedef struct t_tls_context
{
  mbedtls_net_context      server_fd;
  mbedtls_ssl_context      sslContext;

} tls_context_t;

static tls_instance_t gTlsInstance;
static uint32_t       gEnsureInit = 0;

/*

protected Malloc for TLS  overload


*/
static void *avs_tls_safe_calloc(size_t size, size_t elem);
static void *avs_tls_safe_calloc(size_t size, size_t elem)
{
  return avs_core_mem_calloc(avs_mem_type_heap, size, elem);
}

/*

protected free for TLS overload


*/
static void avs_tls_safe_free(void *pMalloc);
static void avs_tls_safe_free(void *pMalloc)
{
  avs_core_mem_free(pMalloc);
}


/*

Called from the TLS API  using a callback and a specific prototype

*/


int  mutex_lock (mbedtls_threading_mutex_t * mutex)
{
  BaseType_t  ret;
  AVS_ASSERT(*mutex);
  ret =  xSemaphoreTakeRecursive(*mutex, portMAX_DELAY);
  return ret == pdPASS ? 0 : -1;
}
/*
  Called from the TLS API  using a callback and a specific prototype
*/

int  mutex_unlock (mbedtls_threading_mutex_t * mutex)
{
  BaseType_t  ret;
  AVS_ASSERT(*mutex );
  ret =  xSemaphoreGiveRecursive(*mutex);
  return ret == pdPASS ? 0 : -1;
}


AVS_Result avs_tls_instance_init(void)
{
  if(gEnsureInit)
  {
    return AVS_OK;
  }
  /* Config TLS for multi threading */
  mbedtls_threading_set_alt( mutex_init, mutex_free, mutex_lock, mutex_unlock);
  mbedtls_platform_set_calloc_free(avs_tls_safe_calloc, avs_tls_safe_free);


  gEnsureInit = 1;
  mbedtls_ssl_config_init(&gTlsInstance.conf);
  mbedtls_x509_crt_init(&gTlsInstance.cacert);
  mbedtls_x509_crt_init(&gTlsInstance.client_cert);
  mbedtls_ctr_drbg_init(&gTlsInstance.ctr_drbg);
  mbedtls_debug_set_threshold(1); /* 1= error ;  4 = debug */
  mbedtls_ssl_conf_dbg( &gTlsInstance.conf, avs_tls_debug, stderr );
  AVS_TRACE_DEBUG("Seeding the random number generator.");
  mbedtls_entropy_init( &gTlsInstance.entropy );
  const uint8_t pSeed[] = "ssl_client_http";
  int32_t ret = mbedtls_ctr_drbg_seed(&gTlsInstance.ctr_drbg, mbedtls_entropy_func, &gTlsInstance.entropy, pSeed, strlen((const char_t *)(uint32_t)pSeed));
  if (ret != 0)
  {
    AVS_TRACE_ERROR("mbedtls_ctr_drbg_seed FAILED : %d", ret) ;
    return AVS_ERROR;
  }


  /*
  * 1. Initialize certificates
  */
  
  
  AVS_instance_handle *pInstance =   avs_core_get_instance();
  char_t *pRootCa   = (char_t *)(uint32_t)mbedtls_test_cas_pem;
  uint32_t szRootCa = mbedtls_test_cas_pem_len;
  gTlsInstance.cert_option=MBEDTLS_SSL_VERIFY_OPTIONAL;
  gTlsInstance.needCertificate = FALSE;
 
  if(pInstance->pFactory->persistentCB )
  {
    uint32_t result = pInstance->pFactory->persistentCB(AVS_PERSIST_GET_POINTER,AVS_PERSIST_ITEM_CA_ROOT,0,0);
    if(result > AVS_END_RESULT)
    {
      /* Use a verified ssl */
      gTlsInstance.cert_option=MBEDTLS_SSL_VERIFY_REQUIRED;
      pRootCa  =  (char_t *)(uint32_t)result;
      szRootCa =  strlen(pRootCa)+1;
    }
  }
  ret = mbedtls_x509_crt_parse(&gTlsInstance.cacert, (const unsigned char *)(uint32_t)pRootCa, szRootCa);
  if( ret < 0 )
  {
    AVS_TRACE_ERROR("Error Root ca: %d", ret);
    return  AVS_ERROR;
  }
  return AVS_OK;
}

void avs_tls_instance_deinit(void)
{
  mbedtls_x509_crt_free  ( &gTlsInstance.client_cert );
  mbedtls_x509_crt_free  ( &gTlsInstance.cacert );
  mbedtls_ssl_config_free( &gTlsInstance.conf );
  mbedtls_ctr_drbg_free  ( &gTlsInstance.ctr_drbg );
  mbedtls_entropy_free   ( &gTlsInstance.entropy );
  gEnsureInit = 0;
}




/** Override Http2 function in order to share the same TLS config*/
mbedtls_ssl_config *  Http2TlsGetConfig(void);
mbedtls_ssl_config *  Http2TlsGetConfig(void)
{
  return &gTlsInstance.conf;
}

/** Override Http2 function in order to share the same  TLS config*/
void  Http2TlsFreeConfig(mbedtls_ssl_config * config);
void  Http2TlsFreeConfig(mbedtls_ssl_config * config)
{
}
static AVS_Result avs_tls_close(tls_context_t *pCtx);
static AVS_Result avs_tls_close(tls_context_t *pCtx)
{

  if(pCtx)
  {
    mbedtls_net_free( &pCtx->server_fd );
    mbedtls_ssl_close_notify( &pCtx->sslContext );
    mbedtls_ssl_free       ( &pCtx->sslContext );
    avs_core_mem_free(pCtx);
  }
  return AVS_OK;

}
static AVS_Result avs_tls_connection(AVS_instance_handle * pHandle,tls_context_t *pCtx, const char *pServer, uint32_t port, const char *cr, uint32_t crLen);
static AVS_Result avs_tls_connection(AVS_instance_handle * pHandle,tls_context_t *pCtx, const char *pServer, uint32_t port, const char *cr, uint32_t crLen)
{
  int32_t  ret = 0;
  uint32_t flags = 0;
  AVS_ASSERT(gEnsureInit);
  mbedtls_ssl_init(&pCtx->sslContext);

  /* TODO: Remove this comment when the patch is verified */

  /*
  * 2. Start the connection
  */
  char_t sPort[10];
  snprintf(sPort, sizeof(sPort), "%lu", port);

  AVS_TRACE_DEBUG( "Connecting to tcp/%s/%s...", pServer, sPort );
  ret = mbedtls_net_connect(&pCtx->server_fd, pServer, sPort, MBEDTLS_NET_PROTO_TCP);
  if(ret  != 0)
  {
    AVS_TRACE_ERROR( "mbedtls_net_connect FAILED :  %d", ret );
    return AVS_ERROR;
  }

  /*
  * 3. Setup stuff
  */
  AVS_TRACE_DEBUG( " Setting up the SSL/TLS structure" );
  ret = mbedtls_ssl_config_defaults(&gTlsInstance.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0)
  {
    AVS_TRACE_ERROR( "mbedtls_ssl_config_defaults FAILED :  %d", ret );
    return AVS_ERROR;
  }

  /* OPTIONAL is not optimal for security,
  * but makes interop easier in this simplified example */
  mbedtls_ssl_conf_authmode( &gTlsInstance.conf, gTlsInstance.cert_option);
  mbedtls_ssl_conf_ca_chain( &gTlsInstance.conf, &gTlsInstance.cacert, NULL );
  mbedtls_ssl_conf_rng( &gTlsInstance.conf, mbedtls_ctr_drbg_random, &gTlsInstance.ctr_drbg );
  if(gTlsInstance.needCertificate)
  {
    
    if( ( ret = mbedtls_ssl_conf_own_cert( &gTlsInstance.conf, &gTlsInstance.client_cert, &gTlsInstance.client_key ) ) != 0 )
    {
      AVS_TRACE_ERROR( "mbedtls_ssl_conf_own_cert FAILED :  %d\n", ret );
      return AVS_ERROR;
    }
    ret = mbedtls_ssl_set_hostname( &pCtx->sslContext, pServer );
    if (ret != 0)
    {
      AVS_TRACE_ERROR( "mbedtls_ssl_set_hostname FAILED :  %d\n", ret );
      return AVS_ERROR;
    }
  }
    
  
  ret = mbedtls_ssl_setup( &pCtx->sslContext, &gTlsInstance.conf);
  if (ret != 0)
  {
    AVS_TRACE_ERROR( "mbedtls_ssl_setup FAILED :  %d\n", ret );
    return AVS_ERROR;
  }


  mbedtls_ssl_set_bio( &pCtx->sslContext, &pCtx->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

  /*
  * 4. Handshake
  */
  AVS_TRACE_DEBUG( "Performing the SSL/TLS handshake\n" );

  while((ret = mbedtls_ssl_handshake( &pCtx->sslContext )) != 0)
  {
    if((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
    {
      if(ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
      {
        avs_core_message_post(pHandle,EVT_TLS_CERT_VERIFY_FAILED,0);
      }
      else
      {
        AVS_TRACE_ERROR( "mbedtls_ssl_handshake FAILED :   -0x%x", -ret );
      }
      return AVS_ERROR;
    }
  }

  /*
  * 5. Verify the server certificate
  */
  AVS_TRACE_DEBUG( "Verifying peer X.509 certificate\n" );

  if(( flags = mbedtls_ssl_get_verify_result( &pCtx->sslContext )) != 0)
  {
    size_t  workSize = 4 * 1024;
    char_t *workBuffer   = avs_core_mem_alloc(avs_mem_type_heap, workSize );
    AVS_ASSERT(workBuffer != NULL);
    AVS_TRACE_DEBUG( "mbedtls_ssl_get_verify_result FAILED \n" );
    mbedtls_x509_crt_verify_info(workBuffer, workSize, "  ! ", flags)  ;
    AVS_PRINT_STRING(AVS_TRACE_LVL_DEBUG, workBuffer );
    avs_core_mem_free(workBuffer);
  }
  return AVS_OK;
}



AVS_Result avs_porting_tls_read_response(AVS_instance_handle*pHandle, AVS_HTls hStream, char_t  *pHeader, uint32_t maxSize, uint32_t *pRetSize)
{


  char_t *pString = pHeader;
  tls_context_t *pCtx = (tls_context_t *)hStream;

  int32_t headerSize    = maxSize;
  uint32_t count = 0;
  while(headerSize > 0)
  {
    uint32_t retCode = mbedtls_ssl_read( &pCtx->sslContext, (uint8_t *)(uint32_t)pString, 1 );
    if(retCode == 0)
    {
      return AVS_EOF;
    }
    pString++;
    headerSize--;
    count++;
    int32_t len = strncmp(pString - 4, "\r\n\r\n", 4);
    if((count > 4) && (len == 0))
    {
      break;
    }
  }
  if(pRetSize)
  {
    *pRetSize = count;
  }
  if(headerSize == 0)
  {
    return AVS_OVERFLOW;
  }
  return AVS_OK;
}


/* Open the stream */
AVS_HTls        avs_porting_tls_open(AVS_instance_handle * pHandle, const char_t *pHost, uint32_t port)
{
  if(pHandle  == 0)
  {
    return NULL;
  }
  tls_context_t *pCtx = avs_core_mem_alloc(avs_mem_type_heap, sizeof(tls_context_t));
  AVS_ASSERT(pCtx);
  memset(pCtx, 0, sizeof(tls_context_t));
  AVS_Result err = avs_tls_connection(pHandle,pCtx, pHost, port, mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
  avs_core_message_post(pHandle,EVT_OPEN_TLS,err);
  
  if(err != AVS_OK)
  {
    avs_tls_close(pCtx);
    pCtx = NULL;
  }
  
  
  return (AVS_HTls) pCtx;
}



/* Close the connection */
AVS_Result        avs_porting_tls_close(AVS_instance_handle * pHandle, AVS_HTls hstream)
{
  tls_context_t *pCtx = (tls_context_t *)hstream;
  return avs_tls_close(pCtx );
}

/* Read data */
AVS_Result        avs_porting_tls_read(AVS_instance_handle * pHandle, AVS_HTls  hStream, void *pBuffer, uint32_t szInSByte, uint32_t *retSize)
{
  tls_context_t *pCtx = (tls_context_t *)hStream;
  int32_t retCode = mbedtls_ssl_read( &pCtx->sslContext, pBuffer, szInSByte );
  if(retCode == 0)
  {
    return AVS_EOF;
  }
  if(retCode  < 0)
  {
    return AVS_ERROR;
  }
  if(retSize)
  {
    *retSize = retCode;
  }
  return AVS_OK;
}
AVS_Result        avs_porting_tls_write(AVS_instance_handle * pHandle, AVS_HTls  hStream, const void *pBuffer, uint32_t szInSBytes, uint32_t *retSize)
{
  tls_context_t *pCtx = (tls_context_t *)hStream;
  int32_t retCode = mbedtls_ssl_write( &pCtx->sslContext, pBuffer, szInSBytes );
  if(retCode == 0)
  {
    return AVS_EOF;
  }
  if(retCode  < 0)
  {
    return AVS_ERROR;
  }
  if(retSize)
  {
    *retSize = retCode;
  }
  return AVS_OK;
}


