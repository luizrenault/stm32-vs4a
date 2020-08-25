/**
******************************************************************************
* @file    avs_http2_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This module manages AVS http2 connection with the AVS server
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

#define MY_BOUNDARY_TERM        "MY_BOUNDARY_TERM"
Http2Status avs_porting_http2_client_connect(AVS_instance_handle * pHandle);

/* Connection thread state */
typedef enum avs_http2_state
{
  AVS_RESTART                       = 0,
  AVS_HOSTNAME_RESOLVED             = 1,
  AVS_HTTP2_CONNECTING              = 2,
  AVS_HTTP2_INITIALIZING_DOWNSTREAM = 3,
  AVS_HTTP2_CONNECTED               = 4,
  AVS_HTTP2_CONNECTED_IDLE          = 5,
  AVS_HTTP2_DYING                   = 6
} Avs_http2_state;



/*

 HTTP2 porting layer, connect the client


*/

WEAK AVS_Result avs_porting_tls_init(void)
{
  return AVS_OK;

}



/* Close kindly the connection */
static void avs_http2_connnection_manager_close(AVS_instance_handle *pHandle);
static void avs_http2_connnection_manager_close(AVS_instance_handle *pHandle)
{

  if(pHandle->hHttpClient)
  {
    avs_core_message_send(pHandle, EVT_HTTP2_CONNECTED, FALSE);
    avs_core_mutex_lock(&pHandle->hHttpLock);

    /* Signal we are closing, in order to detect a lock in this code */
    pHandle->runCnxTaskFlag |= avs_task_about_closing;
    AVS_TRACE_DEBUG("Enter Connection delete manager ");
    /* Reduce the time-out to speed up the exit in time-out error */
    http2ClientSetTimeout(pHandle->hHttpClient, MIN_TIMEOUT);

    /* Close the directive channel */
    if (avs_directive_down_stream_channel_delete(pHandle) != AVS_OK)
    {
      AVS_TRACE_ERROR("Can't terminate down stream channel task");

    }
    /* Terminate json & pools */
    avs_json_formater_term(pHandle);

    http2ClientShutdown(pHandle->hHttpClient, HTTP2_NO_ERROR);

    AVS_TRACE_DEBUG("Close http2 client");
    /* Close the instance */
    http2ClientClose(pHandle->hHttpClient);
    /* Free the instance */
    avs_core_mem_free(pHandle->hHttpClient );
    /* Signal closed */
    pHandle->hHttpClient = 0;
  avs_core_mutex_unlock(&pHandle->hHttpLock);
  }
  /* Signal the task closed */
  AVS_TRACE_DEBUG("Leave Connection delete manager ");


}


/**
* @brief
* @param[in]
* @param[in]
**/

static void avs_http2_connnection_task(const void * argument);
static void avs_http2_connnection_task(const void * argument)
{
  Http2Status err = HTTP2_STATUS_OK;
  uint8_t no_error = 1;
  const char_t * token;
  AVS_instance_handle *pHandle = (AVS_instance_handle *)(uint32_t)argument;
  AVS_ASSERT(pHandle != 0);

  /* Init json parser */
  avs_json_formater_init(pHandle);

  /* Initialises TLS session */
  /* Waits for a valid  IP address */
  /* TODO : include this waiting loop in the thread - so that it can be interrupted */

  /* Wait the DHCP resolve the IP before to start */
  while(avs_network_check_ip_available(pHandle) != AVS_OK)
  {
    avs_core_task_delay(50);
  }

  avs_porting_tls_init(); /* Done once through avs_TlsInit */

  /* Initializes http2 internal structures.*/
  pHandle->hHttpClient = avs_core_mem_alloc(avs_mem_type_heap, sizeof(Http2ClientContext));
  AVS_ASSERT(pHandle->hHttpClient != 0 );

  /* Initialize the  http2 client */
  err = http2ClientInit(pHandle->hHttpClient );
  if (err != HTTP2_STATUS_OK)
  {
    AVS_TRACE_ERROR("FAILED with error : %d ", err);
    return ; /* [TODO] manage this error */
  }

  /* Set the time-out of 1H to follow AVS rules */
  http2ClientSetTimeout(pHandle->hHttpClient, TIMEOUT_CONNECTION);
  /* Register TLS callback as all http2 comm with avs are secured */
  err = http2ClientRegisterTlsInitCallback(pHandle->hHttpClient, avs_porting_http2_client_tls_init_Callback);
  /* Signal the Connection starts */
  avs_core_message_send(pHandle, EVT_CONNECTION_TASK_START, 0);

  avs_set_state(pHandle, AVS_STATE_RESTART);
  static Avs_http2_state status; /* [TODO] should be part of the instance ? why static  ? */

  /* Initialize the task status */
  pHandle->runCnxTaskFlag = avs_task_running;
  /* Update the connection status */
  avs_core_atomic_write(&pHandle->bConnected, FALSE);

  /* Start from restart state */
  status = AVS_RESTART;

  /* Loops until an error or a change state */
  while((no_error!= 0) && ((pHandle->runCnxTaskFlag & avs_task_running)!= 0))
  {
    switch (status)
    {
      case AVS_RESTART:
      {
        /* Signal restarting */
        avs_core_message_send(pHandle, EVT_RESET_HTTP, 0);
        status = AVS_HOSTNAME_RESOLVED;
        break;
      }
      case AVS_HOSTNAME_RESOLVED :
        /* Get Bearer access token needed to establish down-channel*/
        /* cf https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection*/
        token = avs_token_wait_for_access(); /* [TODO]: manage the case where the refresh token in flash is not valid */
        if(token == 0) 
        {
          break;
        }
        AVS_TRACE_DEBUG("Use token (bearer) = %s", token);
        /* Connect to  Amazon server.*/
        err =  avs_porting_http2_client_connect(pHandle);
        if (err == HTTP2_STATUS_OK)
        {
          AVS_TRACE_DEBUG("http2ClientConnect SUCCESSFULL ");
          status = AVS_HTTP2_CONNECTING;
        }
        else
        {
          AVS_TRACE_DEBUG("http2ClientConnect FAILED with error : %d : Retry", err);
          avs_core_task_delay(500);
        }
        break;

      case AVS_HTTP2_CONNECTING:

        /* Signal connecting state */
        avs_core_message_send(pHandle, EVT_HTTP2_CONNECTING, 0);


        /* From Amazon site:
        1.To establish a down-channel stream your client must make a GET request
        to /{{API version}}/directives within 10 seconds of opening the connection with AVS */
        /* Put in place and manage the downstream channel to manage cloud directives */

        if (avs_directive_down_stream_channel_create(pHandle) == AVS_OK)
        {
          status = AVS_HTTP2_INITIALIZING_DOWNSTREAM;
        }
        else
        {
          AVS_TRACE_ERROR("Create http2 downstream channel task FAILED ");
          /* [TODO] manage this fatal error */
        }
        break;

      case AVS_HTTP2_INITIALIZING_DOWNSTREAM:
        err = http2ClientProcessEvents(pHandle->hHttpClient, WAIT_EVENT_TIMEOUT);
        if( (err != HTTP2_STATUS_OK) && (err != HTTP2_STATUS_TIMEOUT) )
        {
          /* Avs_manageHttp2FatalError(); [TODO] just do it ! */
          status = AVS_HTTP2_DYING;
        }

        if (avs_directive_downstream_channel_state_get(pHandle) == DOWNSTREAM_READY)
        {
          status = AVS_HTTP2_CONNECTED;
        }
        break;

      case AVS_HTTP2_CONNECTED: /* Pump message */
        avs_core_atomic_write(&pHandle->bConnected, TRUE);
        /* Signal connected state */
        avs_core_message_send(pHandle, EVT_HTTP2_CONNECTED, TRUE);
        AVS_TRACE_DEBUG("Connection : OK");

        err = http2ClientProcessEvents(pHandle->hHttpClient, WAIT_EVENT_TIMEOUT);
        /* Any error to report? */
        if( (err != HTTP2_STATUS_OK) && (err != HTTP2_STATUS_TIMEOUT) )
        {
          /* Avs_manageHttp2FatalError(); [TODO] just do it ! */
          status = AVS_HTTP2_DYING;
        }
        else
        {
          status = AVS_HTTP2_CONNECTED_IDLE;
        }
        if((pHandle->runCnxTaskFlag  & avs_task_force_exit)!= 0)
        {
          status = AVS_HTTP2_DYING;
        }

        break;


      case AVS_HTTP2_CONNECTED_IDLE:
        err = http2ClientProcessEvents(pHandle->hHttpClient, WAIT_EVENT_TIMEOUT);
        /* Any error to report? */
        if( (err != HTTP2_STATUS_OK) && (err != HTTP2_STATUS_TIMEOUT) )
        {
          status = AVS_HTTP2_DYING;
        }
        if((pHandle->runCnxTaskFlag  & avs_task_force_exit) != 0)
        {
          status = AVS_HTTP2_DYING;
        }
        break;



      case AVS_HTTP2_DYING:

        pHandle->runCnxTaskFlag &= ~avs_task_running;
        break;

      default :
        break;

    }
  }
  /* Close  the connection nd all streams */
  avs_http2_connnection_manager_close(pHandle);
  /* Signal dying.. will cascade the state in the state manager to reset all connection */
  avs_core_message_send(pHandle, EVT_CONNECTION_TASK_DYING, 0);
  /* The thread will be definitively delete in a rendez-vous from the state thread */
  pHandle->runCnxTaskFlag = avs_task_closed;
  avs_core_task_end();

}


/*

 Creates the AVS Thread


*/
AVS_Result avs_http2_connnection_manager_create(AVS_instance_handle *pHandle)
{

  /* Initialize the start-up state */
  pHandle->runCnxTaskFlag = avs_task_closed;
  pHandle->bConnected = FALSE;

  /* Initialize some mutex */
  AVS_VERIFY(avs_core_mutex_create(&pHandle->hHttpLock));

  /* Start the thread */
  pHandle->hHttp2Connnection = avs_core_task_create(HTTP_CTX_TASK_NAME, avs_http2_connnection_task, pHandle, HTTP_CTX_TASK_STACK_SIZE, HTTP_CTX_TASK_PRIORITY);
  if(!pHandle->hHttp2Connnection)
  {
    AVS_TRACE_ERROR("Create task %s", HTTP_CTX_TASK_NAME);
    return AVS_ERROR;
  }

  return AVS_OK;
}

/*

 This function is called from the state thread and is in charge to clean up
 the connection in order to restart a new connection from scratch
 this state in generally called after a disconnection or an error
 it is the tricky part because stopping a connection could generate memory leak
 or hang in the low-level network due to semaphore waiting for ever


*/


AVS_Result avs_http2_connnection_manager_delete(AVS_instance_handle *pHandle)
{

  AVS_TRACE_DEBUG("Enter connection manager delete");
  /* Initialize general time-out to detect panic */
  int32_t timeout = HTTP2_CLOSE_TIMEOUT / 100; /* 10 secs */
  /* Make sure the connection is not already closed */
  if(pHandle->hHttpClient == 0)
  {
    return AVS_OK;
  }

  /* Try first to close kindly the thread */
  pHandle->runCnxTaskFlag   |= avs_task_force_exit;
  /* Make sure the thread is not already waiting for a delete */
  if((pHandle->runCnxTaskFlag & avs_task_closed)==0)
  {
    /* Force the thread to  exit */
    pHandle->runCnxTaskFlag &=  ~avs_task_running;


    /* Wait for the thread waiting for a delete state */
    while(((pHandle->runCnxTaskFlag & avs_task_closed)==0) && (timeout != 0))
    {
      avs_core_task_delay(100);
      timeout--;
    }
    /* Check if we have a panic, and the thread don't exit */
    if(timeout == 0)
    {
      /* The connection doesn't respond, let's close hard */
      AVS_TRACE_ERROR("Delete connnection_manager forced by time-out");
      /* If not locked in the close, try to force the close */
      /* If we can't close the connection, we will get for sure an hang ..... */
      /* We have 2 choices, hard close the connection and praying ... */
      /* or Reboot the board  ..... */
      /* so we choise to reboot the board, it is safer, but if we have at test pending, it will be stopped */ 
#if 0      
        avs_http2_connnection_manager_close(pHandle);
#else
        avs_core_message_send(pHandle,EVT_REBOOT,0);
        
        NVIC_SystemReset();
#endif        
    }
  }
  /* Delete the task */
  if(pHandle->hHttp2Connnection != 0)
  {
    avs_core_task_delete(pHandle->hHttp2Connnection);
  }
  /* Signal the task terminated */

  pHandle->hHttp2Connnection = 0;
  AVS_TRACE_DEBUG("Leave connection manager delete");
  /* Clean-up some mutex */
  avs_core_mutex_delete(&pHandle->hHttpLock);
  if(timeout  <= 0)
  {
    return AVS_ERROR;
  }
  return AVS_OK;
}

/*

 Send an simple event taking the form of a simple json multi part


*/
AVS_Result avs_http2_send_json(AVS_instance_handle *pHandle, const char * pJson )
{
  if(pHandle->hHttpClient == 0)
  {
    return AVS_ERROR;
  }

  Http2ClientStream *stream;
  avs_core_mutex_lock(&pHandle->hHttpLock);
  /* Check if the connection has been closed after the lock ( disconnection) */
  if((pHandle->runCnxTaskFlag & avs_task_closed) != 0)
  {
    avs_core_mutex_unlock(&pHandle->hHttpLock);
    return AVS_ERROR;
  }
  /* Create the http/2 stream to receive AVS server directives */
  stream = (avs_http_stream_handle)http2ClientOpenStream(pHandle->hHttpClient);

  /* Set the stream read time out [TODO] check time-out value */
  http2ClientSetStreamTimeout(stream, TIMEOUT_CONNECTION);


  /* Prepare the POST request header */
  Http2Status err = http2ClientSetMultipartBoundary(stream, MY_BOUNDARY_TERM);
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, ":method", "POST");
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, ":scheme", "https");
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, ":path", "/v20160207/events");
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, "authorization", (char_t *)(uint32_t)avs_token_access_lock_and_get(pHandle));
    avs_token_access_unlock(pHandle);
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, "content-type", "multipart/form-data");
  }
  /* Send HTTP/2 POST request header */
  if(!err)
  {
    err = http2ClientWriteHeader(stream, 0);
  }

  /* Prepare and send HTTP/2 first multi-part header */
  if (!err)
  {
    err = http2ClientSetMultipartHeaderField(stream, "Content-Disposition", "form-data; name=\"metadata\"");
  }
  if (!err)
  {
    err = http2ClientSetMultipartHeaderField(stream, "Content-Type", "application/json; charset=UTF-8");
  }
  if (!err)
  {
    err = http2ClientWriteMultipartHeader(stream, 0);
  }
  if(!err)
  {
    err = http2ClientWriteMultipartBody(stream, pJson, strlen(pJson), NULL, 0);
  }
  if (err)
  {
    http2ClientCloseStream(stream);
    avs_core_mutex_unlock(&pHandle->hHttpLock);
    AVS_TRACE_ERROR("Send json event FAILED");
    return AVS_ERROR;
  }

  /* Dump the message */
  AVS_PRINTF(AVS_TRACE_LVL_JSON, "SEND JSON : ");
  AVS_PRINT_STRING(AVS_TRACE_LVL_JSON, pJson);
  AVS_PRINTF(AVS_TRACE_LVL_JSON, "\r");

  /* Close kindly the stream */
  http2ClientCancelStream(stream);
  http2ClientCloseStream(stream);
  avs_core_mutex_unlock(&pHandle->hHttpLock);
  return AVS_OK;


}



/*

 Prepare a request for a multi-part json



*/
static AVS_Result avs_http2_prepare_multipart_header(AVS_instance_handle *pHandle, Http2ClientStream *stream);
static AVS_Result avs_http2_prepare_multipart_header(AVS_instance_handle *pHandle, Http2ClientStream *stream)
{
  Http2Status err = HTTP2_STATUS_OK;

  /* Prepare the header */
  if(!err)
  {
    err |= http2ClientSetMultipartBoundary(stream, MY_BOUNDARY_TERM);
  }
  if(!err)
  {
    err |= http2ClientSetHeaderField(stream, ":method", "POST");
  }
  if(!err)
  {
    err |= http2ClientSetHeaderField(stream, ":scheme", "https");
  }
  if(!err)
  {
    err |= http2ClientSetHeaderField(stream, ":authority", (const  char_t *)pHandle->pFactory->urlEndPoint);  
  }
  if(!err)
  {
    err |= http2ClientSetHeaderField(stream, ":path", "/v20160207/events");
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, "authorization", (const char_t *)(uint32_t)avs_token_access_lock_and_get(pHandle));
    avs_token_access_unlock(pHandle);
  }
  if(!err)
  {
    err |= http2ClientSetHeaderField(stream, "content-type", "multipart/form-data");
  }
  if(!err)
  {
    err |= http2ClientSetHeaderField(stream, "Content-Type", "application/json; charset=UTF-8");
  }
  if(!err)
  {
    err |= http2ClientWriteHeader(stream, 0);
  }
  if(!err)
  {
    err |= http2ClientSetMultipartHeaderField(stream, "content-type", "multipart/form-data");
  }
  if(!err)
  {
    err |= http2ClientSetMultipartHeaderField(stream, "Content-Type", "application/json; charset=UTF-8");
  }
  if(!err)
  {
    err |= http2ClientWriteMultipartHeader(stream, 0);
  }
  return err == HTTP2_STATUS_OK ? AVS_OK : AVS_ERROR;
}


/*

 Read the incoming frame ( mainly for debug purpose)


*/
AVS_Result avs_http2_read_stream(AVS_instance_handle *pHandle, avs_http_stream_handle stream, uint32_t maxsize, avs_short_alloc_object **pString, uint32_t *pRecevied)
{
  uint32_t     iPage = 0;
  size_t szBlk = 64;
  *pString = 0;
  *pRecevied = 0;
  char_t  *pStr = 0;
  char_t  *ppage = 0;
  /* Loops until the EOF or the max size */
  while(iPage < maxsize)
  {
    size_t received = 0;
    /* Realloc with the max size */
    pStr = (char_t * )avs_core_short_realloc(pStr, iPage + szBlk);
    ppage = pStr + iPage;
    /* Concatenate the block */
    Http2Status err = http2ClientReadStream(stream, ppage, szBlk, &received, 0);
    if((err == HTTP2_STATUS_END_OF_STREAM) || (received == 0))
    {
      break;
    }
    if(err != HTTP2_STATUS_OK)
    {
      AVS_TRACE_ERROR("Error Read  body");
      return AVS_ERROR;
    }
    /* Update the size for the next loop */
    iPage += received;
  }
  /* Rea-just the bloc + 4 bytes to have the possibility to add a 0 for a string  and align */
  pStr = (char_t * )avs_core_short_realloc(pStr, iPage + 4);
  /* Update the returned values */
  *pRecevied = iPage;
  *pString = pStr;
  return AVS_OK;
}

/*

 Read the incoming multi-part frame ( mainly for debug purpose)
 same as avs_http2_read_stream but multi-part

*/

AVS_Result avs_http2_read_multipart_stream(AVS_instance_handle *pHandle, Http2ClientStream *stream, uint32_t maxsize, avs_short_alloc_object **pString, uint32_t *pRecevied)
{
  uint32_t     iPage = 0;
  size_t szBlk = 64;
  *pString = 0;
  *pRecevied = 0;
  char_t  *pStr = 0;
  char_t  *ppage = 0;
  /* Loops until the EOF or the max size */
  while(iPage < maxsize)
  {
    size_t received = 0;
    /* Realloc with the max size */
    pStr = (char_t * )avs_core_short_realloc(pStr, iPage + szBlk);
    ppage = pStr + iPage;
    /* Concatenate the block */
    Http2Status err = http2ClientReadMultipartBody(stream, ppage, szBlk, &received, 0);
    if((err == HTTP2_STATUS_END_OF_STREAM) || (received == 0))
    {
      break;
    }
    if(err != HTTP2_STATUS_OK)
    {
      AVS_TRACE_ERROR("Error Read multi-part body");
      return AVS_ERROR;
    }
    /* Update size for the next loop */
    iPage += received;
  }
  /* Rea-just the bloc + 4 bytes to have the possibility to add a 0 for a string  and align */
  pStr = (char_t * )avs_core_short_realloc(pStr, iPage + 4);
  /* Update the returned values */

  *pRecevied = iPage;
  *pString = pStr;
  return AVS_OK;
}

/*

 Check the request response and read


*/

AVS_Result avs_http2_check_response(AVS_instance_handle *pHandle, Http2ClientStream *stream);
AVS_Result avs_http2_check_response(AVS_instance_handle *pHandle, Http2ClientStream *stream)
{
  /* Read HTTP/2 response header */
  if(http2ClientReadHeader(stream) != HTTP2_STATUS_OK)
  {
    AVS_TRACE_ERROR("Can't read http header");
    return AVS_ERROR;
  }

  uint32_t  statusCode = http2ClientGetStatus(stream);
  /* Check HTTP status code */
  if((statusCode >= 200) && (statusCode < 300))
  {
    return AVS_OK;
  }

  /* If error we dump the result */
  avs_short_alloc_object   *pResult;
  uint32_t szResult;
  if(avs_http2_read_stream(pHandle, stream, MAX_SIZE_JSON, &pResult, &szResult) != AVS_OK)
  {
    return AVS_ERROR;
  }
  ((char_t *)pResult)[szResult] = 0;
  AVS_TRACE_DEBUG("Response : %s", pResult);
  avs_core_short_free(pResult);
  return AVS_ERROR;
}

/*

 Return the content type before to analyse the payload


*/

const char_t  *avs_http2_stream_get_response_type(AVS_instance_handle *pHandle, Http2ClientStream *stream)
{
  Http2Status err;
  AVS_ASSERT(stream != 0);
  /* Read the header */
  err = http2ClientReadMultipartHeader(stream);
  if(err != HTTP2_STATUS_OK)
  {
    return NULL;
  }

  /* Extract the stream type as string */
  const char_t *contentType = http2ClientGetMultipartType(stream);
  return contentType;

}

/*

 Extract the json payload and parse it


*/

AVS_Result avs_http2_stream_process_json(AVS_instance_handle *pHandle, avs_http_stream_handle stream)
{
  avs_short_alloc_object     *pJSon;
  uint32_t szJSon;
  /* Read the header */
  if(avs_http2_read_multipart_stream(pHandle, stream, MAX_SIZE_JSON, &pJSon, &szJSon) != AVS_OK)
  {
    avs_http2_stream_close(pHandle, stream);
    return AVS_ERROR;
  }
  /* Add a 0 to convert it as a string */
  ((char_t*)pJSon)[szJSon] = 0;
  /* Process the JSON string */
  AVS_Result ret = avs_directive_process_json(pHandle, (char_t *)pJSon);
  /* Free the tmp string */
  avs_core_short_free(pJSon);
  return ret;
}

/*

 Process the request response and read



*/
AVS_Result avs_http2_process_response(AVS_instance_handle *pHandle, Http2ClientStream *stream);
AVS_Result avs_http2_process_response(AVS_instance_handle *pHandle, Http2ClientStream *stream)
{
  avs_short_alloc_object *pResult;
  uint32_t szResult;
  if(avs_http2_read_stream(pHandle, stream, MAX_SIZE_JSON, &pResult, &szResult) != AVS_OK)
  {
    return AVS_ERROR;
  }
  ((char_t *)pResult)[szResult] = 0;
  avs_core_short_free(pResult);
  return AVS_ERROR;
}

/*

 Open an audio http2 stream  before to play or rec


*/

Http2ClientStream *avs_http2_stream_open(AVS_instance_handle *pHandle)
{
  /* Create the http/2 stream to receive AVS server directives */
  Http2ClientStream *stream = (avs_http_stream_handle)http2ClientOpenStream(pHandle->hHttpClient);
  if (!stream)
  {
    AVS_TRACE_ERROR("Can't create audio stream");
    return NULL;
  }
  if(avs_http2_prepare_multipart_header(pHandle, stream) != AVS_OK)
  {
    avs_http2_stream_close(pHandle, stream);
    AVS_TRACE_ERROR("Can't create audio stream");
    return NULL;
  }
  return stream;
}


/*

 Force to close the audio stream


*/
AVS_Result avs_http2_stream_close(AVS_instance_handle *pHandle, Http2ClientStream *stream)
{
  if(stream == 0)
  {
    AVS_TRACE_WARNING("Audio stream already closed");
    return AVS_OK;
  }
  http2ClientCancelStream(stream);
  http2ClientCloseStream(stream);
  return AVS_OK;
}



/*

 Check audio stream active


*/

int8_t     avs_http2_stream_is_opened( AVS_instance_handle *pHandle, Http2ClientStream *stream)
{
  return stream != 0  ? TRUE : FALSE;
}


/*

 Start the record for a speaker audio feed


*/

AVS_Result avs_http2_stream_add_body(AVS_instance_handle *pHandle, Http2ClientStream *stream, const char *pJson)
{
  Http2Status err = HTTP2_STATUS_OK;

  AVS_ASSERT(stream != 0);
  /* Send the json body */
  if(!err)
  {
    err |= http2ClientWriteMultipartBody(stream, pJson, strlen(pJson), NULL, 0);
  }
  /* Send the audio stream definition part */
  if(!err)
  {
    err |= http2ClientSetMultipartHeaderField(stream, "Content-Disposition", "form-data; name=\"audio\"");
  }
  if(!err)
  {
    err |= http2ClientSetMultipartHeaderField(stream, "Content-Type", "application/octet-stream");
  }
  if(!err)
  {
    err |= http2ClientWriteMultipartHeader(stream, 0);
  }
  if(err != HTTP2_STATUS_OK)
  {
    avs_http2_stream_close(pHandle, stream);
    AVS_TRACE_ERROR("Create/Send  http multi-part error");
    return AVS_ERROR;
  }

  return AVS_OK;
}

/*

 Pushes some data in the audio stream


*/

AVS_Result avs_http2_stream_write(AVS_instance_handle *pHandle, Http2ClientStream *stream, const void *pBuffer, size_t lengthInBytes)
{
  AVS_ASSERT(stream != 0);

  Http2Status err = HTTP2_STATUS_OK;
  /* Write the payload */
  if (!err)
  {
    err = http2ClientWriteStream(stream, pBuffer, lengthInBytes, NULL, 0);
  }
  if(err)
  {
    AVS_TRACE_ERROR("Write stream error : %d", err);
    return AVS_ERROR;
  }
  return AVS_OK;
}


/*

 Close the audio stream


*/

AVS_Result avs_http2_stream_stop(AVS_instance_handle *pHandle, Http2ClientStream *stream)
{
  Http2Status err = HTTP2_STATUS_OK;
  AVS_ASSERT(stream != 0);

  /* Send the ending multi-part header */

  err |= http2ClientWriteMultipartHeader(stream, HTTP2_CLIENT_FLAG_END_STREAM );
  if(!err)
  {
    /* Check if the response is valid */
    if(avs_http2_check_response(pHandle, stream) == AVS_OK)
    {
      /* We stop the stream without to close it */
      return AVS_OK;
    }
  }
  return AVS_OK;
}


/*

 Pull some data from the audio stream


*/

AVS_Result avs_http2_stream_read(AVS_instance_handle *pHandle, Http2ClientStream *stream, void *pBuffer, uint32_t szInSByte, uint32_t *retSize)
{
  AVS_ASSERT(stream != 0);
  size_t received = 0;
  Http2Status err = HTTP2_STATUS_OK;
  err = http2ClientReadStream(stream, pBuffer, szInSByte, &received, 0);
  if(retSize)
  {
    *retSize = received;
  }
  if(err  ==  HTTP2_STATUS_END_OF_STREAM)
  {
    return AVS_EOF;
  }
  if(err)
  {
    AVS_TRACE_ERROR("Read stream error: %d", err);
    return AVS_ERROR;
  }
  return AVS_OK;
}




/* Send a ping to the server */
/* This function is asynchronous, we need to assume it could be called during a disconnection */

AVS_Result avs_http2_send_ping(AVS_instance_handle *pHandle)
{
  Http2Status err = HTTP2_STATUS_OK;
  Http2Status err2 = HTTP2_STATUS_OK;
  uint32_t  status;
  AVS_Result error = AVS_OK;
  Http2ClientStream *stream;
  avs_core_mutex_lock(&pHandle->hHttpLock);
  /* Check if the connection has been closed after the lock ( disconnection) */
  if((pHandle->runCnxTaskFlag & avs_task_closed) != 0)
  {
    avs_core_mutex_unlock(&pHandle->hHttpLock);
    return AVS_ERROR;
  }

  /* Open an HTTP/2 stream */
  if((stream = http2ClientOpenStream(pHandle->hHttpClient)) == 0)
  {
    return AVS_ERROR;
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, ":method", "GET");
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, ":scheme", "https");
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, ":authority", (const char_t *)pHandle->pFactory->urlEndPoint);
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, ":path", "/ping");
  }
  if(!err)
  {
    err = http2ClientSetHeaderField(stream, "authorization", (const char_t *)avs_token_access_lock_and_get(pHandle));
    avs_token_access_unlock(pHandle);
  }
  if(!err)
  {
    err = http2ClientWriteHeader(stream, HTTP2_CLIENT_FLAG_END_STREAM);
  }
  err2 = http2ClientReadHeader(stream);
  if((err != 0) || (err2 != HTTP2_STATUS_OK))
  {
    error = AVS_ERROR;
  }else
  {
    status = http2ClientGetStatus(stream);
    if(status == 403)
    {
      AVS_TRACE_ERROR("wrong Authentication");
      error = AVS_ERROR;
    }else if((status < 200) || (status >= 300))
    {
      error = AVS_ERROR;
    }
    else
    {
      /* do nothing */
    }
  }
  http2ClientCancelStream(stream);
  http2ClientCloseStream(stream);
  avs_core_mutex_unlock(&pHandle->hHttpLock);

  return error;
}



/*

 Post ping callback
 Called from the State idle, send a ping every PING_TIME_IN_SEC


*/

AVS_Result avs_http2_post_ping(AVS_instance_handle *pHandle)
{
  time_t curTime;
  /* Check if we are connected */
  if((avs_core_atomic_read(&pHandle->runDnChannelFlag) &  avs_task_running)==0)
  {
    return AVS_NOT_AVAILABLE;
  }
  /* Check if we are connected and time synchronized */
  if(avs_network_get_time(pHandle, &curTime) ==  AVS_NOT_SYNC)
  {
    return AVS_NOT_AVAILABLE;
  }
  /* If it is the first time, initialize the reference */
  if(pHandle->pingTime == 0)
  {
    pHandle->pingTime  = curTime + PING_TIME_IN_SEC;
  }

  /* Compute the time from the last reference */
  int32_t deltaTime = pHandle->pingTime - curTime;
  /* Check the the time is elapsed */
  if(deltaTime > 0)
  {
    return AVS_OK;
  }
  /* Re-initialize the reference */
  pHandle->pingTime = curTime + PING_TIME_IN_SEC;
  /* Send the ping */
  AVS_Result result = avs_http2_send_ping(pHandle);
  /* Signal the ping */
  avs_core_message_post(pHandle, EVT_SEND_PING, result);
  return result;
}

/*

 Send a simple notification event


*/

AVS_Result avs_http2_notification_event(AVS_instance_handle *pHandle, const char *pNameSpace, const char *pName, const char *pToken, const char *pDialogID)
{
  AVS_Result err;
  const char_t *pJson = avs_json_formater_notification_event(pHandle, pNameSpace, pName, pToken, pDialogID);
  err = avs_http2_send_json(pHandle, pJson);
  if(err != AVS_OK)
  {
    AVS_TRACE_ERROR("Notification event failed");
  }
  avs_json_formater_free(pJson);
  return AVS_OK;
}

