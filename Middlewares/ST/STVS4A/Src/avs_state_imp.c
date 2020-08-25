/**
******************************************************************************
* @file    avs_state_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Dialogue state management
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


typedef enum mode_state
{
  MODE_PREPARE_IDLE = 0, /* Prepare to enter in idle state ( send some messages) */
  MODE_STOP_HTTP2,      /* Stop all connections */
  MODE_START_HTTP2,     /* Start all connections */
  MODE_START_DIALOGUE,  /* Start a dialogue sequence */
  MODE_OPEN_DIALOGUE,   /* Open the dialogue stream */
  MODE_REC_START,       /* Start record */
  MODE_REC_PUSH,        /* Send the voice to the network */
  MODE_REC_STOP,        /* Stop recording */
  MODE_PARSE_REPONSE,   /* Read and parse the response */
  MODE_PLAYING_START,   /* Start the speaking sequence */
  MODE_PLAYING_PULL,    /* Send the MP3 speaker to the audio */
  MODE_PLAYING_STOP,    /* Stop the speaking sequence */
  MODE_CLOSE_DIALOGUE,  /* Close the dialogue sequence */
  MODE_IDLE,            /* Idle state wait for a new sequence */
} Mode_state;

/*

 Set the current state machine according to the user state


*/

AVS_Result avs_set_state(AVS_instance_handle *pHandle, AVS_Instance_State state)
{
  /* Check of we are connected */
  if(avs_core_atomic_read(&pHandle->bConnected) == FALSE)
  {
    return AVS_BUSY;
  }

  switch(state)
  {
    case    AVS_STATE_RESTART:
    {
      /* Check if we are already  in the same state */
      if(avs_core_atomic_read(&pHandle->curState) == state)
      {
        return AVS_OK;
      }
      /* Update the state */
      avs_core_atomic_write(&pHandle->curState, state);
      /* Set the state machine the restart */
      avs_core_atomic_write(&pHandle->modeState, MODE_STOP_HTTP2);
    }
    break;


    case    AVS_STATE_IDLE:
    {
      /* Check if we are already  in the same state */
      if(avs_core_atomic_read(&pHandle->curState) == state)
      {
        return AVS_OK;
      }
      avs_core_atomic_write(&pHandle->curState, state);
      /* Update the state */
      if(avs_core_atomic_read(&pHandle->modeState) == MODE_IDLE)
      {
        return AVS_OK;
      }
      /* Force to close the dialogue */
      /* Wait for the sleep mode */
      if(avs_http2_stream_is_opened(pHandle, pHandle->hHttpDlgStream))
      {
        /* Force the close dialogue  state */
        avs_core_atomic_write(&pHandle->modeState, MODE_CLOSE_DIALOGUE);
        /* Wait for the termination ie Idle state */
        int32_t timeout = IDLE_TIMEOUT_MS / DELAY_SLEEP;
        while((avs_core_atomic_read(&pHandle->modeState) == MODE_IDLE) && (timeout != 0))
        {
          avs_core_task_delay(DELAY_SLEEP);
          timeout--;
        }

        if(timeout == 0)
        {
          /* Should never be a time-out, ie the thread is locked */
          AVS_TRACE_ERROR("State time-out");
        }
      }
    }

    break;

    case    AVS_STATE_START_CAPTURE:
    {
      /* Check if we are already  in the same state */
      if(avs_core_atomic_read(&pHandle->curState) == state)
      {
        return AVS_OK;
      }

      /* We must be in the idle state, otherwise we can't start a sequence */
      if(avs_core_atomic_read(&pHandle->modeState) != MODE_IDLE)
      {
        AVS_TRACE_WARNING("Can't do it , Busy %d", avs_core_atomic_read(&pHandle->modeState));
        return AVS_BUSY;
      }
      /* Change the state machine to start dialogue */
      avs_core_atomic_write(&pHandle->modeState, MODE_START_DIALOGUE);

    }
    break;

    case    AVS_STATE_STOP_CAPTURE:
      /* The state could be release by the stop of the rect  from the server */
      /* In this case the state will IDLE */
      if(avs_core_atomic_read(&pHandle->curState) == AVS_STATE_IDLE)
      {
        return AVS_OK;
      }

      /* Check if we have already started a start capture */

      if(avs_core_atomic_read(&pHandle->curState) != AVS_STATE_START_CAPTURE)
      {
        AVS_TRACE_ERROR("Capture not started");
        return AVS_ERROR;
      }
      /* Check if the capture is started */
      if(avs_core_atomic_read(&pHandle->modeState) != MODE_REC_PUSH)
      {
        return AVS_BUSY;
      }


      /* Change the state machine to start dialogue */
      avs_core_atomic_write(&pHandle->modeState, MODE_REC_STOP);
      break;
    default:
      AVS_TRACE_DEBUG("Unknow state");
      avs_core_task_delay(2000);
      break;
  }
  return AVS_OK;
}


static AVS_Result avs_state_posts(AVS_instance_handle *pHandle);
static AVS_Result avs_state_posts(AVS_instance_handle *pHandle)
{
  /* Send posted messages */
  avs_core_post_messages(pHandle);

  /* Check ping and process it */
  if(avs_http2_post_ping(pHandle) != AVS_OK)
  {
    avs_core_atomic_write(&pHandle->modeState, MODE_STOP_HTTP2);
  }
  /* Check sync and process it */
  if(avs_directive_post_synchro(pHandle) != AVS_OK)
  {
    avs_core_atomic_write(&pHandle->modeState, MODE_STOP_HTTP2);
  }
  return AVS_OK;
}


/*


 Pull incoming samples from http and pull it in the voice


*/
static AVS_Result avs_state_audio_pull(AVS_instance_handle *pHandle);
static AVS_Result avs_state_audio_pull(AVS_instance_handle *pHandle)
{
  uint32_t ret;
  uint32_t nbHttpbytes ;
  uint32_t blkSize = pHandle->blkSizeStreamBuff * sizeof(int16_t);
  /* Retrieve a block from teh network */
  ret = avs_http2_stream_read(pHandle, pHandle->hHttpDlgStream, pHandle->pBufferStreamBuff, blkSize, &nbHttpbytes);
  /* The stream is complete , exit and signal EOF */
  if(ret == AVS_EOF)
  {
    return AVS_EOF;
  }
  if(ret != AVS_OK)
  {
    return AVS_ERROR;
  }
  /* If the block is ok, we can inject the payload in the MP3 player */
  avs_audio_inject_stream_buffer_mp3(pHandle->pAudio, (uint8_t *)(uint32_t)pHandle->pBufferStreamBuff, nbHttpbytes);
  return AVS_OK;
}





/*


 pushes incoming samples from the microphone to the http stream


*/
static AVS_Result avs_state_audio_push(AVS_instance_handle *pHandle);
static AVS_Result avs_state_audio_push(AVS_instance_handle *pHandle)
{


  uint32_t ret;
  AVS_Result status;
  /* Check the size we can inject in the stream buffer */
  avs_core_mutex_lock(&pHandle->pAudio->recognizerPipe.lock);
  uint32_t sizeStreamSpeaker =  pHandle->pAudio->recognizerPipe.outBuffer.szConsumer;
  uint32_t blkSize           =  pHandle->blkSizeStreamBuff;
  uint32_t flags              = pHandle->pAudio->recognizerPipe.pipeFlags;
  avs_core_mutex_unlock(&pHandle->pAudio->recognizerPipe.lock);

  /* If the ring buffer is lower than the block size we want to inject, wait a bit */
  /* But if we want to purge the stream, we have to wait to send the packet even if it doesn't have blocksize required */
  if(((flags  & AVS_PIPE_PURGE)==0) && (sizeStreamSpeaker < pHandle->blkSizeStreamBuff))
  {
    avs_core_task_delay(IDLE_SLEEP_TIME);
    return AVS_OK;
  }
  /* Clamp the size to the maximum buffer */
  if(blkSize >= sizeStreamSpeaker)
  {
    blkSize = sizeStreamSpeaker;
  }
  /* Capture microphone buffer */
  ret = avs_avs_capture_audio_stream_buffer(pHandle->pAudio, pHandle->pBufferStreamBuff, blkSize);
  if(ret == 0)
  {
    avs_core_task_delay(IDLE_SLEEP_TIME);
    return AVS_OK;
  }

  /* Re-inject  in the network stream */
  status = avs_http2_stream_write(pHandle, pHandle->hHttpDlgStream, pHandle->pBufferStreamBuff, blkSize * pHandle->pAudio->recognizerPipe.outBuffer.nbChannel * sizeof(int16_t));
  return status;

}

int8_t bRecMode;



/* Evaluate  the last audio directive and pre-programme its terminate state */
/* The terminate state may be finish or multi-turn */
static void avs_state_dialogue_terminator_set(AVS_instance_handle *pHandle, uint32_t  audioDirective);
static void avs_state_dialogue_terminator_set(AVS_instance_handle *pHandle, uint32_t  audioDirective)
{
  switch(audioDirective)
  {
    case   AVS_AUDIO_DIR_NONE:
      /* Ignore it */
      /* Parsed in the directive */
      break;

    case   AVS_AUDIO_DIR_SPEAK:
    {
      /* Normal end , we close the sequence */
      avs_core_atomic_write(&pHandle->modeState, MODE_PLAYING_START);
      avs_core_atomic_write(&pHandle->modeTerminator, MODE_PREPARE_IDLE);
      break;
    }

    case   AVS_AUDIO_DIR_EXPECT_SPEECH:
    {
      /* It is a multi-turn  , we re-open teh sequence */
      avs_core_atomic_write(&pHandle->modeTerminator, MODE_OPEN_DIALOGUE);
      bRecMode = TRUE;
      break;
    }
    case   AVS_AUDIO_DIR_EXPECT_SPEECH_TIMEOUT:
    {
      /* Error, we close the sequence */
      avs_core_atomic_write(&pHandle->modeTerminator, MODE_PREPARE_IDLE);
      break;
    }
    case   AVS_AUDIO_DIR_RECOGNIZE:
    default:
    {
      /* Default, we close the sequence */
      avs_core_atomic_write(&pHandle->modeTerminator, MODE_PREPARE_IDLE);
      break;
    }
  }
}


/*

 The task manages the dialogue state  and sequences the rec phase and play


*/
static void avs_state_task(const void *pCookie);
static void avs_state_task(const void *pCookie)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)(uint32_t) pCookie;

  /* Wait the audio audio initialized */
  while(!avs_core_atomic_read(&pHandle->pAudio->runInRuning))
  {
    avs_core_task_delay(100);
  }


  /* For the time being only 1 channel and 16000, let's check it */
  AVS_ASSERT(pHandle->pAudio->recognizerPipe.outBuffer.nbChannel == 1);
  AVS_ASSERT(pHandle->pAudio->recognizerPipe.outBuffer.sampleRate == 16000);

  /* Signal the state */
  avs_core_message_send(pHandle, EVT_STATE_TASK_START, 0);

  /* Compute the buffer size according to the block given in MS */
  uint32_t avspkt = ((pHandle->pAudio->recognizerPipe.outBuffer.sampleRate * AVS_AUDIO_PACKET_IN_MS) / 1000);

  /* Init a tmp buffer for streaming blocks */
  /* Compute size and alloc it */
  pHandle->blkSizeStreamBuff = avspkt * pHandle->pAudio->recognizerPipe.outBuffer.nbChannel;
  pHandle->pBufferStreamBuff = avs_core_mem_alloc(avs_mem_type_heap, pHandle->blkSizeStreamBuff * sizeof(int16_t));
  AVS_ASSERT(pHandle->pBufferStreamBuff);
  /* The state thread is initiator, we start with the start HTTP2 */
  avs_core_atomic_write(&pHandle->modeState, MODE_START_HTTP2);

  /* Init the thread flags as start running */
  pHandle->runStateTaskFlag = avs_task_running;

  /* Loops for ever or explicit dead */
  while((pHandle->runStateTaskFlag & avs_task_running) != 0)
  {
    switch(avs_core_atomic_read(&pHandle->modeState))
    {

      case MODE_STOP_HTTP2:
      {
        /* Stop all connections */
        AVS_Result result;
        /* Make sure the input is muted */
        avs_audio_capture_mute(pHandle->pAudio, TRUE);
        AVS_TRACE_DEBUG("Enter Stop HTTP2");
        /* Assuming that avs_http2_connnection_manager_delete will close all streams */
        pHandle->hHttpDlgStream  = 0;
        /* Delete all connection from another thread */
        result  = avs_http2_connnection_manager_delete(pHandle);
        AVS_TRACE_DEBUG("Leave Stop HTTP2");
        if(result != AVS_OK)
        {
          AVS_TRACE_ERROR("Can't stop http2 connection manager");
          avs_core_atomic_write(&pHandle->modeState, MODE_STOP_HTTP2);
          break;
        }
        /* Next step */
        avs_core_atomic_write(&pHandle->modeState, MODE_START_HTTP2);
        break;
      }

      case MODE_START_HTTP2:
      {
        AVS_Result result;
        AVS_TRACE_DEBUG("Enter Start HTTP2");
        /* Signal the state */
        avs_core_message_send(pHandle, EVT_CHANGE_STATE, AVS_STATE_RESTART);

        /* Update the current state */
        avs_core_atomic_write(&pHandle->curState, AVS_STATE_RESTART);

        /* Make sure the input is muted */
        avs_audio_capture_mute(pHandle->pAudio, TRUE);

        /* Make sure we are in reset wakeup mode */
        drv_audio.platform_Audio_ioctl(pHandle->pAudio, AVS_IOCTL_CAPTURE_FEED, AVS_CAPTURE_WAKEUP, 0);

        /* Initiate the HTTP2 connection */
        result  = avs_http2_connnection_manager_create(pHandle);
        AVS_ASSERT(result  != 0);
        AVS_TRACE_DEBUG("Leave Start HTTP2");
        if(result != AVS_OK)
        {
          AVS_TRACE_ERROR("Can't start http2 connection manager");
          avs_core_atomic_write(&pHandle->modeState, MODE_STOP_HTTP2);
          break;

        }
        /* Wait the connection, or restart from scratch */
        int32_t timeout = WAIT_CONNECT_TIMEOUT_MS / IDLE_SLEEP_TIME;
        /* Wait all connected before to start */
        while((avs_core_atomic_read(&pHandle->bConnected) == FALSE) && (timeout != 0))
        {
          /* dispatch messages in case of */
          avs_state_posts(pHandle);
          avs_core_task_delay(IDLE_SLEEP_TIME);
          timeout--;
        }

        if(timeout == 0)
        {
          /* The connection doesn't restart !!!!  try to stop and restart */
          AVS_TRACE_ERROR("Start time-out > %d secs", WAIT_CONNECT_TIMEOUT_MS / 1000);
          avs_core_atomic_write(&pHandle->modeState, MODE_STOP_HTTP2);
        }
        else
        {
          avs_core_atomic_write(&pHandle->modeState, MODE_PREPARE_IDLE);
        }
        break;
      }

      case MODE_START_DIALOGUE:
      {
        /* Start a new dialogue */
        bRecMode = FALSE;
        /* Next state step */
        avs_core_atomic_write(&pHandle->modeState, MODE_OPEN_DIALOGUE);
      }
      break;


      case MODE_OPEN_DIALOGUE:
      {
        /* Open the stream and manage dlg IDS */
        uint32_t newstate = MODE_REC_START;
        pHandle->hHttpDlgStream = avs_http2_stream_open(pHandle);
        if(!pHandle->hHttpDlgStream )
        {
          newstate = MODE_STOP_HTTP2;

        }
        avs_core_atomic_write(&pHandle->modeState, newstate);
        pHandle->dialogIdCounter++;/* Inc the id to synchronise directive and Request */
        /* Next step */
        break;
      }
      case MODE_REC_START:
      {
        /* Start the record */
        pHandle->cptRecBlk = 0;
        uint32_t newstate = MODE_REC_PUSH;

        /* Make sure we start with and empty pipe */
        avs_audio_capture_reset(pHandle->pAudio);
        avs_audio_capture_mute(pHandle->pAudio, FALSE);
        /* First send a sync state */
        /* Send the Recognize json body */
        const char_t *pMessage = avs_json_formater_recognizer_event(pHandle, "Recognize");

        /* Dump the json scrip dfor debug */
        AVS_PRINTF(AVS_TRACE_LVL_JSON, "SND JSON : ");
        AVS_PRINT_STRING(AVS_TRACE_LVL_JSON, pMessage);
        AVS_PRINT_STRING(AVS_TRACE_LVL_JSON, "\r");
        if(avs_http2_stream_add_body(pHandle, pHandle->hHttpDlgStream, pMessage)  != AVS_OK)
        {
          newstate = MODE_STOP_HTTP2;
        }
        /* Free tmp res */
        avs_json_formater_free(pMessage);

        /* If we are ready, start the next state */
        if(newstate == MODE_REC_PUSH)
        {
          /* Update the API state */
          avs_core_atomic_write(&pHandle->curState, AVS_STATE_START_CAPTURE);
          /* Signal the change state */
          avs_core_message_send(pHandle, EVT_CHANGE_STATE, AVS_STATE_START_CAPTURE);
          /* Signal the state */
          avs_core_message_send(pHandle, EVT_START_REC, 0);
          /* Change the feed mode to map the mic to alexa */
          drv_audio.platform_Audio_ioctl(pHandle->pAudio, AVS_IOCTL_CAPTURE_FEED, AVS_CAPTURE_AVS, 0);
        }

        /* Next step */
        avs_core_atomic_write(&pHandle->modeState, newstate);

        break;
      }
      case MODE_REC_PUSH:
      {
        /* Push data from the microphone in the network */
        pHandle->cptRecBlk++;
        /* Push and check error, if an error occurs, we  change the state for a disconnection error */
        if(avs_state_audio_push(pHandle) != AVS_OK)
        {
          avs_core_atomic_write(&pHandle->modeState, (uint32_t)MODE_STOP_HTTP2);
        }
        /* dispatch messages in case of */
        avs_state_posts(pHandle);
        break;
      }

      case MODE_REC_STOP:
      {
        /* Alexa stops the record ( manually or automatic via Stop capture event) */
        uint32_t newstate = MODE_PARSE_REPONSE;
        /* Signal the state */
        avs_core_message_send(pHandle, EVT_CHANGE_STATE, AVS_STATE_STOP_CAPTURE);
        /* Re-map the microphone to Wake word detection */
        drv_audio.platform_Audio_ioctl(pHandle->pAudio, AVS_IOCTL_CAPTURE_FEED, AVS_CAPTURE_WAKEUP, 0);
        /* Mute the microphone */
        avs_audio_capture_reset(pHandle->pAudio); /* clear the buffer  for reco buff state display */
        avs_audio_capture_mute(pHandle->pAudio, TRUE);
        /* Stop the stream */
        if(avs_http2_stream_stop(pHandle, pHandle->hHttpDlgStream ) != AVS_OK)
        {
          newstate = MODE_STOP_HTTP2;
        }
        /* If no error, signal the state */
        if(newstate == MODE_PARSE_REPONSE)
        {
          avs_core_message_send(pHandle, EVT_STOP_REC, 0);
        }

        avs_core_atomic_write(&pHandle->modeState, newstate);
        break;
      }

      case MODE_PARSE_REPONSE:
      {
        /* In this phase we parse the response from alexa */
        /* Alexa produces a series of multi-part json  we have to parse */
        /* The answer will condition how Alexa will terminate its dialogue */
        /* By a loop to the recognizer or by a dialogue end */

        /* Case of error we pre-programme a state close */
        /* First notify busy */
        avs_core_atomic_write(&pHandle->modeState, AVS_STATE_BUSY);
        /* Signal the state */
        avs_core_message_send(pHandle, EVT_CHANGE_STATE, AVS_STATE_BUSY);

        /* In case of error pre-programme the close */
        avs_core_atomic_write(&pHandle->modeState, MODE_CLOSE_DIALOGUE);
        avs_core_atomic_write(&pHandle->modeTerminator, MODE_PREPARE_IDLE);

        /* We parse all multi-parts */

        const char_t *content;
        uint32_t quit=0;
        while((quit == 0))
        {
          content =  avs_http2_stream_get_response_type(pHandle, pHandle->hHttpDlgStream );
          if(content  == 0)
          {
            break;
          }

          /* Check if we have a json response */
          if(strcmp(content, AVS_CONTENT_TYPE_JSON) == 0)
          {
            /* So parse it and process */
            if(avs_http2_stream_process_json(pHandle, pHandle->hHttpDlgStream ) == AVS_OK)
            {
              avs_state_dialogue_terminator_set(pHandle, pHandle->codeAudioDir);
            }
            else
            {
              AVS_TRACE_ERROR("Directive parse");
              quit=1;
            }

          }
          /* If we have an stream type, Alexa will speak */
          if(strcmp(content, AVS_CONTENT_TYPE_STREAM) == 0)
          {
            /* Next step */
            avs_core_atomic_write(&pHandle->modeState, MODE_PLAYING_START);
            quit=1;
          }
        }
      }
      break;

      case MODE_PLAYING_START:
      {
        uint32_t newstate = MODE_PLAYING_PULL;
        /* Flag used for synchro state */

        pHandle->isSpeaking=1;
        pHandle->speakingStart = osKernelSysTick();

        pHandle->cptPlayBlk = 0;
        /* Make sure the player will wait a  threshold before to start the playback ( avoid pops) */
        avs_audio_inject_stream_set_flags(pHandle->pAudio, AVS_PIPE_PURGE, 0);
        /* Reset the pipe */
        avs_audio_inject_audio_stream_reset(pHandle->pAudio);
        /* Send the even */
        avs_http2_notification_event(pHandle, "SpeechSynthesizer", "SpeechStarted", pHandle->pLastToken, 0);
        if(newstate == MODE_PLAYING_PULL)
        {
          /* TODO : check this ???? */
          avs_core_message_send(pHandle, EVT_START_SPEAK, 0);
        }
        /* Next step */
        avs_core_atomic_write(&pHandle->modeState, newstate);
        break;
      }
      case MODE_PLAYING_PULL:
      {
        /* Pull the MP3 and send it to the audio */
        pHandle->cptPlayBlk++;
        /* Pull some data */
        AVS_Result err = avs_state_audio_pull(pHandle);
        if(err == AVS_EOF)
        {
          /* If EOF , Alexa stops speaking */
          avs_core_atomic_write(&pHandle->modeState, MODE_PLAYING_STOP);
        }
        if(err  == AVS_ERROR)
        {
          /* If error, we get a disconnection */
          avs_core_atomic_write(&pHandle->modeState, MODE_STOP_HTTP2);
        }
        /* dispatch messages in case of */
        avs_state_posts(pHandle);
        break;
      }

      case MODE_PLAYING_STOP:
      {
        pHandle->isSpeaking=0;
        /* We set the terminator state recorded in the directive ( parse response) */
        uint32_t newstate = MODE_CLOSE_DIALOGUE;
        /* Make sure the player has rendered  all existing samples */
        avs_audio_inject_wait_all_consumed(pHandle->pAudio);
        /* We can stop the stream */
        AVS_Result err = avs_http2_stream_stop(pHandle, pHandle->hHttpDlgStream);
        if(err != AVS_OK)
        {
          /* If error, we get a disconnection */
          newstate = MODE_STOP_HTTP2;
        }
        /* Notify event */
        avs_http2_notification_event(pHandle, "SpeechSynthesizer", "SpeechFinished", pHandle->pLastToken, 0);
        /* Signal state */
        avs_core_message_send(pHandle, EVT_STOP_SPEAK, 0);
        /* Next state */
        avs_core_atomic_write(&pHandle->modeState, newstate);
        break;
      }


      case MODE_CLOSE_DIALOGUE:
      {
        /* Close normally the stream  audio */
        avs_http2_stream_close(pHandle, pHandle->hHttpDlgStream );
        pHandle->hHttpDlgStream = 0;
        /* And terminate on IDLE or Re-arm in recognize */
        avs_core_atomic_write(&pHandle->modeState, pHandle->modeTerminator);
        break;
      }

      case MODE_PREPARE_IDLE:
      {
        /* Just send some notification */
        drv_audio.platform_Audio_ioctl(pHandle->pAudio, AVS_IOCTL_CAPTURE_FEED, AVS_CAPTURE_WAKEUP, 0);
        /* Signal state */
        avs_core_message_send(pHandle, EVT_CHANGE_STATE, AVS_STATE_IDLE);
        /* Update API state */
        avs_core_atomic_write(&pHandle->modeState, MODE_IDLE);
        /* Enter in IDLE state */
        avs_core_atomic_write(&pHandle->curState, AVS_STATE_IDLE);

        break;
      }

      case MODE_IDLE:
      {
        /* We are in the idle time */
        /* It is a good occasion to process some not realtime action */

        /* Sleep a bit */
        avs_core_task_delay(DELAY_SLEEP);

        avs_state_posts(pHandle);
        break;
      }

      default:
      {
        AVS_TRACE_ERROR("Bad state mode");
      }
      break;

    }
  }
  /* Signal closed */
  pHandle->runStateTaskFlag |= avs_task_closed;
  /* Free buffer */
  avs_core_mem_free(pHandle->pBufferStreamBuff);
  /* Init default */
  pHandle->pBufferStreamBuff = 0;
  pHandle->blkSizeStreamBuff = 0;
  /* Signal state */
  avs_core_message_send(pHandle, EVT_STATE_TASK_DYING, 0);
  avs_core_task_delete(NULL);
}



/*

 State machine  create Delegation


*/

AVS_Result avs_state_create(AVS_instance_handle *pHandle)
{
  /* Init state */
  pHandle->runStateTaskFlag = avs_task_closed;
  /* Create the task */
  AVS_VERIFY((pHandle->hStateTask = avs_core_task_create(STATE_TASK_NAME, avs_state_task, pHandle, STATE_TASK_STACK_SIZE, STATE_TASK_PRIORITY)) != NULL);
  if(!pHandle->hStateTask)
  {
    AVS_TRACE_ERROR("Create task %s", STATE_TASK_NAME);
    return AVS_ERROR;
  }
  return AVS_OK;
}



/*

 State delete Delegation


*/

AVS_Result avs_state_delete(AVS_instance_handle *pHandle)
{
  uint32_t timeout = IDLE_TIMEOUT_MS / IDLE_SLEEP_TIME; /* 2 sec */

  /* Terminate the thread if running */
  if((pHandle->runStateTaskFlag & avs_task_closed)==0)
  {
    pHandle->runStateTaskFlag &=  ~avs_task_running;
    if(((pHandle->runStateTaskFlag & avs_task_closed)==0) && (timeout != 0))
    {
      avs_core_task_delay(IDLE_SLEEP_TIME);
      timeout--;
    }
  }
  if(pHandle->hStateTask )
  {
    avs_core_task_delete(pHandle->hStateTask );
  }
  pHandle->hStateTask  = 0;
  avs_http2_connnection_manager_delete(pHandle);

  if(timeout == 0)
  {
    return AVS_ERROR;
  }
  return AVS_OK;
}

