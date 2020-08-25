/**
******************************************************************************
* @file    avs_json_formater_imp.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This module manages  avs json message
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

#define FORMAT_MESSAGE_ID "msg-%lu"
#define FORMAT_DIALOG_ID  "dlg-%lu"


/*

 base header formatting


*/
static uint32_t avs_json_format_base_header(AVS_instance_handle *pHandle, json_t *header, const char *pNameSpace, const char *pName);
static uint32_t avs_json_format_base_header(AVS_instance_handle *pHandle, json_t *header, const char *pNameSpace, const char *pName)
{
  uint32_t ret = 0;
  char_t msgid[32];
  sprintf(msgid, FORMAT_MESSAGE_ID, pHandle->messageIdCounter);
  pHandle->messageIdCounter++;
  /* Format "header" object */
  ret |= (uint32_t)json_object_set_new(header, "namespace", json_string(pNameSpace));
  ret |= (uint32_t)json_object_set_new(header, "name", json_string(pName));
  ret |= (uint32_t)json_object_set_new(header, "messageId", json_string(msgid));
  return ret;
}
static uint32_t avs_json_format_dialog(AVS_instance_handle *pHandle, json_t *header);
static uint32_t avs_json_format_dialog(AVS_instance_handle *pHandle, json_t *header)
{
  uint32_t ret = 0;
  char_t dlgid[32];
  sprintf(dlgid, FORMAT_DIALOG_ID, pHandle->dialogIdCounter);
  ret |= (uint32_t)json_object_set_new(header, "dialogRequestId", json_string(dlgid));
  return ret;
}
static void * json_alloc(size_t size);
static void * json_alloc(size_t size)
{
  return avs_core_mem_alloc(avs_mem_type_heap, size);
}
static void json_free(void *ptr);
static void json_free(void *ptr)
{
  avs_core_mem_free(ptr);
}


AVS_Result avs_json_formater_init(AVS_instance_handle *pHandle)
{
  json_set_alloc_funcs(json_alloc, json_free);

  return avs_core_short_object_init(pHandle);
}

AVS_Result avs_json_formater_term(AVS_instance_handle *pHandle)
{
  avs_core_short_object_term(pHandle);
  return AVS_OK;
}


/*
  Format a synth context
*/
static json_t *avs_json_formater_synthesizer_context(AVS_instance_handle *pHandle);
static json_t *avs_json_formater_synthesizer_context(AVS_instance_handle *pHandle)
{
  json_t *root;
  uint32_t err = 0;
  uint32_t offset = 0;
  if(pHandle->isSpeaking)
  {
    offset  = osKernelSysTick() - pHandle->speakingStart;
  }
  root = json_object();
  json_t *header = json_object();
  json_t *payload = json_object();
  err |= avs_json_format_base_header(pHandle, header, "SpeechSynthesizer", "RecognizerState");
  err |= (uint32_t)json_object_set_new(payload, "token", json_string(pHandle->pLastToken != 0 ? (const char_t *)(uint32_t)pHandle->pLastToken :""));
  err |= (uint32_t)json_object_set_new(payload, "offsetInMilliseconds", json_integer(offset));
  err |= (uint32_t)json_object_set_new(payload, "playerActivity", json_string(pHandle->isSpeaking ? "PLAYING" : "FINISHED"));


  err |= (uint32_t)json_object_set_new(root, "header", header);
  err |= (uint32_t)json_object_set_new(root, "payload", payload);
  AVS_ASSERT(err == 0);
  return root;

}
json_t *avs_json_formater_recognizer_context(AVS_instance_handle *pHandle);
json_t *avs_json_formater_recognizer_context(AVS_instance_handle *pHandle)
{
  json_t *root;
  uint32_t err = 0;
  root = json_object();
  json_t *header = json_object();
  json_t *payload = json_object();
  err |= avs_json_format_base_header(pHandle, header, "SpeechRecognizer", "RecognizerState");
  err |= (uint32_t)json_object_set_new(payload, "wakeword", json_string((char *)(uint32_t)pHandle->pFactory->alexaKeyWord));
  err |= (uint32_t)json_object_set_new(root, "header", header);
  err |= (uint32_t)json_object_set_new(root, "payload", payload);
  AVS_ASSERT(err == 0);
  return root;

}

static json_t *avs_json_formater_speaker_context(AVS_instance_handle *pHandle);
static json_t *avs_json_formater_speaker_context(AVS_instance_handle *pHandle)
{
  json_t *context;
  /* Initialize status code */
  /* Initialize JSON objects */
  context = json_object();
  avs_json_formater_speaker_contents(pHandle, context, "VolumeState");

  /* Return a pointer to the JSON-encoded context */
  return context;
}



/*


 format a json that reflect the system state


*/

uint32_t avs_json_formater_add_context(AVS_instance_handle *pHandle, void *root)
{
  json_t *context = json_object_get(root, "context");
  /* Add internal context */
  AVS_VERIFY(json_array_append_new(context, avs_json_formater_speaker_context(pHandle)) == 0);
#if 0
  /* Note: RecognizerState is only required if your client uses Cloud-Based Wake Word Verification. */
  AVS_VERIFY(json_array_append_new(context, avs_json_formater_recognizer_context(pHandle)) == 0);
#endif
   AVS_VERIFY(json_array_append_new(context, avs_json_formater_synthesizer_context(pHandle)) == 0);
  /* Add external context */
  avs_core_message_send(pHandle, EVT_UPDATE_SYNCHRO, AVS_EVT_PARAM(root));
  return 1;
}






/*


 format a json that reflect the system state


*/

const char_t *avs_json_formater_synchro_state_event(AVS_instance_handle *pHandle)
{
  char_t *message;
  json_t *root;
  json_t *event;
  json_t *header;
  json_t *payload;

  /* Initialize JSON objects */
  root = json_object();
  event = json_object();
  header = json_object();
  payload = json_object();

  /* Initialize JSON arrays */
  json_t *context = json_array();

  AVS_VERIFY(avs_json_format_base_header(pHandle, header, "System", "SynchronizeState") == 0);

  /* Links elements */
  AVS_VERIFY(json_object_set_new(event, "header", header) == 0);
  AVS_VERIFY(json_object_set_new(event, "payload", payload) == 0);
  AVS_VERIFY(json_object_set_new(root, "context", context) == 0);
  AVS_VERIFY(json_object_set_new(root, "event", event) == 0);
  /* Add the context state to the event */
  AVS_VERIFY(avs_json_formater_add_context(pHandle, root));


  message = (char_t *)json_dumps(root, 0);
  json_decref(root);
  AVS_ASSERT(message != 0);
  return message;
}

/* Https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/avs-http2-requests#examples */

const char_t *avs_json_formater_recognizer_event(AVS_instance_handle *pHandle, const char_t *pInitiator )
{
  /* Initialize JSON objects */
  char_t   *pJson = 0;
  json_t *root = json_object();
  json_t *evt = json_object();
  json_t *header = json_object();
  json_t *payload = json_object();
  /* Initialize JSON arrays */
  json_t *context = json_array();


  /* Generate the std header */
  AVS_VERIFY(avs_json_format_base_header(pHandle, header, "SpeechRecognizer", pInitiator ) == 0);
  AVS_VERIFY(avs_json_format_dialog(pHandle, header) == 0);

  /* Voice recognition profile */
  if (pHandle->pFactory->profile == AVS_PROFILE_CLOSE_TALK)
  {
    AVS_VERIFY(json_object_set_new(payload, "profile", json_string("CLOSE_TALK")) == 0);
  }
  if (pHandle->pFactory->profile == AVS_PROFILE_NEAR_FIELD)
  {
    AVS_VERIFY(json_object_set_new(payload, "profile", json_string("NEAR_FIELD")) == 0);
  }
  if (pHandle->pFactory->profile == AVS_PROFILE_FAR_FIELD)
  {
    AVS_VERIFY(json_object_set_new(payload, "profile", json_string("FAR_FIELD")) == 0);
  }

  /* Format */
  AVS_ASSERT(pHandle->pFactory->recognizerSampleRate == 16000); /* Only 16KHZ supported */
  AVS_VERIFY(json_object_set_new(payload, "format", json_string("AUDIO_L16_RATE_16000_CHANNELS_1")) == 0);


  json_t *initiator = json_object();


  AVS_VERIFY(json_object_set_new(payload, "initiator", initiator) == 0);

  /* Initiator (and check that profile is consistent) */
  if (pHandle->pFactory->initiator == AVS_INITIATOR_PUSH_TO_TALK)
  {
    if(pHandle->pFactory->profile != AVS_PROFILE_CLOSE_TALK)
    {
      AVS_TRACE_ERROR( "Bad profile, must be AVS_INITIATOR_PUSH_TO_TALK");
    }
    AVS_VERIFY(json_object_set_new(initiator, "type", json_string("PRESS_AND_HOLD")) == 0);
  }
  else
  {
    if(pHandle->pFactory->profile == AVS_PROFILE_CLOSE_TALK)
    {
      AVS_TRACE_ERROR( "Bad profile, must be NEAR_FIELD or FAR_FIELD");
    }
    if (pHandle->pFactory->initiator == AVS_INITIATOR_TAP_TO_TALK)
    {
      AVS_VERIFY(json_object_set_new(initiator, "type", json_string("TAP")) == 0);
    }
    else
    {
      AVS_VERIFY(json_object_set_new(initiator, "type", json_string("WAKEWORD")) == 0);
    }
  }

  /* Link header payload event to the root */
  AVS_VERIFY(json_object_set_new(evt, "header", header) == 0);
  AVS_VERIFY(json_object_set_new(evt, "payload", payload) == 0);
  AVS_VERIFY(json_object_set_new(root, "event", evt) == 0);
  AVS_VERIFY(json_object_set_new(root, "context", context) == 0);

  /* Add the context state to the event */
  AVS_VERIFY(avs_json_formater_add_context(pHandle, root));


  pJson = json_dumps(root, 0);
  json_decref(root);
  AVS_ASSERT(pJson != 0);

  return pJson ;
}


const char_t* avs_json_formater_notification_event(AVS_instance_handle *pHandle, const char *pNameSpace, const char *pName, const char_t *pToken, const char_t*pDialogId)
{
  uint32_t err = 0;
  json_t *root  = json_object();
  json_t *event = json_object();
  json_t *header = json_object();
  json_t *payload = json_object();
  /* Initialize JSON arrays */
  json_t *context = json_array();


  err |= avs_json_format_base_header(pHandle, header, pNameSpace, pName);

  if(pDialogId)
  {
    err |= (uint32_t)json_object_set_new(header, "dialogRequestId", json_string(pDialogId));

  }

  err |= (uint32_t)json_object_set_new(payload, "token", json_string(pToken));
  /* Links */
  err |= (uint32_t)json_object_set_new(root, "event", event);
  err |= (uint32_t)json_object_set_new(event, "header", header);
  err |= (uint32_t)json_object_set_new(event, "payload", payload);

  AVS_VERIFY(json_object_set_new(root, "context", context) == 0);
  /* Add the context state to the event */
  AVS_VERIFY(avs_json_formater_add_context(pHandle, root));


  AVS_ASSERT(err == 0);
  char_t *pJson = json_dumps(root, 0);
  json_decref(root);
  AVS_ASSERT(pJson != 0);
  return pJson;
}


void avs_json_formater_free(const char_t  *pJson)
{
  AVS_ASSERT(pJson != 0);
  jsonp_free((void *)(uint32_t)pJson);
}




uint32_t     avs_json_formater_speaker_contents(AVS_instance_handle *pHandle, void *base, const char_t *pName)
{
  uint32_t err = 0;
  json_t *header = json_object();
  json_t *payload = json_object();

  err |= avs_json_format_base_header(pHandle, header, "Speaker", pName);
  err |= (uint32_t)json_object_set_new(payload, "mute", json_boolean(pHandle->pAudio->volumeMuted));
  err |= (uint32_t)json_object_set_new(payload, "volume", json_integer(pHandle->pAudio->volumeOut));
  err |= (uint32_t)json_object_set_new(base, "header", header);
  err |= (uint32_t)json_object_set_new(base, "payload", payload);


  return err;
}



const char_t * avs_json_formater_speaker_event(AVS_instance_handle *pHandle, const char_t *pName)
{
  uint32_t err = 0;
  json_t *root = json_object();
  json_t *event = json_object();
  /* Initialize JSON arrays */
  json_t *context = json_array();

  err = avs_json_formater_speaker_contents(pHandle, event, pName);
  err |= (uint32_t)json_object_set_new(root, "event", event);

  AVS_VERIFY(json_object_set_new(root, "context", context) == 0);

  /* Add the context state to the event */
  AVS_VERIFY(avs_json_formater_add_context(pHandle, root));


  AVS_ASSERT(err == 0);
  const char_t *pJson = json_dumps(root, 0);
  json_decref(root);
  AVS_ASSERT(pJson != 0);
  return pJson;
}


