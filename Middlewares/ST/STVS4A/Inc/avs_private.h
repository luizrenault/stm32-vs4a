/**
******************************************************************************
* @file    avs.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   it is the private  SDK implementation , all internal implementation must include the header rather than openavs.h
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
#ifndef _avs_private_
#define _avs_private_

#include "avs_misra.h"

MISRAC_DISABLE          /* Disable all C-State for external dependencies */
#include "string.h"
#include "stdlib.h"
#include "stdint.h"
#include "math.h"
#include "stdio.h"
#include "stdarg.h"
#include "time.h"
#include "jansson.h"
#include "jansson_private.h"
#include "cmsis_os.h"
MISRAC_ENABLE          /* Enable all  C-State */


#include "avs.h"
//#include "avs_network_private.h"
#include "../Src/Porting/Network/LWIP/avs_network_private.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef WEAK
#define WEAK    __weak
#endif


  #if defined ( __ICCARM__  )
#define TOOLCHAIN         "IAR"
#elif  defined ( __GNUC__  )
#define TOOLCHAIN         "GCC"
#elif  defined ( __CC_ARM )
#define TOOLCHAIN         "Keil"
#else
#define TOOLCHAIN         "Unknown"
#endif
/* Force Keil compiler to inline critical functions. */
#ifndef __ALWAYS_INLINE
#if defined ( __GNUC__  ) || defined ( __ICCARM__   ) /* GCC + IAR */
#define __ALWAYS_INLINE __STATIC_INLINE
#elif defined (__CC_ARM) /* Keil */
#define __ALWAYS_INLINE __attribute__((always_inline))
#else
#warning "__ALWAYS_INLINE undefined "
#define __ALWAYS_INLINE
#endif
#endif

#if defined ( __GNUC__  )
#define TASK_SIZE 400
#else
#define TASK_SIZE 1500
#endif
  
#define global                               /* prefix alignement */

#define avs_task_running                     1U
#define avs_task_closed                      2U
#define avs_task_force_exit                  4U
#define avs_task_about_closing               8U

#define SHORT_ALLOC_MEM_POOL                 (4*1024)
#define avs_core_task_priority_idle          osPriorityIdle
#define avs_core_task_priority_below_normal  osPriorityBelowNormal
#define avs_core_task_priority_normal        osPriorityNormal
#define avs_core_task_priority_above_normal  osPriorityAboveNormal
#define avs_core_task_priority_realtime      osPriorityRealtime


#define AUDIO_OUT_TASK_STACK_SIZE       300
#define AUDIO_OUT_TASK_PRIORITY         avs_core_task_priority_above_normal
#define AUDIO_OUT_TASK_NAME              "AVS:Audio SYNTHESIZER"
#define AUDIO_OUT_CHANNELS              1
#define DEFAULT_AUDIO_OUT_MULT_SIZE     4 /* Nb sample for the AVS stream out */


#define AUDIO_IN_TASK_STACK_SIZE       300
#define AUDIO_IN_TASK_PRIORITY         avs_core_task_priority_below_normal
#define AUDIO_IN_TASK_NAME              "AVS:Audio RECOGNIZER"
#define AUDIO_IN_CHANNELS              1
#define DEFAULT_AUDIO_IN_MULT_SIZE     10 /* Nb sample for the AVS stream in */

#define AVS_TIME_MS_TO_EPOCH(timeMs) ((time_t)((timeMs)/1000))
#define AVS_INSTANCE_SIGNATURE          0xAA020301U
#define CHECK_INSTANCE_SIGN(a)          AVS_ASSERT((a)->signature == AVS_INSTANCE_SIGNATURE)



#define STATE_TASK_STACK_SIZE           300
#define STATE_TASK_PRIORITY             avs_core_task_priority_above_normal
#define STATE_TASK_NAME                 "AVS:State Manager"

#define TOKEN_TASK_STACK_SIZE           (1000)
#define TOKEN_TASK_PRIORITY             avs_core_task_priority_idle
#define TOKEN_TASK_NAME                 "AVS:Token Refresh"

#define DOWN_STREAM_TASK_STACK_SIZE      TASK_SIZE
#define DOWN_STREAM_TASK_PRIORITY        avs_core_task_priority_below_normal    /* Downstream and Connection sould have the same priority to prevent Priority inversion issue */
#define DOWN_STREAM_TASK_NAME            "AVS:DownStream"

#define HTTP_CTX_TASK_STACK_SIZE         250
#define HTTP_CTX_TASK_PRIORITY           avs_core_task_priority_below_normal    /* Downstream and Connection sould have the same priority to prevent Priority inversion issue */
#define HTTP_CTX_TASK_NAME               "AVS:Connnection"

/* Time and timeout */
/* Http2 threads */

#define HTTP2_CLOSE_TIMEOUT     (5*60*1000)       /* 5 mins due to several internal timeouts */
#define TIMEOUT_CONNECTION      (10*1000)       /* Default is 10 secs */
#define PING_TIME_IN_SEC        (5*60)          /* Delay between each ping */
#define MIN_TIMEOUT             (100)
#define WAIT_EVENT_TIMEOUT      (2*1000)        /* Timeout to pool event from the directive stream */
#define AVS_DIRECTIVE_TIMEOUT   (10*1000)
#define AVS_TOKEN_RENEW_PERIOD  (10 *1000)      /* Check the renew avery 10 secs */
#define AVS_TOKEN_RENEW_MAX     (59 * 60 *1000) /* This token is valid for one hour, let's wait 59' before refreshing it */
/* State thread */
#define IDLE_SLEEP_TIME         (5)            /* Sleeping time to wait an event */
#define DELAY_SLEEP             (500)           /* Sleeping divisor to wait an event */
#define AVS_AUDIO_PACKET_IN_MS  (10)            /* Nb sample to sent at once in the network */
#define WAIT_CONNECT_TIMEOUT_MS ((2*60*1000))   /* Time out 2 minutes */
#define IDLE_TIMEOUT_MS         (10*1000)        /* Maximum time for event waiting */



#define AUTHENTICATION_SERVER_NAME  "www.amazon.com"
#define AUTHENTICATION_SERVER_PORT  443
#define MAX_SIZE_WEB_ELEMENT            2000
#define AUTHENTICATION_GRANT_GET_TOKEN_SERVER_NAME "api.amazon.com"


#define AVS_IS_VOID_NULL_POINTER(p)   ((p) == 0U)  /*!< simplify writing @ingroup macro*/        
#define AVS_IS_VOID_VALID_POINTER(p)  ((p) != 0U)  /*!< simplify writing @ingroup macro*/        
#define AVS_IS_NULL_POINTER(p)        ((p) == 0U)  /*!< simplify writing @ingroup macro*/        
#define AVS_IS_VALID_POINTER(p)       ((p) != 0U)  /*!< simplify writing @ingroup macro*/        


#define AVS_INIT_DEFAULT(var,val) if((var) ==0)\
                                  {\
                                    (var)=(val);\
                                  }

#define MAX_SIZE_JSON            (5*1024)
#define AVS_EVT_PARAM(a)         ((uint32_t)(a))


#define AVS_CONTENT_TYPE_JSON       "application/json"
#define AVS_CONTENT_TYPE_STREAM     "application/octet-stream"


#define AVS_GRANT_CODE_MAX_SIZE (128+1) /* The authorization code can range from 18 to 128 characters. */
/* Maximum size of access token */
/* An access token is an alphanumeric code 350 characters or more in length, with a maximum size of 2048 bytes */
#define AVS_TOKEN_MAX_SIZE 2048
#define AVS_BEARER_MAX_SIZE (AVS_TOKEN_MAX_SIZE+ sizeof("Bearer "))

#define MAX_AVS_HEADER_STRING_LEN       30

/* Os wrappers */

#define avs_infinit_delay         osWaitForever

typedef  struct t_avs_task
{
  osThreadDef_t osThread;
 osThreadId     id;
}avs_task;

typedef  os_pthread      avs_task_cb;
typedef struct
{
  osMutexDef_t     m;
  osMutexId    id;
} avs_mutex;


typedef struct
{
  osMessageQDef_t  m;
  osMessageQId     id;
} avs_queue;


typedef struct
{
  osSemaphoreDef_t  s;
  osSemaphoreId    id;
} avs_event;


typedef struct t_avs_up_sampling
{
  int32_t       cumul ;                 /* Cumulus for down sampling */
  int16_t       *lastSample;            /* Last sample up sampling */
  int32_t       deltaSample;
  int32_t       deltaCpt;

} avs_up_sampling_t;

__ALWAYS_INLINE int16_t avs_upsampling_get_sample(avs_up_sampling_t *upSmp, int16_t curSample, uint32_t ch);
__ALWAYS_INLINE int16_t avs_upsampling_get_sample(avs_up_sampling_t *upSmp, int16_t curSample, uint32_t ch)
{
  /* Interpolate sample */
  register  int16_t lastSmp = 0;
  register int16_t smp = 0;
  if (upSmp->lastSample)
  {
    lastSmp = upSmp->lastSample[ch];
  }
  if (upSmp->deltaSample)
  {
    smp = (lastSmp + ((curSample - lastSmp) * (int16_t)upSmp->deltaCpt) / (int16_t)upSmp->deltaSample);
  }
  return smp;
  
}



__ALWAYS_INLINE int8_t  avs_upsampling_compute(avs_up_sampling_t *upSmp, int32_t freqlow, int32_t freqhigh, int16_t *pCur);
__ALWAYS_INLINE int8_t  avs_upsampling_compute(avs_up_sampling_t *upSmp, int32_t freqlow, int32_t freqhigh, int16_t *pCur)
{
  upSmp->deltaCpt++;
  uint32_t ret=0U;
  if(upSmp->deltaCpt >= upSmp->deltaSample )
  {
    /* Store the last sample for future interpolation */
    upSmp->lastSample  = pCur;
    /* Add the cumul */
    upSmp->cumul += freqhigh;

    AVS_ASSERT(freqlow > 0);
    /* Extract the sample number to duplicate */
    if(freqlow)
    {
      upSmp->deltaSample = upSmp->cumul   / freqlow;
    }
    /* Compute the remaining segment */
    if(freqlow)
    {    
      upSmp->cumul =  upSmp->cumul   % freqlow;
      }
    /* Clear sample counter */
    upSmp->deltaCpt = 0;
    ret= 1U;

  }
  return (int8_t)ret;
}

__ALWAYS_INLINE void avs_upsampling_init(avs_up_sampling_t*upSmp);
__ALWAYS_INLINE void avs_upsampling_init(avs_up_sampling_t*upSmp)
{
  memset(upSmp, 0, sizeof(avs_up_sampling_t));

}


typedef struct t_avs_dn_sampling
{
  uint8_t       bState;                 /* True if we compute the range , false if we compute the average */
  int32_t       cumul ;                 /* Frequency cumul */
  int32_t       sampleRange;            /* Nb sample to average */
  int32_t       indexRange;               /* Current index */
  int32_t       smptAverage[2];         /* Sample average cumul */
} avs_dn_sampling_t;



__ALWAYS_INLINE void avs_dnsampling_init(avs_dn_sampling_t *dnSmp);
__ALWAYS_INLINE void avs_dnsampling_init(avs_dn_sampling_t *dnSmp)
{
  memset(dnSmp, 0, sizeof(avs_dn_sampling_t));
}



/*

 return average sample


*/
__ALWAYS_INLINE int16_t avs_dnsampling_get_sample(avs_dn_sampling_t *dnSmp, int32_t ch );
__ALWAYS_INLINE int16_t avs_dnsampling_get_sample(avs_dn_sampling_t *dnSmp, int32_t ch )
{
  int16_t ret = 0;
  if(ch == 3)
  {
    ret = ((int16_t )((dnSmp->smptAverage[1] + dnSmp->smptAverage[0]) / (dnSmp->sampleRange * 2))); /* Mix both channels */
  }
  else
  {
    ret = ((int16_t )(dnSmp->smptAverage[ch] / dnSmp->sampleRange))  ;
  }
  return ret;
}

/*

 compute the cumulus and average sample


*/
__ALWAYS_INLINE int8_t  avs_dnsampling_compute(avs_dn_sampling_t *dnSmp, int32_t freqhigh, int32_t  freqlow, int32_t  (*getSamplePtr)(void *pCookie), void *pCookie, int32_t ch, uint32_t limite);
__ALWAYS_INLINE int8_t  avs_dnsampling_compute(avs_dn_sampling_t *dnSmp, int32_t freqhigh, int32_t  freqlow, int32_t  (*getSamplePtr)(void *pCookie), void *pCookie, int32_t ch, uint32_t limite)
{
  /* Check the state */
  int8_t ret = FALSE;
  if (dnSmp->bState == (uint8_t)0)
  {
    /* Compute the cumulus */
    dnSmp->cumul += freqhigh;
    /* Extract the sample range we have to average */
    if(freqlow)
    {
      dnSmp->sampleRange = dnSmp->cumul /  freqlow;
    }
    /* Sanity check; never 0 */
    AVS_ASSERT(dnSmp->sampleRange != 0);
    /* Compute the remaining cumul for the next step */
    if(freqlow)
    {
      dnSmp->cumul = dnSmp->cumul %  freqlow;
    }
    /* Reset the state */
    dnSmp->smptAverage[0] = 0;
    dnSmp->smptAverage[1] = 0;
    dnSmp->indexRange = 0;
    dnSmp->bState = 1U;
  }
  if(dnSmp->bState)
  {
    /* Compute the average sample for the range */
    while((limite != 0UL) && (dnSmp->indexRange < dnSmp->sampleRange)  )
    {
      uint32_t sample = getSamplePtr(pCookie);
      dnSmp->smptAverage[0] += (int16_t)(sample) ;
      if(ch == 2)
      {
        dnSmp->smptAverage[1] += (int16_t)(uint32_t)(sample >> 16) ;
      }
      dnSmp->indexRange++;
      limite--;
    }
  }
  /* If we are not couplet , same sample are mandatory to finish the average */
  /* Return false and finish the state in the next step */
  if(dnSmp->indexRange !=  dnSmp->sampleRange )
  {
    ret =  FALSE;
  }
  else
  {
    /* Reset the state */
    dnSmp->bState = 0;
    ret = TRUE;
  }
  return ret;
}

/* Ring buffer support */


typedef enum avs_consumer
{
  AVS_RING_BUFF_PROD_PART1 = 0,         /* Set the ring buffer to first part ready to produce */
  AVS_RING_BUFF_PROD_PART2,             /* Set the ring buffer to the second  part ready to produce */
  AVS_RING_BUFF_CONSUM_PART1,           /* Set the ring buffer to first part ready to consume */
  AVS_RING_BUFF_CONSUM_PART2            /* Set the ring buffer to second part ready to consume */
} Avs_consumer;



/* Ring buffer handle */
/* Manages a input and output data in a ring buffer */
typedef struct  avs_audio_buffer
{
  uint8_t       flags;
  int16_t      *pBuffer;              /* Ring  buffer */
  uint32_t      szBuffer;             /* Nb samples in the ring buffer */
  uint32_t      nbChannel;            /* Size of each element 1 2 4 etc... */
  int32_t       szConsumer;           /* Nb element in available in the ring buffer */
  uint32_t      prodPos;              /* Write  position */
  uint32_t      prodPeak;             /* peak production  size*/
  uint32_t      consumPos;            /* Read position */
  uint32_t      sampleRate;           /* Audio stream sample rate in HZ : example 160000 */
  uint32_t      cumul;
  int32_t       cptCumul;
  int32_t       lastSample;
  avs_up_sampling_t  up_smp;
  avs_dn_sampling_t  dn_smp;
  uint32_t      volume;               /* Processing factor volume from 0 to 100 ( when applicable) */
} Avs_audio_buffer;

#if defined(AVS_USE_DEBUG)
#define AVS_RING_CHECK_PEAK
#endif
#ifdef AVS_RING_CHECK_PEAK

#define AVS_RING_CHECK_PROD_PEAK(ring) \
    if((ring)->szConsumer > (ring)->prodPeak)     \
    {\
      (ring)->prodPeak  = (ring)->szConsumer;\
    }
#else
#define AVS_RING_CHECK_PROD_PEAK(...) ((void)0)
#endif





/*

allows to play a basic sound


*/

typedef struct avs_sound_player
{
  int32_t      count;                  /* Nb sample to play */
  int32_t      curCount;               /* Cur nb sample to play */
  uint32_t      chWave;                 /* Wave channel numbers ( normally always mono) */
  uint32_t      freqWave;               /* Wave frequency */
  int16_t      *pWave;                 /* Pointer on the full wav file */
  int16_t       *pCurWave;              /* Pointer on the current wave payload */
  int16_t       volume;                 /* Volume to play */
  avs_up_sampling_t  up_smp;
  uint32_t      flags;                  /* Flags */
} Avs_sound_player;



/*
Stuct used to exchange audio streams
 audio streams have a 16 bits sample 1 or 2 channels
 if the stream have different frequencies or channels, the object convert
 convert it AUDIO IN to AUDIO OUT stream format


*/

typedef struct avs_audio_pipe
{
  uint32_t         pipeFlags;       /* Some flags to signal particular state, @see Avs_pipe_flags */
  uint32_t         threshold;       /* Threshold data incoming before to inject */
  avs_mutex        lock;            /* Mutual exclusion */
  avs_event        inEvent;         /* Signal event on the in stream */
  Avs_audio_buffer inBuffer;        /* Pipe in */
  avs_event        outEvent;        /* Signal event on the out stream */
  Avs_audio_buffer outBuffer;       /* Pipe out */
} Avs_audio_pipe;



typedef struct avs_token_management_context
{
  char_t grantCode[AVS_GRANT_CODE_MAX_SIZE];
  char_t * pRefreshToken;
  char_t * pBearer;

} avs_tokenContext;


typedef enum avs_down_stream_channel_state
{
  DOWNSTREAM_NONE = 0,         /* Initial state (not created) */
  DOWNSTREAM_INITIALIZING,     /* Currently initializing */
  DOWNSTREAM_READY,            /* Initialized and working */
  DOWNSTREAM_DEAD_OR_DYING     /* The channel is closed, and the thread is dying or dead */
} Avs_down_stream_channel_state;




/* Manage  a ring buffer of n elements */
void            avs_audio_memory_pool_set(uint32_t  hPool);
uint32_t        avs_audio_buffer_produce(Avs_audio_buffer *pHandle, uint32_t nbSample, int16_t *pBuffer);
uint32_t        avs_audio_buffer_consume(Avs_audio_buffer *pHandle, uint32_t nbSample, int16_t *pBuffer);
AVS_Result      avs_audio_buffer_create(Avs_audio_buffer *pHandle, int16_t *pBuffer, uint32_t nbSample, uint32_t sizeElement, uint32_t  sampleRate);
AVS_Result      avs_audio_buffer_delete(Avs_audio_buffer *pHandle);
void            avs_audio_buffer_quick_set(Avs_audio_buffer *pHandle, Avs_consumer type);
void            avs_audio_buffer_reset(Avs_audio_buffer *pHandle);
void            avs_audio_feed_wave(int16_t*pBuffer, uint32_t nbSamples, uint32_t  ch, uint32_t freqBuffer, uint32_t tone, int32_t vol);
AVS_Result  avs_audio_buffer_reset_buffer(Avs_audio_buffer *pHandle, int16_t *pBuffer);
__ALWAYS_INLINE int16_t *avs_audio_buffer_get_consumer(Avs_audio_buffer *pHandle);
__ALWAYS_INLINE int16_t *avs_audio_buffer_get_consumer(Avs_audio_buffer *pHandle)
{
  return &pHandle->pBuffer[((pHandle->consumPos ) % pHandle->szBuffer) * pHandle->nbChannel];
}
__ALWAYS_INLINE int16_t *avs_audio_buffer_get_producer(Avs_audio_buffer *pHandle);
__ALWAYS_INLINE int16_t *avs_audio_buffer_get_producer(Avs_audio_buffer *pHandle)
{
  return &pHandle->pBuffer[((pHandle->prodPos ) % pHandle->szBuffer) * pHandle->nbChannel];
}
__ALWAYS_INLINE void     avs_audio_buffer_move_producer(Avs_audio_buffer *pHandle, uint32_t offset);
__ALWAYS_INLINE void     avs_audio_buffer_move_producer(Avs_audio_buffer *pHandle, uint32_t offset)
{
  pHandle->prodPos = (pHandle->prodPos + offset) % pHandle->szBuffer;
  pHandle->szConsumer += (int32_t)offset;
  AVS_RING_CHECK_PROD_PEAK(pHandle);
}
__ALWAYS_INLINE void     avs_audio_buffer_move_consumer(Avs_audio_buffer *pHandle, uint32_t offset);
__ALWAYS_INLINE void     avs_audio_buffer_move_consumer(Avs_audio_buffer *pHandle, uint32_t offset)
{
  pHandle->consumPos = (pHandle->consumPos + offset) % pHandle->szBuffer;
  pHandle->szConsumer -= (int32_t)offset;
  AVS_ASSERT(pHandle->szConsumer >= 0);
}
__ALWAYS_INLINE uint32_t avs_audio_buffer_get_producer_size_available(Avs_audio_buffer *pHandle);
__ALWAYS_INLINE uint32_t avs_audio_buffer_get_producer_size_available(Avs_audio_buffer *pHandle)
{
  return pHandle->szBuffer - (uint32_t)pHandle->szConsumer;
}
AVS_Result      avs_audio_pipe_create(Avs_audio_pipe *pHandle);
AVS_Result  avs_audio_pipe_delete(Avs_audio_pipe *pHandle);
void            avs_audio_pipe_update_resampler(Avs_audio_pipe *pPipe);
__ALWAYS_INLINE void     avs_audio_buffer_move_ptr_consumer(Avs_audio_buffer *pHandle, uint32_t offset, int16_t **ppBuffer);
__ALWAYS_INLINE void     avs_audio_buffer_move_ptr_consumer(Avs_audio_buffer *pHandle, uint32_t offset, int16_t **ppBuffer)
{
  pHandle->szConsumer -= (int32_t)offset;
  AVS_ASSERT(pHandle->szConsumer >= 0);
  pHandle->consumPos += offset;
  *ppBuffer += (pHandle->nbChannel * offset);
  if (pHandle->consumPos >= pHandle->szBuffer)
  {
    pHandle->consumPos = 0;
    *ppBuffer = pHandle->pBuffer;
  }
}
__ALWAYS_INLINE void     avs_audio_buffer_move_ptr_producer(Avs_audio_buffer *pHandle, uint32_t offset, int16_t **ppBuffer);
__ALWAYS_INLINE void     avs_audio_buffer_move_ptr_producer(Avs_audio_buffer *pHandle, uint32_t offset, int16_t **ppBuffer)
{
  pHandle->szConsumer += (int32_t)offset;
  pHandle->prodPos += offset;
  *ppBuffer += (pHandle->nbChannel * offset);
  if (pHandle->prodPos >= pHandle->szBuffer)
  {
    pHandle->prodPos = 0;
    *ppBuffer = pHandle->pBuffer;
  }
  AVS_RING_CHECK_PROD_PEAK(pHandle);

}

/*


 Create a ring buffer byte 

*/


typedef struct  avs_bytes_buffer
{
  uint8_t      *pBuffer;              /* Ring  buffer */
  uint32_t      szBuffer;              /* Size buffer */
  int32_t       szConsumer;           /* Nb element in available in the ring buffer */
  uint32_t      prodPos;              /* Write  position */
  uint32_t      prodPeak;             /* peak production  size*/
  uint32_t      consumPos;            /* Read position */
  avs_mutex     lock;                 /* Lock mutex */
} Avs_bytes_buffer;


AVS_Result avs_bytes_buffer_Create(Avs_bytes_buffer *pHandle, uint32_t nbbytes);
AVS_Result avs_bytes_buffer_Delete(Avs_bytes_buffer *pHandle);
uint32_t avs_bytes_buffer_consume(Avs_bytes_buffer *pHandle, uint32_t nbByte, uint8_t *pBuffer);
uint32_t avs_bytes_buffer_produce(Avs_bytes_buffer *pHandle, uint32_t nbByte, uint8_t *pBuffer);
__ALWAYS_INLINE uint8_t *avs_bytes_buffer_GetConsumer(Avs_bytes_buffer *pHandle);
__ALWAYS_INLINE uint8_t *avs_bytes_buffer_GetConsumer(Avs_bytes_buffer *pHandle)
{
  return &pHandle->pBuffer[pHandle->consumPos  % pHandle->szBuffer];
}
__ALWAYS_INLINE uint8_t *avs_bytes_buffer_get_consumer_off(Avs_bytes_buffer *pHandle,int32_t offset);
__ALWAYS_INLINE uint8_t *avs_bytes_buffer_get_consumer_off(Avs_bytes_buffer *pHandle,int32_t offset)
{
  return &pHandle->pBuffer[(pHandle->consumPos+offset)  % pHandle->szBuffer];
}


__ALWAYS_INLINE uint32_t avs_bytes_buffer_size_consumer_aligned(Avs_bytes_buffer *pHandle);
__ALWAYS_INLINE uint32_t avs_bytes_buffer_size_consumer_aligned(Avs_bytes_buffer *pHandle)
{
  if(pHandle->prodPos >= pHandle->consumPos)
  {
    return pHandle->prodPos -pHandle->consumPos ;
  }
  return pHandle->szBuffer - pHandle->consumPos;
}

__ALWAYS_INLINE uint8_t avs_bytes_buffer_consumer_get(Avs_bytes_buffer *pHandle, uint32_t off);
__ALWAYS_INLINE uint8_t avs_bytes_buffer_consumer_get(Avs_bytes_buffer *pHandle, uint32_t off)
{
  AVS_ASSERT(pHandle->szConsumer);
  return pHandle->pBuffer[(pHandle->consumPos + off)% pHandle->szBuffer];
};





__ALWAYS_INLINE uint8_t *avs_bytes_buffer_GetProducer(Avs_bytes_buffer *pHandle);
__ALWAYS_INLINE uint8_t *avs_bytes_buffer_GetProducer(Avs_bytes_buffer *pHandle)
{
  return &pHandle->pBuffer[pHandle->prodPos  % pHandle->szBuffer];
}

__ALWAYS_INLINE uint8_t *avs_bytes_buffer_get_producer_off(Avs_bytes_buffer *pHandle,int32_t off);
__ALWAYS_INLINE uint8_t *avs_bytes_buffer_get_producer_off(Avs_bytes_buffer *pHandle,int32_t off)
{
  return &pHandle->pBuffer[(pHandle->prodPos+off)  % pHandle->szBuffer];
}


__ALWAYS_INLINE uint32_t avs_bytes_buffer_GetProducerSizeAvailable(Avs_bytes_buffer *pHandle);
__ALWAYS_INLINE uint32_t avs_bytes_buffer_GetProducerSizeAvailable(Avs_bytes_buffer *pHandle)
{
  return pHandle->szBuffer - pHandle->szConsumer;
}
__ALWAYS_INLINE void     avs_bytes_buffer_MovePtrConsumer(Avs_bytes_buffer *pHandle, uint32_t offset, uint8_t **ppBuffer);
__ALWAYS_INLINE void     avs_bytes_buffer_MovePtrConsumer(Avs_bytes_buffer *pHandle, uint32_t offset, uint8_t **ppBuffer)
{
  pHandle->szConsumer -= offset;
  AVS_ASSERT(pHandle->szConsumer >= 0);
  pHandle->consumPos += offset;
  *ppBuffer += offset;
  if (pHandle->consumPos >= pHandle->szBuffer)
  {
    pHandle->consumPos = 0;
    *ppBuffer = pHandle->pBuffer;
  }
}

__ALWAYS_INLINE void     avs_bytes_buffer_MovePtrProducer(Avs_bytes_buffer *pHandle, uint32_t offset, uint8_t **ppBuffer);
__ALWAYS_INLINE void     avs_bytes_buffer_MovePtrProducer(Avs_bytes_buffer *pHandle, uint32_t offset, uint8_t **ppBuffer)
{
  pHandle->szConsumer += offset;
  pHandle->prodPos += offset;
  *ppBuffer += offset;
  if (pHandle->prodPos >= pHandle->szBuffer)
  {
    pHandle->prodPos = 0;
    *ppBuffer = pHandle->pBuffer;
  }
  AVS_RING_CHECK_PROD_PEAK(pHandle);
}

__ALWAYS_INLINE void     avs_bytes_buffer_move_producer(Avs_bytes_buffer *pHandle, uint32_t offset);
__ALWAYS_INLINE void     avs_bytes_buffer_move_producer(Avs_bytes_buffer *pHandle, uint32_t offset)
{
  pHandle->prodPos = (pHandle->prodPos + offset) % pHandle->szBuffer;
  pHandle->szConsumer += (int32_t)offset;
  AVS_RING_CHECK_PROD_PEAK(pHandle);

};



__ALWAYS_INLINE void     avs_bytes_buffer_move_consumer(Avs_bytes_buffer *pHandle, uint32_t offset);
__ALWAYS_INLINE void     avs_bytes_buffer_move_consumer(Avs_bytes_buffer *pHandle, uint32_t offset)
{
  pHandle->consumPos = (pHandle->consumPos + offset) % pHandle->szBuffer;
  pHandle->szConsumer -= (int32_t)offset;
  AVS_ASSERT(pHandle->szConsumer >= 0);
}
__ALWAYS_INLINE void avs_byte_buffer_reset(Avs_bytes_buffer *pHandle);
__ALWAYS_INLINE void avs_byte_buffer_reset(Avs_bytes_buffer *pHandle)
{
  pHandle->prodPos = 0;
  pHandle->consumPos = 0;
  pHandle->szConsumer = 0;
}



__ALWAYS_INLINE void     avs_bytes_buffer_produce_add(Avs_bytes_buffer *pHandle, uint8_t data);
__ALWAYS_INLINE void     avs_bytes_buffer_produce_add(Avs_bytes_buffer *pHandle, uint8_t data)
{
  AVS_ASSERT(pHandle->szBuffer-pHandle->szConsumer != 0U);
  pHandle->pBuffer[pHandle->prodPos] = data;
  pHandle->prodPos = (pHandle->prodPos + 1) % pHandle->szBuffer;
  pHandle->szConsumer ++;
  AVS_RING_CHECK_PROD_PEAK(pHandle);

};


typedef void * avs_http_client_handle;  /* Opaque handle  to avoid include heavy header */
typedef void * avs_http_stream_handle;  /* Opaque handle  to avoid include heavy header */
typedef enum avs_audio_directive
{
  AVS_AUDIO_DIR_NONE = 0,
  AVS_AUDIO_DIR_SPEAK,
  AVS_AUDIO_DIR_EXPECT_SPEECH,
  AVS_AUDIO_DIR_EXPECT_SPEECH_TIMEOUT,
  AVS_AUDIO_DIR_RECOGNIZE,
} Avs_audio_directive;




typedef struct avs_instance_handle
{
  uint32_t              signature;              /* For debug identify instance */
  struct avs_audio_handle *pAudio;              /* Reciprocal handle to the audio attached to the instance */
  AVS_Instance_Factory *pFactory;               /* The factory */
  avs_tokenContext      tokenContext;           /* Token context */
  avs_network_porting   network;                /* Network instance */
  avs_mutex             lockMessage;            /* Mutual exclusion  for message dispatch */
  avs_mutex             lockTrace;              /* Mutual exclusion for traces */
  int64_t               syncTime;               /* Sync time in ms */
  int64_t               tickBase;               /* Tick ref for the sync time */
  char_t                stringIpAdress[16];     /* String ip */
  char_t                stringIpGateway[16];    /* String ip */
  char_t                stringSubMask[16];      /* String ip */
  char_t                stringDns[2][16];       /* String ip */
  uint8_t               hasIP;                  /* True if the system as aan IP */
  uint32_t              pingTime;                /* Ref time ping */
  uint32_t              bConnected;              /* True if connected */

  /* State machine */
  uint32_t              runStateTaskFlag;         /* Keep alive flag for  avs_state_task */
  avs_task *            hStateTask;              /* Handle task state machine */
  uint32_t              curState;               /* Current state machine */
  uint32_t              modeState;               /* Task mode according to the state */
  uint32_t              modeTerminator;          /* Task mode terminator according to the directive */

  int16_t               *pBufferStreamBuff;     /* Small tmp buffer  to jont stream devices o=in / out ( assumed not used in same time when push/pull) */
  uint32_t               blkSizeStreamBuff;     /* Size in sample allocated in pBufferStreamVoice */
  uint32_t               runDnChannelFlag;      /* Keep alive flag avs_directive_downstream_task */
  avs_queue              hPostMsg;              /* Handle of the message queue */


  /* Http  stream */
  uint32_t               runCnxTaskFlag;         /* Keep alive flag for avs_http2_connnection_task */

  avs_mutex              hHttpLock;            /* Lock http2 entries */
  avs_task              *hHttp2Connnection;     /* Http2 task handle */
  avs_task              *hDownstreamChannel;    /* Directive  task handle */
  avs_http_client_handle hHttpClient;           /* Http2 handle instance */
  avs_http_stream_handle hHttpDlgStream;        /* Http2 audio stream */
  avs_http_stream_handle hDirectiveStream;      /* Directive stream */
  avs_mutex              hDirectiveLock;        /* Lock Directive entries */

  uint32_t               cptRecBlk;            /* Cpt block record */
  uint32_t               cptPlayBlk;           /* Cpt block play */
  uint32_t               messageIdCounter;     /* Message id ( inc for each message) */
  uint32_t               dialogIdCounter;      /* Dialogue id ( inc for each request) */
  uint32_t               dirDownstreamState;
  /* Dialogue */
  char_t                *pLastToken;            /* Last token provided by the directive playback */
  uint32_t               postSynchro;            /* True if a synchro state will be send in the next idle time */
  avs_mutex              lockParseJson;          /* Locks the directive to prevent issues of directive coming from the dn channel and the audio channel */
  uint32_t               codeAudioDir;            /* Audio directive see Avs_audio_directive */
  uint32_t               speakingStart;           /* Start time in MS when alexa speak */
  uint32_t               isSpeaking;              /* True if alexa speaks */

  void                   *pMemoryPool;            /* Memory allocator containing short persistence object */
  avs_mutex              entryLock;               /* Mutex locking the sdk entry. An entry fn is never re-entrant, except, if the entry is atomic, or at the instance creation */
  uint32_t               bInstanceStarted;        /* True if the all task are started */

  /* Token */
  uint32_t                runTokenFlag;          /* Keep alive flag for token task */
  avs_event               hRenewToken;           /* Event to force the token renew */
  avs_task                *hTokenTask;           /* Token task handle */
  avs_mutex               hTokenLock;            /* Token lock */
  uint32_t                tokenRenewTimeStamp;   /* Time stamp the last token renew */
} AVS_instance_handle;


/* Normaly, all gloabal data used for the audio */

typedef struct avs_audio_handle
{
  AVS_Instance_Factory  *pFactory;               /* The factory */
  AVS_instance_handle *pInstance;               /* Main instance AVS attached to this audio */
  avs_task           *taskInjectionIn;          /* Task audio pipe microphone */
  avs_task           *taskInjectionOut;         /* Task audio pipe speaker */
  Avs_audio_pipe     recognizerPipe;            /* Audio pipe  recognizerPipe */
  Avs_audio_pipe     synthesizerPipe;           /* Audio pipe synthesizer */
  Avs_audio_pipe     auxAudioPipe;                /* Audio pipe for external output */
  Avs_sound_player   soundPlayer;                 /* Audio sound player */
  avs_event          taskCreatedEvent;          /* Signal the task is created and active */
  uint32_t           runOutRuning;              /* True if the audio out task in running */
  uint32_t           runInRuning;               /* True if the audio in task in running */
  void               *pMP3Context;              /* Ptr mp3 decoder context */
  uint32_t           volumeMuted;              /* TRUE if muted */
  int32_t            volumeOut;                /* Volume in percent */
  Avs_bytes_buffer   ringMp3Speaker;           /* MP3 Speaker channel */

} AVS_audio_handle;



/*
Abstraction driver to port all system aspect
 network,init, traces ,ect....
 must be initialized to support a new platform

*/

typedef struct avs_porting_sys
{
  AVS_Result(*platform_Sys_init)(void);
  AVS_Result(*platform_Sys_term)(void);
  uint32_t  (*platform_Sys_ioctl)(AVS_instance_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);
  void      (*platform_Sys_puts)(const char *pString, uint32_t flags);
  uint32_t  (*platform_Sys_rnd)(void);
  uint32_t  (*platform_Sys_Default)(void);
  AVS_Result(*platform_Network_init)(AVS_instance_handle *pHandle);
  AVS_Result(*platform_Network_term)(AVS_instance_handle *pHandle);
  uint32_t (*platform_Network_ioctl)(AVS_instance_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);
  AVS_Result (*platform_Network_Solve_macAddr)(AVS_instance_handle *pHandles);
} AVS_porting_sys;



/*
Abstraction driver to port all audio aspect
 ASR, voice, speaker, default factory
 must be initialized to support a new platform

*/


typedef struct avs_porting_audio
{
  AVS_Result (*platform_Audio_init)(AVS_audio_handle *pHandle);
  AVS_Result (*platform_Audio_term)(AVS_audio_handle *pHandle);
  AVS_Result (*platform_Audio_default)(AVS_audio_handle *pHandle);
  uint32_t   (*platform_Audio_ioctl)(AVS_audio_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);

  AVS_Result (*platform_MP3_decoder_init)(AVS_audio_handle *pHandle);
  AVS_Result (*platform_MP3_decoder_term)(AVS_audio_handle *pHandle);
  uint32_t   (*platform_MP3_decoder_ioctl)(AVS_audio_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);
  AVS_Result (*platform_MP3_decoder_inject)(AVS_audio_handle *pAHandle, uint8_t *pMp3Payload, uint32_t MP3PayloadSize);
} AVS_porting_audio;

/*
Abstraction driver to port all instance aspect
 default factory
 must be initialized to support a new platform

*/


typedef struct avs_porting_instance
{
  AVS_Result(*platform_init)(AVS_instance_handle *pHandle);
  AVS_Result(*platform_term)(AVS_instance_handle *pHandle);
  uint32_t (*platform_ioctl)(uint32_t code, uint32_t wparam, uint32_t lparam);
} AVS_porting_instance;


typedef enum  t_avs_mem_type
{
  avs_mem_type_dtcm,            /* Very fast memory */
  avs_mem_type_pram,            /* Standard internal memory */
  avs_mem_type_ncached,         /* Uncached memory  */
  avs_mem_type_heap,            /* Normal heap (sdram) */
  avs_mem_type_best             /* Try alloc dtc -> pram->heap */

} avs_mem_type_t;

typedef struct t_avs_mem_pool
{
  void          *m_pBaseMalloc;
  uint32_t      m_iBaseSize;
  int32_t       m_globalAlloc;
  uint32_t      m_nbFrags;
  uint32_t      m_checkCount;
  uint32_t      m_checkFreq;
  uint32_t      m_flags;
} avs_mem_pool;




/* Singleton and initiator function porting layer */
extern          AVS_porting_audio    drv_audio;
extern          AVS_porting_instance drv_instance;
extern          AVS_porting_sys      drv_sys;
AVS_Result      drv_platform_Init(void);
void avs_init_default_interger(uint32_t *pInterger,uint32_t iDefault);
void avs_init_default_string(char_t **pString,char_t *pDefault);
AVS_Result  avs_sys_get_rng(uint32_t *randomValue);



/* Internal functions not exposed to the SDK*/
/* name space always lower case and spliced with '_' */

typedef struct t_avs_post
{
  AVS_Event evt;
  uint32_t  pparam;

}avs_post_t;
#define AVS_MAX_POST_ELEMENT    30

typedef              void    avs_short_alloc_object;
int8_t avs_core_event_create_named(avs_event *event, const char *pName);
void   avs_core_event_delete(avs_event *event);
void   avs_core_event_set(avs_event *event);
int8_t avs_core_event_set_isr(avs_event *event);
AVS_Result avs_core_event_wait(avs_event *event, uint32_t timeout);
void   avs_core_mutex_unlock(avs_mutex *mutex);
void   avs_core_mutex_lock(avs_mutex *mutex);
int8_t avs_core_mutex_create_named(avs_mutex *mutex, const char_t *pName);
void   avs_core_mutex_delete(avs_mutex *mutex);
void   avs_core_recursive_mutex_unlock(avs_mutex *mutex);
void   avs_core_recursive_mutex_lock(avs_mutex *mutex);
int8_t avs_core_recursive_mutex_create_named(avs_mutex *mutex, const char *pName);
void   avs_core_recursive_mutex_delete(avs_mutex *mutex);
avs_task *avs_core_task_create(const char_t *pName, avs_task_cb  pCb, void *pCookie, size_t stackSize, osPriority priority);
void   avs_core_task_delete(avs_task *task);
void   avs_core_task_delay(uint32_t ms);
uint32_t avs_core_sys_tick(void);

void    avs_core_task_end(void);
int8_t avs_core_queue_create_named(avs_queue *pQueue,uint32_t nbElement,uint32_t itemSize, const char_t *pName);
uint32_t avs_core_queue_delete(avs_queue *pQueue);
AVS_Result avs_core_queue_put(avs_queue *pQueue,void *pElemen,uint32_t timeout);
AVS_Result avs_core_queue_get(avs_queue *pQueue,void *pElemen,uint32_t timeout);


#if defined(AVS_USE_DEBUG) && configQUEUE_REGISTRY_SIZE > 0
#define avs_core_recursive_mutex_create(mutex) avs_core_recursive_mutex_create_named((mutex),(#mutex))
#define avs_core_mutex_create(mutex)           avs_core_mutex_create_named((mutex),(#mutex))
#define avs_core_event_create(evt)             avs_core_event_create_named((evt),(#evt))
#else
#define avs_core_recursive_mutex_create(mutex) avs_core_recursive_mutex_create_named((mutex),NULL)
#define avs_core_mutex_create(mutex)           avs_core_mutex_create_named((mutex),NULL)
#define avs_core_event_create(evt)             avs_core_event_create_named((evt),NULL)
#endif


void    avs_core_mem_init(AVS_instance_handle *pHandle);
void    avs_core_mem_term(void);
avs_mem_type_t avs_core_mem_pool_type(void *pMem);
void    avs_core_mem_free(void *pMemToFree);
void   *avs_core_mem_alloc(avs_mem_type_t type, size_t size);
void   *avs_core_mem_calloc(avs_mem_type_t type, size_t size, size_t elem);
void   *avs_core_mem_realloc(avs_mem_type_t type, void *pBuffer, size_t size);

void   *avs_core_mem_check_corruption(void *pBase);
void    avs_complier_get_heap_info(uint32_t *memTotalSpace,uint32_t *memFreeSpace);
void    avs_core_terminat_isr(uint32_t flag);
void    avs_core_lock_tasks(void);
void    avs_core_unlock_tasks(void);
void    avs_core_trace_json(const char* pJson);
AVS_Result avs_core_sys_info_get(AVS_instance_handle *pHandle, AVS_Sys_info *pSysInfo );
AVS_Result avs_core_audio_wav_sound(void *pWave, Avs_sound_player *pSound);
AVS_Result avs_core_get_key_string(AVS_instance_handle * pHandle, char *pString, char *pKey, char *pValue, uint32_t iMaxSizeValue);
void       avs_core_change_mp3_frequency(AVS_instance_handle *pHandle, uint32_t newFreq);


AVS_Result               avs_core_short_object_init(AVS_instance_handle *pHandle);
void                     avs_core_short_object_term(AVS_instance_handle *pHandle);

void                     avs_core_short_free(avs_short_alloc_object  *ptr);
avs_short_alloc_object  *avs_core_short_alloc(uint32_t maxSize);
avs_short_alloc_object  *avs_core_short_realloc(avs_short_alloc_object  *ptr, uint32_t maxSize);


AVS_instance_handle *avs_core_get_instance(void);
void     avs_core_dump_console(void *pDump, uint32_t sizeinByte);
void     avs_core_dump_console_audio_consumer(Avs_audio_buffer *pBuffer, uint32_t nbsample);
void     avs_core_set_instance(AVS_instance_handle *pHandle);
uint32_t avs_audio_inject_stream_buffer(AVS_audio_handle *pAHandle, void *pSrc, int32_t size );
void     avs_audio_inject_stream_set_flags(AVS_audio_handle *pAHandle, uint32_t clearlags, uint32_t addFlags );
uint32_t avs_avs_capture_audio_stream_buffer(AVS_audio_handle *pAHandle, void *pDst, int32_t size);
void     avs_audio_capture_reset(AVS_audio_handle *pAHandle);
void     avs_audio_capture_mute(AVS_audio_handle *pAHandle, uint32_t state);

void     avs_audio_inject_audio_stream_reset(AVS_audio_handle *pAHandle);
void     avs_audio_capture_wait_all_consumed(AVS_audio_handle *pAHandle);
void     avs_audio_inject_wait_all_consumed(AVS_audio_handle *pAHandle);
void       avs_default_instance_solver(AVS_Instance_Factory *pFactory);
AVS_Result avs_network_config(AVS_instance_handle *pHandle);
AVS_Result avs_audio_create(AVS_audio_handle *pHandle);
AVS_Result avs_audio_delete(AVS_audio_handle *pHandle);
AVS_Result avs_audio_inject_speaker_wave(AVS_audio_handle *pAHandle, void *pWave);
void       avs_default_audio_solver(AVS_Instance_Factory *pFactory);
void       avs_core_set_last_error(AVS_Result err);
void       avs_core_trace_args(uint32_t flag, const char* lpszFormat, va_list args);
AVS_Result avs_audio_play_sound(AVS_audio_handle *pAHandle, uint32_t flags, void *pWave, int32_t volumePercent);
AVS_Result avs_audio_inject_stream_buffer_mp3(AVS_audio_handle *pHandle, uint8_t* pBuffer, uint32_t size);
AVS_Result avs_core_message_send(AVS_instance_handle *pHandle, AVS_Event evt, uint32_t  pparam1);
AVS_Result avs_core_message_post(AVS_instance_handle *pHandle, AVS_Event evt, uint32_t  pparam1);
AVS_Result avs_core_post_messages(AVS_instance_handle *pHandle);


AVS_Result avs_createPorting(void);
AVS_Result avs_deletePorting(void);
void       avs_core_dump_instance_factory(AVS_instance_handle* handle, char *pBanner);
void       avs_core_dump_audio_factory(AVS_audio_handle* handle, char *pBanner);
AVS_Result avs_network_synchronize_time(AVS_instance_handle *pInstance);
AVS_Result  avs_network_get_time(AVS_instance_handle *pHandle, time_t *pEpoch);
AVS_Result avs_network_check_ip_available(AVS_instance_handle *pHandle);
AVS_Result avs_set_state(AVS_instance_handle *pHandle, AVS_Instance_State state);
AVS_Result avs_state_create(AVS_instance_handle *pHandle);
AVS_Result avs_state_delete(AVS_instance_handle *pHandle);
AVS_Result avs_http2_send_ping(AVS_instance_handle *pHandle);
AVS_Result avs_http2_post_ping(AVS_instance_handle *pHandle);

Http2ClientStream *avs_http2_stream_open(AVS_instance_handle *pHandle);
AVS_Result avs_http2_stream_add_body(AVS_instance_handle *pHandle, Http2ClientStream *stream, const char *pJson);
AVS_Result avs_http2_stream_read(AVS_instance_handle *pHandle, Http2ClientStream *stream, void *pBuffer, uint32_t szInSByte, uint32_t *retSize);
AVS_Result avs_http2_stream_write(AVS_instance_handle *pHandle, Http2ClientStream *stream, const void *pBuffer, size_t lengthInBytes);
AVS_Result avs_http2_stream_stop(AVS_instance_handle *pHandle, Http2ClientStream *stream);
AVS_Result avs_http2_stream_close(AVS_instance_handle *pHandle, Http2ClientStream *stream);
const char_t  *avs_http2_stream_get_response_type(AVS_instance_handle *pHandle, Http2ClientStream  *stream);
AVS_Result avs_http2_stream_process_json(AVS_instance_handle *pHandle, avs_http_stream_handle stream);

int8_t     avs_http2_stream_is_opened( AVS_instance_handle *pHandle, Http2ClientStream *stream);
AVS_Result avs_http2_process_json_stream(AVS_instance_handle *pHandle);
AVS_Result avs_http2_read_stream(AVS_instance_handle *pHandle, avs_http_stream_handle stream, uint32_t maxsize, avs_short_alloc_object **pString, uint32_t *pRecevied);
AVS_Result avs_directive_post_synchro(AVS_instance_handle *pHandle);

const char_t *avs_core_get_level_string(uint32_t level);

const char_t *avs_json_formater_recognizer_event(AVS_instance_handle *pHandle, const char_t *pInitiator );
const char_t * avs_json_formater_speaker_event(AVS_instance_handle *pHandle, const char_t *pName);
const char_t* avs_json_formater_notification_event(AVS_instance_handle *pHandle, const char *pNameSpace, const char *pName, const char_t *pToken, const char_t*pDialogId);
const char_t *avs_json_formater_synchro_state_event(AVS_instance_handle *pHandle);
uint32_t    avs_json_formater_add_context(AVS_instance_handle *pHandle, void *root);
AVS_Result avs_json_formater_init(AVS_instance_handle *pHandle);
AVS_Result avs_json_formater_term(AVS_instance_handle *pHandle);
void       avs_json_formater_free(const char_t *pJson);
uint32_t     avs_json_formater_speaker_contents(AVS_instance_handle *pHandle, void *base, const char_t *pName);


AVS_Result avs_directive_process_json(AVS_instance_handle *pHandle, const char *pJson);

int32_t        avs_mem_init(avs_mem_pool *pHandle, void *pBlock, uint32_t  size);
void     avs_mem_term(avs_mem_pool *pHandle);
void *     avs_mem_alloc(avs_mem_pool *pHandle, uint32_t size);
void *     avs_mem_calloc(avs_mem_pool *pHandle, uint32_t size, uint32_t elem);
void *     avs_mem_realloc(avs_mem_pool *pHandle, void *pBlock, uint32_t sizeMalloc);
void     avs_mem_free(avs_mem_pool *pHandle, void *pBlk);
void       avs_mem_check_curruption(avs_mem_pool *pHandle);
uint32_t   avs_mem_check_ptr(avs_mem_pool *pHandle, void *ptr);
uint32_t  avs_mem_check_curruption_blk(avs_mem_pool *pHandle, void *pBlk, int32_t size);
void      avs_mem_leak_detector(avs_mem_pool *pHandle);

/* ARM ARM says that Load and Store instructions are atomic and it's execution is guaranteed to be complete before interrupt handler executes. */
/* Verified by looking at arch/arm/include/asm/atomic.h */
__ALWAYS_INLINE uint32_t avs_core_atomic_read(uint32_t *p32);
__ALWAYS_INLINE uint32_t avs_core_atomic_read(uint32_t *p32)
{
  return ((*(volatile uint32_t *)p32));
}
__ALWAYS_INLINE void avs_core_atomic_write(uint32_t *p32, uint32_t val );
__ALWAYS_INLINE void avs_core_atomic_write(uint32_t *p32, uint32_t val )
{
  (*(volatile uint32_t *)p32) = val;
}
__ALWAYS_INLINE  void avs_atomic_write_charptr(char_t **pChar, char_t  *pVal);
__ALWAYS_INLINE  void avs_atomic_write_charptr(char_t **pChar, char_t  *pVal)
{
/* ARM ARM says that Load and Store instructions are atomic and it's execution is guaranteed to be complete before interrupt handler executes. */
/* Verified by looking at arch/arm/include/asm/atomic.h */
  *pChar = pVal;
}

__ALWAYS_INLINE  char_t  *avs_atomic_read_charptr(char_t **pChar);
__ALWAYS_INLINE  char_t  *avs_atomic_read_charptr(char_t **pChar)
{
/* ARM ARM says that Load and Store instructions are atomic and it's execution is guaranteed to be complete before interrupt handler executes. */
/* Verified by looking at arch/arm/include/asm/atomic.h */
  return *pChar;
}



AVS_Result avs_http2_connnection_manager_create(AVS_instance_handle *pHandle);
AVS_Result avs_http2_connnection_manager_delete(AVS_instance_handle *pHandle);
const int8_t *avs_http2_get_avs_base_url(void);
AVS_Result avs_http2_send_json(AVS_instance_handle *pHandle, const char * pJson );
AVS_Result avs_http2_read_multipart_stream(AVS_instance_handle *pHandle, Http2ClientStream *stream, uint32_t maxsize, avs_short_alloc_object **pString, uint32_t *pRecevied);



AVS_Result avs_directive_down_stream_channel_create( AVS_instance_handle *pHandle);
AVS_Result avs_directive_down_stream_channel_delete(AVS_instance_handle *pHandle);
AVS_Result avs_directive_send_synchro(AVS_instance_handle *pHandle);


uint32_t   avs_directive_downstream_channel_state_get(AVS_instance_handle *pHandle);
AVS_Result avs_token_refresh_create(AVS_instance_handle *pHandle);
AVS_Result avs_token_refresh_delete(AVS_instance_handle *pHandle);
const char_t *avs_token_wait_for_access(void);
const char_t *avs_token_access_lock_and_get(AVS_instance_handle *pHandle);
void        avs_token_access_unlock(AVS_instance_handle *pHandle);


AVS_Result avs_token_authentication_grant_code_set(AVS_instance_handle *pHandle, const char_t *grantCode);
AVS_Result avs_http2_notification_event(AVS_instance_handle *pHandle, const char *pNameSpace, const char *pName, const char *pToken, const char *pDialogID);

/* Porting Tmp placement , should ne in avs_network_private.h */

AVS_HTls          avs_porting_tls_open(AVS_instance_handle * pHandle, const char_t   *pHost, uint32_t port);
AVS_Result        avs_porting_tls_send_header(AVS_instance_handle *pHandle, AVS_HTls hstream, const char_t   * pHeader);
AVS_Result        avs_porting_tls_read_response(AVS_instance_handle*pHandle, AVS_HTls hStream, char_t    *pHeader, uint32_t maxSize, uint32_t *pRetSize);
AVS_Result        avs_porting_tls_close(AVS_instance_handle * pHandle, AVS_HTls hstream);
AVS_Result        avs_porting_tls_read(AVS_instance_handle * pHandle, AVS_HTls  hStream, void *pBuffer, uint32_t szInSByte, uint32_t *retSize);
AVS_Result        avs_porting_tls_write(AVS_instance_handle * pHandle, AVS_HTls  hStream, const void *pBuffer, uint32_t szInSBytes, uint32_t *retSize);



#ifdef __cplusplus
};
#endif


#endif /* _avs_private_ */



