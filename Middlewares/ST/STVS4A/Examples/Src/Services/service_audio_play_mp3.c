/**
******************************************************************************
* @file    service_audio_mp3.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   manage a media player mp3 basic
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


#define CODEC_MP3_TASK_STACK_SIZE       1000
#define CODEC_MP3_TASK_PRIORITY         (osPriorityAboveNormal)
#define CODEC_MP3_TASK_NAME             "CODEC:MP3 player"
#define CODEC_MP3_SLEEP_CONSUM_TIME     1
#define EPSILON                            10

#define SIZE_BUFFER_PCM_IN_SAMPLE       512
#define NB_CHANNEL_MP3                  2
#define SLEEP_CONSUM_TIME               4


static task_t *                      hCodecTask;
static uint32_t                         bStartDecode = 0;

/* Context struct */
typedef struct t_mp3_decoder_context
{
  TSpiritMP3Decoder  instance;          /* Decoder object pointer */
  TSpiritMP3Info     info;              /* Current Stream Info */
  uint64_t           pcmCount;          /* Pcm counter */
  uint64_t           lastPcmCount;      /* Base time to compute offsets */
  task_t *       *mp3Task;           /* Task handle */
  uint32_t           mp3Runing;         /* Run  flag */
} mp3_decoder_context_t;

static mp3_decoder_context_t mp3_context;




/* This task is in charge to decompress mp3 payload and feed the PCM audio feed */
static void service_player_mp3_task(const void *pCookie);
static void service_player_mp3_task(const void *pCookie)
{
  AVS_Handle  hHandle = (AVS_Handle)(uint32_t) pCookie;

  service_player_audio_send(hHandle, EVT_PLAYER_MP3_TASK_START, 0);
  mp3_context.mp3Runing = 1;
  while(mp3_context.mp3Runing)
  {
    if(bStartDecode)
    {

      /* Tmp pcm */
      static int16_t buffer16bitsPCM[SIZE_BUFFER_PCM_IN_SAMPLE * NB_CHANNEL_MP3];

      /* Request for MP3_OUTPUT_PCM_BUFFER_SIZE MP3 decoding */
      int32_t nbSample = SpiritMP3Decode(&mp3_context.instance, buffer16bitsPCM, SIZE_BUFFER_PCM_IN_SAMPLE / 2, &mp3_context.info );
      if(nbSample)
      {

        /* The decoder has decoded nbSample PCM stereo samples */
        /* But the aux audio may be full or partially full */
        /* So we check the buffer and we inject only the part available, and wait for the consumption before to inject the remaining */
        int32_t index = 0;
        while(nbSample)
        {

          AVS_ASSERT(((nbSample >= 0) && (nbSample <= SIZE_BUFFER_PCM_IN_SAMPLE)));
          /* Check the stream properties freq and channels num */
          AVS_Aux_Info info;
          AVS_Get_Audio_Aux_Info(hInstance, &info);

          if(mp3_context.pcmCount  == 0) /* We start a new stream */
          {
            /* Change dynamicaly the freq the channel number */
            info.infoFlags |= AVS_AUX_INFO_FREQ;
            info.frequency = mp3_context.info.nSampleRateHz ;

            /* The threshold must be at last an half of the out buffer  ( the threshold is counted in samples ) */
            /* The mp3 freq and the output freq could be different, so we need to convert first the size of the out in MS and then */
            /* Recompute threshold according to the frequency and convert it in samples */

            info.infoFlags |= AVS_AUX_INFO_THRESHOLD;
            info.threshold = (int32_t)(float_t)(info.frequency * sInstanceFactory.buffSizeOut / 2) / 1000;
            info.infoFlags |= AVS_AUX_INFO_CH;
            /* even in mono, the api process a stream stereo  by mono duplication ch*/
            info.nbChannel = NB_CHANNEL_MP3 /* mp3_context.info.nChannels */ ;
            info.infoFlags |= AVS_AUX_INFO_FLAGS;
            /* Set buffering mode to prevent pop at the starting */
            info.iFlags  |= AVS_PIPE_BUFFERING;

            info.infoFlags |= AVS_AUX_INFO_NBSAMPLE;
            /* The Buffer size is 2 time the threshold and *2 because threshold is only for an half buffer */
            info.szBuffer   = 2 * info.threshold * 2;
            /* Commit the change */
            AVS_Set_Audio_Aux_Info(hInstance, &info);
            /* Print the mp3 parameters */
            char_t tmpString[100];
            snprintf(tmpString, sizeof(tmpString), "MP3: Frq %05lu HZ %lu ch", info.frequency, (long unsigned int) mp3_context.info.nChannels);
            AVS_Send_Evt(hInstance, EVT_ENDURANCE_MSG, (uint32_t)tmpString);
          }
          /* Inject samples in the ring buffer */
          /* Returns the real number of sample injected */
          uint32_t nbInjected = AVS_Feed_Audio_Aux(hInstance, &buffer16bitsPCM[index * NB_CHANNEL_MP3], nbSample);

          /* If the returned nb sample is different this means that the buffer is full, let's sleep a bit and continue */
          if(nbInjected != nbSample)
          {
            osDelay(CODEC_MP3_SLEEP_CONSUM_TIME);
          }
          gAppState.bufAuxPlayer =  (100 * info.szConsumer) / info.szBuffer;


          nbSample -= nbInjected  ;
          index    += nbInjected ;
          mp3_context.pcmCount += nbInjected;

        }
        /* We report the time elapsed in ms */
        /* The mp3 may have several frequency in a stream , in this implementation we assume that only one frequency is used to compute the time elapsed in ms */
        /* Update all 2 ms */
        if((mp3_context.pcmCount - mp3_context.lastPcmCount) > (mp3_context.info.nSampleRateHz / 500))
        {
          uint32_t offMs = mp3_context.pcmCount / (mp3_context.info.nSampleRateHz / 1000);
          mp3_context.lastPcmCount = mp3_context.pcmCount;
          service_player_audio_send(hInstance, EVT_PLAYER_REPORT,  offMs);
        }

      }
      else
      {
        osDelay(SLEEP_CONSUM_TIME);
      }

    }
    else
    {
      osDelay(5 * SLEEP_CONSUM_TIME);
    }

  }

  service_player_audio_send(hHandle, EVT_PLAYER_MP3_TASK_DYING, 0);
  mp3_context.mp3Runing  = 2;
  while(1) 
  {
    osDelay(10000);
  }
  
}


/* Spirit read  CB */
static uint32_t service_player_codec_mp3_read_data_cb(void *    pCompressedData,uint32_t  nDataSizeInChars, void *    pUserData);
static uint32_t service_player_codec_mp3_read_data_cb(void *    pCompressedData,uint32_t  nDataSizeInChars, void *    pUserData)
{

  int32_t nBytes  = 0;
  bytes_buffer_t *pRingBuffer = &gMp3Payload;
  /* Check if there is something new */
  uint32_t szConsumer = service_player_atomic_read(&pRingBuffer->szConsumer);
  if(szConsumer == 0)
  {
    return 0;
  }

  if(pRingBuffer->szBuffer - szConsumer < pRingBuffer->szBuffer / 2)
  {
    /* ring buffer under run */
  }


  mutex_Lock(&pRingBuffer->lock);
  /* Inject the payload MP3 from the ring buffer in spirit */
  uint8_t *pInput = service_player_bytes_buffer_GetConsumer(pRingBuffer);
  uint8_t *pOut = (uint8_t *)pCompressedData;
  for(nBytes  = 0; (nBytes < nDataSizeInChars) && (pRingBuffer->szConsumer) ; nBytes ++)
  {
    *pOut = *pInput;
    pOut++;
    service_player_bytes_buffer_MovePtrConsumer(pRingBuffer, 1, &pInput);

  }
  mutex_Unlock(&pRingBuffer->lock);
  return nBytes ;
}

/* Create the instance */
static AVS_Result service_audio_play_mp3_create(player_context_t *phandle);
static AVS_Result service_audio_play_mp3_create(player_context_t *phandle)
{

  bStartDecode = 0;
  memset(&mp3_context, 0, sizeof(mp3_context));

  /* Initialize the decoder instance */
  SpiritMP3DecoderInit(&mp3_context.instance, (fnSpiritMP3ReadCallback*)service_player_codec_mp3_read_data_cb, NULL, NULL);
  /* Create the codec thread */
  hCodecTask = task_Create(CODEC_MP3_TASK_NAME,service_player_mp3_task,hInstance,CODEC_MP3_TASK_STACK_SIZE,CODEC_MP3_TASK_PRIORITY);
  if(hCodecTask  ==0)
  {
    AVS_TRACE_ERROR("Create task %s", hInstance);
    return AVS_ERROR;
  }

  return AVS_OK;
}

/* Delete the instance */
static AVS_Result service_audio_play_mp3_delete(player_context_t *phandle);
static AVS_Result service_audio_play_mp3_delete(player_context_t *phandle)
{
  if(mp3_context.mp3Runing )
  {
    mp3_context.mp3Runing =  0;
    uint32_t timeout=5000/100; /* 5 secs*/
    while((mp3_context.mp3Runing   != 2) && (timeout != 0) )
    {
      osDelay(100); /* Leave enough time to terminate the thread */
      timeout--;
    }
    /* The flag must be set again to 2 to signal a clean exit */
    AVS_ASSERT(mp3_context.mp3Runing  == 2);
    task_Delete(hCodecTask );
    hCodecTask = 0;
    
  }
  return AVS_OK;
}

/* Start audio playback */
static AVS_Result service_audio_play_mp3_start(player_context_t *phandle);
static AVS_Result service_audio_play_mp3_start(player_context_t *phandle)
{
  AVS_Aux_Info info;
  AVS_Get_Audio_Aux_Info(hInstance, &info);
  info.infoFlags = AVS_AUX_INFO_FLAGS;
  info.iFlags &= ~(uint32_t)AVS_PIPE_MUTED;
  AVS_Set_Audio_Aux_Info(hInstance, &info);
  service_player_atomic_write(&bStartDecode, 1);
  return AVS_OK;
}

/* Stop audio playback */
static AVS_Result service_audio_play_mp3_stop(player_context_t *phandle);
static AVS_Result service_audio_play_mp3_stop(player_context_t *phandle)
{

  /* Mute the aux */
  AVS_Aux_Info info;
  AVS_Get_Audio_Aux_Info(hInstance, &info);
  info.infoFlags = AVS_AUX_INFO_FLAGS;
  info.iFlags |= AVS_PIPE_MUTED;
  AVS_Set_Audio_Aux_Info(hInstance, &info);
  service_player_atomic_write(&bStartDecode, 0);
  return AVS_OK;
}

/* Check the payload and return OK if the codec is identified */
static AVS_Result service_audio_play_mp3_check_codec(player_context_t *phandle, uint8_t *pPayload, int32_t size );
static AVS_Result service_audio_play_mp3_check_codec(player_context_t *phandle, uint8_t *pPayload, int32_t size )
{
  /* TODO : implement a TRUE MP3 signature checker */
  return AVS_OK; /* Always ok for now */
}

static player_codec_t codec =
{
  service_audio_play_mp3_create,
  service_audio_play_mp3_delete,
  service_audio_play_mp3_start,
  service_audio_play_mp3_stop,
  service_audio_play_mp3_check_codec,
};

/* Return the code fnopt */
player_codec_t * service_audio_play_media_mp3(void)
{
  return &codec;
}

