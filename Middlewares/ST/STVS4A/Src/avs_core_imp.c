/**
******************************************************************************
* @file    avs_core_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This module implements in depends objects collection and services
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


#include "avs_private.h"
/*
#define AVS_USE_GLOBAL_CURRUPTION_TRACKER
#define AVS_NO_SHORT_OBJECT_POOL
*/
#ifndef AVS_NO_SHORT_OBJECT_POOL
#define USE_SHORT_OBJECT_POOL
#endif



static  char_t       szBuffer[400];             /* Buffer traces */
extern uint32_t   avs_debugLevel;
extern AVS_Result avs_lastError;
static AVS_instance_handle *avs_pCurrentInstance = 0;


static avs_mem_type_t            gAudioMemoyPool=avs_mem_type_best;
avs_mem_pool                     hPoolDTCM;
avs_mem_pool                     hPoolPRAM;
avs_mem_pool                     hPoolNCACHED;



#define LOCK_ALLOC()    avs_core_lock_tasks()
#define UNLOCK_ALLOC()  avs_core_unlock_tasks()

#if defined(USE_SHORT_OBJECT_POOL)
static avs_mem_pool hShortObjectPool;
#endif

#if defined(AVS_USE_GLOBAL_CURRUPTION_TRACKER)
/*
Warning:

The avs_mem allocator is significantly slower than the original allocator
If there are many sequence malloc/free during a timed sequence
the sequence could detect a time-out using avs_mem

Add  those options in the linker command line
--redirect malloc=SAFE_malloc
--redirect free=SAFE_free
--redirect realloc=SAFE_realloc
--redirect calloc=SAFE_calloc

*/

#define AVS_HEAP_CURRUPTION_SIZE  (1024*1024)
#define tCorruptionHeap           ((char *)0xC0400000)

static uint8_t bFirstAlloc = FALSE;
static avs_mem_pool hCorruptionPool;
void avs_mem_free_trace(avs_mem_pool *pHandle, void *pBlock, char *pString, int line);
void * avs_mem_realloc_trace(avs_mem_pool *pHandle, void *pBlock, uint32_t  sizeMalloc, char *pString, int line);
void * avs_mem_alloc_trace(avs_mem_pool *pHandle, uint32_t  sizeMalloc, char *pString, int line);

#define SAVE_LOCK_ALLOC()    avs_core_lock_tasks()
#define SAVE_UNLOCK_ALLOC()  avs_core_unlock_tasks()


void SAFE_Init_Heap()
{
  if(!bFirstAlloc)
  {
    /* Init the pool */
    avs_mem_init(&hCorruptionPool, tCorruptionHeap, AVS_HEAP_CURRUPTION_SIZE);
    /* The app does a lot of malloc/free, check the corruption every 1000 allocs */
    /* HCorruptionPool.m_checkFreq = 1000; */
    bFirstAlloc = TRUE;
  }
}

void * SAFE_malloc_trace(int  size, char *pstring, int numline)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  void *pMalloc = avs_mem_alloc_trace(&hCorruptionPool, size, pstring, numline);
  SAVE_UNLOCK_ALLOC();
  return pMalloc;
  
}




void * SAFE_malloc(int  size)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  void *pMalloc = avs_mem_alloc(&hCorruptionPool, size);
  SAVE_UNLOCK_ALLOC();
  return pMalloc;
  
}


void SAFE_free_trace(void *pAlloc, char *pstring, int numline)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  avs_mem_free_trace(&hCorruptionPool, pAlloc, pstring, numline);
  SAVE_UNLOCK_ALLOC();
  
  
}

void SAFE_free(void *pAlloc)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  avs_mem_free(&hCorruptionPool, pAlloc);
  SAVE_UNLOCK_ALLOC();
  
}

void * SAFE_realloc_trace(void *pAlloc, int size, char *pstring, int numline)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  void *pMalloc = avs_mem_realloc_trace(&hCorruptionPool, pAlloc, size, pstring, numline);
  SAVE_UNLOCK_ALLOC();
  
  return pMalloc;
}


void * SAFE_realloc(void *pAlloc, int  size)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  void *pMalloc = avs_mem_realloc(&hCorruptionPool, pAlloc, size);
  SAVE_UNLOCK_ALLOC();
  return pMalloc;
}


void* SAFE_calloc_trace (int  num, int size, char *pstring, int numline)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  void *pMalloc = avs_mem_alloc_trace(&hCorruptionPool, num * size, pstring, numline);
  if(pMalloc) memset(pMalloc, 0, num * size);
  SAVE_UNLOCK_ALLOC();
  
  return pMalloc;
}


void* SAFE_calloc (int  num, int size)
{
  SAFE_Init_Heap();
  SAVE_LOCK_ALLOC();
  void *pMalloc = avs_mem_alloc(&hCorruptionPool, num * size);
  if(pMalloc) memset(pMalloc, 0, num * size);
  SAVE_UNLOCK_ALLOC();
  return pMalloc;
}

void CheckLeaks()
{
  
#ifdef AVS_USE_LEAK_DETECTOR
  SAVE_LOCK_ALLOC();
  avs_mem_leak_detector(&hCorruptionPool);
  SAVE_UNLOCK_ALLOC();
#endif /* AVS_USE_LEAK_DETECTOR */
}

#endif

uint32_t avs_core_mem_check_blk(const void *pBlk, uint32_t isize);
uint32_t avs_core_mem_check_blk(const void *pBlk, uint32_t isize)
{
  uint32_t ret = 0;
#if defined(AVS_USE_GLOBAL_CURRUPTION_TRACKER)
  SAVE_LOCK_ALLOC();
  ret = avs_mem_check_curruption_blk(&hCorruptionPool, pBlk, isize);
  SAVE_UNLOCK_ALLOC();
#endif
  return ret;
}

void   avs_core_mem_check(void);
void   avs_core_mem_check(void)
{
#if defined(AVS_USE_GLOBAL_CURRUPTION_TRACKER)
  SAVE_LOCK_ALLOC();
  avs_mem_check_curruption(&hCorruptionPool);
  SAVE_UNLOCK_ALLOC();
  
#endif
}


void avs_mem_pool_set_check_freq(uint32_t freq);
void avs_mem_pool_set_check_freq(uint32_t freq)
{
#if defined(AVS_USE_GLOBAL_CURRUPTION_TRACKER)
  hCorruptionPool.m_checkFreq = freq;
#endif
}




/*
*
*
*
*       Ring buffer Management
*
*
*/





/*
For DMA direct asses buffer
if each ISR call, we can set the ring buffer directly according to the half buffer
the stream will pump  produced data

*/

void avs_audio_buffer_quick_set(Avs_audio_buffer *pHandle, Avs_consumer type)
{
  if ((type == AVS_RING_BUFF_PROD_PART1) || (type == AVS_RING_BUFF_CONSUM_PART2) )
  {
    pHandle->consumPos = pHandle->szBuffer / 2;
    pHandle->prodPos = 0;
    pHandle->szConsumer = pHandle->szBuffer / 2;
    
  }else  if ((type == AVS_RING_BUFF_PROD_PART2)|| (type == AVS_RING_BUFF_CONSUM_PART1 ))
  {
    pHandle->consumPos = 0;
    pHandle->prodPos   =  pHandle->szBuffer / 2;
    pHandle->szConsumer = pHandle->szBuffer / 2;
    
  }else
  {
    /* do nothing */
  }
  
  
}




uint32_t avs_bytes_buffer_produce(Avs_bytes_buffer *pHandle, uint32_t nbByte, uint8_t *pBuffer)
{
  uint32_t curSize = 0;
  AVS_ASSERT(nbByte < pHandle->szBuffer);
  /* Check the remaining data in the ring buffer */
  if(pHandle->szConsumer + nbByte >= pHandle->szBuffer)
  {
    nbByte = pHandle->szBuffer - pHandle->szConsumer;
  }
  
  
  if(pHandle->szConsumer + nbByte <= pHandle->szBuffer)  /* Check if the buffer is too large */
  {
    if(pHandle->prodPos + nbByte >= pHandle->szBuffer) /* Check if we will reach the buffer end */
    {
      /* Nb element before the end of the buffer */
      uint32_t nbElem = (pHandle->szBuffer - pHandle->prodPos);
      /* Fill the remaining space */
      if(pBuffer)
      {
        memcpy((char_t *)&pHandle->pBuffer[pHandle->prodPos ], pBuffer, nbElem );
        pBuffer += nbElem ;
      }
      
      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbElem ;
      pHandle->prodPos = 0;
      nbByte -= nbElem ;
      curSize += nbElem ;
    }
    if(nbByte)
    {
      AVS_ASSERT(pHandle->prodPos + nbByte < pHandle->szBuffer); /* Must not cross the end (bug) */
      if(pBuffer)
      {
        memcpy((char_t *)&pHandle->pBuffer[ pHandle->prodPos ], pBuffer, nbByte );
      }
      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbByte ;
      pHandle->prodPos += nbByte;
      curSize += nbByte ;
    }
    
  }
  AVS_RING_CHECK_PROD_PEAK(pHandle);
  return curSize;
}

uint32_t avs_bytes_buffer_consume(Avs_bytes_buffer *pHandle, uint32_t nbByte, uint8_t *pBuffer)
{
  uint32_t curSize = 0;
  AVS_ASSERT(nbByte < pHandle->szBuffer);
  if(nbByte >= pHandle->szConsumer)
  {
    nbByte = pHandle->szConsumer;
  }
  if(nbByte <= pHandle->szConsumer )  /* Check if the buffer is large enough */
  {
    if(pHandle->consumPos + nbByte >= pHandle->szBuffer) /* Check if we will reach the buffer end */
    {
      /* Nb element before the end of the buffer */
      uint32_t nbElem = (pHandle->szBuffer - pHandle->consumPos);
      if(pBuffer)
      {
        memcpy(pBuffer, (uint8_t *)(uint32_t)&pHandle->pBuffer[ pHandle->consumPos ], nbElem );
        pBuffer += nbElem ;
      }
      /* Update size, pos and remaining elements */
      curSize += nbElem ;
      pHandle->szConsumer -= nbElem ;
      AVS_ASSERT(pHandle->szConsumer >= 0);
      pHandle->consumPos = 0;
      nbByte -= nbElem ;
    }
    if(nbByte)
    {
      AVS_ASSERT(pHandle->consumPos + nbByte < pHandle->szBuffer); /* Must not cross the end (bug) */
      /* Fill the remaining space */
      if(pBuffer)
      {
        memcpy(pBuffer, (uint8_t *)(uint32_t)&pHandle->pBuffer[ pHandle->consumPos ], nbByte );
      }
      /* Update size, pos and remaining elements */
      pHandle->szConsumer -= nbByte ;
      AVS_ASSERT(pHandle->szConsumer >= 0);
      pHandle->consumPos += nbByte;
      curSize += nbByte ;
    }
  }
  return curSize;
}




/*
Injects a memory block in the ring buffer
if pBuffer is null, the function assumes the buffer directly feeded by the caller ( DMA)
return : the number of elements really produced

*/

uint32_t avs_audio_buffer_produce(Avs_audio_buffer *pHandle, uint32_t nbSample, int16_t *pBuffer)
{
  uint32_t curSize = 0;
  AVS_ASSERT(nbSample < pHandle->szBuffer);
  /* Check the remaining data in the ring buffer */
  if(pHandle->szConsumer + nbSample >= pHandle->szBuffer)
  {
    nbSample = pHandle->szBuffer - pHandle->szConsumer;
  }
  
  
  if(pHandle->szConsumer + nbSample <= pHandle->szBuffer)  /* Check if the buffer is too large */
  {
    if(pHandle->prodPos + nbSample >= pHandle->szBuffer) /* Check if we will reach the buffer end */
    {
      /* Nb element before the end of the buffer */
      uint32_t nbElem = (pHandle->szBuffer - pHandle->prodPos);
      /* Fill the remaining space */
      if(pBuffer)
      {
        memcpy((char_t *)&pHandle->pBuffer[pHandle->prodPos * pHandle->nbChannel], pBuffer, nbElem * pHandle->nbChannel * sizeof(int16_t));
        pBuffer += nbElem * pHandle->nbChannel;
      }
      
      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbElem ;
      pHandle->prodPos = 0;
      nbSample -= nbElem ;
      curSize += nbElem ;
    }
    if(nbSample)
    {
      AVS_ASSERT(pHandle->prodPos + nbSample < pHandle->szBuffer); /* Must not cross the end (bug) */
      if(pBuffer)
      {
        memcpy((char_t *)&pHandle->pBuffer[ pHandle->prodPos * pHandle->nbChannel], pBuffer, nbSample * pHandle->nbChannel * sizeof(int16_t));
      }
      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbSample ;
      pHandle->prodPos += nbSample;
      curSize += nbSample ;
    }
    
  }
  AVS_RING_CHECK_PROD_PEAK(pHandle);
  return curSize;
}

/*

Consumes a ring buffer  block
return : the number of elements really consumed


*/

uint32_t avs_audio_buffer_consume(Avs_audio_buffer *pHandle, uint32_t nbSample, int16_t *pBuffer)
{
  uint32_t curSize = 0;
  AVS_ASSERT(nbSample < pHandle->szBuffer);
  if(nbSample >= pHandle->szConsumer)
  {
    nbSample = pHandle->szConsumer;
  }
  if(nbSample <= pHandle->szConsumer )  /* Check if the buffer is large enough */
  {
    if(pHandle->consumPos + nbSample >= pHandle->szBuffer) /* Check if we will reach the buffer end */
    {
      /* Nb element before the end of the buffer */
      uint32_t nbElem = (pHandle->szBuffer - pHandle->consumPos);
      if(pBuffer)
      {
        memcpy(pBuffer, (char *)(uint32_t)&pHandle->pBuffer[ pHandle->consumPos * pHandle->nbChannel], nbElem * pHandle->nbChannel * sizeof(int16_t));
        pBuffer += nbElem * pHandle->nbChannel;
      }
      /* Update size, pos and remaining elements */
      curSize += nbElem ;
      pHandle->szConsumer -= nbElem ;
      AVS_ASSERT(pHandle->szConsumer >= 0);
      pHandle->consumPos = 0;
      nbSample -= nbElem ;
    }
    if(nbSample)
    {
      AVS_ASSERT(pHandle->consumPos + nbSample < pHandle->szBuffer); /* Must not cross the end (bug) */
      /* Fill the remaining space */
      if(pBuffer)
      {
        memcpy(pBuffer, (char *)(uint32_t)&pHandle->pBuffer[ pHandle->consumPos * pHandle->nbChannel], nbSample * pHandle->nbChannel * sizeof(int16_t));
      }
      /* Update size, pos and remaining elements */
      pHandle->szConsumer -= nbSample ;
      AVS_ASSERT(pHandle->szConsumer >= 0);
      pHandle->consumPos += nbSample;
      curSize += nbSample ;
    }
  }
  return curSize;
}


/*


Create a ring buffer object

*/

AVS_Result avs_bytes_buffer_Create(Avs_bytes_buffer *pHandle, uint32_t nbbytes)
{
  memset(pHandle, 0, sizeof(*pHandle));
  pHandle->szBuffer = nbbytes;
  pHandle->pBuffer = (uint8_t*)avs_core_mem_alloc(avs_mem_type_heap, nbbytes);
  avs_core_mutex_create(&pHandle->lock);
  AVS_ASSERT(pHandle->pBuffer);
  /* RAZ by default */
  memset(pHandle->pBuffer, 0, nbbytes);

  return AVS_OK;
}

AVS_Result avs_bytes_buffer_Delete(Avs_bytes_buffer *pHandle)
{
  
  if(pHandle->pBuffer != 0)
  {
    avs_core_mem_free(pHandle->pBuffer);
    avs_core_mutex_delete(&pHandle->lock);
  }
  return AVS_OK;
}

void  avs_audio_memory_pool_set(uint32_t hPool)
{
  gAudioMemoyPool = (avs_mem_type_t)hPool;
}



WEAK AVS_Result avs_audio_buffer_create(Avs_audio_buffer *pHandle, int16_t *pBuffer, uint32_t nbSample, uint32_t sizeElement, uint32_t  sampleRate)
{
  memset(pHandle, 0, sizeof(*pHandle));
  pHandle->szBuffer  = nbSample;
  pHandle->nbChannel = sizeElement;
  if(pBuffer == 0)
  {
    pHandle->pBuffer = (int16_t*)avs_core_mem_alloc(gAudioMemoyPool, nbSample * sizeElement * sizeof(int16_t));
    pHandle->flags |= 1;
  }
  else
  {
    pHandle->pBuffer = pBuffer;
    
  }
  pHandle->sampleRate = sampleRate;
  AVS_ASSERT(pHandle->pBuffer != 0);
  /* RAZ by default */
  memset(pHandle->pBuffer, 0, nbSample * sizeElement * sizeof(int16_t));
  avs_audio_buffer_reset(pHandle);
  return AVS_OK;
}


void avs_audio_buffer_reset(Avs_audio_buffer *pHandle)
{
  pHandle->prodPos = 0;
  pHandle->consumPos = 0;
  pHandle->szConsumer = 0;
  pHandle->cumul = 0;
  pHandle->volume = 100;
  avs_upsampling_init(&pHandle->up_smp);
  avs_dnsampling_init(&pHandle->dn_smp);
}

/*

Debug function that generate a wave in a buffer


*/
void avs_audio_feed_wave(int16_t*pBuffer, uint32_t nbSamples, uint32_t  ch, uint32_t freqBuffer, uint32_t tone, int32_t vol)
{
  static int16_t *pSignal = 0;
  int16_t volume = (int16_t )vol;
  static uint32_t indexFreq = 0;
  /* Compute the period number of  pules by sec */
  uint32_t periode   =0;
  if( tone != 0)
  {
    periode = freqBuffer / tone;
  }
  
  /* compute the step to reach the period */
  uint32_t  step  = 1;
  if(periode != 0)
  {
    step = 360 / periode;
  }
  
  if(step == 0)
  {
    step = 1;
  }
  
  
  if (pSignal == 0)
  {
    /* We need to pre compute the signal because */
    /* The cos is software and heavy to compute */
    /* So we could be in under-run in the interruptions double buffering DMA */
    pSignal = (int16_t *)avs_core_mem_alloc(avs_mem_type_heap, 360 * sizeof(int16_t));
    for (uint32_t a = 0; a < 360; a++)
    {
      avs_double_t alpha= (((3.14) * (a % 360) / 180));
      pSignal[a] = (int16_t)(avs_float_t)(cos(alpha) * volume);
      
    }
    
  }
  for (uint32_t a = 0; a < nbSamples; a++)
  {
    *pBuffer = pSignal[indexFreq % 360];
    pBuffer++;
    
    indexFreq += step;
    if (ch == 2)
    {
      *pBuffer  = pSignal[(indexFreq) % 360];
      pBuffer++;
    }
  }
  
}





AVS_Result avs_audio_buffer_reset_buffer(Avs_audio_buffer *pHandle, int16_t *pBuffer)
{
  if((pBuffer == 0) && ((pHandle->flags & 1U)!=0))
  {
    pHandle->pBuffer = (int16_t*)avs_core_mem_realloc(avs_mem_type_best, pHandle->pBuffer, pHandle->szBuffer * pHandle->nbChannel * sizeof(int16_t));
    if(pHandle->pBuffer == 0) /* In some cases, the reallocation in the same pool is not possible, so we have to realloc in the best pool */
    {
      pHandle->pBuffer = (int16_t*)avs_core_mem_alloc(avs_mem_type_best, pHandle->szBuffer * pHandle->nbChannel * sizeof(int16_t));
    }
    AVS_ASSERT(pHandle->pBuffer != 0);
    pHandle->flags |= 1;
  }
  else
  {
    pHandle->pBuffer = pBuffer;
    pHandle->flags &= ~1U;
    
  }
  return AVS_OK;
}


/* Return sys info */
AVS_Result avs_core_sys_info_get(AVS_instance_handle *pHandle, AVS_Sys_info *pSysInfo )
{
  
  
  memset(pSysInfo, 0, sizeof(AVS_Sys_info));
  LOCK_ALLOC();
   /* Heap info */
  avs_complier_get_heap_info(&pSysInfo->memTotalSpace,&pSysInfo->memFreeSpace);
  
  
  /* DTCM info */
  
  pSysInfo->memDtcmTotalSpace  = hPoolDTCM.m_iBaseSize;
  if(pSysInfo->memDtcmTotalSpace == 0)
  {
    pSysInfo->memDtcmTotalSpace++;
  }
  pSysInfo->memDtcmFreeSpace  =  hPoolDTCM.m_iBaseSize - hPoolDTCM.m_globalAlloc;
  
  /* Pram Info */
  
  pSysInfo->memPRamTotalSpace  = hPoolPRAM.m_iBaseSize;
  
  if(pSysInfo->memPRamTotalSpace == 0)
  {
    pSysInfo->memPRamTotalSpace++;
  }
  
  pSysInfo->memPRamFreeSpace   = hPoolPRAM.m_iBaseSize -  hPoolPRAM.m_globalAlloc;
  
  /* Uncached  Info */
  
  pSysInfo->memNCachedTotalSpace  = hPoolNCACHED.m_iBaseSize;
  
  if(pSysInfo->memNCachedTotalSpace == 0)
  {
    pSysInfo->memNCachedTotalSpace++;
  }
  
  pSysInfo->memNCachedFreeSpace   = hPoolNCACHED.m_iBaseSize -  hPoolNCACHED.m_globalAlloc;
  
  
  
  if(pHandle->pAudio->synthesizerPipe.inBuffer.szBuffer)
  {
    pSysInfo->synthBufferPercent = (pHandle->pAudio->synthesizerPipe.inBuffer.szConsumer * 100) / pHandle->pAudio->synthesizerPipe.inBuffer.szBuffer;
    pSysInfo->synthPeakPercent = (pHandle->pAudio->synthesizerPipe.inBuffer.prodPeak * 100) / pHandle->pAudio->synthesizerPipe.inBuffer.szBuffer;

  }

  if(pHandle->pAudio->recognizerPipe.outBuffer.szBuffer)
  {
    pSysInfo->recoBufferPercent  = (pHandle->pAudio->recognizerPipe.outBuffer.szConsumer * 100) / pHandle->pAudio->recognizerPipe.outBuffer.szBuffer;
    pSysInfo->recoPeakPercent  = (pHandle->pAudio->recognizerPipe.outBuffer.prodPeak * 100) / pHandle->pAudio->recognizerPipe.outBuffer.szBuffer;
  }

  if(pHandle->pAudio->auxAudioPipe.inBuffer.szBuffer)
  {
    pSysInfo->auxBufferPercent = (pHandle->pAudio->auxAudioPipe.inBuffer.szConsumer * 100) / pHandle->pAudio->auxAudioPipe.inBuffer.szBuffer;
    pSysInfo->auxPeakPercent = (pHandle->pAudio->auxAudioPipe.inBuffer.prodPeak * 100) / pHandle->pAudio->auxAudioPipe.inBuffer.szBuffer;
  }
  
  if(pHandle->pAudio->ringMp3Speaker.szBuffer)
  {
    pSysInfo->mp3BufferPercent = (pHandle->pAudio->ringMp3Speaker.szConsumer * 100) / pHandle->pAudio->ringMp3Speaker.szBuffer;
    pSysInfo->mp3PeakPercent = (pHandle->pAudio->ringMp3Speaker.prodPeak * 100) / pHandle->pAudio->ringMp3Speaker.szBuffer;
  }


  UNLOCK_ALLOC();
  return AVS_OK;
}




/*


Delete a ring buffer objects



*/


void avs_audio_pipe_update_resampler(Avs_audio_pipe *pPipe)
{
  
}



/*

Change the MP3 ALEXA Frequency


*/

void avs_core_change_mp3_frequency(AVS_instance_handle *pHandle, uint32_t newFreq)
{
  AVS_ASSERT(pHandle != 0);
  pHandle->pFactory->synthesizerSampleRate = newFreq;
  pHandle->pAudio->synthesizerPipe.inBuffer.sampleRate = newFreq;
  avs_audio_pipe_update_resampler(&pHandle->pAudio->synthesizerPipe);
  avs_core_message_post(pHandle, EVT_CHANGE_MP3_FREQUENCY, newFreq);
}



AVS_Result avs_audio_buffer_delete(Avs_audio_buffer *pHandle)
{
  if((pHandle->flags & 1U) != 0)
  {
    avs_core_mem_free(pHandle->pBuffer);
  }
  return AVS_OK;
}


/*
Audio Pipe Management
Creates an audio pipe
an audio pipe is an object that exchange audio streams between to clients
audio streams have a 16 bits sample 1 or 2 channels
if the stream have different frequencies or channels, the object convert
convert it AUDIO IN to AUDIO OUT stream format. The client must initialize the field inBuffer and the inSampleRate after the avs_createAudioPipe
the consumer must initialize the field outBuffer and the outSampleRate after the avs_createAudioPipe


*/

AVS_Result avs_audio_pipe_create(Avs_audio_pipe *pHandle)
{
  memset(pHandle, 0, sizeof(*pHandle));
  AVS_VERIFY(avs_core_mutex_create(&pHandle->lock));
  AVS_VERIFY(avs_core_event_create(&pHandle->inEvent));
  AVS_VERIFY(avs_core_event_create(&pHandle->outEvent));
  pHandle->pipeFlags = AVS_PIPE_RUN;
  
  return AVS_OK;
}

/*


Delete the audio pipe instance



*/
AVS_Result avs_audio_pipe_delete(Avs_audio_pipe *pHandle)
{
  avs_core_mutex_delete(&pHandle->lock);
  avs_core_event_delete(&pHandle->inEvent);
  avs_core_event_delete(&pHandle->outEvent);
  return AVS_OK;
}

/*

look at messages for internal change state


*/
static  AVS_Result avs_core_mng_internal_message(AVS_instance_handle *pHandle, AVS_Event evt, uint32_t  pparam1);
static  AVS_Result avs_core_mng_internal_message(AVS_instance_handle *pHandle, AVS_Event evt, uint32_t  pparam1)
{
  switch(evt)
  {
  case EVT_CONNECTION_TASK_DYING:
  case EVT_DOWNSTREAM_TASK_DYING:
    avs_set_state(pHandle, AVS_STATE_RESTART);
    break;
    
    
  default:
    break;
  }
  return AVS_OK;
}



/*

Dispatch a message to the user


*/

AVS_Result avs_core_message_send(AVS_instance_handle *pHandle, AVS_Event evt, uint32_t  pparam1)
{
  AVS_Result result = AVS_NOT_IMPL;
  AVS_ASSERT(((pHandle!= 0) && (pHandle->pFactory != 0)));
  if(pHandle != 0)
  {
    if((evt >= EVT_NO_REENTRANT_START) && (evt <= EVT_NO_REENTRANT_END))
    {
      avs_core_mutex_lock(&pHandle->lockMessage);
    }
    if((pHandle->pFactory != 0)  && (pHandle->pFactory->eventCB != 0))
    {
      result = (pHandle->pFactory->eventCB)(pHandle, pHandle->pFactory->eventCB_Cookie, evt, pparam1);
    }
    
    avs_core_mng_internal_message(pHandle, evt, pparam1);
    
    if((evt >= EVT_NO_REENTRANT_START) && (evt <= EVT_NO_REENTRANT_END) )
    {
      avs_core_mutex_unlock(&pHandle->lockMessage);
    }
  }
  return result;
}


AVS_Result avs_core_message_post(AVS_instance_handle *pHandle, AVS_Event evt, uint32_t  pparam1)
{
  AVS_Result result = AVS_ERROR;
  AVS_ASSERT(((pHandle!= 0) && (pHandle->pFactory != 0)));
  if(pHandle != 0)
  {
    avs_post_t msg;
    msg.evt = evt;
    msg.pparam = pparam1;
    result = avs_core_queue_put(&pHandle->hPostMsg,(void *)(uint32_t)&msg,1000U);
  }
  return result;
  
}


AVS_Result avs_core_post_messages(AVS_instance_handle *pHandle)
{
  AVS_Result result;
  do
  {
    avs_post_t msg;
    result = avs_core_queue_get(&pHandle->hPostMsg,&msg,0);
    if(result  == AVS_OK)
    {
      avs_core_message_send( pHandle,msg.evt,msg.pparam);
  }

  }while(result == AVS_OK);
  return AVS_OK;
}
#define  DUMP_FORMAT_S   "\t%-30s : \"%s\"\r"
#define  DUMP_FORMAT_I   "\t%-30s : %d\r"
#define  DUMP_FORMAT_F   "\t%-30s : %.2f\r"
#define  DUMP_FORMAT_H   "\t%-30s : 0x%08X\r"
#define  DUMP_FORMAT_A   "\t%-30s : "

__STATIC_INLINE void avs_core_print_list(int32_t index, char *pVar, ...);
__STATIC_INLINE void avs_core_print_list(int32_t index, char *pVar, ...)
{
  va_list args;
  va_start(args, pVar);
  uint32_t i = index;
  char_t *pText = va_arg(args, char *);
  while(index)
  {
    pText = va_arg(args, char *);
    index--;
  }
  AVS_Printf(AVS_TRACE_LVL_INFO, "\t%-30s : %d:%s\r", pVar, i, pText);
  va_end(args);
}



__STATIC_INLINE void avs_core_print_variable_string(char_t *pName,char_t *pContents);
__STATIC_INLINE void avs_core_print_variable_string(char_t *pName,char_t *pContents)
{
  if(pContents==0)
  {
    AVS_Printf(AVS_TRACE_LVL_INFO,DUMP_FORMAT_S,pName,"NULL");
  }
  else
  {
    AVS_Printf(AVS_TRACE_LVL_INFO,DUMP_FORMAT_S,pName,pContents);
  }
}
__STATIC_INLINE void avs_core_print_array(char_t *pName,char_t *pContents,uint32_t nb);
__STATIC_INLINE void avs_core_print_array(char_t *pName,char_t *pContents,uint32_t nb)
{
  AVS_Printf(AVS_TRACE_LVL_INFO,DUMP_FORMAT_A, pName);
  AVS_Printf(AVS_TRACE_LVL_INFO,"{ ");
  for(uint32_t a = 0; a < nb ; a++)
  {
    if(a)
    {
      AVS_Printf(AVS_TRACE_LVL_INFO,",");
    }
    AVS_Printf(AVS_TRACE_LVL_INFO,"0x%02X",pContents[a]);
  }
  AVS_Printf(AVS_TRACE_LVL_INFO," }\r");
}




#define  DUMP_VARIABLE_STRING(ptr,variable)     avs_core_print_variable_string((variable),(ptr))
#define  DUMP_VARIABLE_INT(ptr,variable)        AVS_Printf(AVS_TRACE_LVL_INFO,DUMP_FORMAT_I, (variable),(ptr))
#define  DUMP_VARIABLE_FLOAT(ptr,variable)      AVS_Printf(AVS_TRACE_LVL_INFO,DUMP_FORMAT_F, (variable),(ptr))
#define  DUMP_VARIABLE_HEX(ptr,variable)        AVS_Printf(AVS_TRACE_LVL_INFO,DUMP_FORMAT_H, (variable),(ptr))
#define  DUMP_VARIABLE_ARRAY(ptr,name,nb)       avs_core_print_array((name),(ptr),(nb))
#define  DUMP_VARIABLE_LIST(index,variable,...) avs_core_print_list((index),(variable),__VA_ARGS__)


void       avs_core_dump_instance_factory(AVS_instance_handle* handle, char *pBanner)
{
  
  AVS_Printf(AVS_TRACE_LVL_INFO, "\r **** %s ***\r", pBanner);
  DUMP_VARIABLE_STRING((handle->pFactory->clientId), "clientId");
  DUMP_VARIABLE_STRING((handle->pFactory->clientSecret), "clientSecret");
  DUMP_VARIABLE_STRING(handle->pFactory->productId, "productId");
  DUMP_VARIABLE_STRING((handle->pFactory->serialNumber), "serialNumber");
  DUMP_VARIABLE_ARRAY((char_t *)handle->pFactory->macAddr,"macAddr", 6);
  DUMP_VARIABLE_STRING((handle->pFactory->hostName), "hostName");
  DUMP_VARIABLE_INT(handle->pFactory->used_dhcp, "used_dhcp");
  DUMP_VARIABLE_INT((handle->pFactory->use_mdns_responder), "use_mdns_responder");
  DUMP_VARIABLE_STRING((handle->pFactory->ipV4_host_addr), "ipV4_host_addr");
  DUMP_VARIABLE_STRING((handle->pFactory->ipV4_subnet_msk), "ipV4_subnet_msk");
  DUMP_VARIABLE_STRING((handle->pFactory->ipV4_default_gatway), "ipV4_default_gatway");
  DUMP_VARIABLE_STRING(handle->pFactory->ipV4_primary_dns, "ipV4_primary_dns");
  DUMP_VARIABLE_STRING(handle->pFactory->ipV4_secondary_dns, "ipV4_secondary_dns");
  DUMP_VARIABLE_STRING(handle->pFactory->urlNtpServer, "urlNtpServer");
  DUMP_VARIABLE_STRING(handle->pFactory->urlEndPoint, "urlEndPoint");
  DUMP_VARIABLE_STRING(handle->pFactory->redirectUri, "redirectUri");
  DUMP_VARIABLE_STRING(handle->pFactory->cpuID, "cpuID");
  DUMP_VARIABLE_STRING(handle->pFactory->toolChainID, "toolChainID");
  DUMP_VARIABLE_LIST(handle->pFactory->profile, "profile", "AVS_PROFILE_DEFAULT", "AVS_PROFILE_CLOSE_TALK", "AVS_PROFILE_NEAR_FIELD", "AVS_PROFILE_FAR_FIELD");
  DUMP_VARIABLE_LIST(handle->pFactory->initiator, "initiator", "AVS_INITIATOR_DEFAULT", "AVS_INITIATOR_PUSH_TO_TALK", "AVS_INITIATOR_TAP_TO_TALK", "AVS_INITIATOR_VOICE_INITIATED");
  DUMP_VARIABLE_STRING(handle->pFactory->portingAudioName, "portingAudioName");
  DUMP_VARIABLE_STRING(handle->pFactory->netSupportName, "netSupportName");
  DUMP_VARIABLE_STRING(handle->pFactory->alexaKeyWord, "alexaKeyWord");
  DUMP_VARIABLE_INT(handle->pFactory->synthesizerSampleRate, "synthesizerSampleRate");
  DUMP_VARIABLE_INT(handle->pFactory->synthesizerSampleChannels, "synthesizerSampleChannels");
  
  DUMP_VARIABLE_INT(handle->pFactory->useAuxAudio, "useAuxAudio");
  DUMP_VARIABLE_INT(handle->pFactory->auxAudioSampleRate, "auxAudioSampleRate");
  DUMP_VARIABLE_INT(handle->pFactory->auxAudioSampleChannels, "auxAudioSampleChannels");
  
  DUMP_VARIABLE_INT(handle->pFactory->recognizerSampleRate, "recognizerSampleRate");
  DUMP_VARIABLE_HEX(handle->pFactory->eventCB, "eventCB");
  DUMP_VARIABLE_HEX(handle->pFactory->eventCB_Cookie, "eventCB_Cookie");
  DUMP_VARIABLE_HEX(handle->pFactory->persistentCB, "persistentCB");
  DUMP_VARIABLE_INT(handle->pFactory->memDTCMSize, "memDTCMSize");
  DUMP_VARIABLE_INT(handle->pFactory->memPRAMSize, "memPRAMSize");
  DUMP_VARIABLE_INT(handle->pFactory->memNCACHEDSize, "memNCACHEDSize");
  
}
void       avs_core_dump_audio_factory(AVS_audio_handle* handle, char *pBanner)
{
  AVS_Printf(AVS_TRACE_LVL_INFO, "\r **** %s ***\r", pBanner);
  
  DUMP_VARIABLE_INT(handle->pFactory->audioInLatency, "audioInLatency");
  DUMP_VARIABLE_INT(handle->pFactory->audioOutLatency, "audioOutLatency");
  DUMP_VARIABLE_INT(handle->pFactory->audioMp3Latency, "audioMp3Latency");
  DUMP_VARIABLE_INT(handle->pFactory->initVolume, "initVolume");
  DUMP_VARIABLE_INT(handle->pFactory->freqenceOut, "freqenceOut");
  DUMP_VARIABLE_FLOAT(handle->pFactory->buffSizeOut, "buffSizeOut");
  DUMP_VARIABLE_INT(handle->pFactory->freqenceIn, "freqenceIn");
  DUMP_VARIABLE_FLOAT(handle->pFactory->buffSizeIn, "buffSizeIn");
  DUMP_VARIABLE_INT(handle->pFactory->chOut, "chOut");
  DUMP_VARIABLE_INT(handle->pFactory->chIn, "chIn");
  
  DUMP_VARIABLE_INT(handle->pFactory->platform.numConfig, "platform.numConfig");
  DUMP_VARIABLE_INT(handle->pFactory->platform.numProfile, "platform.numProfile");
}







/* Wrapper to create an event */
int8_t avs_core_event_create_named(avs_event *event, const char *pName)
{
  AVS_ASSERT(event != 0);
  event->id     = osSemaphoreCreate(&event->s,1);  
  if((event->id!= 0)  && (pName != 0))
  {
    vQueueAddToRegistry (event->id, pName);
  }
  AVS_ASSERT(event->id!= 0);
  if(event->id != 0) 
  {
    osSemaphoreWait(event->id, 0);
  }
  return (int8_t)  ((event->id != 0) ? TRUE : FALSE);
}


/* Wrapper to delete simply an event */

void avs_core_event_delete(avs_event *event)
{
  AVS_ASSERT(event != 0);
  osSemaphoreDelete(event->id);
  
}

/* Signal the event */
void avs_core_event_set(avs_event *event)
{
  AVS_ASSERT(event != 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    osSemaphoreRelease(event->id);
  }
}

/* Wait for an event */
AVS_Result avs_core_event_wait(avs_event *event, uint32_t timeout)
{
  uint32_t ret=osOK;
  AVS_ASSERT(event != 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    ret = osSemaphoreWait(event->id, timeout);
  }
  return (ret == osOK) ? AVS_OK : AVS_ERROR;
}
/* Signal an event form the isr */
int8_t avs_core_event_set_isr(avs_event *event)
{
  avs_core_event_set(event);
  return TRUE;
}

/* Wrapper for Queues */

int8_t avs_core_queue_create_named(avs_queue *pQueue,uint32_t nbElement,uint32_t itemSize, const char_t *pName)
{
  pQueue->m.queue_sz = nbElement;
  pQueue->m.item_sz  = itemSize;
  pQueue->id     = osMessageCreate(&pQueue->m,NULL);
  if((pQueue->id!= 0)  && (pName != 0))
  {
    vQueueAddToRegistry (pQueue->id, pName);
  }
  AVS_ASSERT(pQueue->id!= 0);
  return (int8_t)  ((pQueue->id != 0) ? TRUE : FALSE);
}

/* Delete a Queue*/
uint32_t avs_core_queue_delete(avs_queue *pQueue)
{
  AVS_ASSERT(pQueue != 0);
  AVS_ASSERT(pQueue->id != 0);
  if(pQueue->id != 0)
  {
    vQueueUnregisterQueue(pQueue->id);
  }
  osMessageDelete(pQueue->id);
  pQueue->id = 0;
  return AVS_OK;
}


AVS_Result  avs_core_queue_put(avs_queue *pQueue,void *pElemen,uint32_t timeout)
{
  AVS_ASSERT(pQueue != 0);
  AVS_ASSERT(pQueue->id != 0);
/*
  We don't use because the API doesn't allow to embed statically the object 
*/
   xQueueSend(pQueue->id, pElemen, timeout);

  return AVS_OK;
}

AVS_Result  avs_core_queue_get(avs_queue *pQueue,void *pElemen,uint32_t timeout)
{
  AVS_ASSERT(pQueue != 0);
  AVS_ASSERT(pQueue->id != 0);

/*  
    We don't use because the API doesn't allow to embed statically the object  
*/

  if(xQueueReceive(pQueue->id, pElemen,timeout) != pdTRUE )  
  {
    return AVS_ERROR;
  }
  return AVS_OK;
}


/* Wrapper to simply mutex */


int8_t avs_core_mutex_create_named(avs_mutex *mutex, const char_t *pName)
{
  mutex->id     = osMutexCreate(&mutex->m);  
  if((mutex->id!= 0)  && (pName != 0))
  {
    vQueueAddToRegistry (mutex->id, pName);
  }
  
  AVS_ASSERT(mutex->id!= 0);
  return (int8_t)  ((mutex->id != 0) ? TRUE : FALSE);
}

/* Delete a mutex */
void avs_core_mutex_delete(avs_mutex *mutex)
{
  AVS_ASSERT(mutex != 0);
  AVS_ASSERT(mutex->id != 0);
  if(mutex->id != 0)
  {
    vQueueUnregisterQueue(mutex->id);
  }
  osMutexDelete(mutex->id);
  mutex->id = 0;
}
/* Lock mutex */
void avs_core_mutex_lock(avs_mutex *mutex)
{
  AVS_ASSERT(mutex!= 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    osMutexWait(mutex->id,  osWaitForever);
  }
}

/* Unlock mutex */
void avs_core_mutex_unlock(avs_mutex *mutex)
{
  AVS_ASSERT(mutex != 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    osMutexRelease(mutex->id);
  }
}

/* Create a recursive mutex */
int8_t avs_core_recursive_mutex_create_named(avs_mutex *mutex, const char *pName)
{
  mutex->id     = osRecursiveMutexCreate(&mutex->m);  
  if((mutex->id!= 0)  && (pName != 0))
  {
    vQueueAddToRegistry (mutex->id, pName);
  }
  
  AVS_ASSERT(mutex->id!= 0);
  return (int8_t)  ((mutex->id != 0) ? TRUE : FALSE);

}

/* Delete a recursive mutex */
void avs_core_recursive_mutex_delete(avs_mutex *mutex)
{
  AVS_ASSERT(mutex != 0);
  AVS_ASSERT(mutex->id != 0);
  if(mutex->id != 0)
  {
    vQueueUnregisterQueue(mutex->id);
  }
  osMutexDelete(mutex->id);

}
/* Lock  a recursive mutex */
void avs_core_recursive_mutex_lock(avs_mutex *mutex)
{
  
  AVS_ASSERT(mutex != 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    osRecursiveMutexWait(mutex->id,  osWaitForever);
  }
}

/* Unlock  a recursive mutex */
void avs_core_recursive_mutex_unlock(avs_mutex *mutex)
{
  AVS_ASSERT(mutex != 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    osRecursiveMutexRelease(mutex->id);
  }
  
}
WEAK void avs_core_task_create_prolog(avs_task *pTask);
WEAK void avs_core_task_create_prolog(avs_task *pTask)
{
}
WEAK void avs_core_task_create_epilog(avs_task *pTask);
WEAK void avs_core_task_create_epilog(avs_task *pTask)
{
}

/* Create a task */
avs_task *avs_core_task_create(const char_t *pName, avs_task_cb  pCb, void *pCookie, size_t stackSize, osPriority  priority)
{
  
  avs_task *pTask = (avs_task *)avs_core_mem_alloc(avs_mem_type_heap,sizeof(avs_task ) ) ;
  pTask->osThread.name = (char_t *)(uint32_t)pName;
  pTask->osThread.pthread = pCb;
  pTask->osThread.tpriority = priority;
  pTask->osThread.stacksize = stackSize;
  pTask->osThread.instances = 0;
  
  /* Specific processing before thread creation*/ 
  avs_core_task_create_prolog(pTask);
  
  pTask->id = osThreadCreate(&pTask->osThread,pCookie);
  if(pTask->id ==0)
  {
    avs_core_mem_free(pTask);
    pTask = NULL;
  }
  AVS_ASSERT(pTask != 0);
  
  /* Specific processing after thread creation*/ 
  avs_core_task_create_epilog(pTask);
 
  return pTask;
}
/* Delete a task */
void avs_core_task_delete(avs_task *task)
{
  if(task) 
  {
    osThreadTerminate(task->id);
  }
  avs_core_mem_free(task);
}

/* A task can't return to the user */
/* So, let's wait for a vTaskDelete */
void avs_core_task_end(void)
{
  while(1)
  {
    avs_core_task_delay(1000);
  }
  
}


void avs_core_task_delay(uint32_t ms)
{
  osDelay(ms);
}

uint32_t avs_core_sys_tick(void)
{
  return osKernelSysTick();
}



/* Init mem pools */
WEAK void avs_core_mem_init(AVS_instance_handle *pHandle)
{
  /*
  We assume that the link scrip is correct and the size reserved to STVS4A is at least = to memDTCMSize +    memPRAMSize
  */
  AVS_ASSERT(pHandle != 0);
  AVS_ASSERT(pHandle->pFactory->memDTCMSize);
  AVS_ASSERT(pHandle->pFactory->memPRAMSize);
  
  
  uint8_t *pBaseBoardMem = (uint8_t*)BOARD_RAMDTCM_BASE;
  avs_mem_init(&hPoolDTCM, (void *)pBaseBoardMem, pHandle->pFactory->memDTCMSize*1024);
  pBaseBoardMem  += pHandle->pFactory->memDTCMSize*1024;
  avs_mem_init(&hPoolPRAM, (void *)pBaseBoardMem, pHandle->pFactory->memPRAMSize*1024);
}
/* Term mem pools */
WEAK void avs_core_mem_term(void)
{
}


WEAK avs_mem_type_t avs_core_mem_pool_type(void *pMem)
{
  if(avs_mem_check_ptr(&hPoolDTCM, pMem) != 0)
  {
    return avs_mem_type_dtcm;
  }
  
  if(avs_mem_check_ptr(&hPoolPRAM, pMem) != 0)
  {
    return avs_mem_type_pram;
  }
 
  if(avs_mem_check_ptr(&hPoolNCACHED, pMem) != 0)
  {
    return avs_mem_type_ncached;
  }

  
  return avs_mem_type_heap;
}


/* Malloc + check */
WEAK void *avs_core_mem_alloc(avs_mem_type_t type, size_t size)
{
  void *p=NULL;
  if(type == avs_mem_type_best)
  {
    p = avs_core_mem_alloc(avs_mem_type_dtcm, size);
    if(p != 0)
    {
      return p;
    }
    p = avs_core_mem_alloc(avs_mem_type_pram, size);
    if(p != 0)
    {
      return p;
    }
    return avs_core_mem_alloc(avs_mem_type_heap, size);
  }
  
  
  if(type == avs_mem_type_dtcm)
  {
    LOCK_ALLOC();
    p = avs_mem_alloc(&hPoolDTCM, size);
    UNLOCK_ALLOC();
    return p;
  }
  
  if(type == avs_mem_type_pram)
  {
    LOCK_ALLOC();
    p = avs_mem_alloc(&hPoolPRAM, size);
    UNLOCK_ALLOC();
    return p;
  }

  if(type == avs_mem_type_ncached)
  {
    LOCK_ALLOC();
    p = avs_mem_alloc(&hPoolNCACHED, size);
    UNLOCK_ALLOC();
    return p;
  }

  /* Fall back is the heap */
  LOCK_ALLOC();
  p=pvPortMalloc(size);
//  p = malloc(size);
  if(p) memset(p,0,size);
  UNLOCK_ALLOC();
  return p;
}

/* Calloc + check */
WEAK void   *avs_core_mem_calloc(avs_mem_type_t type, size_t size, size_t elem)
{
  void *p=0;
  
  if(type == avs_mem_type_best)
  {
    p = avs_core_mem_calloc(avs_mem_type_dtcm, size, elem);
    if(p != 0)
    {
      return p;
    }
    p = avs_core_mem_calloc(avs_mem_type_pram, size, elem);
    if(p != 0)
    {
      return p;
    }
    return avs_core_mem_calloc(avs_mem_type_heap, size, elem);
  }
  
  if(type == avs_mem_type_dtcm)
  {
    LOCK_ALLOC();
    p = avs_mem_calloc(&hPoolDTCM, size, elem);
    UNLOCK_ALLOC();
    return p;
  }
  
  if(type == avs_mem_type_pram)
  {
    LOCK_ALLOC();
    p = avs_mem_calloc(&hPoolPRAM, size, elem);
    UNLOCK_ALLOC();
    return p;
  }
  
  if(type == avs_mem_type_ncached)
  {
    LOCK_ALLOC();
    p = avs_mem_calloc(&hPoolNCACHED, size, elem);
    UNLOCK_ALLOC();
    return p;
  }

  
  /* Fallback is heap */
  LOCK_ALLOC();
  p = malloc(size * elem);
  if(p)
  {
    memset(p, 0, size * elem);
  }
  UNLOCK_ALLOC();
  return p;
}

uint8_t isAllocatedMemory(void *pMemToFree);

/* Realloc + check */
WEAK void *avs_core_mem_realloc(avs_mem_type_t type, void *pBuffer, size_t size)
{
  void *p = 0;
  if(pBuffer != 0)
  {
    type = avs_core_mem_pool_type(pBuffer); /* In case the ptr is not 0, we realloc from the right pool */
  }
  if(type == avs_mem_type_best)
  {
    LOCK_ALLOC();
    p = avs_mem_realloc(&hPoolDTCM, pBuffer, size);
    UNLOCK_ALLOC();
    if(p != 0)
    {
      return p;
    }
    LOCK_ALLOC();
    p = avs_mem_realloc(&hPoolPRAM, pBuffer, size);
    UNLOCK_ALLOC();
    if(p != 0)
    {
      return p;
    }
    /* default alloc is the heap */
    LOCK_ALLOC();
    p = realloc(pBuffer, size);
    UNLOCK_ALLOC();
    return p;
  }
  
  
  if(type == avs_mem_type_dtcm)
  {
    LOCK_ALLOC();
    p = avs_mem_realloc(&hPoolDTCM, pBuffer, size);
    UNLOCK_ALLOC();
    return p;
  }
  if(type == avs_mem_type_pram)
  {
    LOCK_ALLOC();
    p = avs_mem_realloc(&hPoolPRAM, pBuffer, size);
    UNLOCK_ALLOC();
    return p;
    
  }
  if(type == avs_mem_type_ncached)
  {
    LOCK_ALLOC();
    p = avs_mem_realloc(&hPoolNCACHED, pBuffer, size);
    UNLOCK_ALLOC();
    return p;
    
  }

  /* Fall-back is the heap */
  LOCK_ALLOC();
  p=pvPortMalloc(size);
  if(p){
	  memcpy(p,pBuffer,size);
  }
  if(isAllocatedMemory(pBuffer)){
	  vPortFree(pBuffer);
  }
//  p = realloc(pBuffer, size);
  UNLOCK_ALLOC();
  return p;
}

/* Free + check */

WEAK void avs_core_mem_free(void *pMemToFree)
{
  /* TLS seems to free NULL pointer */
  if(pMemToFree == 0)
  {
    return;
  }
  if(avs_mem_check_ptr(&hPoolDTCM, pMemToFree))
  {
    LOCK_ALLOC();
    avs_mem_free(&hPoolDTCM, pMemToFree);
    UNLOCK_ALLOC();
    return;
  }
  if(avs_mem_check_ptr(&hPoolPRAM, pMemToFree))
  {
    LOCK_ALLOC();
    avs_mem_free(&hPoolPRAM, pMemToFree);
    UNLOCK_ALLOC();
    return;
  }
  if(avs_mem_check_ptr(&hPoolNCACHED, pMemToFree))
  {
    LOCK_ALLOC();
    avs_mem_free(&hPoolNCACHED, pMemToFree);
    UNLOCK_ALLOC();
    return;
  }
  /* Fall back is the heap */
  
  LOCK_ALLOC();
  if(isAllocatedMemory(pMemToFree)){
	  vPortFree(pMemToFree);
  }
  	//  free(pMemToFree);
  UNLOCK_ALLOC();
  
}


void avs_core_terminat_isr(uint32_t flag)
{
  portEND_SWITCHING_ISR(flag)
}

static int32_t inHandlerMode (void);
static int32_t inHandlerMode (void)
{
  return (__get_IPSR()) != 0;
}


/* Stops all tasks */
void avs_core_lock_tasks(void)
{
  if(inHandlerMode() != 0)
  {
    return;
  }
  if(xTaskGetSchedulerState()  == taskSCHEDULER_NOT_STARTED)
  {
    return;
  }
  vTaskSuspendAll();
}

/* Restart all tasks */
void avs_core_unlock_tasks(void)
{
  if(inHandlerMode() != 0)
  {
    return;
  }
  if(xTaskGetSchedulerState()  == taskSCHEDULER_NOT_STARTED)
  {
    return;
  }
  
  xTaskResumeAll();
  
}


/* Set the global instance ( TODO remove me and pass and handle to each function) */
void avs_core_set_instance(AVS_instance_handle *pHandle)
{
  avs_pCurrentInstance = pHandle;
}

/* Get the global instance ( TODO remove me and pass and handle to each function) */

AVS_instance_handle *avs_core_get_instance(void)
{
  return avs_pCurrentInstance;
}


/*
This function is used for the debug and push an array on the console
the console capture the array to produce a raw file our a wav file
see ST console tool

*/
void avs_core_dump_console(void *pDump, uint32_t sizeinByte)
{
  FILE *fp = NULL;  /* Avoid warning*/
  uint8_t *pByte = (uint8_t *) pDump;
  for(uint32_t a = 0; a < sizeinByte ; a++)
  {
    fputc(*pByte, fp);
    pByte++;
  }
  
}
/*
This function is used for the debug and push an array on the console
the console capture the array to produce a raw file our a wav file
see ST console tool

*/
void avs_core_dump_console_audio_consumer(Avs_audio_buffer *pBuffer, uint32_t nbsample)
{
  int16_t *pIn = avs_audio_buffer_get_consumer(pBuffer);
  for(uint32_t a = 0; a < nbsample ; a++)
  {
    avs_core_dump_console(&pIn[0], 2);
    if(pBuffer->nbChannel == 2)
    {
      avs_core_dump_console(&pIn[1], 2);
    }
    avs_audio_buffer_move_ptr_consumer(pBuffer, 1, &pIn);
  }
}

/* Set the last error (TODO remove me , not used) */
void avs_core_set_last_error(AVS_Result err)
{
  avs_lastError = err;
}

const char_t *avs_core_get_level_string(uint32_t level)
{
  const char_t *pStr=0;
  switch(level)
  {
  case AVS_TRACE_LVL_ERROR:
    {
      pStr = "Error";
      break;
    }
  case AVS_TRACE_LVL_WARNING:
    {
      pStr = "Warning";
      break;
    }
  case AVS_TRACE_LVL_NETWORK:
    {
      
      pStr = "Network";
      break;
    }
  case AVS_TRACE_LVL_DEBUG:
    {
      pStr = "Debug";
      break;
    }
  case AVS_TRACE_LVL_JSON:
    {
      pStr ="JSon";
      break;
      
    }
  case AVS_TRACE_LVL_JSON_FORMATED:
    {
      pStr = "JSon-F";
      break;
    }
  case AVS_TRACE_LVL_DIRECTIVE:
    {
      pStr = "Directive";
      break;
    }
  default:
    {
      break;
    }
  }
  return pStr;
}


/* Hook oryx traces */
void debugfprintf(FILE *stream, ...)
{
  if((avs_debugLevel & (uint32_t)AVS_TRACE_LVL_NETWORK) != 0)
  {
    va_list args;
    avs_core_lock_tasks();
    va_start(args, stream);
    const char_t* lpszFormat= va_arg(args,    const char_t* );
    uint32_t nChars = vsnprintf(szBuffer, (size_t)sizeof(szBuffer), lpszFormat, args);
    if (nChars > sizeof(szBuffer) - 2)
    {
      strcpy( &szBuffer[ sizeof(szBuffer) - 4], "...");
    }
    drv_sys.platform_Sys_puts(szBuffer, 0);
    avs_core_unlock_tasks();
    va_end(args);
  }
}

/* Hook oryx traces */
void debugDisplayArray(FILE *stream, const char_t *prepend, const void *data, size_t length)
{
  if((avs_debugLevel & (uint32_t)AVS_TRACE_LVL_NETWORK) != 0)
  {
    uint_t i;
    
    avs_core_lock_tasks();
    for(i = 0; i < length; i++)
    {
      /* Beginning of a new line? */
      if((i % 16) == 0)
      {
        debugfprintf(stream,"%s", prepend);
      }
      /* Display current data byte */
      debugfprintf(stream,"%02" PRIX8 " ", *((uint8_t *)(uint32_t) data + i));
      /* End of current line? */
      if(((i % 16) == 15) || (i == (length - 1)))
      {
        debugfprintf(stream, "\r\n");
      }
    }
    avs_core_unlock_tasks();
    
  }
}
/* See json formatter */
static void avs_core_insert_tabs(uint32_t index);
static void avs_core_insert_tabs(uint32_t index)
{
  for(uint32_t a = 0; a < index ; a++)
  {
    drv_sys.platform_Sys_puts("  ", 0);
  }
}

void avs_core_trace_json(const char* pJson)
{
  uint32_t level = 0;
  char_t theChar[2] = {0, 0};
  
  while(*pJson)
  {
    theChar[0] = *pJson;
    pJson++;
    
    
    if(theChar[0] == '{')
    {
      drv_sys.platform_Sys_puts("\r", 0);
      avs_core_insert_tabs(level);
      drv_sys.platform_Sys_puts(theChar, 0);
      drv_sys.platform_Sys_puts("\r", 0);
      level++;
      avs_core_insert_tabs(level);
      
    }else if(theChar[0] == '}')
    {
      level--;
      drv_sys.platform_Sys_puts("\r", 0);
      avs_core_insert_tabs(level);
      drv_sys.platform_Sys_puts(theChar, 0);
      drv_sys.platform_Sys_puts("\r", 0);
      avs_core_insert_tabs(level);
    }else if(theChar[0] == ',')
    {
      drv_sys.platform_Sys_puts(theChar, 0);
      drv_sys.platform_Sys_puts("\r", 0);
      avs_core_insert_tabs(level);
    } else
    {
      drv_sys.platform_Sys_puts(theChar, 0);
    }
    
  }
  drv_sys.platform_Sys_puts("\r", 0);
  
}

/* Low level produce a trace and clamp the string if out of buffer */
void avs_core_trace_args(uint32_t flag, const char* lpszFormat, va_list args)
{
  uint32_t nChars = vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
  if (nChars > sizeof(szBuffer) - 2)
  {
    strcpy( &szBuffer[ sizeof(szBuffer) - 4], "...");
  }
  drv_sys.platform_Sys_puts(szBuffer, flag);
  
}




/*
In order to ,not propagate various temporary local buffer, we use a tmp buffer allocator
It is a small system that allocate a small buffer in a bigger buffer static ring buffer
This allocator is done to contain small string or http response page with a short retention
There is no Free, since the next block will start at the end of the previous one

*/


WEAK  AVS_Result avs_core_short_object_init(AVS_instance_handle *pHandle)
{
#if defined(USE_SHORT_OBJECT_POOL)
  pHandle->pMemoryPool = avs_core_mem_alloc(avs_mem_type_heap, SHORT_ALLOC_MEM_POOL);
  AVS_ASSERT(pHandle->pMemoryPool != 0);
  avs_mem_init(&hShortObjectPool, pHandle->pMemoryPool, SHORT_ALLOC_MEM_POOL);
#endif
  return AVS_OK;
  
}
WEAK  void  avs_core_short_object_term(AVS_instance_handle *pHandle)
{
#if defined(USE_SHORT_OBJECT_POOL)
  avs_mem_term(&hShortObjectPool);
  if(pHandle->pMemoryPool)
  {
    avs_core_mem_free(pHandle->pMemoryPool);
  }
  pHandle->pMemoryPool = 0;
#endif
}
/* Free mem */
WEAK  void avs_core_short_free(avs_short_alloc_object  *ptr)
{
#if defined(USE_SHORT_OBJECT_POOL)
  LOCK_ALLOC();
  avs_mem_free(&hShortObjectPool, (void *)ptr);
  UNLOCK_ALLOC();
#else
  avs_core_mem_free((void *)ptr);
#endif
}

/* Alloc mem */
WEAK  avs_short_alloc_object  *avs_core_short_alloc(uint32_t maxSize)
{
  AVS_ASSERT(maxSize < 5 * 1024); /* Make sure it is a small bufer */
  
#if defined(USE_SHORT_OBJECT_POOL)
  LOCK_ALLOC();
  avs_short_alloc_object  *p = (avs_short_alloc_object  *)avs_mem_alloc(&hShortObjectPool, maxSize);
  UNLOCK_ALLOC();
  return p;
#else
  return avs_core_mem_alloc(avs_mem_type_heap, maxSize);
#endif
}
/* Realloc mem */
WEAK  avs_short_alloc_object  *avs_core_short_realloc(avs_short_alloc_object  *ptr, uint32_t maxSize)
{
  AVS_ASSERT(maxSize < 5 * 1024); /* Make sure it is a small buffer */
#if defined(USE_SHORT_OBJECT_POOL)
  LOCK_ALLOC();
  avs_short_alloc_object  *p = (avs_short_alloc_object  *)avs_mem_realloc(&hShortObjectPool, (void*)ptr, maxSize);
  UNLOCK_ALLOC();
  return p;
  
#else
  
  return avs_core_mem_realloc(avs_mem_type_heap, (void*)ptr, maxSize);
#endif
  
}

/*

parses the wav header a,d fill Avs_sound_player struct


*/
AVS_Result avs_core_audio_wav_sound(void *pWave, Avs_sound_player *pSound)
{
  uint32_t *pLong;
  uint16_t *pShort;
  uint8_t  *pByte;
  
  pByte = (uint8_t *)(&((uint8_t *)pWave)[0]);
  
  /* Check some signatures */
  
  if(strncmp((char_t*)pByte, "RIFF", 4) != 0)
  {
    return AVS_PARAM_INCORECT;
  }
  if(strncmp((char_t*)(uint32_t)&pByte[8], "WAVEfmt ", 8) != 0)
  {
    return AVS_PARAM_INCORECT;
  }
  if(strncmp((char_t*)(uint32_t)&pByte[36], "data", 4) != 0)
  {
    return AVS_PARAM_INCORECT;
  }
  
  pLong = (uint32_t *)(uint32_t)(&((uint8_t *)pWave)[16]);
  if(pLong[0] !=  0x10)
  {
    return AVS_PARAM_INCORECT;
  }
  /* Length of the 'fmt' data, must be 0x10 */
  pShort = (uint16_t *)(uint32_t)(&((uint8_t *)pWave)[20]);
  /* Audio format, must be 0x01 (PCM) */
  if(pShort[0] != 1)
  {
    return AVS_PARAM_INCORECT;
  }
  
  /* Num channels */
  uint32_t ch = pByte [22];
  uint32_t numbit   = pByte[34];
  if(numbit != 16)
  {
    return AVS_PARAM_INCORECT;
  }
  
  /* The Sample Rate in Hz */
  /* Little Indian ie. 8000 = 0x00001F40 => byte[24]=0x40, byte[27]=0x00 */
  pLong = (uint32_t *)(uint32_t)(&((uint8_t *)pWave)[24]);
  uint32_t freq = (*pLong);
  pLong = (uint32_t *)(uint32_t)(&((uint8_t *)pWave)[40]);
  
  uint32_t nbSample = (*pLong) / (sizeof(uint16_t) * ch);
  
  uint16_t *pData = (uint16_t *)(uint32_t)&pByte[44];
  memset(pSound, 0, sizeof(*pSound));
  pSound->curCount = nbSample;
  pSound->count = nbSample;
  
  pSound->chWave = ch;
  pSound->freqWave = freq;
  pSound->pWave = (int16_t *)(uint32_t)pData;
  pSound->pCurWave = pSound->pWave ;
  pSound->volume = 100;
  return AVS_OK;
}


/* Fill an array with each line entries */
static uint32_t avs_core_get_lines_from_string( char *pBuffer, char **tLines, uint32_t maxLines);
static uint32_t avs_core_get_lines_from_string( char *pBuffer, char **tLines, uint32_t maxLines)
{
  uint32_t index = 0;
  while((index  < maxLines) && (*pBuffer != 0))
  {
    *tLines = pBuffer;
    tLines++;
    /* Seek to the new line */
    while((*pBuffer != 0) && (*pBuffer != '\n'))
    {
      pBuffer++;
    }
    index++;
    if(*pBuffer != 0)
    {
      pBuffer++;
    }
    
  }
  return index;
}


/*

search a key in each staring line of the string , and extract the string until the end of line
each line must terminate by \r
return AVS_OK if found , or AVS_ERROR


*/
AVS_Result avs_core_get_key_string(AVS_instance_handle * pHandle, char *pString, char *pKey, char *pValue, uint32_t iMaxSizeValue)
{
  AVS_ASSERT(pString != 0);
  AVS_ASSERT(pValue != 0);
  char_t   *tLines[50];
  uint32_t nbLines = avs_core_get_lines_from_string(pString, tLines, 50);
  if(nbLines  == 0)
  {
    return AVS_ERROR;
  }
  /* Parses each header lines and only manage mandatory info */
  for(uint32_t a = 0 ; a < nbLines ; a++)
  {
    if(strncmp(tLines[a], pKey, strlen(pKey)) == 0)
    {
      char_t*pLine = tLines[a] + strlen(pKey);
      /* Copy the value in the buffer */
      for(uint32_t index = 0; index < iMaxSizeValue - 1 ; index++)
      {
        /* If CR , stop and terminate the string */
        if(*pLine == '\r')
        {
          *pValue = 0;
          return AVS_OK;
        }
        *pValue = *pLine;
        pValue++;
        pLine++;
        
      }
    }
  }
  return AVS_ERROR;
}


/*  Initialize the  default factory */
void avs_init_default_string(char_t **pString,char_t *pDefault)
{
  if(*pString == 0)
  {
    *pString = pDefault;
  }
}

/*  Initialize the default factory */
void avs_init_default_interger(uint32_t *pInterger,uint32_t iDefault)
{
  if(*pInterger == 0)
  {
    *pInterger = iDefault;
  }
}

