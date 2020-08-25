/**
******************************************************************************
* @file    avs_sdk_entry_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   API SDK entries
*
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include "avs_private.h"
/* Some globals */

AVS_Result avs_lastError  = AVS_OK;
uint32_t   avs_debugLevel = AVS_TRACE_LVL_DEFAULT;      /* All message by default */

#if 0
#define LOCK_ENTRY()
#define UNLOCK_ENTRY()
#define LOCK_TRACE()
#define UNLOCK_TRACE()
#else
#define LOCK_ENTRY()   avs_api_lock()
#define UNLOCK_ENTRY() avs_api_unlock()
#define LOCK_TRACE()   avs_core_lock_tasks()
#define UNLOCK_TRACE() avs_core_unlock_tasks()
#endif
void avs_api_lock(void);
void avs_api_lock(void)
{
  if(avs_core_get_instance())
  {
    avs_core_recursive_mutex_lock(&avs_core_get_instance()->entryLock);
  }
}
void avs_api_unlock(void);
void avs_api_unlock(void)
{
  if(avs_core_get_instance())
  {
    avs_core_recursive_mutex_unlock(&avs_core_get_instance()->entryLock);
  }
}
void  avs_gen_long_delay(uint64_t delay);
void  avs_gen_long_delay(uint64_t delay)
{
  for( uint64_t a = 0; a < delay ; a++)
  {
  }
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
#include "avs.h"


/*!
@brief Initialize the AVS API

First call to initialize an STVS4A session.

@return AVS_OK or AVS_ERROR
*/

AVS_Result AVS_Init(void)
{
  
  if (avs_createPorting() != AVS_OK)
  {
    AVS_TRACE_ERROR("Creating porting  layer");
    return AVS_ERROR;
  }
  return AVS_OK;
}

/*!
@brief Terminate the AVS API

Last call to terminate the session.

@return AVS_OK or AVS_ERROR
*/

AVS_Result AVS_Term(void)
{
  avs_deletePorting();
  return AVS_OK;
}




/*!
@brief Print an output string on the console

This function doesn't format the string, and don't have string size limitation
Warning, if the flag AVS_TRACE_LVL_JSON_FORMATED is set, the function will try
to format and indent the string with a JSON format

@param level   Debug level 
@param pString String to print 
@return void

*/
void  AVS_Puts(uint32_t level,const char *pString)     
{
  if((level & avs_debugLevel) != 0U)
  {
    LOCK_TRACE();
    if((avs_debugLevel & (uint32_t)AVS_TRACE_LVL_JSON_FORMATED) !=0U)
    {
      avs_core_trace_json(pString);
    }
    else
    {
      drv_sys.platform_Sys_puts(pString, 0);
    }
    UNLOCK_TRACE();
    
  }
  
}

/*!
@brief  Dumps a memory block on the console 

This function is mainly used for debug purpose, It allow to dump a memory block on the serial console 
the dump display the offset, bytes, and ascii.

@param level Debug level
@param pTitle The string that will be displayed in the dump header 
@param pData  Pointer on the memory block
@param size Block size
@return void

*/
void       AVS_Dump(uint32_t level,const char_t *pTitle, void *pData,uint32_t size)
{
  uint32_t segment = 8;
  uint8_t  *pData8 = (uint8_t  *)pData;
  if(pTitle) 
  {
    AVS_Printf(level,"%s 0x%p:0x%03X \n",pTitle,pData,size);
  }
  uint32_t index = 0;
  while(index <  size)
  {
    uint8_t  *p8= &(pData8[index]);
    AVS_Printf(level,"%03x: ",index);
    
    for (uint32_t b=0 ; b < segment; b++)
    {
      if((b+index) < size)
      {
        AVS_Printf(level,"%02x ", p8[b]);
      }
      else
      {
        AVS_Printf(level,"   ");
      }
    }
    AVS_Printf(level," ");
    
    for (uint32_t b=0 ; b < segment; b++)
    {
      char_t ascii;
      if(b+index < size)
      {
        if ((p8[b] < (uint8_t)' ') || (p8[b] > (uint8_t)128)) 
        {
          ascii = '.';
        }
        else 	
        {
          ascii = (char_t)p8[b];
        }
      }
      else
      {
        ascii=' ';
      }
      AVS_Printf(level,"%c", ascii);
      
    }
    index += (uint32_t)8;
    AVS_Printf(level,"\n");
  }
  AVS_Printf(level,"\n");
}



/*!
@brief  Print an output string on the console + carriage return

This function is similar to AVS_Printf, but it is limited in size, the final string should be less than 400 bytes.
If the result of the string is higher, the end of string after 400 will be clamped with "..."
AVS_Trace is used for TRACE and ASSERT, so we can pass the filename and the line number to localize an error
@param level  Debug level 
@param pFile  File name to print 
@param line   Line number 
@param  ...   Format string and parameters 
@return void
*/
void AVS_Trace(uint32_t level, const char_t *pFile, uint32_t line, ...)
{
  if (((level & avs_debugLevel) != 0U))
  {
    
    va_list args;
    va_start(args, line);
    
    char_t *pFormat = va_arg( args, char *);
    const char_t *pPrefix = avs_core_get_level_string(level);
    if(pPrefix )
    {
      AVS_Printf(level, "%s : ", pPrefix );
    }
    if(pFile)
    {
      /* Remove the full path */
      char_t *pf = strchr(pFile, 0);
      while((AVS_IS_VALID_POINTER(pf)) && (pf != pFile) &&  (*pf != '\\') &&  (*pf != '/'))
      {
        pf--;
      }
      if(pFile != pf)
      {
        pf++;
      }
      pFile = pf;
      AVS_Printf(level, "%s:%d : ", pFile, line);
    }
    LOCK_TRACE();
    avs_core_trace_args(1, pFormat, args);
    UNLOCK_TRACE();
    
    va_end(args);
  }
}




/*!
@brief  Print an output string on the console

This function is similar AVS_Trace but doesn't have filename, line number parameter
The function doesn't add a carriage return at the end of the string

@param level Debug level 
@param pFormat Format string, similar to the stdlib printf 
@param  ...   Format string and parameters 
@return void

*/
void  AVS_Printf(uint32_t level, const char_t *pFormat, ...)
{
  if((level & avs_debugLevel) != 0U )
  {
    va_list args;
    va_start(args, pFormat);
    LOCK_TRACE();
    avs_core_trace_args(0, pFormat, args);
    UNLOCK_TRACE();
    va_end(args);
    
  }
}


/*!
@brief  Set the  trace debug level

Allows to select the type of debug info

@param level Bits combination, see also AVS_TRACE_LVL_XXX
@return The previous level

*/
uint32_t AVS_Set_Debug_Level(uint32_t level)
{
  LOCK_ENTRY();
  uint32_t  prevLvl = avs_debugLevel;
  avs_debugLevel = level;
  UNLOCK_ENTRY();
  return prevLvl;
}



/*!
@brief  Check the condition  and produces a Assert message

This function is called by Macros such as AVS_TRACE_XXX

@param eval Evaluation condition, TRUE or FALSE 
@param peval Condition string 
@param line  Line where occurs the assert
@param file  File where occurs the assert
@return void

*/
void AVS_Assert(int32_t eval, const char *peval, uint32_t line, const char *file)
{
  if (eval == 0)
  {
    if(AVS_IS_VALID_POINTER(file))
    {
      /* Remove the full path */
      char_t *pFile = strchr(file, 0);
      while(AVS_IS_VALID_POINTER(pFile)  && (pFile != file) &&  (*pFile != '\\') &&  (*pFile != '/'))
      {
        pFile--;
      }
      if(file != pFile)
      {
        pFile++;
      }
      file = pFile;
    }
    AVS_Printf((uint32_t)AVS_TRACE_LVL_ERROR, "Assert : %s %s:%d\n", peval, file, line);
      AVS_Signal_Exeception(NULL, AVS_SIGNAL_EXCEPTION_ASSERT);
    }
  
  
}

/*!
@brief Creates the AVS instance

Creates the AVS instance using the factory parameter. The Factory parameter must be set to zero before to be filled with desired value , when a zero is set on any field ,the API will use a default value *

@param  pFactory Factory pointer, this object must be set to zero before to fill it
@return Instance Handle
*/

AVS_Handle AVS_Create_Instance(AVS_Instance_Factory *pFactory)
{
  AVS_Result result;
  avs_default_instance_solver(pFactory);
  AVS_instance_handle *pHandle = (AVS_instance_handle *)pvPortMalloc(sizeof(AVS_instance_handle));
  AVS_ASSERT(pHandle != 0);
  memset(pHandle, 0, sizeof(*pHandle));
  pHandle->pFactory = pFactory;
  pHandle->signature = AVS_INSTANCE_SIGNATURE;
  avs_core_set_instance(pHandle);
  
  if (drv_instance.platform_init(pHandle) != AVS_OK)
  {
    AVS_TRACE_ERROR("Can't Create the AVS instance");
    return (AVS_Handle)AVS_NULL;
  }
  avs_core_mem_init(pHandle);
  /* Mutex lock the for message dispatch  & entry re-entrance */
  avs_core_recursive_mutex_create(&pHandle->lockMessage);
  avs_core_recursive_mutex_create(&pHandle->entryLock);
  avs_core_mutex_create(&pHandle->lockTrace);
  avs_core_queue_create_named(&pHandle->hPostMsg,AVS_MAX_POST_ELEMENT,sizeof(avs_post_t),NULL);
  /* Init audio instance */
  avs_default_audio_solver(pFactory);
  AVS_audio_handle *pAHandle = (AVS_audio_handle *)pvPortMalloc(sizeof(AVS_audio_handle));
  AVS_ASSERT(pHandle != 0);
  memset(pAHandle, 0, sizeof(*pAHandle));
  pAHandle->pFactory = pFactory;
  drv_audio.platform_Audio_default(pAHandle);
  pAHandle->pInstance = pHandle;
  pHandle->pAudio = pAHandle;
  pAHandle->volumeOut = (int32_t)pFactory->initVolume;
  result = avs_audio_create(pAHandle);
  AVS_ASSERT((result == AVS_OK));
  if(result != AVS_OK)
  {
    AVS_TRACE_ERROR("Error : Create audio device!");
    return AVS_ERROR;
  }
  /* Initialize the IP stack */
  result = avs_network_config(pHandle);
  if(result != AVS_OK)
  {
    AVS_TRACE_ERROR("Network  initialization error");
  }
  result = avs_state_create(pHandle);
  AVS_ASSERT((result == AVS_OK));
  if(result != AVS_OK)
  {
    vPortFree(pHandle);
    return AVS_ERROR;
  }
  result  = avs_token_refresh_create(pHandle);
  AVS_ASSERT((result == AVS_OK));
  if(result != AVS_OK)
  {
    vPortFree(pHandle);
    return AVS_ERROR;
  }
  pHandle->bInstanceStarted = 1;
  return (AVS_Handle)pHandle;
}

/*!
@brief Close the AVS instance

All objects and threads will be deleted

@param hHandle Instance Handle 
@return AVS_OK or AVS_ERROR
*/

AVS_Result AVS_Delete_Instance(AVS_Handle hHandle)

{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hHandle;
  AVS_ASSERT(pHandle != 0);
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid Handle ");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  AVS_VERIFY(avs_state_delete(pHandle));
  AVS_VERIFY(avs_token_refresh_delete(pHandle));
  AVS_VERIFY(avs_audio_delete(pHandle->pAudio));
  AVS_VERIFY(avs_core_queue_delete(&pHandle->hPostMsg));

  if(pHandle->pLastToken)
  {
    avs_core_mem_free(pHandle->pLastToken);
  }
  avs_core_mutex_delete(&pHandle->lockTrace);
  avs_core_recursive_mutex_delete(&pHandle->lockMessage);
  avs_core_recursive_mutex_delete(&pHandle->entryLock);
  drv_instance.platform_term(pHandle);
  avs_core_mem_term();
  vPortFree(pHandle->pAudio);
  vPortFree(pHandle);
  avs_core_set_instance((AVS_instance_handle *)AVS_NULL);
  
  return AVS_OK;
}


/*!
@brief Set the authentication Grant Code

The Grant Code is mandatory to start an AVS session, the Grant Code comes from the 
authentication processes and allow to get the first Access Token

@param hHandle   Instance Handle 
@param grantCode The string that represents the Grant Code 
@return AVS_OK or AVS_ERROR
*/


AVS_Result  AVS_Set_Grant_Code(AVS_Handle hHandle, const char_t *grantCode)
{
  AVS_instance_handle *pHandle= (AVS_instance_handle *)hHandle;
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  LOCK_ENTRY();
  AVS_Result ret = avs_token_authentication_grant_code_set(pHandle, grantCode);
  UNLOCK_ENTRY();
  return ret;
}



/*!
@brief Get the HTTP/2 instance

It is an opaque handle you can use to open a stream
this handle dependences on the HTTP2 porting layer implementation

@param hInstance Instance Handle 
@return void * Opaque handle
*/

void *  AVS_Get_Http2_Instance(AVS_Handle hInstance )
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT((AVS_IS_VALID_POINTER(pHandle)));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return (void *)AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  return pHandle->hHttpClient;
}


/*!
@brief Returns the Access token Code (bearer)

The Access token code is used for all http/2 transaction between the ALEXA and the device 

@param hInstance Instance Handle
@param bearer  A string container where to copy the access token 
@param maxSize  Size of the bearer buffer 
Return the current Access Token used by STVS4A 
@return  A pointer to the copied string, or null in case of error
*/

const char_t *AVS_Get_Token_Access(AVS_Handle hInstance,char_t *bearer,size_t maxSize )
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle) || AVS_IS_NULL_POINTER(bearer))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return (const char_t *)NULL;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  const char_t *ret = NULL;
  LOCK_ENTRY();
  
  const char_t *token = (const char_t *)avs_token_access_lock_and_get(pHandle);
  if (strlen(token) < maxSize)
  {
    ret = strcpy(bearer, token);
  }
  avs_token_access_unlock(pHandle);
  
  UNLOCK_ENTRY();
  return ret;
}



/*!
@brief Allows to deal directly with the porting layer

This function allows to deal directly with the porting layer
and call a specific function, some IOCTL codes are standard
some other are specific to a porting and are defined in porting.h

@param hInstance  Instance Handle 
@param id       IOCTL code @see AVS_CodeCtl_t 
@param wparam   First parameter 
@param lparam   Second parameter
@return AVS_NOT_IMPL of the IOCTL is not handled or the IOCTL result
*/

AVS_Result      AVS_Ioctl(AVS_Handle hInstance, AVS_CodeCtl id , uint32_t wparam, uint32_t lparam)
{
  AVS_Result  result;
  AVS_instance_handle *pHandle;
  AVS_VERIFY((pHandle = (AVS_instance_handle *)hInstance));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Handle invalid");
    return AVS_ERROR;
  }
  /* Check the handle validity */
  CHECK_INSTANCE_SIGN(pHandle);
  /* Notice that IOCTL has no lock entry to prevent deadlock */
  /* The lock must be implemented inside the ioctl */
  
  result = (AVS_Result)drv_sys.platform_Sys_ioctl(pHandle, id, wparam, lparam);
  if(result  != AVS_NOT_IMPL)
  {
    return result;
  }
  
  result = (AVS_Result)drv_sys.platform_Network_ioctl(pHandle, id, wparam, lparam);
  if(result  != AVS_NOT_IMPL)
  {
    return result;
  }
  result = (AVS_Result)drv_audio.platform_MP3_decoder_ioctl(pHandle->pAudio, id, wparam, lparam);
  if(result  != AVS_NOT_IMPL)
  {
    return result;
  }
  result = (AVS_Result)drv_audio.platform_Audio_ioctl(pHandle->pAudio, id, wparam, lparam);
  if(result  != AVS_NOT_IMPL)
  {
    return result;
  }
  return AVS_NOT_IMPL;
  
}


/*!
@brief Play a short sound.

It is possible to play only one sound at once, if a sound is already in progress
AVS_Play_Sound will return AVS_BUSY
In order to limit the foot print and the CPU load, we can produce only up sampling. 
The sample must always be mono and never have an higher frequency than the native audio device

@param hHandle Instance Handle 
@param flags   PlaySound flag bits @see Avs_PlaySound_Flags_t
@param pWave   Raw .WAV file
@param volumePercent The sound volume in percentage from 0 to 200 
@return AVS_BUSY, AVS_OK,AVS_ERROR
*/

AVS_Result AVS_Play_Sound(AVS_Handle hHandle, uint32_t flags,void *pWave,int32_t volumePercent)
{
  AVS_ASSERT(hHandle != 0);
  if(AVS_IS_NULL_POINTER(hHandle))
  {
    AVS_TRACE_ERROR("Invalid Handle ");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(((AVS_instance_handle *)hHandle));
  
  LOCK_ENTRY();
  AVS_Result ret = avs_audio_play_sound(((AVS_instance_handle *)hHandle)->pAudio, flags, pWave, volumePercent);
  UNLOCK_ENTRY();
  return ret;
}




/*!
@brief  Inject a payload in the audio auxiliary channel

Notice the payload must be accurate with the auxiliary parameter ( frequency, channels) 
STVS4A will apply a Sample Rate conversion to fit the device output frequency and format 

@param hHandle   Instance Handle 
@param pSamples  Pointer on samples 
@param nbSample  Samples number to inject 
@return The real sample number injected. If the number is different of the nbSample parameter, the ring buffer is full.
*/

uint32_t AVS_Feed_Audio_Aux(AVS_Handle hHandle,int16_t *pSamples, uint32_t nbSample)
{
  AVS_instance_handle *pHandle;
  AVS_VERIFY((pHandle = (AVS_instance_handle *)hHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Handle invalid");
    return (uint32_t)AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  if(pHandle->pFactory->useAuxAudio == 0U)
  {
    AVS_TRACE_ERROR("Auxiliary channel disabled");
    return (uint32_t)AVS_ERROR;
  }
  LOCK_ENTRY();
  avs_core_mutex_lock(&pHandle->pAudio->auxAudioPipe.lock);
  nbSample = avs_audio_buffer_produce(&pHandle->pAudio->auxAudioPipe.inBuffer, nbSample, pSamples);
  avs_core_mutex_unlock(&pHandle->pAudio->auxAudioPipe.lock);
  UNLOCK_ENTRY();
  return nbSample;
}


/*!
@brief   Get auxiliary channel info

This function is used to manipulate the auxiliary channel ( ring buffer, formats)

@param hHandle  Instance Handle 
@param pInfo    Pointer on Auxiliary channel info
@return AVS_ERROR or AVS_OK
*/
AVS_Result AVS_Get_Audio_Aux_Info(AVS_Handle hHandle,AVS_Aux_Info *pInfo)
{
  AVS_instance_handle *pHandle;
  AVS_VERIFY((pHandle = (AVS_instance_handle *)hHandle));
  if(AVS_IS_NULL_POINTER(pHandle) || AVS_IS_NULL_POINTER(pInfo))
  {
    AVS_TRACE_ERROR("Handle invalid");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  if(pHandle->pFactory->useAuxAudio == 0U)
  {
    AVS_TRACE_ERROR("Auxiliary channel disabled");
    return AVS_ERROR;
  }
  LOCK_ENTRY();
  avs_core_mutex_lock(&pHandle->pAudio->auxAudioPipe.lock);
  pInfo->infoFlags  = 0;
  pInfo->szBuffer = pHandle->pAudio->auxAudioPipe.inBuffer.szBuffer;
  pInfo->szConsumer = pHandle->pAudio->auxAudioPipe.inBuffer.szConsumer;
  pInfo->szProducer = (int32_t)avs_audio_buffer_get_producer_size_available(&pHandle->pAudio->auxAudioPipe.inBuffer);
  pInfo->iFlags     = pHandle->pAudio->auxAudioPipe.pipeFlags;
  pInfo->frequency  = pHandle->pAudio->auxAudioPipe.inBuffer.sampleRate;
  pInfo->threshold  = pHandle->pAudio->auxAudioPipe.threshold;
  pInfo->nbChannel  = pHandle->pAudio->auxAudioPipe.inBuffer.nbChannel;
  pInfo->volume     = pHandle->pAudio->auxAudioPipe.inBuffer.volume;
  avs_core_mutex_unlock(&pHandle->pAudio->auxAudioPipe.lock);
  UNLOCK_ENTRY();
  return AVS_OK;
}


/*!
@brief   Set auxiliary channel info

This function is used to manipulate the auxiliary channel  ( ring buffer, formats)

@param hHandle  Instance Handle 
@param pInfo    Pointer on auxiliary channel info 
@return AVS_ERROR  or AVS_OK
*/

AVS_Result AVS_Set_Audio_Aux_Info(AVS_Handle hHandle, AVS_Aux_Info *pInfo)
{
  AVS_instance_handle *pHandle;
  AVS_VERIFY((pHandle = (AVS_instance_handle *)hHandle));
  if(AVS_IS_NULL_POINTER(pHandle)  || AVS_IS_NULL_POINTER(pInfo) )
  {
    AVS_TRACE_ERROR("Handle invalid");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  if(pHandle->pFactory->useAuxAudio == 0U)
  {
    AVS_TRACE_ERROR("Auxiliary channel disabled");
    return AVS_ERROR;
  }
  LOCK_ENTRY();
  avs_core_mutex_lock(&pHandle->pAudio->auxAudioPipe.lock);
  if((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_FLAGS)!= 0U)
  {
    pHandle->pAudio->auxAudioPipe.pipeFlags = pInfo->iFlags;
  }
  
  
  
  if( (((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_SZBUFF))!= 0U)   && (pInfo->szConsumer == 0) && (pInfo->szProducer == 0))
  {
    /* Notice, we have not implemented a full control of szConsumer && szProducer */
    /* Because it is useless */
    /* We just use this fields to signal a ring buffer reset when both are 0 */
    
    /* TODO : implement the full control of the ring buffer */
    
    avs_audio_buffer_reset(&pHandle->pAudio->auxAudioPipe.inBuffer);
  }
  if((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_NBSAMPLE) != 0U)
  {
    pHandle->pAudio->auxAudioPipe.inBuffer.szBuffer = pInfo->szBuffer ;
  }
  if((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_THRESHOLD) != 0U)
  {
    pHandle->pAudio->auxAudioPipe.threshold = pInfo->threshold;
  }
  if((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_FREQ) != 0U)
  {
    pHandle->pAudio->auxAudioPipe.inBuffer.sampleRate = pInfo->frequency;
  }
  if((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_CH) != 0U)
  {
    pHandle->pAudio->auxAudioPipe.inBuffer.nbChannel = pInfo->nbChannel;
  }
  if((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_VOLUME) != 0U)
  {
    pHandle->pAudio->auxAudioPipe.inBuffer.volume = pInfo->volume;
  }
  /* If the topologies is changed we have to recompute the malloc */
  if((pInfo->infoFlags & ((uint32_t)AVS_AUX_INFO_CH | (uint32_t)AVS_AUX_INFO_NBSAMPLE)) != 0U)
  {
    avs_audio_buffer_reset_buffer(&pHandle->pAudio->auxAudioPipe.inBuffer, 0);
    
  }
  
  /* If the frequency is changed , we have to re-map the re sampler-callbacks */
  if((pInfo->infoFlags & (uint32_t)AVS_AUX_INFO_FREQ) != 0U)
  {
    avs_audio_pipe_update_resampler(&pHandle->pAudio->auxAudioPipe);
  }
  
  
  avs_core_mutex_unlock(&pHandle->pAudio->auxAudioPipe.lock);
  UNLOCK_ENTRY();
  
  return AVS_OK;
  
}






/*!
@brief   Change  the STVS4A engine state

This function allows to start or stop a dialog with Alexa according to the imitator mode
@param hHandle Instance Handle
@param state   Engine State  
@return AVS_ERROR,AVS_OK,AVS_BUSY
*/

AVS_Result AVS_Set_State(AVS_Handle hHandle,AVS_Instance_State state)
{
  AVS_Result result = AVS_ERROR;
  AVS_ASSERT(hHandle);
  if(hHandle == 0)
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(((AVS_instance_handle*)hHandle));
  LOCK_ENTRY();
  switch(state)
  {
  case AVS_STATE_START_CAPTURE:
    {
      result = avs_set_state((AVS_instance_handle*)hHandle, AVS_STATE_START_CAPTURE);
      break;
    }
  case AVS_STATE_STOP_CAPTURE:
    {
      result = avs_set_state((AVS_instance_handle*)hHandle, AVS_STATE_STOP_CAPTURE);
      break;
    }
  default:
    break;
    
  }
  UNLOCK_ENTRY();
  
  return result;
}


/*!
@brief   Get the AVS engine state

This function manipulate the STVS4A state

@param hHandle Instance Handle
@return AVS_Instance_State_t
*/

AVS_Instance_State AVS_Get_State(AVS_Handle hHandle)
{
  
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hHandle;
  AVS_ASSERT(pHandle);
  if(hHandle == 0)
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_STATE_RESTART;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  return (AVS_Instance_State) avs_core_atomic_read(&pHandle->curState);
}


/*!
@brief   Shows on the debug console, the actual configuration

This function shows the configuration on the serial output

@param hInstance Instance Handle
@return  AVS_ERROR,AVS_OK
*/

AVS_Result AVS_Show_Config(AVS_Handle hInstance)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if (AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  LOCK_ENTRY();
  avs_core_dump_instance_factory(pHandle, "Instance");
  if(pHandle->pAudio)
  {
    avs_core_dump_audio_factory(pHandle->pAudio, "Audio");
  }
  UNLOCK_ENTRY();
  return AVS_OK;
}

/*!
@brief   Returns the current time using a AVS_TIME standard type

@param hInstance Instance Handle
@param pEpoch    Epoch time ( time_t) or NULL
@return  AVS_ERROR,AVS_OK
*/

AVS_Result  AVS_Get_Sync_Time(AVS_Handle hInstance , AVS_TIME *pEpoch)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if (AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  if(pEpoch )  
  {
    *pEpoch = 0;
  }
  LOCK_ENTRY();
  AVS_Result ret = avs_network_get_time(pHandle, (time_t *)pEpoch);
  UNLOCK_ENTRY();
  return ret;
}


/*!
@brief   Synchronize the local time with the network time
This function allows to synchronize the time using an NTP server
@param hInstance Instance Handle 
@return  AVS_ERROR,AVS_OK
*/

AVS_Result  AVS_Set_Sync_Time(AVS_Handle hInstance )
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  LOCK_ENTRY();
  AVS_Result ret = avs_network_synchronize_time(pHandle);
  UNLOCK_ENTRY();
  return ret;
}



/*!
@brief   Send an evt message using the AVS_EventCB handler

This function is useful to communicate with other services connected to the handler without conflict with the SDK

@param hInstance Instance Handle 
@param evt  Event code
@param pparam   Contextual parameter 
@return  AVS_Result
*/

AVS_Result AVS_Send_Evt(AVS_Handle hInstance, AVS_Event evt, uint32_t  pparam)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  /* Notice there is no lock since the lock is done the avs_core_message_send */
  AVS_Result ret = avs_core_message_send(pHandle, evt, pparam);
  return ret;
}



/*!
@brief   post an evt message using the AVS_EventCB handler

This function is  similar to AVS_Send_Evt but the evt will be sent from another thread during the state machine idle time
We can use this function when it is impossible to use AVS_Send_Evt 

@param hInstance Instance Handle 
@param evt  Event code
@param pparam   Contextual parameter 
@return  AVS_Result
*/

AVS_Result AVS_Post_Evt(AVS_Handle hInstance, AVS_Event evt, uint32_t  pparam)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  /* Notice there is no lock since the lock is done the avs_core_message_send */
  AVS_Result ret = avs_core_message_post(pHandle, evt, pparam);
  return ret;
}




/*!
@brief   Send a JSON message to AVS

Send an event to the ALEXA server, an event is a JSON string formatted according its requirement 
usually an event includes a payload , header and context objects

@param hInstance Instance Handle
@param pJson  The JSON string
@return AVS_BUSY, AVS_OK, AVS_ERROR
*/


AVS_Result      AVS_Send_JSon(AVS_Handle hInstance, const char_t *pJson)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  LOCK_ENTRY();
  AVS_Result ret = avs_http2_send_json(pHandle, pJson);
  UNLOCK_ENTRY();
  return ret;
}

/*!
@brief   update the context member with the context state

The root json_t object must contain a "context" item array. This function will send a message EVT_UPDATE_SYNCHRO to each service connected. 
Each service will add its context to the json array. Usually before to use AVS_Send_JSon you must call this function to make sure the synchro state is completed.

@param    hInstance Instance Handle
@param    json_t_root json_t *pRoot 
@return AVS_BUSY, AVS_OK, AVS_ERROR
*/

AVS_Result      AVS_Json_Add_Context(AVS_Handle hInstance, void *json_t_root)    
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle) || AVS_IS_VOID_NULL_POINTER(json_t_root))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  avs_json_formater_add_context(pHandle, json_t_root);
  return AVS_OK;
}




/*!
@brief   Post a synchro state 

The synchro state will be posted in the next STVS4A idle time state

@param hInstance  Instance Handle
@return AVS_BUSY, AVS_OK,AVS_ERROR
*/


AVS_Result AVS_Post_Sychro(AVS_Handle hInstance)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  avs_core_atomic_write(&pHandle->postSynchro, 1);
  return AVS_OK;
  
}


/*!
@brief   fills a structure with various system information

The function returns various info such as the mem available, buffer statistics, etc...

@param  hInstance Instance Handle
@param  pSysInfo pointer on AVS_Sys_info 
@return AVS_OK or AVS_ERROR
*/

AVS_Result AVS_Get_Sys_Info(AVS_Handle hInstance, AVS_Sys_info *pSysInfo )
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle) ||AVS_IS_NULL_POINTER(pSysInfo))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  LOCK_ENTRY();
  
  AVS_Result ret = avs_core_sys_info_get(pHandle, pSysInfo);
  UNLOCK_ENTRY();
  return ret;
}



/*!
@brief   Open a AVS Channel

The open number Limited to the max stream number opened at once from the http2 stack
The set of function is used to create a custom  AVS event using a multi-part stream
For example, HTTP2 HEADER + JSON + AUDIO[in/out]

@param hHandle  Instance Handle

@return  AVS_HStream Handle
*/



AVS_HStream *AVS_Open_Stream(AVS_Handle hHandle)
{
  
  AVS_instance_handle *pHandle= (AVS_instance_handle *)hHandle;
   if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return (AVS_HStream *)NULL;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  LOCK_ENTRY();
  AVS_HStream hStream = avs_http2_stream_open(pHandle);
  UNLOCK_ENTRY();
  return hStream;
}


/*!
@brief   Add a JSON body to the multi-part channel

This call must be done once by stream and just after AVS_Stream_Open

@param hInstance  Instance Handle
@param hStream Stream Handle
@param pJson   Json string
@return AVS_OK,AVS_ERROR
*/


AVS_Result   AVS_Add_Body_Stream(AVS_Handle hInstance, AVS_HStream hStream,const char *pJson)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  Http2ClientStream *pStream = (Http2ClientStream *)hStream; 
  AVS_ASSERT( AVS_IS_VALID_POINTER (pStream ));
  if(AVS_IS_NULL_POINTER(pStream))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  AVS_ASSERT(pJson != 0);
  
  if(AVS_IS_NULL_POINTER(pJson))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  LOCK_ENTRY();
  AVS_Result ret = avs_http2_stream_add_body(pHandle, pStream, pJson);
  UNLOCK_ENTRY();
  return ret;
}


/*!
@brief   Read  some data from the stream channel

This function reads incoming data from the channel.

@param hInstance       Instance Handle
@param hStream         Stream Handle
@param pBuffer          Buffer receiving data
@param szInSByte       size to read
@param retSize         size really received 
@return AVS_OK, AVS_ERROR
*/

AVS_Result   AVS_Read_Stream(AVS_Handle hInstance, AVS_HStream hStream, void *pBuffer, uint32_t szInSByte, uint32_t *retSize)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  Http2ClientStream *pStream = (Http2ClientStream *)hStream;
  AVS_ASSERT((AVS_IS_VALID_POINTER(pStream)));
  if(AVS_IS_NULL_POINTER(pStream))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  AVS_ASSERT(AVS_IS_VOID_NULL_POINTER(pBuffer));
  AVS_ASSERT(AVS_IS_VALID_POINTER(retSize));
  
  if(AVS_IS_VOID_NULL_POINTER(pBuffer) || AVS_IS_NULL_POINTER(retSize))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  LOCK_ENTRY();
  AVS_Result ret = avs_http2_stream_read(pHandle, pStream, pBuffer, szInSByte, retSize);
  UNLOCK_ENTRY();
  return ret;
}


/*!
@brief   Write some data in the stream channel

This function writes data in the channel.

@param hInstance Instance Handle 
@param hStream  Stream Handle 
@param pBuffer  Buffer to push
@param lengthInBytes size to push
@return AVS_OK, AVS_ERROR
*/

AVS_Result   AVS_Write_Stream(AVS_Handle hInstance, AVS_HStream hStream, const void *pBuffer, size_t lengthInBytes)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  Http2ClientStream *pStream = (Http2ClientStream *)hStream;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pStream));
  if(AVS_IS_NULL_POINTER(pStream))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  AVS_ASSERT(AVS_IS_VALID_POINTER(pBuffer));
  if(AVS_IS_NULL_POINTER(pBuffer))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  LOCK_ENTRY();
  AVS_Result ret = avs_http2_stream_write(pHandle, pStream, pBuffer, lengthInBytes);
  UNLOCK_ENTRY();
  return ret;
}




/*!
@brief   Stop the stream channel 

Send also end of stream state

@param hInstance Instance Handle
@param hStream Stream Handle
@return AVS_OK,AVS_ERROR
*/


AVS_Result   AVS_Stop_Stream(AVS_Handle hInstance, AVS_HStream hStream)            
{
  
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  Http2ClientStream *pStream= (Http2ClientStream *)hStream;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pStream));
  if(AVS_IS_NULL_POINTER(pStream))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  LOCK_ENTRY();
  AVS_Result ret = avs_http2_stream_stop(pHandle, pStream);
  UNLOCK_ENTRY();
  return ret;
}

/*!
@brief   Close the stream channel 
Send Also and End Of Stream state before to close 

@param hInstance Instance Handle
@param hStream   Stream Handle 
@return AVS_OK, AVS_ERROR
*/
AVS_Result   AVS_Close_Stream(AVS_Handle hInstance, AVS_HStream hStream);
AVS_Result   AVS_Close_Stream(AVS_Handle hInstance, AVS_HStream hStream)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  Http2ClientStream *pStream = (Http2ClientStream *)hStream;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pStream));
  if(AVS_IS_NULL_POINTER(pStream))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  LOCK_ENTRY();
  AVS_Result ret = avs_http2_stream_close(pHandle, pStream);
  UNLOCK_ENTRY();
  return ret;
}



/*!
@brief   Process the incoming JSON multi-part stream 

This function read the multi-part, parses its content dispatch the event via the EventCB.

@param hInstance Instance Handle
@param hStream   Stream Handle
@return  AVS_Result
*/

AVS_Result   AVS_Process_Json_Stream(AVS_Handle hInstance,AVS_HStream hStream)
{
  
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  
  Http2ClientStream *pStream = (Http2ClientStream *)hStream;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pStream));
  if(AVS_IS_NULL_POINTER(pStream))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  LOCK_ENTRY();
  AVS_Result ret = avs_http2_stream_process_json(pHandle, pStream);
  UNLOCK_ENTRY();
  return ret;
  
}



/*!
@brief   Read the multi-part header and returns the response content type from the incoming stream

The content type is usually "application/json" or "application/octet-stream"

@param hInstance   Instance Handle
@param hStream  Stream Handle

@return  The contents stream type string
*/


const char_t * AVS_Get_Reponse_Type_Stream(AVS_Handle hInstance , AVS_HStream hStream )
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return (char_t *)NULL;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  Http2ClientStream *pStream = (Http2ClientStream *)hStream;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pStream));
  if(AVS_IS_NULL_POINTER(pStream))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return (char_t *)NULL;
  }
  
  LOCK_ENTRY();
  const char_t *ret = avs_http2_stream_get_response_type(pHandle, pStream);
  UNLOCK_ENTRY();
  return ret;
  
  
}


/*!
@brief Mutes the main audio channel
If the main audio channel is muted, Alexa can't speak, but mute doesn't mute the auxiliary channel

@param hInstance     The Instance handle 
@param state         TRUE or FALSE
@return AVS_OK or AVS_ERROR
*/

AVS_Result AVS_Set_Audio_Mute(AVS_Handle hInstance, uint32_t state)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  if(AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  CHECK_INSTANCE_SIGN(pHandle);
  uint32_t iflag =   avs_core_atomic_read(&pHandle->pAudio->synthesizerPipe.pipeFlags);
  if(state)
  {
    iflag |=  (uint32_t)AVS_PIPE_MUTED;
  }
  else
  {
    iflag &= ~(uint32_t)AVS_PIPE_MUTED;
  }
  avs_core_atomic_write(&pHandle->pAudio->synthesizerPipe.pipeFlags, iflag);
  return AVS_OK;
}


/*!
@brief Signal an exception

The function use the level parameter to produce a LED lighting using the level parameter
This function is blocking and must be used for panic issues

@param hInstance The Instance handle or null 
@param level Lighting type 
@return void

*/

void AVS_Signal_Exeception(AVS_Handle hInstance,AVS_Signal_Exception level)
{
  if (AVS_IS_VOID_NULL_POINTER(hInstance))
  {
    hInstance = avs_core_get_instance();
  }
  
  uint32_t  flag  = (uint32_t)level & 3U;
  uint64_t  llevel= (uint64_t)level >> 2;
//  uint64_t  delay = (llevel + 1)  * (uint64_t)10000 * (uint64_t)1000;
  uint64_t  delay = (llevel + 1)  * (uint64_t)500;
  while(1)
  {
    osDelay(delay);
    if((flag & 1U) != 0U)
    {
      drv_sys.platform_Sys_ioctl(hInstance, AVS_SYS_SET_LED, 0, 1);
    }
    if((flag & 2U) != 0U)
    {
      drv_sys.platform_Sys_ioctl(hInstance, AVS_SYS_SET_LED, 1, 1);
    }
    osDelay(delay);
    
    if((flag & 1U) != 0U)
    {
      drv_sys.platform_Sys_ioctl(hInstance, AVS_SYS_SET_LED, 0, 0);
    }
    if((flag & 2U)!= 0U)
    {
      drv_sys.platform_Sys_ioctl(hInstance, AVS_SYS_SET_LED, 1, 0);
    }
  }
}


/*!

@brief  Open a TLS stream

Open a TLS stream from the host URL and port. The TLS set of function are basic and used mainly to abstract TLS support in demo services

@param hHandle The Instance handle
@param pHost The URL string   example www.st.com
@param port  Port  to open 
@return and handle AVS_HTls or null if failure
*/


AVS_HTls AVS_Open_Tls(AVS_Handle hHandle,char_t *pHost,uint32_t port)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hHandle;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  AVS_ASSERT(pHost);
  
  if (AVS_IS_NULL_POINTER(pHandle))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
 if (AVS_IS_NULL_POINTER(pHost))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  CHECK_INSTANCE_SIGN(pHandle);
  AVS_HTls hStream = avs_porting_tls_open(pHandle, pHost, port);
  return hStream;
}

/*!

@brief   Read some data from the TLS stream

This function could be blocking if your more data than the stream has 

@param hInstance The Instance handle
@param hStream The AVS_HTls handle
@param pBuffer Buffer receiving data 
@param szInSByte Size to pull
@param retSize Size really received 
@return AVS_OK or AVS_ERROR

*/

AVS_Result AVS_Read_Tls(AVS_Handle hInstance,AVS_HTls hStream,void *pBuffer,uint32_t szInSByte,uint32_t *retSize)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  AVS_ASSERT(AVS_IS_VOID_VALID_POINTER(pBuffer));
  AVS_ASSERT(AVS_IS_VALID_POINTER(hStream));
  if(AVS_IS_NULL_POINTER(pHandle) || AVS_IS_NULL_POINTER(hStream))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  if (AVS_IS_VOID_NULL_POINTER(pBuffer))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  CHECK_INSTANCE_SIGN(pHandle);
  /* No lock mandatory */
  AVS_Result ret =  avs_porting_tls_read(pHandle, hStream, pBuffer, szInSByte, retSize);
  return ret;
}

/*!
@brief   Write some TLS data in the stream

This function could be blocking until the timeout if server doesn't consume the data

@param hInstance The Instance handle
@param hStream The AVS_HTls handle
@param pBuffer Buffer to push
@param lengthInBytes Size to push
@param retSize size really sent
@return AVS_OK or AVS_ERROR

*/

AVS_Result              AVS_Write_Tls(AVS_Handle hInstance,AVS_HTls hStream,const void *pBuffer,uint32_t lengthInBytes,uint32_t *retSize)    
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  AVS_ASSERT(AVS_IS_VALID_POINTER(pBuffer));
  AVS_ASSERT(AVS_IS_VALID_POINTER(hStream));
  if(AVS_IS_NULL_POINTER(pHandle) || AVS_IS_NULL_POINTER(hStream))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  if (AVS_IS_NULL_POINTER(pBuffer))
  {
    AVS_TRACE_ERROR("Invalid parameter");
    return AVS_ERROR;
  }
  
  CHECK_INSTANCE_SIGN(pHandle);
  /* No lock mandatory */
  AVS_Result ret =  avs_porting_tls_write(pHandle, hStream, pBuffer, lengthInBytes, retSize);
  return ret;
}

/*!
@brief   Close the TLS stream

Close the TLS stream

@param hInstance The Instance handle
@param hStream The TLS handle 

@return AVS_OK or AVS_ERROR

*/

AVS_Result AVS_Close_Tls(AVS_Handle hInstance,AVS_HTls hStream)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)hInstance;
  AVS_ASSERT(AVS_IS_VALID_POINTER(pHandle));
  AVS_ASSERT(hStream);
  if(AVS_IS_NULL_POINTER(pHandle) || AVS_IS_NULL_POINTER(hStream))
  {
    AVS_TRACE_ERROR("Invalid handle");
    return AVS_ERROR;
  }
  
  CHECK_INSTANCE_SIGN(pHandle);
  /* No lock mandatory */
  AVS_Result ret =  avs_porting_tls_close(pHandle, hStream);
  return ret;
  
  
}

