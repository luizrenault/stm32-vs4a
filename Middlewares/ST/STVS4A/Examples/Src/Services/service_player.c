
/******************************************************************************
* @file    service_player.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   manage the audio player
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
#undef close   /* Prevent an API clash with anoter function */

/* USE_PAUSE_RESUME   Enable pause and resume when alexa speaks */

#define BASE_YEAR_EPOCH                1900
#define PLAYER_PERIOD_PRECISION         500
#define PLAYER_PERIOD_STOP              2000
#define MAX_SIZE_TOKEN                  350
#define FORMAT_MESSAGE_ID               "Player-%lu"
#define MAX_PLAYER_ENTRY                6
#define PLAYER_TASK_STACK_SIZE          100
#define TASK_NAME_PLAYER               "AVS:Player Idle"
#define TASK_PRIORITY_PLAYER           (osPriorityAboveNormal)
#define NB_CHANNEL_MP3                  2
#define PAYLOAD_SIZE                   (10*1024)                /* 10K reprents at 192 kb/s ~0.5 secs of latency */
#define VOL_MIN                        4                        /* Volume when alexa speaks */
#define VOL_MAX_DEFAULT                100                      /* Normal audio player volume */
#define FADE_STEP                      3                        /* Max fade in 1 sec */
#define MAX_CODEC                      5
#define PLAYER_TIMEOUT                 1000

#define SIZE_STREAM_BUFFER              (442)
#define IDLE_SLEEP_TIME                 100
typedef enum 
{
  PLAYER_STATE_IDLE = 0,   /* Player is booting */
  PLAYER_STATE_STOP,      /* The player is armed but wait for a playlist */
  PLAYER_STATE_OPEN,      /* The player  is opening its source */
  PLAYER_STATE_CHECK_CODEC, /* Check the media codec */
  PLAYER_STATE_BUFFERING,   /* Buffering */
  PLAYER_STATE_PLAY,      /* The player  is pulling data */
  PLAYER_STATE_CLOSE,     /* The player  is closing the playback normaly*/
  PLAYER_STATE_PAUSE,     /* The player  is waiting for a resume */
  PLAYER_STATE_UNDERRUN   /* The player is buffering */
} service_player_playerState;


static player_codec_t                   *tCodec[MAX_CODEC];                     /* Codec registry */
static uint32_t                         nbCodec = 0;                            /* Nb codec */
static int32_t                          curCodec = -1;                          /* Current codec playing */
static player_reader_t                  reader;                                 /* Current mass storage reader, Https, File system, else .... */
static mutex_t                 lockList;                              			/* General lock */
static mutex_t                 nextLock  ;                            			 /* Lock when we move to the next playback */
struct t_player_context                 playerContext;                          /* Playback context instance */
static task_t *                      hPlayerTask;                            /* Main player task handle */
static player_slot_t                    *tPlayer[MAX_PLAYER_ENTRY];             /* Slots for an player */
static uint32_t                         messageIdCounter;                       /* Message id index */
static uint32_t                         statePlayer;                            /* State player */
static uint32_t                         nbPlayerItem;                           /* Nb item in the play list */
static uint8_t                          *pPullBuffer;                           /* Small buffer receiving pulled data */
static uint32_t                         szPullBuffer = SIZE_STREAM_BUFFER;      /* Max size PullBuffer */
static uint32_t                         bPlayerRun;                             /* Keep alive task */
static int32_t                         iCurVol;                                 /* Current volume */
static int32_t                         iTargetVol;                              /* Target volume for fades */
static int32_t                         iMaxVol;                                 /* Current volume */
static uint32_t                         bForceFS = 0;                           /* Force File System streaming for debug */
bytes_buffer_t                          gMp3Payload;                            /* Payload ring buffer */
static uint32_t                         bHardClosed = 0;                        /* true if the stream has been stopped */



/* Specific lock lock */
__STATIC_INLINE void service_player_lock_next(void);
__STATIC_INLINE void service_player_lock_next(void)
{
  mutex_Lock(&nextLock);

}
/* Specific lock unlock */
__STATIC_INLINE  void service_player_unlock_next(void);
__STATIC_INLINE  void service_player_unlock_next(void)
{
  mutex_Unlock(&nextLock);
}

/* Specific lock the playlist */
__STATIC_INLINE  void service_player_lock_list(void);
__STATIC_INLINE  void service_player_lock_list(void)
{
  mutex_Lock(&lockList);

}

/* Specific unlock the playlist */
__STATIC_INLINE  void service_player_unlock_list(void);
__STATIC_INLINE  void service_player_unlock_list(void)
{
  mutex_Unlock(&lockList);
}




uint32_t  service_player_get_buffer_percent(AVS_Handle handle)
{

  uint32_t percent = 0;
  if(gMp3Payload.szBuffer)
  {
    percent = (gMp3Payload.szConsumer*100)/gMp3Payload.szBuffer;
  }

  return percent ;
}


/* Reset the volume */

void service_player_volume_max(AVS_Handle handle, uint32_t max)
{
  iMaxVol = max;
  iTargetVol = max;
}

/* Add the codec to the list */
static AVS_Result service_player_add_media(player_codec_t *pMedia);
static AVS_Result service_player_add_media(player_codec_t *pMedia)
{
  if(nbCodec >= MAX_CODEC) 
  {
    return AVS_ERROR;
  }
  tCodec[nbCodec] = pMedia;
  nbCodec++;
  return AVS_OK;
}

/* Returns the current codec */
static player_codec_t *service_player_audio_get_codec(void);
static player_codec_t *service_player_audio_get_codec(void)
{
  AVS_ASSERT(curCodec != -1);
  return tCodec[curCodec];
}

/* Check if the codec recognize its format */
int32_t service_player_audio_codec_check(void);
int32_t service_player_audio_codec_check(void)
{
  for(int32_t a = 0; a < nbCodec ; a++)
  {
    if(tCodec[a]->check_codec(&playerContext, gMp3Payload.pBuffer, gMp3Payload.szConsumer) == AVS_OK) 
    {
      return a;
    }
  }
  return -1;
}


/* Find an entry  for an token and return its index */
int32_t service_player_find(const char_t *pToken);
int32_t service_player_find(const char_t *pToken)
{
  for(int32_t index = 0; index  < nbPlayerItem; index++)
  {
    if(strcmp(pToken, tPlayer[index]->audioItem_stream_token) == 0)   
    {
      return index;
    }
  }
  return -1;
}

/* Return the item number in the play list */
static uint32_t service_player_get_nb_item(void);
static uint32_t service_player_get_nb_item(void)
{
  return service_player_atomic_read(&nbPlayerItem);
}

/* Check if the player is active */
uint32_t  service_player_is_active(void);
uint32_t  service_player_is_active(void)
{
  if((service_player_get_nb_item() == 0) && (playerContext.pItemPlaying == 0)) 
  {
    return 0;
  }
  return 1;
}
/* Free allocated items */
static void service_player_free_item(player_slot_t *pSlot);
static void service_player_free_item(player_slot_t *pSlot)
{

  if(pSlot ==0) 
  {
    return;
  }
  if(pSlot->audioItem_audioItemId)         
  {
    vPortFree(pSlot->audioItem_audioItemId);
  }
  if(pSlot->audioItem_stream_url)          
  {
    vPortFree(pSlot->audioItem_stream_url);
  }
  if(pSlot->audioItem_stream_token)        
  {
    vPortFree(pSlot->audioItem_stream_token);
  }
  if(pSlot->audioItem_stream_streamFormat) 
  {
    vPortFree(pSlot->audioItem_stream_streamFormat);
  }
  vPortFree(pSlot);
}

/* Clear the player context */
static void TermPlayerContext(void);
static void TermPlayerContext(void)
{
  service_player_lock_list();
  service_player_free_item(playerContext.pItemPlaying);
  playerContext.pItemPlaying = 0;
  service_player_unlock_list();
}

/*
    Resume the stream
*/
static void service_player_resume(AVS_Handle instance);
static void service_player_resume(AVS_Handle instance)
{
  if(playerContext.pItemPlaying==0)    
  {
    return;
  }
  if(playerContext.pItemPlaying->flags.playing==0) 
  {
    return;
  }
  if(playerContext.pItemPlaying->flags.pause==0) 
  {
    return ;
  }
  /* if a stop is posted , ignore the resume */
  if(bHardClosed)        
  {
    return;
  }
  if(service_player_atomic_read(&statePlayer) != PLAYER_STATE_PAUSE)
  {
    AVS_TRACE_ERROR("the state should be  PAUSED ( %d)  ",statePlayer);
  }
  playerContext.pItemPlaying->flags.pause = 0;
  service_player_atomic_write(&statePlayer, PLAYER_STATE_PLAY);
  service_player_event(instance, "PlaybackResumed");
  service_player_audio_send(hInstance, EVT_PLAYER_RESUMED, 0);
}


/*
    Stop the stream without to close the stream
*/
static void service_player_pause(AVS_Handle instance);
static void service_player_pause(AVS_Handle instance)
{
  if(playerContext.pItemPlaying==0)    
  {
    return;
  }
  if(playerContext.pItemPlaying->flags.pause) 
  {
    return ;
  }
  /* at this pint we must be in playing mode */

  AVS_ASSERT(playerContext.pItemPlaying->flags.playing);
  /* but not sure we are playing ( buffering , codec etc....), so wait this state */
  uint32_t timeout=3000/100; /* wait this state max n secs */
  while(timeout)
  {
    if(service_player_atomic_read(&statePlayer) == PLAYER_STATE_PLAY) 
    {
      break;
    }
    osDelay(100);
    timeout--;
  }
  AVS_ASSERT(timeout);
  playerContext.pItemPlaying->flags.pause = 1;
  /* we can safely turn on pause */
  service_player_atomic_write(&statePlayer, PLAYER_STATE_PAUSE);
  /* notify Alexa */
 service_player_event(instance, "PlaybackPaused");
  service_player_audio_send(hInstance, EVT_PLAYER_PAUSED, 0);
}

void service_player_change_state(AVS_Handle instance,uint32_t state);
 void service_player_change_state(AVS_Handle instance,uint32_t state)
 {
   /* Check if the player is playing*/
   if(playerContext.pItemPlaying ==0) 
   {
     return;
   }
   if((state == AVS_STATE_IDLE) && (playerContext.pItemPlaying->flags.pause))
   {
     service_player_resume(instance);
     return;
   }
   if((state == AVS_STATE_START_CAPTURE) && (playerContext.pItemPlaying->flags.pause==0))
   {
     service_player_pause(instance);
     return;
   }
 }


/* Initialize player context */
static void InitPlayerContext(player_slot_t *pSlot);
static void InitPlayerContext(player_slot_t *pSlot)
{
  /* Must always be free */
  AVS_ASSERT(playerContext.pItemPlaying == 0 );
  if(playerContext.pItemPlaying) 
  {
    TermPlayerContext();
  }
  playerContext.pItemPlaying = pSlot;
}




/* Remove an entry from the list */
static AVS_Result  service_player_remove_item(uint32_t index, uint32_t bFree);
static AVS_Result  service_player_remove_item(uint32_t index, uint32_t bFree)
{
  if(nbPlayerItem == 0)
  {
    AVS_TRACE_ERROR("Item is playing");
    return AVS_ERROR;
  }
  service_player_lock_list();
  /* Free the slot before to delete */
  if(bFree) 
  {
    service_player_free_item(tPlayer[index]);
  }
  /* Fill the gap in the play-list */
  for(int32_t a = index; a < nbPlayerItem - 1 ; a++)
  {
    tPlayer[index] = tPlayer[index + 1];
  }
  /* Update the item number */
  nbPlayerItem--;
  service_player_unlock_list();

  return AVS_OK;
}

/* Insert an item in the play list */
static AVS_Result service_player_insert_item(uint32_t index, player_slot_t *pSlot);
static AVS_Result service_player_insert_item(uint32_t index, player_slot_t *pSlot)
{
  /* Check limiters */
  if(nbPlayerItem + 1 >=  MAX_PLAYER_ENTRY)  
  {
    return AVS_ERROR;
  }

  service_player_lock_list ();
  /* Move backward and create the gap in the play-list */
  for(int32_t a = nbPlayerItem ; a != index ; a--)
  {
    tPlayer[a + 1] = tPlayer[a];
  }
  tPlayer[index] = pSlot;
  nbPlayerItem++;
  service_player_unlock_list ();
  return AVS_OK;
}


/* Delete all items the queue */
static void service_player_clear_items(void);
static void service_player_clear_items(void)
{
  service_player_lock_list ();

  for(int32_t a = 0 ; a < nbPlayerItem ; a++)
  {
    service_player_free_item(tPlayer[a]);

  }
  nbPlayerItem = 0;
  service_player_unlock_list ();
}


/* Force to pause  the player */
static AVS_Result service_player_stop(AVS_Handle instance);
static AVS_Result service_player_stop(AVS_Handle instance)
{
  bHardClosed = 1;
  /* If already stopped it is OK */
  if(playerContext.pItemPlaying==0)    
  {
    return AVS_OK;
  }
  if(playerContext.pItemPlaying->flags.playing ==0)    
  {
    return AVS_OK;
  }
  /* Close the current playback */
  service_player_atomic_write(&statePlayer, PLAYER_STATE_CLOSE);
  return AVS_OK;
}

/* Check and execute the next pending entry */

static AVS_Result  service_player_next(void);
static AVS_Result  service_player_next(void)
{
  AVS_Result ret = AVS_ERROR;

  /* If the player is stopped , we can evaluate the play list and */
  /* Check if we have a title to start */

  /* Can seek next if we arte playing */

  if(playerContext.pItemPlaying)  
  {
    return ret;
  }
  service_player_lock_next();

  while(service_player_get_nb_item())
  {
    AVS_TIME curtime;
    AVS_Get_Sync_Time(hInstance, &curtime);
    /* Check if the time is ok */
    /* The URI stays active until this time limit */
    if(curtime > tPlayer[0]->audioItem_stream_expiryTime )
    {
      /* The link has expired, we can't play it, so remove it, and pass to the next one */
      service_player_remove_item(0, TRUE);
      continue;
    }

    if((uint32_t)tPlayer[0]->audioItem_stream_streamFormat != 0U)
    {
      if  ((uint32_t)strcmp(tPlayer[0]->audioItem_stream_streamFormat, "AUDIO_MPEG")  != 0U)
      {
      /* Format not supported */
      /* Normally this field is useless , since the media player identify by itself the format */
      service_player_remove_item(0, TRUE);
      }
      continue;
    }



    /* The play-list is played from the top to the bottom */
    /* Let's evaluate the index 0 since we are sure there is an item in the list */

    if((tPlayer[0]->flags.network != 0)  && (bForceFS==0))     /* Today the stream network is not implemented, we debug this app using file system fake only */
    {
      /* Ok we have a network stream to play */
      /* Initialize an http reader */
      service_audio_read_http_create(&reader);

      /* The player must be stopped */
      AVS_ASSERT( statePlayer == PLAYER_STATE_STOP);

      /* We can transition from stopped to open and signal playing */
      InitPlayerContext(tPlayer[0]);
      service_player_remove_item(0, FALSE);

    }
    else
    {
      if(bForceFS == 1)
      {
        /* If we force the FS we replace by an existing ressource in the FS */
        vPortFree(tPlayer[0]->audioItem_stream_url);
        tPlayer[0]->audioItem_stream_url = alloc_string("cid:nina_128kb_mp3_raw_disabled");
      }
      /* Ok we have a File system to play */
      /* Init a File system reader */
      service_audio_read_fs_create(&reader);

      /* The player must be stopped */
      AVS_ASSERT( statePlayer == PLAYER_STATE_STOP);
      /* We can transition from stopped to open and signal playing */
      InitPlayerContext(tPlayer[0]);
      service_player_remove_item(0, FALSE);

    }
    /* Start the playback  state */
    service_player_atomic_write(&statePlayer, PLAYER_STATE_OPEN);
    ret = AVS_OK;
    break;

  }
  service_player_unlock_next();
  return ret;
}



/* Return a pointer on a string describing the player activity  for the UI */


char_t* service_player_get_string(void)
{
  static char_t tBufferPlayerString[512];        /* Buffer used to exchange info with the UI */
  static char_t title[15];                       /* Extracted from audio id */

  memset(tBufferPlayerString, 0, sizeof(tBufferPlayerString));

  if(nbPlayerItem == 0)
  {
    snprintf(tBufferPlayerString, sizeof(tBufferPlayerString), "Player Empty");
    return tBufferPlayerString;
  }

  service_player_lock_list();

  AVS_TIME curTime ;
  AVS_Get_Sync_Time(hInstance, &curTime);
  static char_t localLine[60];

  for(int32_t a = 0; a < nbPlayerItem  ; a++)
  {
    char_t *pString = strchr( tPlayer[a]->audioItem_audioItemId, ':');
    if(pString) 
    {
      strncpy(title, pString + 1, sizeof(title));
    }
    snprintf(localLine, sizeof(localLine), "%02ld: %s\r", a, title);
    AVS_ASSERT(strlen(localLine) + 1 +  strlen(tBufferPlayerString) + 1 < sizeof(tBufferPlayerString) - 1);
    strcat(tBufferPlayerString, localLine);

  }
  service_player_unlock_list();
  return tBufferPlayerString;
}


/* Send a private event to all listener */

void service_player_audio_send(AVS_Handle instance, uint32_t evt, uint32_t pparam)
{
  AVS_Send_Evt(instance, (AVS_Event)evt, pparam);
}


/*

 Pull incoming samples & fill the ring buffer


*/
static AVS_Result service_player_audio_pull(struct t_player_context *pHandle);
static AVS_Result service_player_audio_pull(struct t_player_context *pHandle)
{
  uint32_t ret;
  uint32_t nbHttpbytes ;
  uint8_t *pProd ;
  uint8_t *pSrc;
  /* Pull some data from the reader */
  ret = reader.pull(pHandle, pPullBuffer, szPullBuffer, &nbHttpbytes);
  /* The stream is complete , exit and signal EOF */
  if(ret == AVS_EOF)
  {
    return AVS_EOF;
  }
  if(ret != AVS_OK)
  {
    return AVS_EOF;
  }

  /* Waits for enough room in the ring buffer */
  while(1)
  {
    mutex_Lock(&gMp3Payload.lock);
    uint32_t szProd = service_player_bytes_buffer_GetProducerSizeAvailable(&gMp3Payload);
    mutex_Unlock(&gMp3Payload.lock);
    if(szProd >= nbHttpbytes)  
    {
      break;
    }
    osDelay(1);
  }
  /* Push the payload in the codec pipe */

  mutex_Lock(&gMp3Payload.lock);
  pProd = service_player_bytes_buffer_GetProducer(&gMp3Payload);
  pSrc = pPullBuffer;
  for(int32_t a = 0; a < nbHttpbytes ; a++)
  {
    *pProd = *pSrc++;
    service_player_bytes_buffer_MovePtrProducer(&gMp3Payload, 1, &pProd);

  }
  mutex_Unlock(&gMp3Payload.lock);
  return AVS_OK;
}


/* Manage the fade audio for the foreground and background management */

void service_player_mng_fade(void)
{
  static AVS_TIME lastTime = 0;
  static uint32_t bFade = FALSE;

  /* Manage fades */
  if(iCurVol != iTargetVol)
  {
    AVS_TIME curTime = xTaskGetTickCount();
    if(!bFade) 
    {
      lastTime = curTime;
    }
    bFade = TRUE;
    if(curTime - lastTime > 1) /* At least 1 ms */
    {
      int32_t  inc =  FADE_STEP;
      lastTime = curTime;
      if( iCurVol  < iTargetVol)
      {
        iCurVol += inc;
        if(iCurVol   >= iTargetVol)
        {
          iCurVol   = iTargetVol;
        }
      }
      else
      {
        iCurVol -= inc;
        if(iCurVol   < iTargetVol)
        {
          iCurVol   = iTargetVol;
        }
      }
      AVS_Aux_Info info = {0};
      info.infoFlags = AVS_AUX_INFO_VOLUME;
      info.volume = iCurVol;
      AVS_Set_Audio_Aux_Info(hInstance, &info);
    }
  }
  else
  {
    bFade = FALSE;
  }

}

/* This  task is in charge to play an audio stream from the server or from a local contents */
static void service_player_audio_task(const void *pCookie);
static void service_player_audio_task(const void *pCookie)
{
  /* Wait the AVS stack started */
  AVS_Instance_State State  = AVS_STATE_RESTART;
  while(1) 
  {
    if(hInstance) 
    {
      State = AVS_Get_State(hInstance) ;
    }
    if(State == AVS_STATE_IDLE) 
    {
      break;
    }
    osDelay(500);
  }

  service_player_audio_send(hInstance, EVT_PLAYER_TASK_START, 0);

  bPlayerRun = 1;
  while(bPlayerRun )
  {
    switch(statePlayer)
    {
    case PLAYER_STATE_OPEN:
      {
        uint32_t nextSate = PLAYER_STATE_CHECK_CODEC;
        bHardClosed = 0;

        /* First reset the ring buffer to make sure first bytes will be continuous to ease the codex analyze */
        service_player_bytes_buffer_reset(&gMp3Payload);
        /* Signal state */
        service_player_audio_send(hInstance, EVT_PLAYER_OPEN_STREAM, 0);

        /* Open the stream */
        if(reader.open(&playerContext, playerContext.pItemPlaying->audioItem_stream_url) != AVS_OK)
        {
          service_player_audio_send(hInstance, EVT_PLAYER_ERROR, 0);
          nextSate  = PLAYER_STATE_STOP;
        }

        /* Send some events and go to the next step */
        service_player_atomic_write(&statePlayer, nextSate);
        break;
      }
    case PLAYER_STATE_PAUSE:
      {
        /* Warning : Waiting for the resume*/
        osDelay(PLAYER_PERIOD_PRECISION);

        break;
      }


    case PLAYER_STATE_CHECK_CODEC:
      {
        /* signal playing*/
        playerContext.pItemPlaying->flags.playing = 1;

        /* At this point, we don't know the codec */
        /* We need to identify it to start the right codec pipe */

        /* Pull data */
        if(service_player_audio_pull(&playerContext) == AVS_EOF)
        {
          service_player_atomic_write(&statePlayer, PLAYER_STATE_CLOSE);
        }

        /* The analyse limit is the max size of the ring buffer */
        if(gMp3Payload.szConsumer >= gMp3Payload.szBuffer / 2)
        {
          AVS_TRACE_ERROR("Can't identify the codec");
          service_player_atomic_write(&statePlayer, PLAYER_STATE_CLOSE);
        }
        /* Check if we have a compatible the codec */
        int32_t  index = service_player_audio_codec_check();
        if(index   != -1)
        {
          /* Yes we have a codec */
          /* Init some values */
          curCodec = index;
          iMaxVol = VOL_MAX_DEFAULT;
          iCurVol = iMaxVol;
          iTargetVol = iMaxVol;
          /* Reset the auxiliary channel volume */
          AVS_Aux_Info info;
          AVS_Get_Audio_Aux_Info(hInstance, &info);
          info.infoFlags = AVS_AUX_INFO_SZBUFF | AVS_AUX_INFO_VOLUME ;
          info.volume = iCurVol;
          /* Reset the audio aux ring buffer */
          info.szConsumer = 0;
          info.szProducer = 0;
          AVS_Set_Audio_Aux_Info(hInstance, &info);

          /* Create the codec player instance */
          AVS_VERIFY(service_player_audio_get_codec()->create(&playerContext));
          /* Signal state */

          service_player_audio_send(hInstance, EVT_PLAYER_START, 0);
          service_player_audio_send(hInstance, EVT_PLAYER_BUFFERING, 0);
          /* Go to the next step */
          service_player_atomic_write(&statePlayer, PLAYER_STATE_BUFFERING);

        }

        osDelay(10); /* Guard in case of error */
        break;
      }




    case PLAYER_STATE_BUFFERING:
      {

        /* Stop  the codec to prevent PCM consumption */
        /* We need to buffering a bit, before to state to prevent net latency */
        service_player_audio_get_codec()->stop(&playerContext);
        if(service_player_audio_pull(&playerContext) == AVS_EOF)
        {
          service_player_atomic_write(&statePlayer, PLAYER_STATE_CLOSE);
        }
        /* Load the ring buffer at the  half of the max size */
        if(gMp3Payload.szConsumer > gMp3Payload.szBuffer / 2)
        {
          /* If the target is reached */
          /* We can restart the play back */
          service_player_audio_get_codec()->start(&playerContext);
          service_player_atomic_write(&statePlayer, PLAYER_STATE_PLAY);

        }
        break;
      }

    case PLAYER_STATE_PLAY:
      {
        /* Just pull data in the ring buffer and waits the end */
        AVS_Result ret = service_player_audio_pull(&playerContext);
        if(ret == AVS_EOF)
        {
          service_player_atomic_write(&statePlayer, PLAYER_STATE_CLOSE);
        }
        if(ret == AVS_ERROR)
        {
          service_player_atomic_write(&statePlayer, PLAYER_STATE_CLOSE);
        }


        break;
      }

    case PLAYER_STATE_CLOSE:
      {
        /* Wait the buffer audio aux is empty before to close the session */
        AVS_Aux_Info info;
        AVS_Get_Audio_Aux_Info(hInstance, &info);
        /* Set the purge flag to empty the fifo */
        info.infoFlags = AVS_AUX_INFO_FLAGS;
        info.iFlags |= AVS_PIPE_PURGE;
        AVS_Set_Audio_Aux_Info(hInstance, &info);

        /* Waits no audio */
        while(1)
        {
          AVS_Get_Audio_Aux_Info(hInstance, &info);
          /* If the consumer is 0 , we can quit because all data are consumed */
          if(info.szConsumer == 0) 
          {
            break;
          }
          osDelay(1);
        }

        /* Now we can stop the player */
        uint32_t nextSate = PLAYER_STATE_STOP;
        /* Delete the codec instance */
        service_player_audio_get_codec()->delete(&playerContext);
        /* Signal no codec */
        curCodec = -1;


        /* Delete the stream reader instance */
        if(reader.close(&playerContext) != AVS_OK)
        {
          service_player_audio_send(hInstance, EVT_PLAYER_ERROR, 0);
        }
        else
        {
          service_player_audio_send(hInstance, EVT_PLAYER_CLOSE_STREAM, bHardClosed);
        }
        /* Playback finished */
        service_player_atomic_write(&statePlayer, nextSate );
        break;
      }


    case PLAYER_STATE_STOP:
      {
        /* Just signal a stop state */
        /* signal playing*/
        playerContext.pItemPlaying->flags.playing = 0;

        service_player_audio_send(hInstance, EVT_PLAYER_STOPPED, 0);
        osDelay(PLAYER_PERIOD_STOP);
        break;
      }
    case PLAYER_STATE_IDLE:
      {
        osDelay(500);
        break;
      }
    default:
      {
        osDelay(500);
        break;
      }
      
      

    }
  }
  service_player_audio_send(hInstance, EVT_PLAYER_TASK_DYING, 0);
  osThreadTerminate(NULL);

}


/* Return a string corresponding to the internal player state */
static char_t *service_player_get_activity_string(int32_t state);
static char_t *service_player_get_activity_string(int32_t state)
{

  if(bHardClosed) 
  {
    return "STOPPED";
  }
  if((playerContext.pItemPlaying==0) && (nbPlayerItem == 0)) 
  {
    return "FINISHED";
  }
  if((playerContext.pItemPlaying!= 0)   && (playerContext.pItemPlaying->flags.pause==1) && (playerContext.pItemPlaying->flags.playing==1) ) 
  {
    return "PAUSED";
  }
  if((playerContext.pItemPlaying!= 0)   && (playerContext.pItemPlaying->flags.pause==0) && (playerContext.pItemPlaying->flags.playing==1) ) 
  {
    return "PLAYING";
  }
  if((playerContext.pItemPlaying!= 0)   && (state == PLAYER_STATE_BUFFERING)   && (playerContext.pItemPlaying->flags.playing==1) )           
  {
    return "BUFFER_UNDERRUN";
  }
  return "IDLE";
}
static json_t *service_player_synchro(void);
static json_t *service_player_synchro(void)
{
  json_t *ctx;
  json_t *header;
  json_t *jpayload;
  uint32_t err = 0;
  /* If no entry , no update */
  if(playerContext.pItemPlaying == 0) 
  {
    return 0;
  }


  ctx          = json_object();
  header       = json_object();
  jpayload      = json_object();

  /* Create the base entries */
  if(!err) 
  {
    err = json_object_set_new(header, "namespace", json_string("AudioPlayer"));
  }
  if(!err) 
  {
    err = json_object_set_new(header, "name", json_string("PlaybackState"));
  }
  service_player_lock_list();
  json_object_set_new(jpayload, "token", json_string(playerContext.pItemPlaying->audioItem_stream_token));
  json_object_set_new(jpayload, "offsetInMilliseconds", json_integer(playerContext.pItemPlaying->audioItem_stream_offsetInMilliseconds));
  json_object_set_new(jpayload, "playerActivity", json_string(service_player_get_activity_string(statePlayer)));
  service_player_unlock_list();

  /* Link header and payload  to the context */
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(ctx, "header", header);
  }
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(ctx, "payload", jpayload);
  }

  if(err)
  {
    json_decref(ctx);
    return NULL;
  }
  return ctx;
}

/* Dispatch a AVS event */

void service_player_event(AVS_Handle handle, const char *pName)
{
  uint32_t err = 0;
  static char_t msgid[32];
  char_t tMsg[50];
  json_t *root  = json_object();
  json_t *event = json_object();
  json_t *header = json_object();
  json_t *jpayload = json_object();
  /* Initialize JSON arrays */
  json_t *context = json_array();

  snprintf(tMsg, sizeof(tMsg), "Player Evt %s", pName);
  AVS_Send_Evt(handle, EVT_ENDURANCE_MSG, (uint32_t)tMsg);


  snprintf(msgid, sizeof(msgid), FORMAT_MESSAGE_ID, messageIdCounter++);
  err |= (uint32_t)json_object_set_new(header, "namespace", json_string("AudioPlayer"));
  err |= (uint32_t)json_object_set_new(header, "name", json_string(pName));
  err |= (uint32_t)json_object_set_new(header, "messageId", json_string(msgid));
  err |= (uint32_t)json_object_set_new(jpayload, "token", json_string(playerContext.pItemPlaying->audioItem_stream_token));
  AVS_ASSERT(err == 0);

  err |= (uint32_t)json_object_set_new(jpayload, "offsetInMilliseconds", json_integer(playerContext.pItemPlaying->audioItem_stream_offsetInMilliseconds + playerContext.pItemPlaying->playDelay));
  /* Links */
  err |= (uint32_t)json_object_set_new(root, "event", event);
  err |= (uint32_t)json_object_set_new(event, "header", header);
  err |= (uint32_t)json_object_set_new(event, "payload", jpayload);
  AVS_ASSERT(err == 0);

  AVS_VERIFY(json_object_set_new(root, "context", context) == 0);
  /* Add the context state to the event */
  AVS_VERIFY(AVS_Json_Add_Context(handle, root));


  const char_t *pJson = json_dumps(root, 0);
  if(pJson)
  {
    if(AVS_Send_JSon(handle, pJson) != AVS_OK)
    {
      AVS_TRACE_ERROR("Send Json Event");
    }
  }
  json_decref(root);
  jsonp_free((void *)(uint32_t)pJson);

}
/* Returns an epoch time  from a iso_8601 string */
static AVS_TIME service_player_parse_avs_time(const char_t *pTime);
static AVS_TIME service_player_parse_avs_time(const char_t *pTime)
{
  if(pTime == 0) 
  {
    return 0;
  }
  int32_t y = -1, m = -1, d = -1, h = -1, min = -1, sec = -1, mil = -1;
  sscanf(pTime, "%ld-%ld-%ldT%ld:%ld:%ld+%ld", &y, &m, &d, &h, &min, &sec, &mil);
  AVS_ASSERT((y != -1) && (m != -1) && (d != -1) &&  (h != -1) && (min != -1) && (sec != -1) && (mil != -1));
  AVS_ASSERT(m); /* Never 0 */
  struct tm t = {0};  /* Initialize to all 0's */
  t.tm_year = y - BASE_YEAR_EPOCH; /* This is year-1900, so 112 = 2012 */
  t.tm_mon  = m - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = min;
  t.tm_sec = sec;
  return mktime(&t) /*+service_alarm_get_time_zone() * HOUR_IN_SEC*/;
}



/* Clear the queue */
static AVS_Result service_player_process_directive_clearqueue(AVS_Handle handle, json_t *payload );
static AVS_Result service_player_process_directive_clearqueue(AVS_Handle handle, json_t *payload )
{
  const char_t *pBehavior  = json_string_value(json_object_get(payload, "clearBehavior"));
  if(strcmp(pBehavior, "CLEAR_ALL") == 0)
  {
    /* CLEAR_ALL, which clears the entire playback queue and stops the currently playing stream (if applicable). */
    /* Make sure we are stopped */

    service_player_stop(handle);
    /* Delete all entries */
    service_player_clear_items();
    return AVS_OK;


  }
  if(strcmp(pBehavior, "CLEAR_ENQUEUED ") == 0)
  {
    /* Remove all items from the index 1  ( ie 0=current playing) */
    service_player_lock_list ();
    for(int32_t a = 1 ; a < nbPlayerItem ; a++)
    {
      service_player_remove_item(1, TRUE);
    }
    service_player_unlock_list();
    return AVS_OK;

  }
  return AVS_ERROR;
}

/*

 Execute the play directive and manage Behavior


*/
static AVS_Result  service_player_process_directive_play(AVS_Handle handle, json_t *payload );
static AVS_Result  service_player_process_directive_play(AVS_Handle handle, json_t *payload )
{
  int32_t indexItem = -1;

  const char_t *pBehavior  = json_string_value(json_object_get(payload, "playBehavior"));
  json_t *audioItem = json_object_get(payload, "audioItem");
  json_t *stream    = json_object_get(audioItem, "stream");

  if(strcmp(pBehavior, "REPLACE_ALL") == 0)
  {
    /* Immediately begin playback of the stream returned with the Play directive, and replace current and enqueued streams */
    /* We need to clear all before to add an new entry, */
    service_player_clear_items();
    indexItem  = 0;
  }
  if(strcmp(pBehavior, "ENQUEUE") == 0)
  {
    /* Adds a stream to the end of the current queue. */

    indexItem  = nbPlayerItem;

  }
  if(strcmp(pBehavior, "REPLACE_ENQUEUED") == 0)
  {
    /* Replace all streams in the queue. This does not impact the currently playing stream. */
    /* Remove all items from the index 1 = ( ie 0=current playing) */
    service_player_lock_list ();
    for(int32_t a = 1 ; a < nbPlayerItem ; a++)
    {
      service_player_remove_item(1, TRUE);
    }
    service_player_unlock_list();
    indexItem  = nbPlayerItem;

  }

  if(indexItem != -1)
  {

    /* Fill the tmp slot with directive data */
    player_slot_t *pItem = (player_slot_t *)pvPortMalloc(sizeof(player_slot_t));
    memset(pItem, 0, sizeof(player_slot_t));

    pItem->audioItem_audioItemId       = alloc_string(json_string_value(json_object_get(audioItem, "audioItemId")));
    pItem->audioItem_stream_url        = alloc_string(json_string_value(json_object_get(stream, "url")));
    if(strncmp(pItem->audioItem_stream_url, "cid:", 4) != 0) 
    {
      pItem->flags.network = 1;
    }

    pItem->audioItem_stream_expiryTime = service_player_parse_avs_time(json_string_value(json_object_get(stream, "expiryTime")));
    if(pItem->audioItem_stream_expiryTime == 0) 
    {
      pItem->audioItem_stream_expiryTime = (AVS_TIME)-1;
    }
    pItem->audioItem_stream_token      = alloc_string(json_string_value(json_object_get(stream, "token")));

    pItem->audioItem_stream_offsetInMilliseconds = json_integer_value(json_object_get(stream, "offsetInMilliseconds"));
    pItem->audioItem_stream_streamFormat = alloc_string(json_string_value(json_object_get(stream, "streamFormat")));

    json_t * progressReport = json_object_get(audioItem, "progressReport");
    if(progressReport)
    {
      pItem->audioItem_stream_progressReport_progressReportDelayInMilliseconds    = json_integer_value(json_object_get(progressReport, "progressReportDelayInMilliseconds"));
      pItem->audioItem_stream_progressReport_progressReportIntervalInMilliseconds = json_integer_value(json_object_get(progressReport, "progressReportIntervalInMilliseconds"));
    }
    service_player_insert_item(indexItem, pItem);
  }
  return (( indexItem != -1) ? AVS_OK : AVS_ERROR);
}


/* Event callback delegation, we should watch here for events and catch player directive */
/* And add or delete players according to the directive */
AVS_Result service_player_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
AVS_Result service_player_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  uint32_t extendedEvent = (uint32_t)evt; /* Avoid compiler complain when handling extended events ie non defined inside AVS_Event enum*/

  switch(extendedEvent)
  {

    case EVT_HTTP2_CONNECTED:
    {
      /* If we are connected, go in stop and start a playback */
      service_player_atomic_write(&statePlayer, PLAYER_STATE_STOP);
      break;
    }
    case EVT_CHANGE_STATE:
    {
      if(pparam == AVS_STATE_RESTART)
      {
        /* If we are restating, go in idle and wa */
        service_player_atomic_write(&statePlayer, PLAYER_STATE_IDLE);
      }
      /* we check the state to pause and resume the playback if an capture and speak occurs */
#ifdef USE_PAUSE_RESUME
      service_player_change_state(handle,pparam);
#endif
      if(pparam == AVS_STATE_IDLE)
      {
        service_player_atomic_write((uint32_t *)(uint32_t)&iTargetVol, iMaxVol);

      }
      else
      {
        /* If the device is muted , force the muted value */
        uint32_t minVol = VOL_MIN;
        if(iMaxVol < minVol ) 
        {
          minVol = iMaxVol ;
        }
        service_player_atomic_write((uint32_t *)(uint32_t)&iTargetVol, minVol);
      }
      break;
    }


    case EVT_UPDATE_SYNCHRO:
    {

      /* When the SDK send a synchrostate ( initiated by the SDK or using AVS_Post_Sychro) */
      /* The synchro must reflect the player state in the json context array */
      /* Each time the synchro will sbe done, the system will create a json and will send EVT_UPDATE_SYNCHRO */
      /* In order that all apps or user services update the synchro state and reflects changes */
      /* The parameter is the root Json handle  pre-filled by the sdk and read to be send to AVS after the EVT_UPDATE_SYNCHRO update */

      json_t *root = (json_t *)pparam;
      json_t *ctx = json_object_get(root, "context");
      json_t *player =  service_player_synchro();
      if(player)
      {
        AVS_VERIFY(json_array_append_new(ctx, player) == 0);
      }
      break;
    }


    case EVT_DIRECTIVE_AUDIO_PLAYER:
    {
      /* The json is the parameter */
      json_t *root      = (json_t *)pparam;
      json_t *directive = json_object_get(root, "directive");
      json_t *header    = json_object_get(directive, "header");
      json_t *jpayload   = json_object_get(directive, "payload");
      const char_t *pName  = json_string_value(json_object_get(header, "name"));
      if(strcmp(pName, "Play") == 0)
      {
        service_player_process_directive_play(handle, jpayload);
        return  AVS_EVT_HANDLED;
      }
      if(strcmp(pName, "Stop") == 0)
      {
        service_player_stop(handle);
        return  AVS_EVT_HANDLED;
      }
      if(strcmp(pName, "ClearQueue") == 0)
      {
        service_player_process_directive_clearqueue(handle, jpayload);
        return  AVS_EVT_HANDLED;
      }

      break;
    }

    case EVT_PLAYER_ERROR:
    {
      /* If an error occurs, cancel the current playback */
      /* Next will start on the next stopped event */
      TermPlayerContext();

      break;
    }

    case EVT_PLAYER_STOPPED:
    {
      service_player_next();
      break;
    }
    case EVT_PLAYER_START:
    {
      service_player_event(handle, "PlaybackStarted");
      break;
    }
    case EVT_PLAYER_BUFFERING:
    {
      /* If the player has some room in the list, we send  PlaybackNearlyFinished to inform Alexa we can add stream in the list */

      if(nbPlayerItem+1 <  MAX_PLAYER_ENTRY) 
      {
        service_player_event(handle, "PlaybackNearlyFinished");
      }
      break;
    }
    case EVT_PLAYER_CLOSE_STREAM:
    {
      /* Signal finished if not hard stopped*/
      if(pparam ==0) 
      {
        service_player_event(handle, "PlaybackFinished");
      }
      else           
      {
        service_player_event(handle, "PlaybackStopped");
      }
      TermPlayerContext();
      break;

    }

    case EVT_PLAYER_REPORT:
    {
      /* Manage the progressReport report event */
      playerContext.pItemPlaying->playDelay = pparam;
      playerContext.pItemPlaying->audioItem_stream_offsetInMilliseconds = pparam;

      service_player_mng_fade();

      if((playerContext.pItemPlaying->audioItem_stream_progressReport_progressReportDelayInMilliseconds != 0) && (playerContext.pItemPlaying->playDelay > playerContext.pItemPlaying->audioItem_stream_progressReport_progressReportDelayInMilliseconds) )
      {
        playerContext.pItemPlaying->flags.sent_report = 1;
        service_player_event(handle, "ProgressReportDelayElapsed");

      }
      if((playerContext.pItemPlaying->audioItem_stream_progressReport_progressReportIntervalInMilliseconds != 0) && (playerContext.pItemPlaying->flags.sent_report != 0)  && (playerContext.pItemPlaying->playDelay  > playerContext.pItemPlaying->audioItem_stream_progressReport_progressReportIntervalInMilliseconds))
      {
        service_player_event(handle, "ProgressReportIntervalElapsed");
        playerContext.pItemPlaying->playDelay = 0;
      }
      break;
    }

    default:
    	break;
  }
  return AVS_OK;
}





/* Create the player service */
AVS_Result service_player_create(AVS_Handle hHandle);
AVS_Result service_player_create(AVS_Handle hHandle)
{
  /* Enable aux audio channel */
  sInstanceFactory.useAuxAudio  = 1;

  /* Set current volume */

  nbCodec = 0;
  curCodec = -1;
  messageIdCounter = 0;
  nbPlayerItem = 0;


  pPullBuffer = (uint8_t *)pvPortMalloc(szPullBuffer);

  /* Add mp3 codec */
  service_player_add_media(service_audio_play_media_mp3());



  AVS_VERIFY(mutex_Create(&lockList,"lockList"));
  AVS_VERIFY(mutex_Create(&nextLock,"nextLock"));

  memset(tPlayer, 0, sizeof(tPlayer));
  service_player_bytes_buffer_Create(&gMp3Payload, PAYLOAD_SIZE,"MP3 Ring Buffer");

  /* Create the main thread */
  hPlayerTask = task_Create(TASK_NAME_PLAYER,service_player_audio_task,hHandle,PLAYER_TASK_STACK_SIZE,TASK_PRIORITY_PLAYER);
  if(hPlayerTask  ==0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_PLAYER);
    return AVS_ERROR;
  }

  return AVS_OK;

}

/* Delete the player service */
void  service_player_delete(AVS_Handle hHandle);
void  service_player_delete(AVS_Handle hHandle)
{
  if(sInstanceFactory.useAuxAudio == 0) 
  {
    return ;
  }
  bPlayerRun = 0;
  osDelay(10); /* Leave enough time to terminate */
  service_player_bytes_buffer_Delete(&gMp3Payload);
  vPortFree(pPullBuffer);
  mutex_Delete(&lockList);
  mutex_Delete(&nextLock);
}
