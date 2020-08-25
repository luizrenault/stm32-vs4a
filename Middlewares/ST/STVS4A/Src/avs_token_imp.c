/**
******************************************************************************
* @file    avs_token_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This module manages the AVS token
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

/* Dependencies */
#include "stdlib.h"
#include "avs_private.h"
/*
///////////////////////////
Format body  request
 https://api.amazon.com/auth...

*/

static  const char_t tokenRequestTemplate[] =
  "grant_type=%s&"
  "%s=%s&"
  "client_id=%s&"
  "client_secret=%s&"
  "redirect_uri=%s";


/*
///////////////////////////
Format header request
 https://api.amazon.com/auth...

*/


static const char_t tokenPostTemplate[] =
  "POST /auth/o2/token HTTP/1.1\r\n"
  "Host: api.amazon.com\r\n"
  "Content-Type: application/x-www-form-urlencoded;charset=UTF-8\r\n"
  "Content-Length: %lu\r\n"
  "\r\n";


/* This mutex must be used each time the tokenContext structure is modified (inside this file) */
/* And when the structure is read from another file (meaning probably other thread) */

AVS_Result avs_token_authentication_grant_code_set(AVS_instance_handle *pHandle, const char_t *grantCode)
{
  avs_core_mutex_lock (&pHandle->hTokenLock);
  strncpy(avs_core_get_instance()->tokenContext.grantCode, grantCode, AVS_GRANT_CODE_MAX_SIZE);
  avs_core_mutex_unlock (&pHandle->hTokenLock);
  avs_core_message_post(pHandle, EVT_RENEW_ACCESS_TOKEN, 0);
  pHandle->tokenRenewTimeStamp = 0;
  avs_core_event_set(&pHandle->hRenewToken);
  

  return AVS_OK;
}

/* Retrieve the access token. The access must be unlocked after the string has been used */
/* Return  the bearer */

const char_t *  avs_token_access_lock_and_get(AVS_instance_handle *pHandle)
{
  avs_core_mutex_lock (&pHandle->hTokenLock);
  return avs_core_get_instance()->tokenContext.pBearer;
}

/*

  unlock the token access


*/

void avs_token_access_unlock(AVS_instance_handle *pHandle)
{
  avs_core_mutex_unlock (&pHandle->hTokenLock);
}


/*

 Wait until the task has extracted the Bearer


*/
const char_t *avs_token_wait_for_access(void)
{
  AVS_instance_handle * pHandle = avs_core_get_instance();
  char_t *pBearer= avs_atomic_read_charptr(&pHandle->tokenContext.pBearer);
  if ((pBearer != 0)  && (pBearer[0] != '\0')) 
  {
    AVS_TRACE_DEBUG("A Token is already available");
    return pBearer;
  }
  uint32_t  timeout = 10000 / 1000;
  while(timeout != 0)
  {
    AVS_TRACE_DEBUG("Wait for a token available");
    pBearer= avs_atomic_read_charptr(&pHandle->tokenContext.pBearer);
    
    if ((pBearer != 0)  && (*pBearer != '\0'))
    {
      AVS_TRACE_DEBUG("--------Got a token-----------");
      return pBearer;
    }
    avs_core_task_delay(1000);
    timeout --;
  }
  return NULL;
}


/*
Extracts a token value (tokenFlag is the name between ") from http Access Token Response
  See developer.amazon.com/public/apis/engage/login-with-amazon/docs/authorization_code_grant.html
  pbuf pointer to the buffer to be scanned. Then moved to the end of the scanned part
  tokenFlag: string to be searched
  storage: where to write the value
  AVS_OK or AVS_ERROR

*/
static AVS_Result avs_token_extract_from_http_string(char_t **pbuffer, const char_t *tokenFlag, char_t *storage);
static AVS_Result avs_token_extract_from_http_string(char_t **pbuffer, const char_t *tokenFlag, char_t *storage)
{

  *storage = '0'; /* Default value if error */

  char_t *ptr = strstr(*pbuffer, tokenFlag);

  if (ptr)
  {
    ptr = strchr(ptr + strlen(tokenFlag), '\"');
    if (ptr)
    {
      ptr++;
      char_t *ptr2 = strchr(ptr, '\"');
      if (ptr2)
      {
        *ptr2 = '\0';
        strcpy(storage, ptr);
        *pbuffer = ptr2 + 1;
        return AVS_OK;
      }
    }
  }

  AVS_TRACE_DEBUG("ERROR: Could not extract token(%s) from %s", tokenFlag, *pbuffer);
  return AVS_ERROR;

}

/*
	Get tokens from Amazon form grant code or refresh token
	refreshing: false if doing from authorization code, true if doing with refresh code
	AVS_OK or AVS_ERROR

*/



static AVS_Result avs_token_get_from_amazon(AVS_instance_handle *pHandle, int8_t refreshing);
static AVS_Result avs_token_get_from_amazon(AVS_instance_handle *pHandle, int8_t refreshing)
{
  AVS_Result result = AVS_ERROR;
  AVS_HTls  hCnx;
  uint32_t  retSize = 0;
  uint32_t  bExit=0;
  uint32_t  bodySize = 0;
  char_t *pBody = NULL;
  char_t *pHeader = NULL;
  char_t *pBuf = NULL;
  char_t valueStr[20];

  /* Open the TLS connection */
  hCnx =  avs_porting_tls_open(pHandle, AUTHENTICATION_GRANT_GET_TOKEN_SERVER_NAME, AUTHENTICATION_SERVER_PORT);
  if(hCnx == 0)
  {
    return result;
  }
  /* Allocate header & body max elements */
  pBody  = avs_core_mem_alloc(avs_mem_type_heap, MAX_SIZE_WEB_ELEMENT);
  AVS_ASSERT(pBody);
  pHeader = avs_core_mem_alloc(avs_mem_type_heap, MAX_SIZE_WEB_ELEMENT);
  AVS_ASSERT(pHeader);

  /* Compose the body */
  if (refreshing)
  {
    bodySize = snprintf(pBody, MAX_SIZE_WEB_ELEMENT,
                        tokenRequestTemplate,
                        "refresh_token",
                        "refresh_token",
                        pHandle->tokenContext.pRefreshToken,
                        pHandle->pFactory->clientId,
                        pHandle->pFactory->clientSecret,
                        pHandle->pFactory->redirectUri);
  }
  else
  {
    bodySize = snprintf(pBody, MAX_SIZE_WEB_ELEMENT,
                        tokenRequestTemplate,
                        "authorization_code",
                        "code",
                        pHandle->tokenContext.grantCode,
                        pHandle->pFactory->clientId,
                        pHandle->pFactory->clientSecret,
                        pHandle->pFactory->redirectUri);

  }
  /* Compose the header */
  snprintf(pHeader, MAX_SIZE_WEB_ELEMENT, tokenPostTemplate, bodySize);

  /* Dump request */
  AVS_TRACE_DEBUG("Token: Header");
  AVS_PRINT_STRING(AVS_TRACE_LVL_DEBUG, pHeader);
  AVS_TRACE_DEBUG("Token: Body");
  AVS_PRINT_STRING(AVS_TRACE_LVL_DEBUG, pBody);

  /* Check limits */
  AVS_ASSERT(strlen(pHeader) + 1 < MAX_SIZE_WEB_ELEMENT);
  AVS_ASSERT(strlen(pBody) + 1 < MAX_SIZE_WEB_ELEMENT);


  /* Send the TLS header */
  result = avs_porting_tls_write(pHandle, hCnx, pHeader, strlen(pHeader), NULL );
  if(result  != AVS_OK )
  {
    AVS_TRACE_ERROR("Write Header  error %d", result );
    bExit = 1;
  }
  /* Send the TLS body */
  if(bExit ==0)
  {
    result  = avs_porting_tls_write(pHandle, hCnx, pBody, strlen(pBody), NULL );
    if(result != AVS_OK  )
    {
      AVS_TRACE_ERROR("Write Body  error %d", result );
      bExit = 1;
    }
  }
  if(bExit == 0)
  {
    /* Read the tls body */
    result = avs_porting_tls_read_response(pHandle, hCnx, pHeader, MAX_SIZE_WEB_ELEMENT, &retSize) ;
    if( result != AVS_OK)
    {
      AVS_TRACE_ERROR("Read response  %d", result);
      bExit = 1;
    }
  }
  if(bExit == 0)
  {
    /* Stringify */
    pHeader[retSize] = 0;

    /* Check limits */
    AVS_ASSERT(strlen(pHeader) + 1 < MAX_SIZE_WEB_ELEMENT);


    /* Dump header response */
    AVS_TRACE_DEBUG("Token: response header");
    AVS_PRINT_STRING(AVS_TRACE_LVL_DEBUG, pHeader);

    /* Extract the body len */
    AVS_VERIFY(avs_core_get_key_string(pHandle, pHeader, "Content-Length:", valueStr, sizeof(valueStr)));
    bodySize = atoi(valueStr);

    /* Read the TLS body */
    result = avs_porting_tls_read(pHandle, hCnx, pBody, MAX_SIZE_WEB_ELEMENT, &retSize );
    if(result  != AVS_OK )
    {
      AVS_TRACE_ERROR("read response body %d", result );
      bExit = 1;
    }
  }
  /* Stringify */
  *(pBody+retSize) = 0;

  /* Check limites */
  AVS_ASSERT(strlen(pBody) + 1 < MAX_SIZE_WEB_ELEMENT);

  /* Dump response   body */
  AVS_TRACE_DEBUG("Token: Response body");
  AVS_PRINT_STRING(AVS_TRACE_LVL_DEBUG, pBody);


  /* Extract the bearer (mutex protected) */
  avs_core_mutex_lock (&pHandle->hTokenLock);

  /* Pre alloc Bearer */
  pHandle->tokenContext.pBearer = (char_t*)avs_core_mem_realloc(avs_mem_type_heap, pHandle->tokenContext.pBearer, AVS_BEARER_MAX_SIZE);
  AVS_ASSERT(pHandle->tokenContext.pBearer);
  /* Copy the prefix */
  strcpy(pHandle->tokenContext.pBearer, "Bearer ");
  /* Concatenate the token */
  pBuf = pBody;
  result = avs_token_extract_from_http_string(&pBuf, "\"access_token\"", &pHandle->tokenContext.pBearer[strlen("Bearer ")]);
  if (result != AVS_OK)
  {
    /* If error ignore it */
    pHandle->tokenContext.pBearer = 0;
  }

  /* If the token is ok , re-just the alloc top the right size */
  if(pHandle->tokenContext.pBearer)
  {
    pHandle->tokenContext.pBearer = (char_t*)avs_core_mem_realloc(avs_mem_type_heap, pHandle->tokenContext.pBearer, strlen(pHandle->tokenContext.pBearer) + 1);
  }
  /* Release the mutex */
  avs_core_mutex_unlock (&pHandle->hTokenLock);

  if (result != AVS_OK)
  {
    AVS_TRACE_ERROR("could not extract the access token from http string");
    /* In case the failure is due to a wrong refresh token (wrong value read in persistent storage ?), */
    /* The http status is 400 */
    /* And the expected answer contain a part like: */
    /* "error_description":"The request has an invalid grant parameter : refresh_token","error":"invalid_grant" */
    /* Note that status 400 with "invlid_grant" can be seen in other case (such as redirect_uri) */

    /* Those information are printed in the previous dump */

    if(pHandle->tokenContext.pRefreshToken)
    {
      pHandle->tokenContext.pRefreshToken[0] = '\0';
    }
    bExit = 1;
  }

  if( bExit==0)
  {
    /* Lock the token */
    avs_core_mutex_lock (&pHandle->hTokenLock);
    /* Extract the token */
    result = avs_token_extract_from_http_string(&pBuf, "\"refresh_token\"", pHandle->tokenContext.pRefreshToken);
    /* Release lock */
    avs_core_mutex_unlock (&pHandle->hTokenLock);

    if (result != AVS_OK)
    {
      AVS_TRACE_ERROR("Could not extract the refresh token from http string");
      bExit=1;
    }
  }
  if(bExit ==0)
  {
    result = AVS_ERROR;
    /* Save the refresh token for next boot */
    if (pHandle->pFactory->persistentCB)
    {
      result = (AVS_Result)pHandle->pFactory->persistentCB(AVS_PERSIST_SET,AVS_PERSIST_ITEM_TOKEN,pHandle->tokenContext.pRefreshToken,strlen(pHandle->tokenContext.pRefreshToken)+1);
    }
    else
    {
      AVS_TRACE_ERROR("tokenWriteToStorageCB is null... can't store the token in persistant memory");
    }
    /* Signal state */
    avs_core_message_post(pHandle, EVT_WRITE_TOKEN, result);
    /* Signal ok */
    result = AVS_OK;
  }

  /* Free block used and not free */
  if(pHeader)
  {
    avs_core_mem_free(pHeader);
  }
  avs_core_mem_free(pBody);
  /* Close the connection */
    avs_porting_tls_close(pHandle, hCnx);
  hCnx = 0;

  if(result != AVS_OK)
  {
    AVS_TRACE_ERROR("Failed to communicate with SSL server ? error=%d", result);
  }
  /* Return status code */
  return result;
}


/*

 try once to get tokens from refresh code, if none or failing, tries from grant code (found in tokenContext)

*/
static AVS_Result avs_token_refresh_or_get_from_amazon(AVS_instance_handle *pHandle);
static AVS_Result avs_token_refresh_or_get_from_amazon(AVS_instance_handle *pHandle)
{
  /* Get tokens from an existing refresh code */
  if (strcmp(pHandle->tokenContext.pRefreshToken, ""))
  {
    AVS_TRACE_DEBUG("refreshOrGetTokensFromAmazon: I already have a refresh token");
    AVS_TRACE_DEBUG("gonna renew tokens");
    if (avs_token_get_from_amazon(pHandle, TRUE) == AVS_OK)
    {
      AVS_TRACE_DEBUG("refreshOrGetTokensFromAmazon: refreshed the tokens");
      return AVS_OK;
    }
  }

  AVS_Result result = AVS_ERROR;
  if(strcmp(pHandle->tokenContext.grantCode, ""))
  {
    /* We have a grant authorization code, try to get an access code from there */
    result = avs_token_get_from_amazon(pHandle, FALSE);
  }

  if (result != AVS_OK)
  {
    /* Lock token */
    avs_core_mutex_lock (&pHandle->hTokenLock);
    if(pHandle->tokenContext.pBearer)
    {
      avs_core_mem_free(pHandle->tokenContext.pBearer);
    }
    pHandle->tokenContext.pBearer = 0;
    /* Unlock token */
    avs_core_mutex_unlock (&pHandle->hTokenLock);
    AVS_TRACE_DEBUG("could not get token from amazon with code %s", pHandle->tokenContext.grantCode);
  }
  return result;
}


/*

 Token task in charge to retrieve the grand code at a periodical time


*/
static void avs_token_refresh_task(const void *param);
static void avs_token_refresh_task(const void *param)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)(uint32_t)param;

  /* Before to check the token, we need an IP address and time OK*/
  /* Let's wait for it */
  
  while(avs_network_synchronize_time(pHandle) != AVS_OK)
  {
    AVS_TRACE_ERROR("Sync Time error");
  }

  while(TRUE)
  {
    if(avs_network_get_time(pHandle,NULL) == AVS_OK) 
    {
      break;
    }
    osDelay(200);
  }
  

  /* Signal state */
  avs_core_message_send(pHandle, EVT_REFRESH_TOKEN_TASK_START, 0);

  /* Get refresh token that had been store in flash in a previous session (if any) */
  AVS_Result  ret = AVS_ERROR;
  if (pHandle->pFactory->persistentCB)
  {
    /* Alloc tmp buffer */
    pHandle->tokenContext.pRefreshToken = avs_core_mem_calloc(avs_mem_type_heap, AVS_TOKEN_MAX_SIZE, 1);
    AVS_ASSERT(pHandle->tokenContext.pRefreshToken);

    uint32_t err = pHandle->pFactory->persistentCB(AVS_PERSIST_GET,AVS_PERSIST_ITEM_TOKEN, pHandle->tokenContext.pRefreshToken, AVS_TOKEN_MAX_SIZE);
    if((pHandle->tokenContext.pRefreshToken != 0)  && (err == AVS_OK))
    {
      /* Ajust  the block size */
      pHandle->tokenContext.pRefreshToken = avs_core_mem_realloc(avs_mem_type_heap, pHandle->tokenContext.pRefreshToken, strlen(pHandle->tokenContext.pRefreshToken) + 1);
    }
  }
  /* If the string is empty or NULL, we can't extract  the token from  the app registry */
  uint32_t len = strlen(pHandle->tokenContext.pRefreshToken );
  if((pHandle->tokenContext.pRefreshToken  == NULL) || (len==0))
  {
    AVS_TRACE_WARNING("RefreshToken is null... can't get a token from a previous session from persistant memory");
  }
  /* Signal state */
  avs_core_message_post(pHandle, EVT_READ_TOKEN, ret);

  AVS_Result result = AVS_ERROR;

  /* Init task status */
  pHandle->runTokenFlag = avs_task_running;

  pHandle->tokenRenewTimeStamp = 0;
  /* Signal state */
  avs_core_message_post(pHandle, EVT_WAIT_TOKEN, 0);

  /* Renew the token if true */
  uint32_t bRenew = TRUE;

  /* Get and refresh tokens */

  /* In order to be able to force the renew on demand */
  /* We check the time periodicaly and renew only after AVS_TOKEN_RENEW_MAX */

  while((pHandle->runTokenFlag != 0) && (avs_task_running != 0))
  {
    /* No renew by default */
    bRenew = FALSE;

    /* Init   last time renew , if first time */
    if(pHandle->tokenRenewTimeStamp == 0)
    {
      bRenew = TRUE;
      avs_core_message_post(pHandle, EVT_WAIT_TOKEN, 0);
    }
    else
    {
      /* If the time is elapsed, we force the renew */
      if(osKernelSysTick() - pHandle->tokenRenewTimeStamp > AVS_TOKEN_RENEW_MAX)
      {
        bRenew = TRUE;
      }

    }
    if(bRenew)
    {
      /* Retrieve the Token */
      result = avs_token_refresh_or_get_from_amazon(pHandle);
      if (result == AVS_OK)
      {
        /* Signal state */
        avs_core_message_post(pHandle, EVT_VALID_TOKEN, 0);
        /* Time stamp the token renew */
        pHandle->tokenRenewTimeStamp = osKernelSysTick();
        AVS_TRACE_DEBUG("Time %d",pHandle->tokenRenewTimeStamp );
      }
      else
      {
        if(pHandle->tokenRenewTimeStamp)
        {
          /* Signal state */
          avs_core_message_post(pHandle, EVT_WAIT_TOKEN, 0);

        }
        /* Signal renew state */
        pHandle->tokenRenewTimeStamp = 0;

        /* Sleep a bit */
        avs_core_task_delay(DELAY_SLEEP);
      }
    }
    /* This token is valid for one hour, set the wakeup delay */
    avs_core_event_wait(&pHandle->hRenewToken, AVS_TOKEN_RENEW_PERIOD);
  }
  pHandle->runTokenFlag  = avs_task_closed;
  /* Signal state */
  avs_core_message_send(pHandle, EVT_REFRESH_TOKEN_TASK_DYING, 0);
  /* The thread will be definitively deleted from another thread */
  avs_core_task_end();
}

/*

 Create the refresh token task


*/

AVS_Result avs_token_refresh_create(AVS_instance_handle *pHandle)
{
  /* Create needed mutex */
  AVS_VERIFY(avs_core_mutex_create(&pHandle->hTokenLock));
  AVS_VERIFY(avs_core_event_create(&pHandle->hRenewToken));

  AVS_VERIFY((pHandle->hTokenTask = avs_core_task_create(TOKEN_TASK_NAME, avs_token_refresh_task, pHandle, TOKEN_TASK_STACK_SIZE, TOKEN_TASK_PRIORITY)) != NULL);
  if(!pHandle->hTokenTask )
  {
    AVS_TRACE_ERROR("Create task %s", TOKEN_TASK_NAME);
    return AVS_ERROR;
  }
  return AVS_OK;
}

/*

 Delete the refresh token task


*/

AVS_Result avs_token_refresh_delete(AVS_instance_handle *pHandle)
{
  /* Init general time-out to detect panic */
  int32_t timeout = HTTP2_CLOSE_TIMEOUT / 100; /* 10 secs */
  if( (pHandle->runTokenFlag==0)   && (avs_task_closed != 0))
  {
    /* Remove the run flag */
    pHandle->runTokenFlag   &= ~avs_task_running;
    /* Signal the event to exit de long time-out */
    avs_core_event_set(&pHandle->hRenewToken);

    /* Wait for the thread waiting for a delete state */
    while(((pHandle->runTokenFlag & avs_task_closed)==0) && (timeout != 0))
    {
      avs_core_task_delay(100);
      timeout--;
    }
    /* Check if we have a panic, and the thread don't exit */
    if(timeout == 0)
    {
      /* The connection doesn't respond, let's close hard */
      AVS_TRACE_ERROR("Delete token time-out");
    }
  }
  /* We can delete the task */
  if(pHandle->hTokenTask)
  {
    avs_core_task_delete(pHandle->hTokenTask);
  }
  /* Delete some objects */
  avs_core_mutex_delete(&pHandle->hTokenLock);
  avs_core_event_delete(&pHandle->hRenewToken);


  return AVS_OK;
}
