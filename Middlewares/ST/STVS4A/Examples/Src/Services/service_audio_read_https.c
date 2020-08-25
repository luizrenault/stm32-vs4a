/**
******************************************************************************
* @file    service_audio_read_https.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   manage the sample reading from https
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
/******************************************************************************
 *
 * this code is provided as an example , It is not a production code
 *
 ******************************************************************************/


#include "service.h"



#define   MAX_HOST_SIZE         128
#define   HTTPS_PORT             443
#define   MAX_BUFFER            1024

typedef struct t_https_context
{
  AVS_HTls   hStream;
  uint32_t   status;
  uint64_t   contentLen;
  uint64_t   posFile;
} https_context_t;


const char_t DOWNLOAD_HEADER[] =
  "GET  %s HTTP/1.1\r\n"
  "Host: %s\r\n"\
  "User-Agent: STVS4A\r\n"\
  "\r\n";


#define MAX_LINES       30



AVS_Result service_read_response(AVS_Handle hHandle, AVS_HTls hStream, char_t  *pHeader, uint32_t maxSize, uint32_t *pRetSize);
AVS_Result service_read_response(AVS_Handle hHandle, AVS_HTls hStream, char_t  *pHeader, uint32_t maxSize, uint32_t *pRetSize)
{
  char_t *pString = pHeader;
  int32_t headerSize    = maxSize;
  uint32_t count = 0;
  uint32_t ret=0;
  while(headerSize > 0)
  {
    uint32_t retCode = AVS_Read_Tls(hHandle,hStream, (uint8_t *)(uint32_t)pString, 1,&ret);
    if((ret== 0)|| (retCode  == AVS_ERROR))
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




/* Send an header in TLS */
AVS_Result  service_send_header(AVS_Handle hHandle, AVS_HTls hstream, const char_t * pHeader);
AVS_Result  service_send_header(AVS_Handle hHandle, AVS_HTls hstream, const char_t * pHeader)
{

  const char_t *pString = pHeader;
  int32_t ret = 0;

  uint32_t headerSize    = strlen(pHeader);
  while(headerSize)
  {
    uint32_t blk = 1024;
    if(blk >= headerSize)
    {
      blk = headerSize;
    }
    ret = AVS_Write_Tls(hHandle,hstream, (unsigned char const *)(uint32_t)pString, blk,NULL);
    if(ret == AVS_ERROR )
    {
      return AVS_ERROR;
    }
    pString += ret;
    headerSize -= ret;
  }
  return AVS_OK;
}



/* Fill a Array with each line entries */
static uint32_t service_audio_parse_lines( char_t *pBuffer, char_t **tLines, uint32_t maxLines);
static uint32_t service_audio_parse_lines( char_t *pBuffer, char_t **tLines, uint32_t maxLines)
{
  uint32_t index = 0;
  while((index  < maxLines) && (*pBuffer != 0))
  {
    *tLines = pBuffer;
    tLines++;
    /* Seek to the new line */
    while((*pBuffer!=0) && (*pBuffer != '\n'))
    {
      pBuffer++;
    }
    index++;
    if(*pBuffer)
    {
      pBuffer++;
    }

  }
  return index;
}

/* Move to char ... */
static char_t *service_audio_parse_seek_to(char_t *pString, char_t key);
static char_t *service_audio_parse_seek_to(char_t *pString, char_t key)
{
  while((*pString != 0)  && (*pString != key))
  {
    pString++;
  }
  if(*pString)
  {
    pString ++;
  }
  return pString;
}

/* Move to char ... */
static char_t *service_audio_parse_seek_while(char_t *pString, char_t key);
static char_t *service_audio_parse_seek_while(char_t *pString, char_t key)
{
  while((*pString != 0)  && (*pString == key))
  {
    pString++;
  }
  return pString;
}
/* Move to char ... */
static char_t *service_audio_parse_seek_while_not(char_t *pString, char_t key);
static char_t *service_audio_parse_seek_while_not(char_t *pString, char_t key)
{
  while((*pString != 0)  && (*pString != key))
  {
    pString++;
  }
  return pString;
}

/* Check the response */
static AVS_Result service_audio_check_response(https_context_t *pCtx, char_t *pBuffer);
static AVS_Result service_audio_check_response(https_context_t *pCtx, char_t *pBuffer)
{
  char_t   *tLines[MAX_LINES];
  uint32_t nbLines = service_audio_parse_lines(pBuffer, tLines, MAX_LINES);
  pCtx->contentLen = (uint64_t) -1;
  pCtx->status     = 400;
  pCtx->posFile    = 0;
  if(nbLines < 1)
  {
    return AVS_ERROR;
  }

  /* Extract the status from the first line */
  char_t *pString = tLines[0];
  pString = service_audio_parse_seek_while_not(pString, ' ');
  pString = service_audio_parse_seek_while(pString, ' ');
  sscanf(pString, "%lu", &pCtx->status);
  /* Parses each header lines and only manage mandatory info */
  for(int32_t a = 0 ; a < nbLines ; a++)
  {
    if(strncmp(tLines[a], "Content-Length", strlen("Content-Length")) == 0)
    {
      /* Move to the size */
      char_t *pParam = service_audio_parse_seek_to(tLines[a], ':');
      pParam = service_audio_parse_seek_while(pParam, ' ');
      uint64_t len;
      sscanf(pParam, "%llu", &len);
      /* Fill len */
      pCtx->contentLen = len;
    }
  }
  /* Check error status */
  if((pCtx->status < 200) || (pCtx->status >= 300))
  {
    return AVS_ERROR;
  }
  return AVS_OK;
}


/* Close the connection */
static AVS_Result service_audio_read_http_close(struct t_player_context *pHandle);
static AVS_Result service_audio_read_http_close(struct t_player_context *pHandle)
{
  if(pHandle  == 0)
  {
    return AVS_ERROR;
  }
  https_context_t *pCtx = (https_context_t *) pHandle->pReaderCookie ;
  if(pCtx == 0)
  {
    return AVS_ERROR;
  }
  AVS_Result ret = AVS_Close_Tls(hInstance, pCtx->hStream);
  vPortFree(pCtx);
  return ret;

}

/* Split the server host name and the path */
static AVS_Result pservice_audio_split_host_request(const char_t *pURL, char_t **pHttpHost, char_t **pHttpReq);
static AVS_Result pservice_audio_split_host_request(const char_t *pURL, char_t **pHttpHost, char_t **pHttpReq)

{
  static char_t host[MAX_HOST_SIZE];
  AVS_ASSERT(pHttpHost);
  AVS_ASSERT(pHttpReq);

  if(strncmp(pURL, "https://", strlen("https://")) != 0)
  {
    return AVS_ERROR;
  }
  const char_t *pStr = ((const char_t *)pURL) + strlen("https://");
  char_t *pHost = host;
  while((*pStr != 0) && (*pStr != '/'))
  {
    *pHost = *pStr;
    pHost++;
    pStr++;
  }
  *pHost = 0;
  *pHttpHost  = host;
  *pHttpReq   = (char_t*)(uint32_t)pStr;
  return AVS_OK;
}



/* Open the stream https and send the request */
static AVS_Result service_audio_read_http_open(struct t_player_context *phandle, const char_t *pURL);
static AVS_Result service_audio_read_http_open(struct t_player_context *phandle, const char_t *pURL)

{
  char_t *pHost = 0;
  char_t *pReq = 0;
  char_t *pBuffer = 0;
  uint32_t retSize;

  if(phandle  == 0)
  {
    return AVS_ERROR;
  }
  /* Split the host part and the request */
  if(pservice_audio_split_host_request(pURL, &pHost, &pReq) != AVS_OK)
  {
    return AVS_ERROR;
  }
  /* Create the handle */
  https_context_t *pCtx = pvPortMalloc(sizeof(https_context_t ));
  AVS_ASSERT(pCtx);
  if(pCtx == 0 )
  {
    return AVS_ERROR;
  }
  memset(pCtx, 0, sizeof(https_context_t ));
  /* Open the https stream */
  pCtx->hStream = AVS_Open_Tls(hInstance, pHost, HTTPS_PORT);
  if(pCtx->hStream == NULL)
  {
    vPortFree(pCtx);
    phandle->pReaderCookie = 0;
    return AVS_ERROR;

  }
  /* Prepare to send the header */
  pBuffer = pvPortMalloc(MAX_BUFFER);
  AVS_ASSERT(pBuffer);
  if(pBuffer == 0)
  {
    AVS_Close_Tls(hInstance, pCtx->hStream);
    vPortFree(pCtx);
    phandle->pReaderCookie = 0;
    return AVS_ERROR;
  }
  /* Construct the header from the host and the request */
  snprintf(pBuffer, MAX_BUFFER - 1, DOWNLOAD_HEADER, pReq, pHost);
  /* Send the header */
  if(service_send_header(hInstance, pCtx->hStream, pBuffer) != AVS_OK)
  {
    AVS_Close_Tls(hInstance, pCtx->hStream);
    vPortFree(pCtx);
    vPortFree(pBuffer);
    phandle->pReaderCookie = 0;

    return AVS_ERROR;
  }
  /* Wait for the response */
  if(service_read_response(hInstance, pCtx->hStream, pBuffer, MAX_BUFFER - 1, &retSize) != AVS_OK)
  {
    AVS_Close_Tls(hInstance, pCtx->hStream);
    vPortFree(pCtx);
    vPortFree(pBuffer);
    phandle->pReaderCookie = 0;

    return AVS_ERROR;
  }
  pBuffer[retSize] = 0;
  /* Check the response and fill the context */
  if(service_audio_check_response(pCtx, pBuffer) != AVS_OK)
  {
    AVS_Close_Tls(hInstance, pCtx->hStream);
    vPortFree(pCtx);
    vPortFree(pBuffer);
    phandle->pReaderCookie = 0;
    return AVS_ERROR;
  }
  vPortFree(pBuffer);
  phandle->pReaderCookie = pCtx;
  return AVS_OK;
}



/*

 pull some data


*/
static AVS_Result service_audio_read_http_pull(player_context_t *pHandle, void *pBuffer, uint32_t szInSByte, uint32_t *retSize);
static AVS_Result service_audio_read_http_pull(player_context_t *pHandle, void *pBuffer, uint32_t szInSByte, uint32_t *retSize)
{
  if(pHandle  == 0)
  {
    return AVS_ERROR;
  }
  https_context_t *pCtx = (https_context_t *) pHandle->pReaderCookie ;
  if(pCtx == 0)
  {
    return AVS_ERROR;
  }
  /* Check the limite */
  if(szInSByte >= pCtx->contentLen )
  {
    szInSByte = pCtx->contentLen;
  }
  uint32_t count;
  /* Read the pool */
  AVS_Result ret = AVS_Read_Tls(hInstance, pCtx->hStream, pBuffer, szInSByte, &count);
  pCtx->posFile += count;
  if(retSize)
  {
    *retSize = count;
  }
  /* Check error/end and update position */
  if(ret == AVS_ERROR)
  {
    return AVS_ERROR;
  }
  if(ret == AVS_EOF)
  {
    return AVS_EOF;
  }
  pCtx->contentLen  -= count;
  if(pCtx->contentLen == 0)
  {
    return AVS_EOF;
  }
  return AVS_OK;
}

/* Create the HTTP reader instance */

void service_audio_read_http_create(player_reader_t *pReader)
{

  /* Initialize a network stream support */
  pReader->open  = service_audio_read_http_open;
  pReader->pull  = service_audio_read_http_pull;
  pReader->close  = service_audio_read_http_close;
}







