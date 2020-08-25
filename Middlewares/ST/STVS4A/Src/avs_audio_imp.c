/**
******************************************************************************
* @file    main.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Manage audio streaming
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
/*******************************************************************************
*
* This module  implements all the audio device feeds
* Init the audio device HP and microphone
* Gets samples from the network stream and convert and send the result to the audio speaker device
* Gets Device audio microphone devices and send to the network stream queue
*
*******************************************************************************/


#include "avs_private.h"

#define USE_VOLUME_CLAMP   0    /* Merge sample a addition and clamp on limits */

static void avs_audio_copy_producer(Avs_audio_buffer  *pDst, int16_t *pSrc, uint32_t freqSrc, uint32_t chSrc, uint32_t nSamples);
uint32_t  bOutputDebugWave = 0;

/*

Mono to stereo converter, basic formula


*/
__STATIC_INLINE int16_t mixStereo2Mono(int16_t l, int16_t r);
__STATIC_INLINE int16_t mixStereo2Mono(int16_t l, int16_t r)
{
#if USE_VOLUME_CLAMP
  register int a = r + l;
  if(a < -32768 ) a = -32768;
  if( a > 32767)  a = 32767;
#else
  register int32_t a = (r + l) / 2;

#endif
  return  a;
}

/*

Basic mixing  function, 2 samples applying a scaling then a clamping the result to a 16 bits


*/
__STATIC_INLINE int16_t merge_sample_volume(int16_t a, int16_t b, int32_t  vol);
__STATIC_INLINE int16_t merge_sample_volume(int16_t a, int16_t b, int32_t  vol)
{
  register int32_t r;
  if(vol == 100)
  {
    r = a + b;
  }
  else
  {
    r = ((a * vol) / 100) + b;
  }

#if USE_VOLUME_CLAMP
  if(r < -32768 )
  {
    r = -32768;
  }
  if( r > 32767)
  {
    r = 32767;
  }
#else
  r = r / 2;
#endif
  return r;
}


/* Just patch the sample */

__STATIC_INLINE void write_sample(int16_t *pSample, int16_t smp);
__STATIC_INLINE void write_sample(int16_t *pSample, int16_t smp)
{
  *pSample = smp;
}
/* Mix sample, and apply a volume */
__STATIC_INLINE void mix_sample_volume(int16_t *pSample, int16_t a, int32_t  vol);
__STATIC_INLINE void mix_sample_volume(int16_t *pSample, int16_t a, int32_t  vol)
{
  register int32_t r;
  if(vol == 100)
  {
    r =  a + *pSample;
  }
  else
  {
    r = ((a * vol) / 100) + (*pSample);
  }
#if USE_VOLUME_CLAMP
  if( r < -32768 )
  {
    r = -32768;
  }
  if( r > 32767)
  {
    r = 32767;
  }
#else
  r = r / 2;
#endif
  *pSample = r;
}



/*


Play sound manager, check if the wave is valid and prepare the playing



*/

AVS_Result avs_audio_play_sound(AVS_audio_handle *pAHandle, uint32_t flags, void *pWave, int32_t volumePercent)
{
  Avs_sound_player wav;
  if((flags & AVS_PLAYSOUND_STOP) != 0)
  {
    /* Force to stop the playback */
    /* Lock the pipe */
    avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
    /* Raz the player */
    avs_core_atomic_write((uint32_t *)(uint32_t)&pAHandle->soundPlayer.count, 0);
    avs_core_atomic_write((uint32_t *)(uint32_t)&pAHandle->soundPlayer.curCount, 0);
    avs_core_atomic_write((uint32_t *)(uint32_t)&pAHandle->soundPlayer.flags, 0);
    /* Unlock the pipe */
    avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);
    return AVS_OK;
  }

  /* We can play only a stream at once */
  if(pAHandle->soundPlayer.curCount != 0)
  {
    return AVS_BUSY;
  }

  /* Parse the header */
  if(avs_core_audio_wav_sound(pWave, &wav) != AVS_OK)
  {
    AVS_TRACE_ERROR("Wav file invalid");
    return AVS_ERROR;
  }

  /* Check min max value */
  if((volumePercent < 0) || (volumePercent > 200))
  {
    AVS_TRACE_ERROR("Parameter invalid");
    return AVS_PARAM_INCORECT;
  }
  /* Save values */
  wav.volume = volumePercent;


  /* Accept only up sampling */
  if(wav.freqWave > pAHandle->pFactory->freqenceOut )
  {
    AVS_TRACE_ERROR("Wave not compatible with audio output; frequency must be lower than %d HZ", pAHandle->pFactory->freqenceOut);
    return AVS_ERROR;
  }

  /* Accept only 1 channel */
  if(wav.chWave != 1)
  {
    AVS_TRACE_ERROR("Wave not compatible with audio output, must be mono");
    return AVS_ERROR;
  }

  /* Enable the player */
  wav.flags  = flags | AVS_PLAYSOUND_ACTIVE;

  /* Start the playback */
  avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
  memcpy(&pAHandle->soundPlayer, &wav, sizeof(wav));
  avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);

  /* Wait the end of the sound if the option is enabled */
  if((flags & AVS_PLAYSOUND_WAIT_END) != 0)
  {
    while((avs_core_atomic_read(&pAHandle->soundPlayer.flags) & AVS_PLAYSOUND_ACTIVE) != 0)
    {
      avs_core_task_delay(100);
    }
  }
  return AVS_OK;
}

typedef struct t_dnSample
{
  int16_t            *pIn; /* Src ptr */
  Avs_audio_buffer   *pRb; /* Ring buffer instance */
} dnSample_t;

static int32_t nextDnSample(void *pCookie);
static int32_t nextDnSample(void *pCookie)
{
  register dnSample_t *pHandle = (dnSample_t*)pCookie;
  register uint32_t *p32 = (uint32_t *)(uint32_t )pHandle->pIn;
  /* Caste in uin32_t for stereo */
  uint32_t sample = *p32;
  if(pHandle->pRb->szConsumer)
  {
    avs_audio_buffer_move_ptr_consumer(pHandle->pRb, 1, &pHandle->pIn);
  }
  else
  {
    AVS_TRACE_INFO("Fix me : Why i am here ? ");
  }
  return sample;


}



/*

Re sampling : For a best performance we must prevent the audio re-sampling , it can be done using the audio configuration


*/
static void avs_audio_copy_consumer(Avs_audio_buffer  *pSrc, int16_t *pDst, uint32_t freqDst, uint32_t chDst, uint32_t nSamples);
static void avs_audio_copy_consumer(Avs_audio_buffer  *pSrc, int16_t *pDst, uint32_t freqDst, uint32_t chDst, uint32_t nSamples)
{
  uint32_t freqSrc;
  uint32_t chSrc;
  freqSrc = pSrc->sampleRate;
  chSrc   = pSrc->nbChannel;
  uint32_t   vol = pSrc->volume;

  if (freqSrc == freqDst) /* Optimized version */
  {
    /* Sample format is ==  just copy */
    int16_t  *pIn = avs_audio_buffer_get_consumer(pSrc);
    for(int32_t a = 0; (a < nSamples) && (pSrc->szConsumer); a++)
    {
      if(chSrc == 1) /* If the stream is mono */
      {
        mix_sample_volume(pDst, *pIn, vol); /* Copy at least 1 channel */
        pDst++;
        if(chDst == 2) /* If the destination is stereo */
        {
          mix_sample_volume(pDst, *pIn, vol); /* Copy at least 1 channel */
          pDst++;
        }

      }
      else
      {
        /* The source is stereo */
        if(chDst == 1)
        {
          mix_sample_volume(pDst, mixStereo2Mono(pIn[0], pIn[1]), vol); /* Copy at least 1 channel */
          pDst++;
        }
        else
        {

          mix_sample_volume(pDst, pIn[0], vol); /* Copy at least 1 channel */
          pDst++;
          mix_sample_volume(pDst, pIn[1], vol); /* Copy at least 1 channel */
          pDst++;
        }
      }
      avs_audio_buffer_move_ptr_consumer(pSrc, 1, &pIn);
    }
    return;
  }
  if (freqSrc <= freqDst)
  {
    /* We must apply an up sampling */
    register int16_t sample;
    /* Get the current buffer */
    int16_t  *pIn = avs_audio_buffer_get_consumer(pSrc);
    /* Loops until the buffer end to feed */
    for (int32_t a = 0; (a < nSamples) && (pSrc->szConsumer); a++)
    {
      /* Prepare the sample, and check if we have to move in the ring buffer */
      if(avs_upsampling_compute(&pSrc->up_smp, freqSrc, freqDst, pIn))
      {
        avs_audio_buffer_move_ptr_consumer(pSrc, 1, &pIn);
      }
      /* Manage channel */
      if(chSrc == 1)
      {
        /* Interpolate  1 or more samples */
        sample = avs_upsampling_get_sample(&pSrc->up_smp, *pIn, 0);
        /* Manage the volume */
        mix_sample_volume(pDst, sample, vol );
        pDst++;
        /* Manage the channel number */
        if (chDst == 2)
        {
          mix_sample_volume(pDst, sample, vol);
          pDst++;

        }
      }
      else
      {
        /* Interpolate  1 or more samples */
        sample = avs_upsampling_get_sample(&pSrc->up_smp, *pIn, 0);
        /* Manage the volume */
        mix_sample_volume(pDst, sample, vol );
        pDst++;
        /* Manage the channel number */
        if (chDst == 2)
        {
          sample = avs_upsampling_get_sample(&pSrc->up_smp, pIn[1], 1);
          mix_sample_volume(pDst, sample, vol);
          pDst++;

        }

      }
    }
  }
  else
  {
    register int16_t  sample;
    /* Get the current buffer */
    int16_t  *pIn = avs_audio_buffer_get_consumer(pSrc);

    /* Create the down sampling cookie */
    dnSample_t hData;
    hData.pIn = pIn;
    hData.pRb = pSrc;



    /* Loops until the buffer end to feed */
    while((nSamples != 0) && (pSrc->szConsumer != 0))
    {
      /* Loops until we have compressed samples and move in the ring buffer */

      if(!avs_dnsampling_compute(&pSrc->dn_smp, freqSrc, freqDst, nextDnSample, &hData, chSrc, pSrc->szConsumer))
      {

        break;
      }
      nSamples--;
      /* Manage the channel number */
      if(chSrc == 1)
      {
        /* Interpolates  1 or more samples */
        sample = avs_dnsampling_get_sample(&pSrc->dn_smp, 0);
        /* Apply the volume */
        mix_sample_volume(pDst, sample, vol );
        pDst++;
        /* Manages the channel number */
        if (chDst == 2)
        {
          /* Apply the volume */
          mix_sample_volume(pDst, sample, vol);
          pDst++;

        }
      }
      else
      {
        /* Interpolates  1 or more samples */
        sample = avs_dnsampling_get_sample(&pSrc->dn_smp, 0);
        /* Apply the volume */
        mix_sample_volume(pDst, sample, vol );
        pDst++;
        /* Manage the channel number */
        if (chDst == 2)
        {
          sample = avs_dnsampling_get_sample(&pSrc->dn_smp, 1);
          mix_sample_volume(pDst, sample, vol);
          pDst++;

        }

      }
    }
  }
}

/*

Re sampling : for a best performance we must prevent the audio re sampling , it can be done using the audio configuration
TODO : review re sampling

*/

static void avs_audio_copy_producer(Avs_audio_buffer  *pDst, int16_t *pSrc, uint32_t freqSrc, uint32_t chSrc, uint32_t nSamples)
{
  uint32_t freqDst;
  uint32_t chDst;
  int32_t  a=0;

  freqDst = pDst->sampleRate;
  chDst = pDst->nbChannel;


  if (freqSrc == freqDst) /* Optimized version */
  {
    /* Get the out buffer */
    int16_t  *pOut = avs_audio_buffer_get_producer(pDst);
    /* Loops until the end of the buffer to feed */
    for (a = 0; a < nSamples; a++)
    {
      /* Manages channel number */
      if (chSrc == 2)
      {
        /* TODO : FIX-ME Keep only one channel */
        write_sample(pOut, *pSrc);
        pSrc += 2;
      }
      else
      {
        write_sample(pOut, *pSrc);
        pSrc++;

      }
      /* Manages the ring buffer */
      avs_audio_buffer_move_ptr_producer(pDst, 1, &pOut);
    }
    return;
  }
  if (freqSrc < freqDst)
  {
    /* Warning : this re-sampling is not verified */
    static int16_t  sample;
    int16_t  *pOut = avs_audio_buffer_get_producer(pDst);
    /* We have to apply a up sampling because the micro freq is lower than the stream freq */
    for (a = 0; a < nSamples; a++)
    {

      if (chSrc == 2)
      {
        sample = mixStereo2Mono(*pSrc, *(pSrc + 1));
        pSrc += 2;
      }
      else
      {
        sample = *pSrc;
        pSrc++;
      }

      while (pDst->cumul < freqDst)
      {
        pDst->cptCumul++;
        pDst->cumul += freqSrc;
      }

      for (a = 1; a <= pDst->cptCumul; a++)
      {
        int32_t dlt = sample - pDst->lastSample;
        int16_t  smp = pDst->lastSample + ((dlt * a) / pDst->cptCumul);
        write_sample(pOut, smp);
        if (chDst == 2)
        {
          smp = sample + ((sample - pDst->lastSample) * a) / pDst->cptCumul;
          write_sample(pOut + 1, smp);
        }
        avs_audio_buffer_move_ptr_producer(pDst, 1, &pOut);
      }
      pDst->lastSample = sample;
      pDst->cptCumul = 0;
      pDst->cumul -= freqDst;
    }
  }
  else
  {
    static  int32_t  sample = 0;
    uint32_t cpt = 0;
    int16_t  *pOut = avs_audio_buffer_get_producer(pDst);
    /* We have to apply a up sampling */
    for (a = 0; a < nSamples; a++)
    {
      if (pDst->cumul >= freqSrc)
      {
        pDst->cumul -= freqSrc;
        if(cpt)
        {
          sample = sample / cpt;
        }
        write_sample(pOut, sample);
        avs_audio_buffer_move_ptr_producer(pDst, 1, &pOut);
        cpt = 0;
        pDst->cumul = 0;
      }
      pDst->cumul += freqDst;
      cpt++;
      if (chSrc == 2)
      {
        sample += mixStereo2Mono(*pSrc, *(pSrc + 1));
        pSrc += 2;
      }
      else
      {
        sample += *pSrc;
        pSrc++;
      }
    }
  }
}

/*

	Mix an incoming sample directly in the ring buffer. up sample and duplicate the channel if needed
	notice: assuming we can do only up sampling. A sound is an effect that don't need a very hight quality
	so we assume the sample is always mono and never have an higher frequency than the native audio device


*/

void avs_audio_mix_sound(Avs_sound_player *pHandle, int16_t  *pBuffer, uint32_t freqDst, uint32_t chDst, uint32_t size);
void avs_audio_mix_sound(Avs_sound_player *pHandle, int16_t  *pBuffer, uint32_t freqDst, uint32_t chDst, uint32_t size)
{
  int32_t a;
  /* Exit if nothing to do */
  if(pHandle->curCount == 0)
  {
    return;
  }
  AVS_ASSERT((pHandle->curCount >= 0));
  /* Loads the WAV buffer */
  int16_t  *pDst = pBuffer;

  register int16_t  sample;
  register int32_t volume = pHandle->volume;


  /* We have to apply a up sampling */
  for (a = 0; (a < size) && (pHandle->curCount) ; a++)
  {
    if(pHandle->curCount!= 0)
    {
      if(avs_upsampling_compute(&pHandle->up_smp, pHandle->freqWave, freqDst, pHandle->pCurWave) != 0)
      {

        AVS_ASSERT(pHandle->curCount);
        pHandle->pCurWave++;
        pHandle->curCount--;
      }
    }
    /* Interpolates  1 or more samples */
    sample = avs_upsampling_get_sample(&pHandle->up_smp, *pHandle->pCurWave, 0);
    /* Finalize */
    *pDst = merge_sample_volume(sample, *pDst, volume);
    pDst++;
    /* Manages the channel number */
    if (chDst == 2)
    {
      *pDst = merge_sample_volume(sample, *pDst, volume);
      pDst++;

    }
  }
  /* Check the playback end */
  if(pHandle->curCount  == 0)
  {
    /* Playback end */
    avs_upsampling_init(&pHandle->up_smp);
    /* If the sample loops, restart the playback */
    if((pHandle->flags & AVS_PLAYSOUND_PLAY_LOOP)!= 0)
    {
      /* Rearm the sound */
      pHandle->curCount = pHandle->count;
      pHandle->pCurWave = pHandle->pWave;
    }
    else
    {
      /* If end , stop... */
      pHandle->flags =  pHandle->flags  & (uint32_t)~AVS_PLAYSOUND_ACTIVE;
    }
  }
}

/*


Main audio injection task, this task initialize the audio device and negotiates the  ring buffer size.
Starts to pump buffers from the stream an pipe it in the audio device output.
To keep the audio quality  as mush as possible. the audio stream has no constraint and
the task is in charge to convert the audio format stream to the audio format out

The re sampling has a performance impact, so to avoid this issue, the stream must audio format
must fit the device audio format. In this case this task will be faster
when the pumping  starts, the task signal to the Input task to start the its job



*/
static void avs_audio_voice_injection_task(const void *pCookie);
static void avs_audio_voice_injection_task(const void *pCookie)
{
  uint32_t      halfSize;
  AVS_audio_handle *pAHandle = (AVS_audio_handle *)(uint32_t)pCookie;
  AVS_instance_handle *pIHandle = pAHandle->pInstance;
  /* Wait the instance is created */
  while(avs_core_atomic_read(&pIHandle->bInstanceStarted) == 0)
  {
    avs_core_task_delay(500);
  }
  /* Leave some time to print message without events at the start-up */
  avs_core_task_delay(2000);



  /* First we have to initialize pipes (in/out) */
  /* A pipe is an object that takes a ring buffer input and post process it, and injects the result in the ring buffer out */

  AVS_VERIFY(avs_audio_pipe_create(&pAHandle->recognizerPipe));
  AVS_VERIFY(avs_audio_pipe_create(&pAHandle->synthesizerPipe));
  if(pIHandle->pFactory->useAuxAudio)
  {
    AVS_VERIFY(avs_audio_pipe_create(&pAHandle->auxAudioPipe));
  }

  /* Now we can initialize the low level audio */
  /* The porting layer will complete the AudioPipe initialization by */
  /* The initialization of frequency info a ring buffer sizing */

  AVS_VERIFY(drv_audio.platform_Audio_init(pAHandle));

  /* Init factory buffer lenght in MS */
  

  pAHandle->pFactory->buffSizeOut = (pAHandle->synthesizerPipe.outBuffer.szBuffer / ((float)pAHandle->pFactory->freqenceOut)) * 1000;
  pAHandle->pFactory->buffSizeIn =  (pAHandle->recognizerPipe.inBuffer.szBuffer / ((float)pAHandle->pFactory->freqenceIn)) * 1000;


  /* Init the mp3 decoder */
  AVS_VERIFY(drv_audio.platform_MP3_decoder_init(pAHandle));



  /* Compute the  buffer size in sec in order to size cleanly the stream buffer. */
  /* Streams must have a latency in order to absorb the delay due to the network. */
  /* So, streams buffer size is a factor of the native audio size */
  /* We compute audio buffer size in seconds, then we compute the stream size with a latency complicator of this audio size in sec taking in account a different frequency */
  /* We need also to take in account the re sampling */
  /* Teh buffer size mist reflect the difference between frequent rate */
  /* Example 1 ms at 16K = 16 samples will produce 1ms at 22K = 22 samples , */
  /* So if we up sample a stream the low freq buffer must be  smaller than the high rate buffer because 1ms will produce more sample if we up sample */
  /* And vice versa */


  /* Convert the buffer size in time */
  avs_float_t secSynth  =  ((avs_float_t)pAHandle->synthesizerPipe.outBuffer.szBuffer) / pAHandle->synthesizerPipe.outBuffer.sampleRate;
  avs_float_t secReco   =  ((float_t)pAHandle->recognizerPipe.inBuffer.szBuffer) / pAHandle->recognizerPipe.inBuffer.sampleRate;

  /* Now we have to compute the buffer size for the synthesizer and the recognizer given that the audio driver already initialized its part */
  /* First speakers */


  /* Compute a difference factor between both frequencies at the same time */
  /* if the user has defined its latency, we use it */
  avs_float_t diffFactor =0;
  uint32_t    szSynth  =0;
  if(pIHandle->pFactory->audioOutLatency ==0)
  {
    diffFactor = ((avs_float_t)pAHandle->synthesizerPipe.outBuffer.sampleRate) /  pIHandle->pFactory->synthesizerSampleRate;
    /* Now we can compute the stream size with a factor complicator to absorb the network latency */
    szSynth  = (uint32_t)(avs_float_t)(pIHandle->pFactory->synthesizerSampleRate * secSynth * diffFactor * DEFAULT_AUDIO_OUT_MULT_SIZE);
  }
  else
  {
    szSynth  = pIHandle->pFactory->audioOutLatency ;
  }


  /* Compute the recognizer */
  uint32_t szReco    = 0;
  /* if the user has defined its latency, we use it */
  if(pIHandle->pFactory->audioInLatency ==0)
  {
    diffFactor = ((avs_float_t)pAHandle->recognizerPipe.inBuffer.sampleRate) /  pIHandle->pFactory->recognizerSampleRate;
    szReco    = (uint32_t)(avs_float_t)(pIHandle->pFactory->recognizerSampleRate   * secReco * diffFactor * DEFAULT_AUDIO_IN_MULT_SIZE);
  }
  else
  {
    szReco     = pIHandle->pFactory->audioInLatency ;
  }
    



  /* Compute the auxaudio */
  diffFactor = ((float)pIHandle->pFactory->auxAudioSampleRate / pAHandle->auxAudioPipe.outBuffer.sampleRate);

  /* Initialize the stream microphone & speaker ring buffer */
  /* AudioIn.outBuffer is the stream ring buffer (audioIn.inBuffer will be filled by the device audio microphone) */

  AVS_VERIFY(avs_audio_buffer_create(&pAHandle->synthesizerPipe.inBuffer, NULL, szSynth, pIHandle->pFactory->synthesizerSampleChannels, pIHandle->pFactory->synthesizerSampleRate));
  if(pIHandle->pFactory->useAuxAudio)
  {
    /* Compute the auxaudio */
    /* Initialize auxiliary at a low value, will be re-Initialize at the right value when used */
    uint32_t szAux    =  1000;
    /* Initialize the default aux audio channel */
    AVS_VERIFY(avs_audio_buffer_create(&pAHandle->auxAudioPipe.inBuffer, NULL, szAux, pIHandle->pFactory->auxAudioSampleChannels, pIHandle->pFactory->auxAudioSampleRate));
    pAHandle->auxAudioPipe.threshold    = pAHandle->auxAudioPipe.inBuffer.szBuffer; /* TODO : check */
    /* Muted by default */
    pAHandle->auxAudioPipe.pipeFlags  |= AVS_PIPE_MUTED;
  }
  /* Initialize the default synthesizer buffer */
  AVS_VERIFY(avs_audio_buffer_create(&pAHandle->recognizerPipe.outBuffer, NULL, szReco, AUDIO_IN_CHANNELS, pIHandle->pFactory->recognizerSampleRate));


  /* Buffer load threshold to inject the payload in the native audio buffer */
  /* We compute the threshold at least one half at  equal frequency */
  pAHandle->synthesizerPipe.threshold = (uint32_t)(avs_float_t)(pAHandle->pFactory->freqenceOut * pAHandle->pFactory->buffSizeOut / 2) / 1000;



  /* Set-up Re samplers */

  avs_audio_pipe_update_resampler(&pAHandle->synthesizerPipe);
  avs_audio_pipe_update_resampler(&pAHandle->recognizerPipe);
  avs_audio_pipe_update_resampler(&pAHandle->auxAudioPipe);



  /* Loads some variables  for the perf */
  
  halfSize            = pAHandle->synthesizerPipe.outBuffer.szBuffer / 2 ;
  uint32_t freqOut    = pAHandle->pFactory->freqenceOut;
  uint32_t channelOut = pAHandle->synthesizerPipe.outBuffer.nbChannel;

  /* We are ready to pump  buffer from AVS and push them in the HW device audio */
  /* Signal event */
  avs_core_message_send(pIHandle, EVT_AUDIO_RECOGNIZER_TASK_START, 0);
  pAHandle->runOutRuning = 1;
  /* Notifies  the audio task out is completely initialized */
  avs_core_event_set(&pAHandle->taskCreatedEvent);

  while(pAHandle->runOutRuning)
  {

    /* Waits for a regalement that the audio has consumer a part of the buffer */
    avs_core_event_wait(&pAHandle->synthesizerPipe.outEvent, INFINITE_DELAY);
    /* Lock it */
    avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
    /* Set the ring buffer according to the double buffering situation ( full or half ) */
    pAHandle->synthesizerPipe.pipeFlags |= ((pAHandle->synthesizerPipe.pipeFlags  & AVS_PIPE_EVT_HALF) != 0 ? (uint32_t)AVS_PIPE_PROCESS_PART1 : (uint32_t)AVS_PIPE_PROCESS_PART2);

    avs_audio_buffer_quick_set(&pAHandle->synthesizerPipe.outBuffer, (pAHandle->synthesizerPipe.pipeFlags & AVS_PIPE_EVT_HALF) != 0 ? AVS_RING_BUFF_PROD_PART1 : AVS_RING_BUFF_PROD_PART2);
    int16_t *pDst = avs_audio_buffer_get_producer(&pAHandle->synthesizerPipe.outBuffer);

    /* Assumes buffer empty by default */
    memset(pDst, 0, halfSize * pAHandle->synthesizerPipe.outBuffer.nbChannel * sizeof(int16_t));
    /* Check if we have to inject or not */
    /* In the normal case, we start the playback and we wait a threshold to  prevent cut in the buffer */
    /* At the end of the playback we want to make sure that all data will be pushed in the device ( PURGE flag) before to stop */
    /* Alexa voice injection */
    if( (pAHandle->synthesizerPipe.inBuffer.szConsumer>=pAHandle->synthesizerPipe.threshold) || ((pAHandle->synthesizerPipe.pipeFlags & (uint32_t)AVS_PIPE_PURGE) != 0))
    {
      /* If we are in buffering mode, we wait for the threshold before to start */
      if((pAHandle->synthesizerPipe.pipeFlags & (uint32_t)AVS_PIPE_BUFFERING)==0 )
      {
        /* After the buffering, we need only and threshold halfSize before to inject */
        pAHandle->synthesizerPipe.threshold  = halfSize;
        /* If the pipe is muted, don't inject in the audio beut consume the payload */
        if((pAHandle->synthesizerPipe.pipeFlags & AVS_PIPE_MUTED)==0)
        {
          avs_audio_copy_consumer(&pAHandle->synthesizerPipe.inBuffer, pDst, freqOut, channelOut, halfSize);

        }
        else
        {
          /* If the device is muted, we consume the buffer without to feed the output */
          uint32_t sz2Consume = (halfSize * pAHandle->synthesizerPipe.outBuffer.sampleRate)  / pAHandle->synthesizerPipe.inBuffer.sampleRate;
          avs_audio_buffer_move_consumer(&pAHandle->synthesizerPipe.inBuffer, sz2Consume);
        }
      }
      else
      {
        /* If we are in buffering mode, we wait until the buffer is full , then we remove the flag */
        if(pAHandle->synthesizerPipe.inBuffer.szConsumer >= pAHandle->synthesizerPipe.inBuffer.szBuffer)
        {
          pAHandle->synthesizerPipe.pipeFlags &= ~(uint32_t)AVS_PIPE_BUFFERING;
        }
      }
    }
    /* Apply and Mix  the sound on the output buffer */
    avs_audio_mix_sound(&pAHandle->soundPlayer, pDst, freqOut, channelOut, halfSize);

    /* Unlock */
    avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);

    /* AuxAudio Injection */
    /* Process the auxiliary pipe */
    if(pIHandle->pFactory->useAuxAudio)
    {
      /* If we are in buffering mode, we wait for the threshold before to start */
      if((pAHandle->auxAudioPipe.pipeFlags & AVS_PIPE_BUFFERING)==0 )
      {
        /* Check the threshold before to inject, if the purge is set we inject all data and forget the threshold */
        if((pAHandle->auxAudioPipe.inBuffer.szConsumer >= pAHandle->auxAudioPipe.threshold) || ((pAHandle->auxAudioPipe.pipeFlags & AVS_PIPE_PURGE)!= 0))
        {
          /* Lock the pipe */
          avs_core_mutex_lock(&pAHandle->auxAudioPipe.lock);
          /* If the pipe is muted, don't inject in the audio beut consume the payload */
          if((pAHandle->auxAudioPipe.pipeFlags & AVS_PIPE_MUTED)==0)
          {
            avs_audio_copy_consumer(&pAHandle->auxAudioPipe.inBuffer, pDst, freqOut, channelOut, halfSize);
          }
          else
          {
            /* If the device is muted, we consume the buffer without to patch the output */
            uint32_t sz2Consume = (halfSize * pAHandle->auxAudioPipe.inBuffer.sampleRate)  / pAHandle->auxAudioPipe.inBuffer.sampleRate;
            if(sz2Consume >= pAHandle->auxAudioPipe.inBuffer.szConsumer )
            {
              sz2Consume = pAHandle->auxAudioPipe.inBuffer.szConsumer;
            }
            avs_audio_buffer_move_consumer(&pAHandle->auxAudioPipe.inBuffer, sz2Consume);

          }
          avs_core_mutex_unlock(&pAHandle->auxAudioPipe.lock);
        }
        else
        {
          if((pAHandle->auxAudioPipe.pipeFlags & AVS_PIPE_MUTED)==0 )
          {
            AVS_TRACE_DEBUG("Auxiliary Pop Detected ");
          }

        }
      }
      else
      {
        /* If we are in buffering mode, we wait until the buffer is full , then we remove the flag */
        if(pAHandle->auxAudioPipe.inBuffer.szConsumer >= pAHandle->auxAudioPipe.inBuffer.szBuffer)
        {
          pAHandle->auxAudioPipe.pipeFlags &= ~(uint32_t)AVS_PIPE_BUFFERING;
        }
      }

    }
    /* For Debug purpose or AEC tuning, overload all samples and produce a wave, produce a tone A ( la) */
    if(bOutputDebugWave)
    {
      avs_audio_feed_wave(pDst, halfSize, pAHandle->synthesizerPipe.outBuffer.nbChannel, pAHandle->synthesizerPipe.outBuffer.sampleRate, 440, 31000);
    }
    /* Signal packet processed in order to detect over-run in the ISR */
    pAHandle->synthesizerPipe.pipeFlags &=  ~((pAHandle->synthesizerPipe.pipeFlags  & (uint32_t)AVS_PIPE_EVT_HALF) != 0 ? (uint32_t)AVS_PIPE_PROCESS_PART1 : (uint32_t)AVS_PIPE_PROCESS_PART2);

  }

  /* Delete pipes */
  avs_audio_pipe_delete(&pAHandle->recognizerPipe);
  avs_audio_pipe_delete(&pAHandle->synthesizerPipe);
  if(pIHandle->pFactory->useAuxAudio)
  {
    avs_audio_pipe_delete(&pAHandle->auxAudioPipe);
  }


  /* Clean-up audio devices */
  AVS_VERIFY(drv_audio.platform_MP3_decoder_term(pAHandle));
  AVS_VERIFY(drv_audio.platform_Audio_term(pAHandle));
  /* Notifies the task is finished */
  avs_core_event_set(&pAHandle->taskCreatedEvent) ;
  avs_core_message_send(pIHandle, EVT_AUDIO_RECOGNIZER_TASK_DYING, 0);

}


/*

Audio Microphone task, this task pumps samples coming from  the microphone
re sample the stream if it is mandatory and re-injects the result in the network stream
The re sampling has a performance impact, so to avoid this issue, the stream must audio format
must fit the device audio format. In this case this task will be faster


*/

static void avs_audio_speaker_injection_task(const void *pCookie);
static void avs_audio_speaker_injection_task(const void *pCookie)
{
  uint32_t      halfSize;
  AVS_audio_handle *pAHandle = (AVS_audio_handle *)(uint32_t) pCookie;
  AVS_instance_handle *pIHandle = pAHandle->pInstance;

  /* Waits for the complete audio initialization */
  avs_core_event_wait(&pAHandle->taskCreatedEvent, INFINITE_DELAY);

  pAHandle->recognizerPipe.threshold = pAHandle->recognizerPipe.inBuffer.szBuffer / 2; /* TODO  : check me */

  /* Loads some variables  for the perf */
  halfSize               = pAHandle->recognizerPipe.inBuffer.szBuffer / 2 ;
  uint32_t freqMic       = pAHandle->pFactory->freqenceIn;

  /* Initialize the  audio micro ( out = stream , in = DMA producer) */

  uint32_t channelStream = pAHandle->recognizerPipe.outBuffer.nbChannel;
  uint32_t channelMic  = pAHandle->recognizerPipe.inBuffer.nbChannel;

  /* We assume that the microphone network stream  is always mono */
  AVS_ASSERT(channelStream  == 1);


  pAHandle->runInRuning = 1;
  avs_core_message_send(pIHandle, EVT_AUDIO_SYNTHESIZER_TASK_START, 0);

  pAHandle->recognizerPipe.pipeFlags   |= AVS_PIPE_MUTED;

  while(pAHandle->runInRuning)
  {
    /* Waits for a signalment that the audio has produced  a part of the buffer */
    avs_core_event_wait(&pAHandle->recognizerPipe.inEvent, avs_infinit_delay);
    /* Lock it */
    avs_core_mutex_lock(&pAHandle->recognizerPipe.lock);
    pAHandle->recognizerPipe.pipeFlags |= ((pAHandle->recognizerPipe.pipeFlags  & AVS_PIPE_EVT_HALF) != 0 ? (uint32_t)AVS_PIPE_PROCESS_PART1 : (uint32_t)AVS_PIPE_PROCESS_PART2);

    /* Set the ring buffer according to the double buffering situation ( full or half ) */
    avs_audio_buffer_quick_set(&pAHandle->recognizerPipe.inBuffer, (pAHandle->recognizerPipe.pipeFlags & AVS_PIPE_EVT_HALF) != 0 ? AVS_RING_BUFF_CONSUM_PART1 : AVS_RING_BUFF_CONSUM_PART2);
    int16_t *pSrc = avs_audio_buffer_get_consumer(&pAHandle->recognizerPipe.inBuffer);
    /* Check if there are enough space in the buffer to produce the packet */
    if((pAHandle->recognizerPipe.pipeFlags & AVS_PIPE_RUN) != 0)
    {
      /* Takes the raw microphone buff and feed the stream buffer at the right format */
      if((pAHandle->recognizerPipe.pipeFlags & AVS_PIPE_MUTED)==0 )
      {

        /* Gives  n chances (ie n ms) to send the packet, otherwise the stream is too slow to be consumed and we forget this packet */
        int32_t timeOut = 5;
        int32_t nbSamples = 0;
        while(timeOut)
        {
          nbSamples = avs_audio_buffer_get_producer_size_available(&pAHandle->recognizerPipe.outBuffer);
          if(nbSamples   >= halfSize)
          {
            avs_audio_copy_producer(&pAHandle->recognizerPipe.outBuffer, pSrc, freqMic, channelMic, halfSize);
            break;
          }
          else
          {
            /* Sleep 1 MS to leave enough time for the stream consumer */
            avs_core_task_delay(1);
            timeOut--;
          }

        }
      }
    }
    /* Signal packet processed in order to detect over-run in the ISR */
    pAHandle->recognizerPipe.pipeFlags &= ~((pAHandle->recognizerPipe.pipeFlags  & AVS_PIPE_EVT_HALF) != 0 ? (uint32_t)AVS_PIPE_PROCESS_PART1 : (uint32_t)AVS_PIPE_PROCESS_PART2);
    /* Unlock it */
    avs_core_mutex_unlock(&pAHandle->recognizerPipe.lock);
  }
  /* Notifies the task is finished */
  avs_core_event_set(&pAHandle->taskCreatedEvent) ;
  avs_core_message_send(pIHandle, EVT_AUDIO_SYNTHESIZER_TASK_DYING, 0);


}

/*
This function capture a raw payload from the network stream.
The function return the size ready captured in the buffer
The function clamps the size to the maximum available but never exceed the expected size

*/

uint32_t avs_avs_capture_audio_stream_buffer(AVS_audio_handle *pAHandle, void *pDst, int32_t size )
{
  uint32_t nbSamples = 0;
  AVS_ASSERT(pDst != 0);
  nbSamples = size;
  if(size == 0)
  {
    return 0;
  }
  avs_core_mutex_lock(&pAHandle->recognizerPipe.lock);
  /* Payload must be aligned on the sample size */
  nbSamples = avs_audio_buffer_consume(&pAHandle->recognizerPipe.outBuffer, nbSamples, pDst);
  avs_core_mutex_unlock(&pAHandle->recognizerPipe.lock);
  return nbSamples;
}

/*

reset safely incoming data  in the speaker pipe


*/
void avs_audio_inject_audio_stream_reset(AVS_audio_handle *pAHandle)
{

  avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
  /* Reset the buffer  */
  avs_audio_buffer_reset(&pAHandle->synthesizerPipe.inBuffer);
  /* Set the buffering mode before playing to prevent audio pops */
  pAHandle->synthesizerPipe.pipeFlags |= (uint32_t)AVS_PIPE_BUFFERING;
  pAHandle->synthesizerPipe.threshold = pAHandle->synthesizerPipe.inBuffer.szBuffer/2;
  avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);
}

/*

mute the input


*/
void avs_audio_capture_mute(AVS_audio_handle *pAHandle, uint32_t state)
{
  avs_core_mutex_lock(&pAHandle->recognizerPipe.lock);
  if(state)
  {
    pAHandle->recognizerPipe.pipeFlags  |= AVS_PIPE_MUTED;
  }
  else
  {
    pAHandle->recognizerPipe.pipeFlags  &= ~(uint32_t)AVS_PIPE_MUTED;
  }
  avs_core_mutex_unlock(&pAHandle->recognizerPipe.lock);
}

/*

Reset the capture ring buffer


*/

void avs_audio_capture_reset(AVS_audio_handle *pAHandle)
{

  avs_core_mutex_lock(&pAHandle->recognizerPipe.lock);
  avs_audio_buffer_reset(&pAHandle->recognizerPipe.outBuffer);
  avs_core_mutex_unlock(&pAHandle->recognizerPipe.lock);
}



/*

We have injected some samples coming from the  the speaker
Waits until all data are consumed


*/

void     avs_audio_capture_wait_all_consumed(AVS_audio_handle *pAHandle)
{
  /* Wait all data consumed */
  avs_core_mutex_lock(&pAHandle->recognizerPipe.lock);
  pAHandle->recognizerPipe.pipeFlags |= AVS_PIPE_PURGE;
  avs_core_mutex_unlock(&pAHandle->recognizerPipe.lock);
  uint32_t timeout=5000/100; /* Waits 5 sec for the time-out */ 
  while(timeout != 0)
  {
    int32_t szConsumer = pAHandle->recognizerPipe.outBuffer.szConsumer;
    if(szConsumer == 0)
    {
      break;
    }
    avs_core_task_delay(100);
	timeout--;
  }

  avs_core_mutex_lock(&pAHandle->recognizerPipe.lock);
  pAHandle->recognizerPipe.pipeFlags &= ~(uint32_t)(AVS_PIPE_MUTED | AVS_PIPE_PURGE);
  avs_core_mutex_unlock(&pAHandle->recognizerPipe.lock);
}


/*


Waits until all data are consumed bye


*/

void     avs_audio_inject_wait_all_consumed(AVS_audio_handle *pAHandle)
{
  /* Wait all data consumed */
  avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
  pAHandle->synthesizerPipe.pipeFlags |= AVS_PIPE_PURGE;
  avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);

   uint32_t timeout=5000/100; /* Waits 5 sec for the time-out */ 

  while(timeout != 0)
  {
    int32_t szConsumer = pAHandle->synthesizerPipe.inBuffer.szConsumer;
    if(szConsumer == 0)
    {
      break;
    }
    avs_core_task_delay(100);
	timeout--;
  }
  avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
  pAHandle->synthesizerPipe.pipeFlags &= ~(uint32_t)(AVS_PIPE_MUTED | AVS_PIPE_PURGE);
  avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);
}





/*
This function inject a raw payload in the voice network stream.
we assume that this payload is syntacticaly perfect
ie : never terminated with an  half or part sample ( ie sample means 16 bits *2 if stereo)
The function return the size really injected in the buffer
The function clamps the size to the maximum available but never exceed the expected size

*/


uint32_t avs_audio_inject_stream_buffer(AVS_audio_handle *pAHandle, void *pSrc, int32_t size )
{
  uint32_t nbSamples = 0;
  nbSamples = size;
  if(size == 0)
  {
    return 0;
  }
  avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
  nbSamples = avs_audio_buffer_produce(&pAHandle->synthesizerPipe.inBuffer, nbSamples, pSrc);
  avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);
  return nbSamples;
}

/*

Manipulate safely pipe flags


*/

void avs_audio_inject_stream_set_flags(AVS_audio_handle *pAHandle, uint32_t clearlags, uint32_t addFlags )
{
  avs_core_mutex_lock(&pAHandle->synthesizerPipe.lock);
  uint32_t flags = pAHandle->synthesizerPipe.pipeFlags;
  flags &= ~clearlags;
  flags |= addFlags;
  pAHandle->synthesizerPipe.pipeFlags = flags;
  avs_core_mutex_unlock(&pAHandle->synthesizerPipe.lock);

}


/*

This function apply a mp3 decompression, then feed the speaker stream
if the stream is full, the function waits until the stream is consumed by the audio player
this function is completely delegated to the porting layer in order to ease the change of MP3 codec

*/

AVS_Result avs_audio_inject_stream_buffer_mp3(AVS_audio_handle *pHandle, uint8_t* pBuffer, uint32_t size)
{
  while(size)
  {
    uint32_t blkSize = 360;
    if(size < blkSize)
    {
      blkSize = size;
    }
    drv_audio.platform_MP3_decoder_inject(pHandle, pBuffer, blkSize);
    pBuffer += blkSize;
    size -= blkSize;
  }
  return AVS_OK;
}

/*

Audio create Delegation
instantiates the audio device ( speaker  and microphone  )


*/

AVS_Result avs_audio_create(AVS_audio_handle *pHandle)
{
  AVS_VERIFY(avs_core_event_create(&pHandle->taskCreatedEvent));
  pHandle->taskInjectionOut = avs_core_task_create(AUDIO_OUT_TASK_NAME, avs_audio_voice_injection_task, pHandle, AUDIO_OUT_TASK_STACK_SIZE, AUDIO_OUT_TASK_PRIORITY);
  AVS_ASSERT(pHandle->taskInjectionOut );
  if(pHandle->taskInjectionOut == 0)
  {
    AVS_TRACE_ERROR("Create task %s", AUDIO_OUT_TASK_NAME);
    return AVS_ERROR;
  }

  pHandle->taskInjectionOut = avs_core_task_create(AUDIO_IN_TASK_NAME, avs_audio_speaker_injection_task, pHandle, AUDIO_IN_TASK_STACK_SIZE, AUDIO_IN_TASK_PRIORITY);
  AVS_ASSERT(pHandle->taskInjectionOut);
  if(pHandle->taskInjectionOut == 0)
  {
    AVS_TRACE_ERROR("Create task %s", AUDIO_OUT_TASK_NAME);
    return AVS_ERROR;
  }
  return AVS_OK;
}



/*

Audio delete Delegation
terminate the audio device ( speaker  and microphone  )


*/

AVS_Result avs_audio_delete(AVS_audio_handle *pHandle)
{

  pHandle->runInRuning =  0;
  avs_core_event_set(&pHandle->synthesizerPipe.inEvent); /* Make sure we are no locked in the loop */
  avs_core_event_wait(&pHandle->taskCreatedEvent, INFINITE_DELAY);    /* Wait the exit task */

  pHandle->runOutRuning =  0;
  avs_core_event_set(&pHandle->synthesizerPipe.outEvent);           /* Make sure we are no locked in the loop */
  avs_core_event_wait(&pHandle->taskCreatedEvent, INFINITE_DELAY);   /* Wait the exit task */
  /* Delete object */
  avs_core_event_delete(&pHandle->taskCreatedEvent);
  return AVS_OK;
}


