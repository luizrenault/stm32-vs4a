/*******************************************************************************
* @file    avs_porting_bsp_F7_audio_STM32F769I_DISCO.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   porting layer file
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
/*
*
*
*
*
* This module is in charge to inititalise the audio input and output
*
* initialize the audio porting layer means to init the audio hardware microhone and speakers ( in and out )
* then initialize audio pipes
* the audio pipe is created by the AVS instance and the AVS part ( its ring buffer)
* the driver is responsaible to init its part ( its ring buffers)
* the handle audio pipe IN ( HW microhone) to OUT AVS ring buffer  and fill it using HAL IN callbacks ( pipe producer)
* the handle audio pipe OUT (HW speaker )  from IN AVS ring buffer and consume it using  HAL OUT callbacks ( pipe consumer)
* buffers must fit as mush as possible pFactory recomandation
* buffer samples are assumed natif thanks to the avs_createRingBuffer declaration.

*
* for each packet consumed our injected, the driver must signal inEvent or outEvent object
*
*/


#include "avs_private.h"
#include "avs_porting_bsp_F7.h"

#define   AUDIO_IN_SIZE           (512)       /* (must be splited by 2 for half buffer)  real buffer size is AUDIO_SIZE_DMA_SIZE*AUDIO_SIZE_DMA_SIZE_ELEMENT TODO: check this size */
#define   AUDIO_OUT_SIZE          (2*1024)    /* (must be splited  by 2 for half buffer) */


/* Gives the oportunity to put those buffers in a specific segement */

#define AUDIO_BUFFER_IN    ((int16_t*)NULL)
#define AUDIO_BUFFER_OUT   ((int16_t*)NULL)
#if 0
#define  INFO_OVER_RUN_IN(flag)   if(gHandle->recognizerPipe.pipeFlags  & flag) AVS_TRACE_ERROR("DMA-IN over-run");
#define  INFO_OVER_RUN_OUT(flag) if(gHandle->synthesizerPipe.pipeFlags  & flag) AVS_TRACE_ERROR("DMA-OUT over-run");
#else
#define  INFO_OVER_RUN_IN(flag)
#define  INFO_OVER_RUN_OUT(flag)
#endif




#define SCRATCH_BUFF_SIZE  1024
static    int32_t          audioInScratchBuffer[SCRATCH_BUFF_SIZE];         /* Bsp scratch buffer */
static    AVS_audio_handle *gHandle;                          /* Mono instance global handle */
static Avs_sound_player      gWavFile;
static uint32_t              gMuteMicro = 0;
static AVS_Audio_Feed_CB    gAvs_Input_Feed_CB;
static AVS_Audio_Feed_CB    gAvs_Output_Feed_CB;



/* From BSP audio */
extern SAI_HandleTypeDef          haudio_out_sai;
//extern DFSDM_Filter_HandleTypeDef hAudioInTopLeftFilter;
//extern DFSDM_Filter_HandleTypeDef hAudioInTopRightFilter;
extern SAI_HandleTypeDef          haudio_in_sai;


static  void process_out_part(uint32_t part);
static  void process_out_part(uint32_t part)
{
  if(gAvs_Output_Feed_CB)
  {
    int16_t *pData;
    uint16_t nbSample  = (gHandle->synthesizerPipe.outBuffer.szBuffer / 2);
    uint16_t nbChannel  = gHandle->synthesizerPipe.outBuffer.nbChannel;

    uint32_t id         = 0;

    if(part == AVS_PIPE_PROCESS_PART2)
    {
      /* Complet */
      pData = &gHandle->synthesizerPipe.outBuffer.pBuffer[nbSample * nbChannel];

    }
    else
    {
      pData = gHandle->synthesizerPipe.outBuffer.pBuffer;
    }

    uint16_t nbPcmSample = nbSample * nbChannel;
    gAvs_Output_Feed_CB(&id, pData, &nbChannel, &nbPcmSample );
  }


}

static  void process_part(uint32_t part);
static  void process_part(uint32_t part)
{
  int16_t *pData;
  uint16_t nbSample  = (gHandle->recognizerPipe.inBuffer.szBuffer / 2);
  uint16_t nbChannel  = gHandle->recognizerPipe.inBuffer.nbChannel;

  uint32_t id         = 0;

  if(part == AVS_PIPE_PROCESS_PART2)
  {
    /* Complet */
    pData = &gHandle->recognizerPipe.inBuffer.pBuffer[nbSample * nbChannel];

  }
  else
  {
    pData = gHandle->recognizerPipe.inBuffer.pBuffer;
  }
  if(gMuteMicro)
  {
    memset(pData, 0, nbSample * sizeof(int16_t)*nbChannel);
  }

  if((gWavFile.flags & AVS_PLAYSOUND_ACTIVE)!= 0)
  {
    if(gWavFile.curCount == 0)
    {
      gWavFile.flags &= ~(uint32_t)AVS_PLAYSOUND_ACTIVE;
      return ;
    }
    int16_t *pBuffer = pData;
    register int32_t stereo = (nbChannel == 1) ? 0 : 1;

    if(nbSample >= gWavFile.curCount)
    {
      nbSample = gWavFile.curCount;
    }
    for(int32_t a = 0; a <  nbSample ; a++)
    {
      register int16_t smp = *gWavFile.pCurWave;
      gWavFile.pCurWave++;
      *pBuffer = smp ;
      pBuffer++;
      if(stereo)
      {
        *pBuffer = smp ;
        pBuffer++;
      }
    }
    gWavFile.curCount -= nbSample;
  }
  if(gAvs_Input_Feed_CB)
  {
    uint16_t nbPcmSample = nbSample * nbChannel;
    gAvs_Input_Feed_CB(&id, pData, &nbChannel, &nbPcmSample );
  }


}




uint32_t   platform_Audio_ioctl(AVS_audio_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam)
{

  if( code == AVS_IOCTL_MUTE_MICROPHONE)
  {
    avs_core_atomic_write(&gMuteMicro, wparam);

    return  AVS_OK;

  }


  if(code ==  AVS_IOCTL_AUDIO_FEED_CALLBACK)
  {
    if(wparam == AVS_FEED_INPUT_CALLBACK)
    {

      avs_core_atomic_write((uint32_t *)(uint32_t)&gAvs_Input_Feed_CB, lparam);
      return AVS_OK;
    }
  }
  if(code ==  AVS_IOCTL_AUDIO_FEED_CALLBACK)
  {
    if(wparam == AVS_FEED_OUTPUT_CALLBACK)
    {

      avs_core_atomic_write((uint32_t *)(uint32_t)&gAvs_Output_Feed_CB, lparam);
      return AVS_OK;
    }
  }



  if( code == AVS_IOCTL_INJECT_MICROPHONE)
  {

    Avs_sound_player  wav;
    if(avs_core_atomic_read((uint32_t *)(uint32_t)&gWavFile.curCount) != 0)
    {

      return AVS_BUSY;
    }
    if(avs_core_audio_wav_sound((void *)lparam, &wav) != AVS_OK)
    {
      AVS_TRACE_ERROR("Invalid Wav file");
      return AVS_ERROR;
    }
    /* Inject a wav in the microphone */
    if((wav.freqWave !=  pHandle->pFactory->freqenceIn) || (wav.chWave == 2))
    {
      AVS_TRACE_ERROR("Invalid Wav file");
      return AVS_ERROR;
    }
    uint32_t flags = wav.flags;
    memcpy(&gWavFile, &wav, sizeof(wav));
    flags |= AVS_PLAYSOUND_ACTIVE;
    /* Write to start safely the injection */
    avs_core_atomic_write(&gWavFile.flags, flags);
    /* Wait untel end ? */
    if(wparam)
    {
      /* If flg  != AVS_SOUND_PLAYER_ACTIVE injection finished */
      while((avs_core_atomic_read(&gWavFile.flags) & AVS_PLAYSOUND_ACTIVE) != 0)
      {
        avs_core_task_delay(10);
    }
    }

    return AVS_OK;


  }

  if(code == AVS_IOCTL_AUDIO_VOLUME)
  {
    if(wparam == 0 )
    {
      int32_t vol = (int)lparam;
      if(vol < 0)
      {
        vol = 0;
      }
      if(vol >= 100)
      {
        vol = 100;
      }
      gHandle->volumeOut = vol ;
//      BSP_AUDIO_OUT_SetVolume(gHandle->volumeOut);
      return AVS_OK;

    }

    if(wparam == 1 )
    {
      int32_t vol = (int)lparam;
      if((int)vol < -100)
      {
        vol = -100;
      }
      if(vol  >= 100)
      {
        vol = 100;
      }
      gHandle->volumeOut += vol;
      if(gHandle->volumeOut < 0)
      {
        gHandle->volumeOut = 0;
      }
      if(gHandle->volumeOut >= 100)
      {
        gHandle->volumeOut = 100;
      }
//      BSP_AUDIO_OUT_SetVolume(gHandle->volumeOut);
      return AVS_OK;

    }
    return gHandle->volumeOut;
  }
  return AVS_NOT_IMPL;
}



AVS_Result init_bsp_audio(AVS_audio_handle *pHandle);
AVS_Result init_bsp_audio(AVS_audio_handle *pHandle)
{

  uint8_t ret;
  /* We need to store the handle has gloable since the HAL sdk has no cookie to retrieve in callbacks */
  gHandle  = pHandle;

  /* Configure audio out peripheral */
//  ret = BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, pHandle->pFactory->initVolume, pHandle->pFactory->freqenceOut);
//  AVS_ASSERT((ret  == AUDIO_OK));
//  if(ret != AUDIO_OK)
//  {
//    AVS_TRACE_ERROR("Audio device !");
//
//    return AVS_ERROR;
//  }

  /* Force the channels to 2 because BSP fails if only 1 */
  pHandle->pFactory->chOut =  2;
  pHandle->pFactory->chIn  =  2;

  /* Configure audio in peripheral */
//  ret = BSP_AUDIO_IN_InitEx(INPUT_DEVICE_DIGITAL_MIC, pHandle->pFactory->freqenceIn, DEFAULT_AUDIO_IN_BIT_RESOLUTION, pHandle->pFactory->chIn);
//  AVS_ASSERT((ret == AUDIO_OK));
//  if(ret != AUDIO_OK)
//  {
//    AVS_TRACE_ERROR("Audio device !");
//    return AVS_ERROR;
//  }

  /* Allocate scratch buffer */
//  BSP_AUDIO_IN_AllocScratch(audioInScratchBuffer, sizeof(audioInScratchBuffer) / sizeof(int32_t));

  /* Set audio frame slot configuration */
//  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);

  /* Create the ring buffer for the mic and speaker */



  AVS_VERIFY(avs_audio_buffer_create(&pHandle->recognizerPipe.inBuffer, AUDIO_BUFFER_IN, AUDIO_IN_SIZE, pHandle->pFactory->chIn, pHandle->pFactory->freqenceIn)); /* Init audio micro to stream( out = stream , in = dma producer) */
  AVS_VERIFY(avs_audio_buffer_create(&pHandle->synthesizerPipe.outBuffer, AUDIO_BUFFER_OUT, AUDIO_OUT_SIZE, pHandle->pFactory->chOut, pHandle->pFactory->freqenceOut)); /* Init audio stream to speaker ( in = stream ; out = dma consumer) */
  if(gHandle->pInstance->pFactory->useAuxAudio)
  {
    AVS_VERIFY(avs_audio_buffer_create(&pHandle->auxAudioPipe.outBuffer, NULL, AUDIO_OUT_SIZE, pHandle->pFactory->chOut, pHandle->pFactory->freqenceOut));  /* Init audio stream audio mixed ( in = stream ; out = dma consumer) */
  }
//  ret = BSP_AUDIO_IN_Record((uint16_t *)(uint32_t)pHandle->recognizerPipe.inBuffer.pBuffer, pHandle->recognizerPipe.inBuffer.szBuffer * pHandle->recognizerPipe.inBuffer.nbChannel);
//  AVS_ASSERT((ret  == AUDIO_OK));
//  if(ret != AUDIO_OK)
//  {
//    AVS_TRACE_ERROR("Create audio device in!");
//    return AVS_ERROR;
//  }
//  ret = BSP_AUDIO_OUT_Play((uint16_t *)(uint32_t)pHandle->synthesizerPipe.outBuffer.pBuffer, pHandle->synthesizerPipe.outBuffer.szBuffer * pHandle->synthesizerPipe.outBuffer.nbChannel * sizeof(int16_t));
  
//  AVS_ASSERT((ret  == AUDIO_OK));
//  if(ret != AUDIO_OK)
//  {
//    AVS_TRACE_ERROR("Create Audio device output");
//    return AVS_ERROR;
//  }

  /* Audio peripheral is ready */
  return AVS_OK;
}

AVS_Result term_bsp_audio(AVS_audio_handle *pHandle);
AVS_Result term_bsp_audio(AVS_audio_handle *pHandle)
{
  AVS_VERIFY(avs_audio_buffer_delete(&pHandle->recognizerPipe.inBuffer));
  AVS_VERIFY(avs_audio_buffer_delete(&pHandle->synthesizerPipe.inBuffer));
//  BSP_AUDIO_OUT_DeInit();
//  BSP_AUDIO_IN_DeInit();

  return AVS_OK;

}

/**
 * @brief DMA2 Stream 0 interrupt handler ( audio in)
 **/
void AUDIO_DFSDMx_DMAx_TOP_LEFT_IRQHandler(void);
void AUDIO_DFSDMx_DMAx_TOP_LEFT_IRQHandler(void)
{
//  HAL_DMA_IRQHandler(hAudioInTopLeftFilter.hdmaReg);
}


/**
 * @brief DMA2 Stream 5 interrupt handler ( audio in)
 **/
void AUDIO_DFSDMx_DMAx_TOP_RIGHT_IRQHandler(void);
void AUDIO_DFSDMx_DMAx_TOP_RIGHT_IRQHandler(void)
{
//  HAL_DMA_IRQHandler(hAudioInTopRightFilter.hdmaReg);
}



/**
 * @brief DMA2 Stream 4 interrupt handler ( audio out)
 **/

void AUDIO_OUT_SAIx_DMAx_IRQHandler(void);
void AUDIO_OUT_SAIx_DMAx_IRQHandler(void)
{
  HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}


/**
 * @brief DMA half transfer complete interrupt handler
 **/

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
  /* Signal ISR the half part to inject */
#ifdef CHECK_OVER_RUN
  INFO_OVER_RUN_OUT(AVS_PIPE_PROCESS_PART1);
#endif
  process_out_part(AVS_PIPE_PROCESS_PART1);
  gHandle->synthesizerPipe.pipeFlags |= AVS_PIPE_EVT_HALF;
  avs_core_terminat_isr(avs_core_event_set_isr(&gHandle->synthesizerPipe.outEvent));
}


/**
 * @brief DMA full transfer complete interrupt handler
 **/

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  /* Signal ISR the last part to inject */
  /* Signal complet,w the first part is playing */
#ifdef CHECK_OVER_RUN


  INFO_OVER_RUN_OUT(AVS_PIPE_PROCESS_PART2);
#endif
  process_out_part(AVS_PIPE_PROCESS_PART2);
  gHandle->synthesizerPipe.pipeFlags &= (~(uint32_t)AVS_PIPE_EVT_HALF);
  avs_core_terminat_isr(avs_core_event_set_isr(&gHandle->synthesizerPipe.outEvent));
}




void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
  /* Signal ISR the half part to inject */
#ifdef CHECK_OVER_RUN
  INFO_OVER_RUN_IN(AVS_PIPE_PROCESS_PART1);
#endif
  gHandle->recognizerPipe.pipeFlags |= AVS_PIPE_EVT_HALF;
  process_part(AVS_PIPE_PROCESS_PART1);
  avs_core_terminat_isr(avs_core_event_set_isr(&gHandle->recognizerPipe.inEvent));

}

/**
 * @brief DMA full transfer complete interrupt handler
 **/
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
  /* Signal ISR the last part to inject */
#ifdef CHECK_OVER_RUN
  INFO_OVER_RUN_IN(AVS_PIPE_PROCESS_PART2);
#endif
  gHandle->recognizerPipe.pipeFlags &= ~(uint32_t)AVS_PIPE_EVT_HALF;
  process_part(AVS_PIPE_PROCESS_PART2);
  avs_core_terminat_isr(avs_core_event_set_isr(&gHandle->recognizerPipe.inEvent));
}




