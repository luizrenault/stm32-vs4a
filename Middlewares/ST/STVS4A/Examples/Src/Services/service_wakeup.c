/**
******************************************************************************
* @file    service_wakeup.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   wakeup and button management
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

/*

  Tmp Src to manage push button and push to talk wakeup ect..

*/

#define BNT_TIME_SHORT  60
#define BNT_RELEASE    1
#define BNT_SET_SHORT  2
#define BNT_SET_LONG   3





#define TASK_NAME_WAKEUP              "AVS:Task wakeup"
#define TASK_NAME_PRIORITY_WAKEUP     (osPriorityIdle)
#define TASK_STACK_WAKEUP             100

static task_t *        hBntTask;
const void *              pSoundWav;
extern uint32_t           bOutputDebugWave;   /* Debug purpose, enable the tone production on the speake if true */



/* Check the button state and returns if the button is released or if it is a long push or short push */
static uint32_t GetButtonState(void);
static uint32_t GetButtonState(void)
{
  static uint32_t bntCount;
  if(HAL_GPIO_ReadPin(BTN_USER_GPIO_Port, BTN_USER_Pin) != GPIO_PIN_SET)
  {
    if((bntCount)  && (bntCount < BNT_TIME_SHORT))
    {
      bntCount = 0;
      return BNT_SET_SHORT;
    }
    bntCount = 0;
    return BNT_RELEASE;
  }
  bntCount++;
  if(bntCount > BNT_TIME_SHORT)
  {
    return BNT_SET_LONG;
  }
  return BNT_RELEASE;

}


static void service_wakeup_task(const void *pCookie);
static void service_wakeup_task(const void *pCookie)
{
  while(1)
  {
    osDelay(10);

    if(GetButtonState() == BNT_SET_SHORT)
    {
#ifdef AFE_CONF_AEC_BUFFER_PROBE
      /* Tool to Debug/tune  AEC delay between speaker and microphone */
      /* Wait 2 secs to remove the click of the button on the speaker, then enable the tone production on the speaker */
      /* AEC debug and tuning */
      osDelay(2000);
      bOutputDebugWave = (bOutputDebugWave + 1) & 1;
#endif
      service_alarm_snooze();
    }

    if (sInstanceFactory.initiator == AVS_INITIATOR_PUSH_TO_TALK )
    {
      /* Don't block the CB to leave the UI display change state */
      static int32_t buttonState = FALSE;
      if(!buttonState)
      {
        /* The capture starts when the button is pushed */
        /* And stopped when the button is released */
        int32_t eRet = AVS_Get_State(hInstance) ;
        if( (GetButtonState() == BNT_SET_LONG) &&  (eRet  == AVS_STATE_IDLE) )
        {
          AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY, (void *)(uint32_t)pSoundWav, 100);

          /* Starts the capture only if northing occurs on the system ( ie BUSY etc...) */
          if(AVS_Set_State(hInstance, AVS_STATE_START_CAPTURE) == AVS_OK)
          {
            /* Capture started and button pressed */
            buttonState = TRUE;
          }
        }
      }
      if(buttonState)
      {
        /* Now wait the release of the button */
        if(GetButtonState() == BNT_RELEASE)
        {
          /* Now try to stop the capture */
          if( AVS_Set_State(hInstance, AVS_STATE_STOP_CAPTURE) == AVS_OK)
          {
            /* Good the capture is safely stopped */
            buttonState =  FALSE;
          }
        }

      }
    }
    if (sInstanceFactory.initiator != AVS_INITIATOR_PUSH_TO_TALK )
    {
      /* The capture starts when the button is pushed */
      /* And will be stopped by a cloud directive */
      /* Starts the capture only nothing occurs on the system ( ie BUSY etc...) */
      uint32_t sRet = AVS_Get_State(hInstance);
      if((GetButtonState() == BNT_SET_LONG)  && (sRet  == AVS_STATE_IDLE) )
      {
        while(AVS_Set_State(hInstance, AVS_STATE_START_CAPTURE) != AVS_OK) 
        {
          osDelay(100);
        }
        AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY, (void *)(uint32_t)pSoundWav, 100);
      }

    }

  }
}

/* Create the wakeup service */
AVS_Result service_wakeup_create(AVS_Handle hHandle)
{

  pSoundWav = service_assets_load("drip_echo_mono_16K_wav", 0, 0);
  AVS_ASSERT(pSoundWav );
  hBntTask = task_Create(TASK_NAME_WAKEUP,service_wakeup_task,NULL,TASK_STACK_WAKEUP,TASK_NAME_PRIORITY_WAKEUP);
  if(hBntTask  == 0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_WAKEUP);
    return AVS_ERROR;
  }
  return AVS_OK;
}


/* Event callback for alarm */
AVS_Result service_wakeup_event_cb(AVS_Handle hInst, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  switch(evt)
  {
    case EVT_WAKEUP:
    {
      if(sInstanceFactory.initiator == AVS_INITIATOR_VOICE_INITIATED)
      {
        /* No wake up if not AVS_INITIATOR_VOICE_INITIATED */
        /* Start the capture */
        AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY, (void *)(uint32_t)pSoundWav, 100);
        AVS_Set_State(hInstance, AVS_STATE_START_CAPTURE);

      }
    }
    break;

    case EVT_STOP_CAPTURE:
    {
      /* Stop capture */
      AVS_Set_State(hInstance, AVS_STATE_STOP_CAPTURE);
      break;
    }


    case EVT_CHANGE_MUTE:
    {
      const void *pBip = service_assets_load("ding_notification_sound_wav", 0, 0);

      /* In this test app , we don't mute completely Alexa, for mute means we reduce the player volume to 0 and leave alxa speaking */
      /* But it is possible to use AVS_Audio_Mute to mute completely Alexa */

      if((pparam == 1) && (pBip))
      {
        /* Muted  bip once */
        AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY, (void *)(uint32_t)pBip, 100);
        service_player_volume_max(hInstance, 0); /* Mute audio player */
      }
      if((pparam == 0) && (pBip))
      {
        /* Un-muted  bip twice */
        service_player_volume_max(hInstance, 100); /* Un-mute audio player */
        AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY | AVS_PLAYSOUND_WAIT_END, (void *)(uint32_t)pBip, 100);
        AVS_Play_Sound(hInstance, AVS_PLAYSOUND_PLAY, (void *)(uint32_t)pBip, 100);
      }


      service_assets_free(pBip);

      break;


    }


    default:
      break;
  }
  return AVS_OK;
}
