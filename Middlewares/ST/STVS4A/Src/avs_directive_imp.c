/**
******************************************************************************
* @file    avs_directive_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   main application file
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
* Thit module  implements the directive management
*
*******************************************************************************/

#include "avs_private.h"

#define SIMULATE_VERY_BAD_NETWORK
#define   NETWORK_ERROR_PERIOD_MS       (30*1000)

/* Static variables */

/* Function prototypes */
static void avs_directive_downstream_task(const void  * argument);




/* Process speaker directive */
static AVS_Result avs_directive_process_standard_speaker(AVS_instance_handle *pHandle, json_t *root, const char_t  *pName);
static AVS_Result avs_directive_process_standard_speaker(AVS_instance_handle *pHandle, json_t *root, const char_t  *pName)
{
  if(strcmp(pName, "SetVolume") == 0)
  {
    json_t *directive = json_object_get(root, "directive");
    json_t *payload     = json_object_get(directive, "payload");
    json_t *volume     = json_object_get(payload, "volume");
    int32_t vol = json_integer_value(volume);
    AVS_ASSERT(((vol >= 0) && (vol <= 100)));
    drv_audio.platform_Audio_ioctl(pHandle->pAudio, AVS_IOCTL_AUDIO_VOLUME, 0, vol);
    const char_t  *pJson = avs_json_formater_speaker_event(pHandle, "VolumeChanged");
    if(avs_http2_send_json(pHandle, pJson) != AVS_OK)
    {
      AVS_TRACE_ERROR("Notification event failed");
    }
    avs_json_formater_free(pJson);
    avs_core_message_send(pHandle, EVT_CHANGE_VOLUME, pHandle->pAudio->volumeOut);
    return AVS_EVT_HANDLED;
  }


  if(strcmp(pName, "AdjustVolume") == 0)
  {
    json_t *directive = json_object_get(root, "directive");
    json_t *payload     = json_object_get(directive, "payload");
    json_t *volume     = json_object_get(payload, "volume");
    int32_t vol = json_integer_value(volume);
    AVS_ASSERT(((vol >= -100)  && (vol <= 100)));
    drv_audio.platform_Audio_ioctl(pHandle->pAudio, AVS_IOCTL_AUDIO_VOLUME, 1, vol);
    const char_t *pJson = avs_json_formater_speaker_event(pHandle, "VolumeChanged");
    if(avs_http2_send_json(pHandle, pJson) != AVS_OK)
    {
      AVS_TRACE_ERROR("Notification event failed");
    }
    avs_json_formater_free(pJson);
    avs_core_message_send(pHandle, EVT_CHANGE_VOLUME, pHandle->pAudio->volumeOut);

    return AVS_EVT_HANDLED;
  }



  if(strcmp(pName, "SetMute") == 0)
  {
    json_t *directive = json_object_get(root, "directive");
    json_t *payload     = json_object_get(directive, "payload");
    json_t *mute     = json_object_get(payload, "mute");
    if(json_is_true(mute) != 0 )
    {
      pHandle->pAudio->volumeMuted = TRUE;
    }
    if(json_is_false(mute) != 0 )
    {
      pHandle->pAudio->volumeMuted  = FALSE;
    }

    /* Notice it is up to the user app to manage the mute flag by calling AVS_Audio_Mute or to act differently */

    const char_t  *pJson = avs_json_formater_speaker_event(pHandle, "MuteChanged");
    if(avs_http2_send_json(pHandle, pJson) != AVS_OK)
    {
      AVS_TRACE_ERROR("Notification event failed");
    }
    avs_json_formater_free(pJson);
    avs_core_message_send(pHandle, EVT_CHANGE_MUTE, pHandle->pAudio->volumeMuted);

    return AVS_EVT_HANDLED;
  }
  return AVS_OK;
}





/*

 This function catches directive and process it to  send a simplified evt to the user
 such as EVT_ADD_ALARM,etc....
 this is useful only for standard  messages
 if the call returns false ( default state) , the SDK will send a general evt directive
 if the call returns true, this means that the event is cautch and already processed , the generic EVT_DIRECTIVE won't be send

*/

static AVS_Result avs_directive_process_standard(AVS_instance_handle *pHandle, json_t *root, const char  *pNameSpace, const char  *pName, const char *pJSon);
static AVS_Result avs_directive_process_standard(AVS_instance_handle *pHandle, json_t *root, const char  *pNameSpace, const char  *pName, const char *pJSon)
{
  if(strcmp(pNameSpace, "SpeechSynthesizer") == 0)
  {
    if(strcmp(pName, "Speak") == 0)
    {
      avs_core_atomic_write(&pHandle->codeAudioDir, AVS_AUDIO_DIR_SPEAK);
    }

    return avs_core_message_send(pHandle, EVT_DIRECTIVE_SYNTHESIZER, AVS_EVT_PARAM(root));
  }

  if(strcmp(pNameSpace, "Notifications") == 0)
  {
    return avs_core_message_send(pHandle, EVT_DIRECTIVE_SYNTHESIZER, AVS_EVT_PARAM(root));
  }
  if(strcmp(pNameSpace, "SpeechRecognizer") == 0)
  {
    if(strcmp(pName, "ExpectSpeech") == 0)
    {
      avs_core_atomic_write(&pHandle->codeAudioDir, AVS_AUDIO_DIR_EXPECT_SPEECH);
    }

    if(strcmp(pName, "ExpectSpeechTimedOut") == 0)
    {
      avs_core_atomic_write(&pHandle->codeAudioDir, AVS_AUDIO_DIR_EXPECT_SPEECH_TIMEOUT);

    }

    if(strcmp(pName, "Recognizes") == 0)
    {
      avs_core_atomic_write(&pHandle->codeAudioDir, AVS_AUDIO_DIR_RECOGNIZE);
    }

    if(strcmp(pName, "StopCapture") == 0)
    {

      if(avs_core_message_send(pHandle, EVT_STOP_CAPTURE, AVS_EVT_PARAM(root)) == AVS_EVT_HANDLED)
      {
        return AVS_EVT_HANDLED;
      }
    }
    return avs_core_message_send(pHandle, EVT_DIRECTIVE_SPEECH_RECOGNIZER, AVS_EVT_PARAM(root));
  }
  if(strcmp(pNameSpace, "AudioPlayer") == 0)
  {
    return avs_core_message_send(pHandle, EVT_DIRECTIVE_AUDIO_PLAYER, AVS_EVT_PARAM(root));
  }

  if(strcmp(pNameSpace, "Speaker") == 0)
  {
    /* Https://developer.amazon.com/docs/alexa-voice-service/speaker.html */
    if(avs_directive_process_standard_speaker(pHandle, root, pName) != AVS_EVT_HANDLED)
    {
      return avs_core_message_send(pHandle, EVT_DIRECTIVE_SPEAKER, AVS_EVT_PARAM(root));
    }
  }
  if(strcmp(pNameSpace, "System") == 0)
  {
    return avs_core_message_send(pHandle, EVT_DIRECTIVE_SYSTEM, AVS_EVT_PARAM(root));
  }

  if(strcmp(pNameSpace, "Alerts") == 0)
  {

    return avs_core_message_send(pHandle, EVT_DIRECTIVE_ALERT, AVS_EVT_PARAM(root));
  }
  return AVS_OK;
}


/*

 Process a directive coming from the down stream or the state machine
 the function analyse the directive type and notify the app or call standard parser


*/
static AVS_Result avs_directive_process(AVS_instance_handle *pHandle, const char *pJSon, json_t *root);
static AVS_Result avs_directive_process(AVS_instance_handle *pHandle, const char *pJSon, json_t *root)
{
  AVS_PRINTF(AVS_TRACE_LVL_DIRECTIVE, "Directive :");
  AVS_PRINT_STRING(AVS_TRACE_LVL_DIRECTIVE, pJSon);
  AVS_TRACE_USER(AVS_TRACE_LVL_DIRECTIVE, "");

  json_t *directive = json_object_get(root, "directive");
  json_t *payload    = json_object_get(directive, "payload");
  json_t *token     = json_object_get(payload, "token");
  json_t *header     = json_object_get(directive, "header");

  /* First we have to keep the directive token */
  const char_t  *pToken =   json_string_value(token);
  if(token && pToken)
  {
    pHandle->pLastToken = avs_core_mem_realloc(avs_mem_type_heap, pHandle->pLastToken, strlen(pToken) + 1);
    strcpy((char *)pHandle->pLastToken, pToken);
  }

  /* Pre-parse name space and name */
  const char_t *pNameSpace = json_string_value(json_object_get(header, "namespace"));
  const char_t *pName      = json_string_value(json_object_get(header, "name"));
  AVS_ASSERT( ((pNameSpace != 0)  && (pName != 0)));


  /* Dump the directive */
  AVS_PRINTF(AVS_TRACE_LVL_JSON, "RCV JSON : ");
  AVS_PRINT_STRING(AVS_TRACE_LVL_JSON, pJSon);
  AVS_PRINT_STRING(AVS_TRACE_LVL_JSON, "\r");

  /* Clear the directive type */
  /* It will be used by the state thread to process its end dialogue */

  avs_core_atomic_write(&pHandle->codeAudioDir, AVS_AUDIO_DIR_NONE);

  /* Now let's parse the directive */
  if(avs_directive_process_standard(pHandle, root, pNameSpace, pName, pJSon) != AVS_EVT_HANDLED )
  {
    avs_core_message_send(pHandle, EVT_DIRECTIVE, AVS_EVT_PARAM(root));
  }

  return AVS_OK;
}

/*

 Extract json payload and parse it


*/
AVS_Result avs_directive_process_json(AVS_instance_handle *pHandle, const char *pJson)
{
  json_error_t err;
  avs_core_mutex_lock(&pHandle->lockParseJson);

  json_t*  root = json_loads(pJson, 0, &err);
  json_t* dir=json_object_get((root), "directive");
  if(json_is_object(dir)!=0)
  {
    avs_directive_process(pHandle, pJson, root);
  }
  json_decref(root);
  avs_core_mutex_unlock(&pHandle->lockParseJson);
  return  AVS_OK;
}


/*

 TODO check  this function ...


*/
void avs_directive_downstream_channel_state_set(AVS_instance_handle *pHandle, uint32_t  state);
void avs_directive_downstream_channel_state_set(AVS_instance_handle *pHandle, uint32_t  state)
{
  avs_core_atomic_write(&pHandle->dirDownstreamState, state);
}


/*

 TODO check  this function ...


*/
uint32_t  avs_directive_downstream_channel_state_get(AVS_instance_handle *pHandle)
{
  return   avs_core_atomic_read(&pHandle->dirDownstreamState);

}



/*

 Create the down stream channel ( directive channel)


*/
AVS_Result avs_directive_down_stream_channel_create( AVS_instance_handle *pHandle)
{


  /* Create some mutex */
  AVS_VERIFY(avs_core_mutex_create(&pHandle->lockParseJson));
  AVS_VERIFY(avs_core_mutex_create(&pHandle->hDirectiveLock));

  /* Initialize the thread status */
  pHandle->runDnChannelFlag =  avs_task_closed;

  /* Create the thread */
  pHandle->hDownstreamChannel = avs_core_task_create(DOWN_STREAM_TASK_NAME, avs_directive_downstream_task, pHandle, DOWN_STREAM_TASK_STACK_SIZE, DOWN_STREAM_TASK_PRIORITY);
  if(!pHandle->hDownstreamChannel)
  {
    AVS_TRACE_ERROR("Create task %s", DOWN_STREAM_TASK_NAME);
    return AVS_ERROR;
  }
  return AVS_OK;

}
/*

 Close kindly the Directive connection


*/
void avs_http2_directive_close(AVS_instance_handle *pHandle);
void avs_http2_directive_close(AVS_instance_handle *pHandle)
{
  AVS_TRACE_DEBUG("Enter close directive channel");
  /* Make sure we don't force close from http cnx */
  avs_core_mutex_lock(&pHandle->hDirectiveLock);
  /* Check if are already closed */
  if(pHandle->hDirectiveStream)
  {
    pHandle->runDnChannelFlag |= avs_task_about_closing;
    /* Move back the time-out from 1H to 500 ms to exit the close as fast as possible */
    http2ClientSetStreamTimeout(pHandle->hDirectiveStream, MIN_TIMEOUT);
    /* Cancel the stream */
    http2ClientCancelStream(pHandle->hDirectiveStream);
    /* Close stream */
    http2ClientCloseStream(pHandle->hDirectiveStream);
    /* Notify closed */
    pHandle->hDirectiveStream = 0;
  }
  AVS_TRACE_DEBUG("Leave close directive channel");
  avs_core_mutex_unlock(&pHandle->hDirectiveLock);

}

/*

 Delete directive connection from another thread
 This function is called from the connection thread and is in charge to clean up
 the connection in order to restart a new connection from scratch
 this state in generally called after a disconnection or an error
 it is the tricky part because stopping a connection could generate memory leak
 or hang in the low-level network due to semaphore waiting for ever

*/


AVS_Result avs_directive_down_stream_channel_delete(AVS_instance_handle *pHandle)
{
  AVS_TRACE_DEBUG("Enter delete directive channel");
  /* Check if already closed */
  if(pHandle->hDownstreamChannel  )
  {
    /* Time-out guard */
    int32_t timeout = WAIT_EVENT_TIMEOUT / 100; /* 2 secs */

    /* Check if the tread is already terminated */
    if((pHandle->runDnChannelFlag & (uint32_t)avs_task_closed)==0)
    {
      /* Force it to release */
      pHandle->runDnChannelFlag &=  ~avs_task_running;

      /* Default time-out is  1H : too much when network is down => set time-out to 2 secs instead of 1H */
      http2ClientSetStreamTimeout(pHandle->hDirectiveStream, WAIT_EVENT_TIMEOUT);

      /* Cancel the stream, to release the event */
      http2ClientCancelStream(pHandle->hDirectiveStream);

      /* Wait for the thread end */
      while(((pHandle->runDnChannelFlag & (uint32_t)avs_task_closed)==0) &&(timeout!=0))
      {
        avs_core_task_delay(100);
        timeout--;
      }
      if(timeout == 0)
      {
        /* The connection doesn't respond, close hard */
        AVS_TRACE_ERROR("Delete directive forced by timeout");
        /* Check if we are in the close, otherwise force the close from another thread */
        if((pHandle->runDnChannelFlag & (uint32_t)avs_task_about_closing)==0)
        {
          avs_http2_directive_close(pHandle);
        }

      }

    }
    /* Signal closed */
    pHandle->runDnChannelFlag =  avs_task_closed;
    /* Delete some mutex */
    avs_core_mutex_delete(&pHandle->hDirectiveLock);
    avs_core_mutex_delete(&pHandle->lockParseJson);
    /* Delete thread */
    if(pHandle->hDownstreamChannel != 0 )
    {
      avs_core_task_delete(pHandle->hDownstreamChannel );
    }
    /* Signal thread terminated */
    pHandle->hDownstreamChannel  = 0;
  }
  AVS_TRACE_DEBUG("Leave delete directive channel");
  return AVS_OK;
}

/* Developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection */
/* Directive thread */

static void avs_directive_downstream_task(const void  * argument)
{
  const char_t *pType=0;
  Http2Status err = HTTP2_STATUS_OK;
  Http2Status result = HTTP2_STATUS_OK;
  AVS_instance_handle *pHandle = (AVS_instance_handle *)(uint32_t)argument;
  AVS_ASSERT( argument  != 0);
  avs_core_message_send(pHandle, EVT_DOWNSTREAM_TASK_START, 0);
  pHandle->runDnChannelFlag = avs_task_running;
  while((pHandle->runDnChannelFlag & avs_task_running) != 0)
  {
    avs_directive_downstream_channel_state_set(pHandle, DOWNSTREAM_INITIALIZING);
    /* Create the http/2 stream to receive AVS server directives */
    pHandle->hDirectiveStream = http2ClientOpenStream(pHandle->hHttpClient);
    /* Set the stream read time out to 60 minutes, as specified by Amazon */
    http2ClientSetStreamTimeout(pHandle->hDirectiveStream, AVS_DIRECTIVE_TIMEOUT);

    err = http2ClientSetHeaderField(pHandle->hDirectiveStream, ":method", "GET");
    if(!err)
    {
      err = http2ClientSetHeaderField(pHandle->hDirectiveStream, ":scheme", "https");
    }
    if(!err)
    {
      err = http2ClientSetHeaderField(pHandle->hDirectiveStream, ":authority", (char_t *)pHandle->pFactory->urlEndPoint);  /* [TODO] is that needed ? seen in http2 stack user manual, but not in amazon doc */
    }
    if(!err)
    {
      err = http2ClientSetHeaderField(pHandle->hDirectiveStream, ":path", "/v20160207/directives");
    }
    if(!err)
    {
      err = http2ClientSetHeaderField(pHandle->hDirectiveStream, "authorization", (char_t *)(uint32_t)avs_token_access_lock_and_get(pHandle));
      avs_token_access_unlock(pHandle);
    }
    if(!err)
    {
      err = http2ClientWriteHeader(pHandle->hDirectiveStream, HTTP2_CLIENT_FLAG_END_STREAM);
    }
    if(!err)
    {
      if(http2ClientReadHeader(pHandle->hDirectiveStream) != 0)
      {
        break;
      }
      uint32_t status = http2ClientGetStatus(pHandle->hDirectiveStream);
      if ((status >= 200) && (status < 300) )
      {
        avs_directive_downstream_channel_state_set(pHandle, DOWNSTREAM_READY);
        /* Loops until error */
        while((err==0) && (( pHandle->runDnChannelFlag & (uint32_t)avs_task_running) != 0))
        {
          /* Wait and read the header */
          result = http2ClientReadMultipartHeader(pHandle->hDirectiveStream);
          if(result == HTTP2_STATUS_TIMEOUT)
          {

          }
          else
          {
            if(result!= 0)
            {
              break;
            }
              pType = http2ClientGetMultipartType(pHandle->hDirectiveStream);
            /* If Json , let's parce it */
            if(strcmp(pType, AVS_CONTENT_TYPE_JSON) == 0)
            {
              avs_short_alloc_object     *pJSon;
              uint32_t szJSon;
              /* Read the json body */
              if(avs_http2_read_multipart_stream(pHandle, pHandle->hDirectiveStream, MAX_SIZE_JSON, &pJSon, &szJSon) == AVS_OK)
              {
                /* Stringify */
                ((char_t *)pJSon)[szJSon] = 0;
                /* Parse the json */
                avs_directive_process_json(pHandle, pJSon);
                avs_core_short_free(pJSon);
              }
            }

          }
        }
      }
    }
    /* If an error occurs, quit the thread */
    pHandle->runDnChannelFlag &= ~avs_task_running;
  }
  /* Close the directive stream */
  avs_http2_directive_close(pHandle);
  /* Signal the thread is terminate */
  avs_core_message_send(pHandle, EVT_DOWNSTREAM_TASK_DYING, 0);
  avs_directive_downstream_channel_state_set(pHandle, DOWNSTREAM_DEAD_OR_DYING);
  pHandle->runDnChannelFlag |= avs_task_closed;
  avs_core_task_end();
}

/* Send the SynchronizeState Event */
AVS_Result avs_directive_send_synchro(AVS_instance_handle *pHandle)
{
  /* Check if the client is disconnected */
  if(pHandle->hHttpClient == 0)
  {
    return AVS_ERROR;
  }

  /* Create the synchro state json */
  const char_t   *pJson = avs_json_formater_synchro_state_event(pHandle);
  /* Send the payload */
  AVS_Result result = avs_http2_send_json(pHandle, pJson);
  /* Free the tmp string */
  avs_json_formater_free(pJson );
  /* Signal the event */
  avs_core_message_send(pHandle, EVT_SEND_SYNCHRO_STATE, result);
  if (result != AVS_OK)
  {
    AVS_TRACE_ERROR(" Send synchro event");
    return AVS_ERROR;
  }
  return AVS_OK;
}


/*
Post a synchro state
 the function is called form the idle state
 if pHandle->posSynchro is true we send a synchro
 this function is mandatory due to the fact we can't call an event from a event ( ie EVT_UPDATE_SYNCHRO)
 synchro has no real-time requirement, so we can dispatch this message in the idle time

*/

AVS_Result avs_directive_post_synchro(AVS_instance_handle *pHandle)
{
  if(avs_core_atomic_read(&pHandle->postSynchro) == 0)
  {
    return AVS_OK;
  }
  avs_core_atomic_write(&pHandle->postSynchro, 0);
  AVS_Result ret = avs_directive_send_synchro(pHandle);
  return ret;
}

