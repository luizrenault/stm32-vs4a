/**
  *  Portions COPYRIGHT 2016 STMicroelectronics
  *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
  *
  ******************************************************************************
  * @file    net_sockets.c
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    20-02-2018
  * @brief   TCP/IP or UDP/IP networking functions iplementation based on LwIP API
             see the file "mbedTLS/library/net_socket_template.c" for the standard
       implmentation
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

#ifndef MBEDTLS_NET_C
#include "avs_private.h"

#define TLS_TIMEOUT     3000
static int32_t net_would_block( const mbedtls_net_context *ctx );

/*
* Initiate a TCP connection with host:port and the given protocol
*/

/*
Notice :, Generate MISRA error MISRA C 2004 rule 6.3
Called from the TLS API  using a callback and a specific prototype

*/


int mbedtls_net_connect( mbedtls_net_context *ctx, const char *host, const char *port, int proto )
{
  int32_t ret;
  struct addrinfo hints;
  struct addrinfo *list;
  struct addrinfo *current;
  struct timeval tv;


  /* Do name resolution with both IPv6 and IPv4 */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;

  hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
  hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;

  if(getaddrinfo(host, port, &hints, &list) != 0)
  {
    return MBEDTLS_ERR_NET_UNKNOWN_HOST;
  }

  /* Try the sockaddrs until a connection succeeds */
  ret = MBEDTLS_ERR_NET_UNKNOWN_HOST;
  for( current = list; current != NULL; current = current->ai_next)
  {
    ctx->fd = (int32_t) socket(current->ai_family, current->ai_socktype, current->ai_protocol);
    if(ctx->fd < 0)
    {
      ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
    }
    else
    {
      tv.tv_sec = TLS_TIMEOUT / 1000;
      tv.tv_usec = (TLS_TIMEOUT % 1000) * 1000;

      /* Set timeout */
      ret = setsockopt(ctx->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
      AVS_ASSERT(ret == 0 );
      ret = setsockopt(ctx->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
      AVS_ASSERT(ret == 0 );


      if(connect(ctx->fd, current->ai_addr, (uint32_t)current->ai_addrlen) == 0)
      {
        ret = 0;
        break;
      }

      close( ctx->fd );
      ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
  }
  }

  freeaddrinfo(list);

  return ret;
}

/*
* Create a listening socket on bind_ip:port

Notice : Generate MISRA error MISRA C 2004 rule 6.3
Called from the TLS API  using a callback and a specific prototype
*/
int mbedtls_net_bind( mbedtls_net_context *ctx, const char *bind_ip, const char *port, int proto )
{
  /* Implementation not required for this application */
  AVS_TRACE_DEBUG ("%s() NOT IMPLEMENTED!", __FUNCTION__);

  return 0;
}

/*
 * Accept a connection from a remote client
  Notice : Generate MISRA error MISRA C 2004 rule 6.3
  Called from the TLS API  using a callback and a specific prototype

 */
int mbedtls_net_accept( mbedtls_net_context *bind_ctx,
                        mbedtls_net_context *client_ctx,
                        void *client_ip, size_t buf_size, size_t *ip_len )
{
  /* Implementation not required for this application */
  AVS_TRACE_DEBUG ("%s() NOT IMPLEMENTED!!\n", __FUNCTION__);
  return 0;
}

/*
 * Set the socket blocking or non-blocking
  Notice : Generate MISRA error MISRA C 2004 rule 6.3
  Called from the TLS API  using a callback and a specific prototype

 */
int  mbedtls_net_set_block( mbedtls_net_context *ctx )
{
  /* Implementation not required for this application */
  AVS_TRACE_DEBUG ("%s() NOT IMPLEMENTED!!\n", __FUNCTION__);
  return 0;
}


/*
 * Set the socket non-blocking
  Notice : Generate MISRA error MISRA C 2004 rule 6.3
  Called from the TLS API  using a callback and a specific prototype

 */

int  mbedtls_net_set_nonblock( mbedtls_net_context *ctx )
{
  /* Implementation not required for this application */
  AVS_TRACE_DEBUG ("%s() NOT IMPLEMENTED!!\n", __FUNCTION__);
  return 0;
}

/*
 * Portable usleep helper
  Notice : Generate MISRA error MISRA C 2004 rule 6.3
  Called from the TLS API  using a callback and a specific prototype

 */
void mbedtls_net_usleep( unsigned long  usec )
{
  /* Implementation not required for this application */
  AVS_TRACE_DEBUG ("%s() NOT IMPLEMENTED!!\n", __FUNCTION__);
}

/*
 * Read at most 'len' characters
  Notice : Generate MISRA error MISRA C 2004 rule 6.3 and 20.5

  Called from the TLS API  using a callback and a specific prototype
  code provided by LWIP API usingh errno
 */

int  mbedtls_net_recv( void *ctx, unsigned char *buf, size_t len )
{
  int32_t ret;
  int32_t fd = ((mbedtls_net_context *) ctx)->fd;

  if( fd < 0 )
  {
    return MBEDTLS_ERR_NET_INVALID_CONTEXT;
  }

  ret = (int32_t) read( fd, buf, len );

  if( ret < 0 )
  {
    if(net_would_block(ctx) != 0)
    {
      return MBEDTLS_ERR_SSL_WANT_READ;
    }
    if((errno == EPIPE) || (errno == ECONNRESET))
    {
      return MBEDTLS_ERR_NET_CONN_RESET;
    }

    if(errno == EINTR)
    {
      return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return MBEDTLS_ERR_NET_RECV_FAILED;
  }

  return ret;
}

/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
  Notice : Generate MISRA error MISRA C 2004 rule 6.3
  Called from the TLS API  using a callback and a specific prototype

 */
int  mbedtls_net_recv_timeout( void *ctx, unsigned char *buf, size_t len,
                              uint32_t timeout )
{
  return mbedtls_net_recv( ctx, buf, len );
}

/*

  Notice : Generate MISRA error MISRA C 2004 rule 6.3 and 20.5
  Called from the TLS API  using a callback and a specific prototype
  reference code provided by LWIP API usingh errno

*/
static int32_t  net_would_block( const mbedtls_net_context *ctx )
{
  int32_t err = 0;
  /*
   * Never return 'WOULD BLOCK' on a non-blocking socket
   */
  int32_t val = 0;
  if( (( (uint32_t)fcntl( ctx->fd, F_GETFL, val) & ((uint32_t)O_NONBLOCK) )) != O_NONBLOCK )
  {
    return( 0 );
  }

  switch( errno )
  {
#if defined EAGAIN
    case EAGAIN:
      err = 1;
      break;
#endif
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
      err = 1;
      break;
#endif

  default:
      break;
  }

  return err;
}

/*
 * Write at most 'len' characters
  Notice : Generate MISRA error MISRA C 2004 rule 6.3 and 20.5
  Called from the TLS API  using a callback and a specific prototype
  reference code provided by LWIP API usingh errno
 */
int  mbedtls_net_send( void *ctx, const unsigned char *buf, size_t len )
{
  int32_t ret;
  int32_t fd = ((mbedtls_net_context *) ctx)->fd;

  if( fd < 0 )
  {
    return MBEDTLS_ERR_NET_INVALID_CONTEXT;
  }

  ret = (int32_t) write(fd, buf, len);

  if( ret < 0 )
  {
    if(net_would_block(ctx) != 0)
    {
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    if((errno == EPIPE) || (errno == ECONNRESET))
    {
      return MBEDTLS_ERR_NET_CONN_RESET;
    }

    if(errno == EINTR)
    {
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    return MBEDTLS_ERR_NET_SEND_FAILED;
  }

  return ret;
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free( mbedtls_net_context *ctx )
{
  if( ctx->fd == -1 )
  {
    return;
  }

  shutdown( ctx->fd, 2 );
  close( ctx->fd );

  ctx->fd = -1;
}


int32_t mbedtls_hardware_poll( void *Data, unsigned char *Output, size_t Len, size_t *oLen )
{
  uint32_t index;
  uint32_t randomValue;

  for (index = 0; index < Len / 4; index++)
  {
    if (avs_sys_get_rng(&randomValue) == AVS_OK)
    {
      *oLen += 4;
      memset(&(Output[index * 4]), (int)randomValue, 4);
    }
    else
    {
      AVS_ASSERT(0);
    }
  }

  return 0;
}



#endif /* MBEDTLS_NET_C */
