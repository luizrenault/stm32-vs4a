/**
******************************************************************************
* @file    service_authentication.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   retrieve the first token
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
MISRAC_DISABLE
#include "avs_network.h"
MISRAC_ENABLE


#define TASK_NAME_AUTH                  "AVS:Srv Auth"
#define TASK_PRIORITY_AUTH              (osPriorityIdle)
#define MAX_PAGE_WEB                     2000
#define GRANT_CODE_MAX_SIZE              (128+1) /* The authorization code can range from 18 to 128 characters. */
#define SELECT_TIMEOUT                   (5 * 60 *1000)
#define READ_TIMEOUT                     (1000)

#define GRANT_REQ_STRING                 "/grant_me"
#define FIRST_REQ_STRING                 "/"
#define TASK_STACK                       700


const static char_t HTTP_HTML_HDR[] =
  "HTTP/1.1 200 OK\r\n"
  "Content-type: text/html\r\n\r\n";

const static char_t HTTP_HTML_GOTCHA[] =
  "<html><head><title>Alexa authentication status"
  "</title></head><body><h1>Got the Alexa access token : </h1><p>%s</body></html>";

const static char_t HTTP_HTML_UNKNOWN_REQUEST[] =
  "<html><head><title>Alexa authentication status"
  "</title></head><body><h1>Unknown request : </h1><p>%s</body></html>";


const static char_t HTTP_REDIRECT_HEADER[] =
  "HTTP/1.1 302 Found\r\n"
  "Location: https://www.amazon.com/ap/oa?"
  "client_id=%s&"
  "scope=alexa:all&"
  "scope_data="
  "{"
  "\"alexa:all\": {"
  "\"productID\": \"%s\","
  "\"productInstanceAttributes\": {"
  "\"deviceSerialNumber\": \"%s\""
  "}"
  "}"
  "}&"
  "response_type=code&"
  "redirect_uri=%s&"
  "state=%s  "
  "Content-type: text/html\r\n\r\n";

static const char_t state_request_GrantCode[]   =  "stm32F7_RqGC"; /* [TODO] randomize ??? */
static uint32_t   gTaskRunning;
static task_t * hAuthTask;
static char_t grantCode[GRANT_CODE_MAX_SIZE];


/*

 send a buffer and scatter if mandatory
 we send this block using a loop because we are not sure that the block can be written at once


*/

static AVS_Result server_auth_write(int32_t s, const void *data, size_t length, int32_t  flags);
static AVS_Result server_auth_write(int32_t s, const void *data, size_t length, int32_t  flags)
{
  uint8_t *pCur =  (uint8_t *)(uint32_t)data;
  while(length)
  {
    int32_t written  = send( s, pCur, length, flags);
    if(written  == -1)
    {
      return AVS_ERROR;
    }
    pCur += written;
    length -= written;
  }
  return AVS_OK;
}

/*

 read a response client


*/
static AVS_Result service_auth_read_header(int32_t socket, char_t *pHeader, uint32_t maxSize, uint32_t *pRetSize);
static AVS_Result service_auth_read_header(int32_t socket, char_t *pHeader, uint32_t maxSize, uint32_t *pRetSize)
{
  char_t *pString = pHeader;
  int32_t headerSize    = maxSize;
  uint32_t count = 0;
  /* Loops until max size or end */
  while(headerSize > 0)
  {
    /* Read 1 char_t */
    int32_t read = recv(socket, (void *)pString, 1, 0);
    if(read  == -1)
    {
      return AVS_EOF;
    }
    if(read == 0)
    {
      return AVS_ERROR;
    }
    /* Update string */
    pString += read ;
    headerSize -= read ;
    count += read ;
    /* If the string finishes with to new line ... it is the end of the header */
    int32_t bFound = strncmp((char_t *)pString - 4, "\r\n\r\n", 4);
    if((count > 4) && (bFound  == 0))
    {
      break;
    }
  }
  /* Update parameters */
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


/*

 seek to char_t ...


*/
static char_t *string_seek_while(char_t *pString, char_t key);
static char_t *string_seek_while(char_t *pString, char_t key)
{
  while((*pString) && (*pString == key))
  {
    pString++;
  }
  return pString;
}
/*

 move to char_t ...


*/
static char_t *string_seek_while_not(char_t *pString, char_t key);
static char_t *string_seek_while_not(char_t *pString, char_t key)
{
  while((*pString) && (*pString != key))
  {
    pString++;
  }
  return pString;
}

/*

 parses and extract Command and request from an header


*/
void  server_auth_parse_header(char_t *pHeader, char_t *pGet, char_t *pRequest, uint32_t maxbuffer);
void  server_auth_parse_header(char_t *pHeader, char_t *pGet, char_t *pRequest, uint32_t maxbuffer)
{

  /* Isolate the command */
  char_t *pCmd = pHeader;
  char_t *pCur = string_seek_while_not(pHeader, ' ');
  AVS_ASSERT(pCur );
  uint16_t sizeBlk = pCur - pCmd;
  AVS_ASSERT(sizeBlk + 1 < maxbuffer);
  if( sizeBlk >= maxbuffer - 1)
  {
    sizeBlk = maxbuffer - 1;
  }
  memcpy(pGet, pCmd, sizeBlk );
  /* Stringify */
  *(pGet+sizeBlk) = 0;
  /* Next char_t */
  pCur++;
  pCur = string_seek_while(pCur, ' ');
  char_t *pReq = pCur;
  pCur = string_seek_while_not(pCur, ' ');
  AVS_ASSERT(pCur );
  sizeBlk = pCur - pReq;
  AVS_ASSERT(sizeBlk + 1 < maxbuffer);
  if( sizeBlk >= maxbuffer - 1)
  {
    sizeBlk = maxbuffer - 1;
  }
  memcpy(pRequest, pReq, sizeBlk );
  /* Stringfy */
  pRequest[sizeBlk] = 00;
}

/*

 Check if it is the 1 er connection , we have to redirect the client to the login page


*/
static AVS_Result server_auth_check_redirect_ctx(int32_t ctxSocket, char_t *pHeader);
static AVS_Result server_auth_check_redirect_ctx(int32_t ctxSocket, char_t *pHeader)
{
  static char_t tGet[10];
  static char_t tRequest[1000];
  server_auth_parse_header(pHeader, tGet, tRequest, sizeof(tRequest));
  /* Should be "GET" */
  if(strcmp(tGet, "GET") != 0)
  {
    return AVS_ERROR;
  }

  if(strcmp(tRequest, FIRST_REQ_STRING) != 0)
  {
    return AVS_ERROR;
  }


  char_t *pResponse = (char_t *)pvPortMalloc(MAX_PAGE_WEB);
  AVS_ASSERT(pResponse);
  snprintf(pResponse, MAX_PAGE_WEB,  HTTP_REDIRECT_HEADER,
           sInstanceFactory.clientId,/* Client_id */
           sInstanceFactory.productId,/* ProductID */
           sInstanceFactory.serialNumber,/* DeviceSerialNumber */
           sInstanceFactory.redirectUri,/* Redirect_uri */
           state_request_GrantCode);/* State */


  /* Dump the response */
  AVS_TRACE_DEBUG("Auth Server Send :");
  AVS_PRINT_STRING(AVS_TRACE_LVL_DEBUG, pResponse);

  /* Send to the client */
  server_auth_write(ctxSocket, pResponse, strlen(pResponse), 0);

  if(pResponse)
  {
    vPortFree(pResponse);
  }
  pResponse = 0;

  return AVS_OK;
}

/*

 Check the second connection  that return the grand code


*/
static AVS_Result server_auth_check_grant_code_ctx(int32_t ctxSocket, char_t *pHeader);
static AVS_Result server_auth_check_grant_code_ctx(int32_t ctxSocket, char_t *pHeader)
{
  char_t tGet[10];
  char_t tRequest[1000];
  server_auth_parse_header(pHeader, tGet, tRequest, sizeof(tRequest));

  /* Should be "GET" */
  if(strcmp(tGet, "GET") != 0)
  {
    return AVS_ERROR;
  }

  if(strncmp(tRequest, GRANT_REQ_STRING, strlen(GRANT_REQ_STRING)) != 0)
  {
    return AVS_ERROR;
  }

  if(strstr(pHeader, state_request_GrantCode) == 0)
  {
    return AVS_ERROR;
  }
  /* Extracts the access code like "code=XXXXXXXXXX" */
  int32_t i = 0;
  char_t * p = strstr(pHeader, "code");
  if (p)
  {
    p = strtok(p, " =&");
  } /* Point to "code" */
  if (p)
  {
    p = strtok(NULL, " =&");
  } /* Point to XXXXXXXX */
  while ( p && (i < sizeof(grantCode) - 1) )
  {
    if (isalnum(*p))
    {
      grantCode[i] = *p;
      i++;
      p++;
    }
    else
    {
      p = NULL;
    }
  }
  grantCode[i] = 0;
  /* Send the header OK */
  server_auth_write(ctxSocket, HTTP_HTML_HDR, sizeof(HTTP_HTML_HDR) - 1, 0);
  /* Send our HTML page */
  char_t *pResponse = (char_t *)pvPortMalloc(MAX_PAGE_WEB);
  AVS_ASSERT(pResponse);
  /* Compose the body */
  snprintf(pResponse, MAX_PAGE_WEB, HTTP_HTML_GOTCHA, grantCode);
  /* Send the authentication status */
  server_auth_write(ctxSocket, pResponse, strlen(pResponse), 0);
  /* Free */
  if(pResponse)
  {
    vPortFree(pResponse);
  }
  pResponse = 0;

  /* Provides the grant code to the avs_token   thread */
 AVS_Set_Grant_Code(hInstance, grantCode);
  return AVS_OK;
}

/*

 Manage a connection accepted


*/
static  AVS_Result  service_auth_manage_cnx(int32_t ctxSocket);
static  AVS_Result  service_auth_manage_cnx(int32_t ctxSocket)
{

  uint32_t   retSize = 0;
  char_t *pHeader = 0;
  AVS_Result result = AVS_ERROR;

  /* Pre-alloc buffers */
  pHeader = (char_t *)pvPortMalloc(MAX_PAGE_WEB);
  AVS_ASSERT(pHeader);
  if(pHeader ==0)
  {
    return AVS_ERROR;
  }


  /* Read the page header */
  service_auth_read_header(ctxSocket, pHeader, MAX_PAGE_WEB, &retSize);
  /* Stringify */
  *(pHeader+retSize) = 0;
  /* Anwser to the GET / / => redirection to Amazon */
  if(server_auth_check_redirect_ctx(ctxSocket, pHeader) != AVS_OK)
  {
    if(server_auth_check_grant_code_ctx(ctxSocket, pHeader) != AVS_OK)
    {
      char_t *pResponse = (char_t *)pvPortMalloc(MAX_PAGE_WEB);
      AVS_ASSERT(pHeader);

      /* Compose the req */
      snprintf(pResponse, MAX_PAGE_WEB, HTTP_HTML_UNKNOWN_REQUEST, pHeader);
      /* Send the page */
      server_auth_write(ctxSocket, pResponse, strlen(pResponse), 0);
      if(pResponse)
      {
        vPortFree(pResponse);
      }
    }
  }

  result = AVS_OK;
  
  if(pHeader)
  {
    vPortFree(pHeader);
  }
  pHeader = NULL;
  return result;

}


/*

 Create a server Instance


*/
static AVS_Result  service_run_server_http(void);
static AVS_Result  service_run_server_http(void)
{
  struct sockaddr_in serverAddr;
  int32_t ctxSocket = -1;
  struct timeval tv;
  AVS_Result result = AVS_ERROR;
  int32_t   hSocket = -1;
  /* Create the socket */
  hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(hSocket  < 0)
  {
    return AVS_ERROR;
  }

  /* Accept http cnx */
#ifdef   AVS_USE_NETWORK_LWIP
  serverAddr.sin_len  = sizeof(serverAddr);
#endif
  serverAddr.sin_port = htons(80);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  /* Bind on the socket to any */
  int32_t bindRet = bind(hSocket, (const struct sockaddr *)(uint32_t) &serverAddr, sizeof(serverAddr));
  if(bindRet  < 0)
  {
    AVS_TRACE_ERROR("Server bind");
    if(ctxSocket >= 0 )
    {
      closesocket(ctxSocket);
    }
    closesocket(hSocket);
    return result;

  }
  /* Listen 1 cxt at once */
  if(listen(hSocket, 1) < 0)
  {
    AVS_TRACE_ERROR("Server Listen");
    if(ctxSocket >= 0 )
    {
      closesocket(ctxSocket);
    }
    closesocket(hSocket);
    return result;
  }

  /* Run until error or end */
 while(gTaskRunning)
  {
    /* change to a long timeout for select */
    tv.tv_sec  = SELECT_TIMEOUT  / 1000;
    tv.tv_usec = (SELECT_TIMEOUT % 1000) * 1000;
    /* Set timeout */
    setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));


    /* Waits for a ctx */
    struct sockaddr from;
    socklen_t fromLen = sizeof(from);
    ctxSocket = accept(hSocket, (struct sockaddr*)&from, &fromLen);
    if(ctxSocket  >= 0)
    {
    /* change to a short timeout to have a chance to exit locking read */

      tv.tv_sec  = READ_TIMEOUT  / 1000;
      tv.tv_usec = (READ_TIMEOUT % 1000) * 1000;
      /* Set timeout */
      setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
      setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));


     /* Manage the connection */
     result = service_auth_manage_cnx(ctxSocket);
    /* Wait a bit to leave all pending operation finish */
    osDelay(100);
    }
    osDelay(100);
    closesocket(ctxSocket);

  }

  return AVS_OK;
}

/*

 main task loops for ever or delete


*/
static void service_auth_task(const void *pCookie);
static void service_auth_task(const void *pCookie)
{
  while(gTaskRunning)
  {
    service_run_server_http();
  }

}


/*

 create the authentication service


*/

AVS_Result service_authentication_create(AVS_Handle  hHandle, AVS_Instance_Factory *pFactory)
{

  gTaskRunning = TRUE;
  hAuthTask = task_Create(TASK_NAME_AUTH,service_auth_task,NULL,TASK_STACK,TASK_PRIORITY_AUTH);
  if(hAuthTask  ==0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_AUTH);
    return AVS_ERROR;
  }
  return AVS_OK;
}


/*

 delete the authentication service


*/
void  service_authentication_delete(void);
void  service_authentication_delete(void)
{
  gTaskRunning = FALSE;
  osDelay(40 * 1000);
  task_Delete(hAuthTask);

}

