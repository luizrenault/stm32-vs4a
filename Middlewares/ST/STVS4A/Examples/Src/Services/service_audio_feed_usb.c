/**
******************************************************************************
* @file    service_audio_feed_usb.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   manage the USB output
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
* Connect a board streams to the USB ( microphone)
*
******************************************************************************/


/*

Inject sample manually 

  It is the default mode, this mode supposes that you call the function USER_USB_CopyBuffer with the rights parameters and you push stereo samples block manually. This mode is used to debug audio streams and capture the audio wave from a buffer.

Inject sample automatically

  In this mode, the USB will be feed by a callback directly from the audio porting layer. In this mode you have just to define USB_CAPTURE_HOOK with an AVS_FEED_XXXX.  And the audio driver will feed the USB driver directly. There are 3 callbacks registered in the porting layer.

      AVS_FEED_INPUT_CALLBACK   The USB will be connected to the microphone , only the 1 first microphone will be used, the current implementation copy the Left sample in the right channel  
      AVS_FEED_OUTPUT_CALLBACK	The USB will be connected to the speaker at 48K stereo. 
      AVS_FEED_AEC_CALLBACK	If the porting layer has an echo cancellation, The USB will be connected to:  Channel Left audio coming from the microphone. Channel right, the microphone processed with the echo cancelation.
How to use this tool

  The first thing to do this to select the right frequency in the “STM32 Audio STVS4A in FS Mode” device property. If you capture the microhone or AEC, you have to select 16K otherwise 48K
  Then, you can use a tool such as Audacity (https://sourceforge.net/projects/audacity/) . Select the same sampling rate and the “STM32 Audio STVS4A in FS Mode” device.
  STVS4A will acts such as a microphone that you can record


*/



#include "service.h"

MISRAC_DISABLE
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_audio_in.h"
MISRAC_ENABLE



#define   NB_BUFFER_SAMPLE              (16*1024)       /* for 48K we need to support a bigger buffer */
#define   STREAM_STARTED                1
#define   DEFAULT_VOLUME_DB_256         0
#define   MAX_SIZE_BLK_SAMPLE           512

#define USB_NB_CHANNELS                    2            /* Warning, only this constant works */
/*
#define  USB_CAPTURE_HOOK                AVS_FEED_INPUT_CALLBACK             // capture Mics 16K
#define  USB_CAPTURE_HOOK                AVS_FEED_OUTPUT_CALLBACK              // capture Speaker 
#define  USB_CAPTURE_HOOK                AVS_FEED_AEC_CALLBACK                 // capture AEC 16K
*/


extern PCD_HandleTypeDef        hpcd;


typedef void (*AVS_FEED_CB)(uint32_t *pIdStream, void *pData, uint16_t *NbOutChannels, uint16_t *NbOutPcmSamples);

typedef struct  t_audio_buffer
{
  uint8_t      *pBuffer;              /* Ring  buffer */
  uint32_t      szBuffer;              /* Size buffer */
  int32_t       szConsumer;           /* Nb element in available in the ring buffer */
  uint32_t      prodPos;              /* Write  position */
  uint32_t      consumPos;            /* Read position */
} audio_buffer_t;


static USBD_HandleTypeDef       USBD_Device;
static record_instance_t        *gInstance;
static int16_t                  usbAudioData[2 * MAX_SIZE_BLK_SAMPLE ];
static audio_buffer_t           usbTxBuffer;
static uint8_t                  mute;
static int16_t                  audio_volume_db_256;
static uint32_t                 audio_new_frequency;
static uint32_t                 audio_old_frequency;
static uint32_t                 tFreq[]={16000,48000};

/*


 Create a ring buffer object



*/
__STATIC_INLINE  uint32_t audio_buffer_Create(audio_buffer_t *pHandle, int32_t nbbytes);
__STATIC_INLINE  uint32_t audio_buffer_Create(audio_buffer_t *pHandle, int32_t nbbytes)
{
  memset(pHandle, 0, sizeof(*pHandle));
  pHandle->szBuffer = nbbytes;
  pHandle->pBuffer = (uint8_t*)USBD_malloc(nbbytes);
  AVS_ASSERT(pHandle->pBuffer);
  /* RAZ by default */
  memset(pHandle->pBuffer, 0, nbbytes);

  return 1;
}

/*

 Delete the ring buffer


*/
__STATIC_INLINE uint32_t audio_buffer_Delete(audio_buffer_t *pHandle);
__STATIC_INLINE uint32_t audio_buffer_Delete(audio_buffer_t *pHandle)
{
  USBD_free(pHandle->pBuffer);
  return 1;
}


/*

 Some ring buffer operations


*/
__STATIC_INLINE uint8_t *audio_buffer_GetConsumer(audio_buffer_t *pHandle);
__STATIC_INLINE uint8_t *audio_buffer_GetConsumer(audio_buffer_t *pHandle)
{
  return &pHandle->pBuffer[pHandle->consumPos  % pHandle->szBuffer];
}

/*

 Move the ring buffer pointer


*/
__STATIC_INLINE void     audio_buffer_MovePtrConsumer(audio_buffer_t *pHandle, uint32_t offset, uint8_t **ppBuffer);
__STATIC_INLINE void     audio_buffer_MovePtrConsumer(audio_buffer_t *pHandle, uint32_t offset, uint8_t **ppBuffer)
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


/* Injects a memory block in the ring buffer */
static uint32_t sendUsb( record_instance_t *pInstance, void  *pData, uint16_t nbSamples);
static uint32_t sendUsb( record_instance_t *pInstance, void  *pData, uint16_t nbSamples)
{
  uint32_t curSize = 0;
  uint8_t *pBuffer = pData;
  if(!pInstance->recState)
  {
    return (uint32_t)-1;
  }
  audio_buffer_t *pHandle = &usbTxBuffer;
  uint32_t nbBytes = nbSamples*2; /* Sample = 2 bytes */
  /* Check the remaining data in the ring buffer */
  if(pHandle->szConsumer + nbBytes >= pHandle->szBuffer)
  {
    nbBytes = pHandle->szBuffer - pHandle->szConsumer;
  }
  if(pHandle->szConsumer + nbBytes <= pHandle->szBuffer)  /* Check if the buffer is too large */
  {
    if(pHandle->prodPos + nbBytes >= pHandle->szBuffer) /* Check if we will reach the buffer end */
    {
      /* Nb element before the end of the buffer */
      uint32_t nbElem = (pHandle->szBuffer - pHandle->prodPos);
      /* Fill the remaining space */
      if(pBuffer)
      {
        memcpy((char_t*)(uint32_t)&pHandle->pBuffer[pHandle->prodPos ], pBuffer, nbElem);
        pBuffer += nbElem;
      }

      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbElem ;
      pHandle->prodPos = 0;
      nbBytes -= nbElem ;
      curSize += nbElem ;
    }
    if(nbBytes)
    {
      AVS_ASSERT(pHandle->prodPos + nbBytes < pHandle->szBuffer); /* Must not cross the end (bug) */
      if(pBuffer)
      {
        memcpy((char_t *)(uint32_t)&pHandle->pBuffer[ pHandle->prodPos ], pBuffer, nbBytes);
      }
      /* Update size, pos and remaining elements */
      pHandle->szConsumer += nbBytes ;
      pHandle->prodPos += nbBytes;
      curSize += nbBytes ;
    }

  }
  if( (curSize != nbBytes) && (pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_OVERRRUN, 0, 0);
  }
  return curSize;
}



/* Extract  2 channels from the stream and copy in the buffer */
static void copyStream2Stereo(int32_t streamIndex, int16_t *pDst, int16_t *pSrc, uint32_t nbSample, uint32_t nChannel);
static void copyStream2Stereo(int32_t streamIndex, int16_t *pDst, int16_t *pSrc, uint32_t nbSample, uint32_t nChannel)
{
  /* Move to the asr ref stream */
  pSrc += streamIndex;
  for(int32_t a = 0; a < nbSample ; a++)
  {
    *pDst  = pSrc[0];
    pDst++;
    if(nChannel < 2)
    {
      /* If the number of channels is not at least stereo, copy ch0 in ch1 */
      *pDst  = pSrc[0];
      pDst++;
    }
    else
    {
#if   USB_NB_CHANNELS == 2
      *pDst  = pSrc[1];
      pDst++;
#endif
    }
    pSrc += nChannel;
  }
}


/* Feed  callback, format a stereo tracks and feed the USB */

void usb_feed_input_cb(uint32_t *pIdStream, void *pData, uint16_t *NbOutChannels, uint16_t *NbOutPcmSamples);
void usb_feed_input_cb(uint32_t *pIdStream, void *pData, uint16_t *NbOutChannels, uint16_t *NbOutPcmSamples)
{
  static uint32_t indexStream = 0;

  if(*pIdStream == indexStream)
  {


    uint16_t nbchannels  = NbOutChannels[indexStream];
    int32_t nbSample =0;
    if(nbchannels)
    {
      nbSample    = NbOutPcmSamples[indexStream] / nbchannels;
    }
    if(nbchannels ==2)
    {
      /* Inject directly because the stream has 2 channels */
      
      sendUsb(gInstance, pData, nbSample * USB_NB_CHANNELS);
    }
    else
    {
      uint32_t nb =  nbSample;
      while(nb )
      {
        int32_t blk = MAX_SIZE_BLK_SAMPLE;
        if(blk  >= nb)
        {
          blk = nb ;
        }
        copyStream2Stereo(indexStream, usbAudioData, pData, blk, nbchannels);
        /* Audio_inject_wave(usbAudioData,blk,  2); debug inject a wave */
        /* Copy the stereo stream in the USB */
        sendUsb(gInstance, usbAudioData, blk * USB_NB_CHANNELS);
        pData = ((uint8_t*)(uint32_t)pData) + blk * nbchannels * sizeof(uint16_t);
        nb -= blk;

      }
    }
  }
}


/*

 Makes the link with the AFE debug and check tools


*/
void     USER_USB_CopyBuffer (void *pBuff, uint32_t nbCh, uint32_t nbSample, int32_t type);
void     USER_USB_CopyBuffer (void *pBuff, uint32_t nbCh, uint32_t nbSample, int32_t type)
{
#ifndef USB_CAPTURE_HOOK
  uint32_t id = 0;
  uint16_t ch = (uint16_t)nbCh;
  uint16_t smp = (uint16_t)nbSample;
  usb_feed_input_cb(&id, pBuff,&ch, &smp);
#endif
}


/*

 callbak notifier from the USB, manage the control


*/
static uint32_t usbNotifyState(record_instance_t *pInstance, usbd_audio_in_evt_t evt, uint32_t wparam, uint32_t lparam);
static uint32_t usbNotifyState(record_instance_t *pInstance, usbd_audio_in_evt_t evt, uint32_t wparam, uint32_t lparam)
{
  switch(evt)
  {
  case AUDIO_SET_FREQ:
    {
      audio_new_frequency =  wparam;
      audio_old_frequency =  lparam;
      AVS_TRACE_INFO("USB frequency changed from %d to %d",audio_old_frequency,audio_new_frequency);
      break;
    }
    
    case AUDIO_INIT:
    {
      audio_volume_db_256 = DEFAULT_VOLUME_DB_256;

      break;

    }
    case AUDIO_TERM:
    {
      /* Delete the ring buffer */
      AVS_VERIFY(audio_buffer_Delete(&usbTxBuffer));
      break;
    }

    case AUDIO_SET_VOLUME:
    {
      /* Set volume */
      VOLUME_USB_TO_DB_256((audio_volume_db_256), ((int32_t)wparam));
      break;
    }

    case AUDIO_GET_VOLUME:
    {
      /* Get  volume */
      VOLUME_DB_256_TO_USB(*((int16_t *)wparam), audio_volume_db_256);
      break;
    }


    case AUDIO_SET_MUTE:
    {
      /* Set mute */
      mute = wparam;
      break;
    }

    case AUDIO_GET_MUTE:
    {
      /* Set mute */
      *((uint16_t *)wparam) = mute;
      break;
    }
  default:
      break;
  }
  return 0;
}

/*

  return the RAW bytes coming from the input * packet_length must be updated with the size injected

 note Depending on the calling frequency, a coherent amount of samples must be passed to
       the function. E.g.: assuming a Sampling frequency of 16 KHz and 1 channel,
       you can pass 16 PCM samples if the function is called each millisecond,
       32 samples if called every 2 milliseconds and so on.


*/
static uint8_t* usbGetBuffer(record_instance_t *pInstance, uint16_t* packet_length);
static uint8_t* usbGetBuffer(record_instance_t *pInstance, uint16_t* packet_length)
{

  *packet_length = 0;
  if((pInstance->recState) && (pInstance->hValid))
  {
    if((pInstance->state & STREAM_STARTED)==0)
    {
      if(usbTxBuffer.szConsumer > usbTxBuffer.szBuffer / 2)
      {
        pInstance->state |= STREAM_STARTED;
      }
    }
    else
    {
      int32_t nbBytes = pInstance->packet_length;
      if(nbBytes  >= usbTxBuffer.szConsumer)
      {
        nbBytes = usbTxBuffer.szConsumer;
        memset(&pInstance->buff, 0, pInstance->packet_length);
        if((nbBytes != 0) && (pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
        {
          pInstance->pInterface->notifyEvt(pInstance, AUDIO_UNDERRUN, 0, 0);
        }
      }
      if(usbTxBuffer.szConsumer + nbBytes > usbTxBuffer.szBuffer)
      {
        if((nbBytes != 0) && (pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
        {
          pInstance->pInterface->notifyEvt(pInstance, AUDIO_OVERRRUN, 0, 0);
        }
      }
      uint8_t *pSrc = audio_buffer_GetConsumer(&usbTxBuffer);
      uint8_t *pDest = pInstance->buff;
      for(int32_t a = 0; a <  nbBytes ; a++)
      {
        *pDest = *pSrc;
        pDest++;
        audio_buffer_MovePtrConsumer(&usbTxBuffer, 1, &pSrc);
      }
      *packet_length = pInstance->packet_length;
      return pInstance->buff;
    }
  }
  return pInstance->buff;

}


/* Declare the top level interface */
static usbd_audio_in_if_t usbIf =
{
  .notifyEvt = usbNotifyState,
  .getBuffer = usbGetBuffer
};


/* Initialize the USB service, initialize the USB device and connect the callback to the USB feed */

AVS_Result service_audio_feed_usb_create(AVS_Handle handle);
AVS_Result service_audio_feed_usb_create(AVS_Handle handle)
{

  AVS_Result ret = AVS_OK;
#ifdef USB_CAPTURE_HOOK
  ret = (AVS_Result )AVS_Ioctl(handle, AVS_IOCTL_AUDIO_FEED_CALLBACK, USB_CAPTURE_HOOK, (uint32_t)(AVS_FEED_CB)usb_feed_input_cb);
#endif

  if( ret == AVS_OK)
  {
    
    /* Allocate the ring buffer */
    AVS_VERIFY(audio_buffer_Create(&usbTxBuffer, NB_BUFFER_SAMPLE * sizeof(int16_t)));

    /* Create the input interface */
    gInstance = USBD_Audio_Input_Create(tFreq,(sizeof(tFreq)/sizeof(tFreq[0])),USB_NB_CHANNELS,&usbIf );
    AVS_ASSERT(gInstance );

    /* Init Device Library */
    USBD_Init(&USBD_Device, &AUDIO_Desc, 0);
    /* Add Supported Class */
    USBD_RegisterClass(&USBD_Device, USBD_AUDIO_CLASS);
    /* Add Interface callbacks for AUDIO Class */
    USBD_AUDIO_RegisterInterface(&USBD_Device, &audio_class_interface);
    /* Start Device Process */
    USBD_Start(&USBD_Device);
  }


  return AVS_OK;
}

/* Manage the  USB IRQ */

void USB_IN_IRQHandler(void);
void USB_IN_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd);
}

void USBD_error_handler(void)
{
  AVS_Signal_Exeception(NULL, AVS_SIGNAL_EXCEPTION_GENERAL_ERROR);
}



