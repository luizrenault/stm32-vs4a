/**
******************************************************************************
* @file    service_serial_trace.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Trace event instrumentation
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


typedef struct service_serial_entry
{
  char_t          *pText;
  uint32_t      evt_code;
  char_t          *pDetail;
} Service_serial_entry;
#define TEXT_DEFINE(a) #a,a

static Service_serial_entry tTracetList[58] =
{
  { TEXT_DEFINE(EVT_RESET_HTTP), "Reset Http2"},
  { TEXT_DEFINE(EVT_RESTART), "Booting"},
  { TEXT_DEFINE(EVT_WAIT_NETWORK_IP), "Wait an IP"},
  { TEXT_DEFINE(EVT_AUDIO_SYNTHESIZER_TASK_START), "Synthesizer task starts"},
  { TEXT_DEFINE(EVT_AUDIO_SYNTHESIZER_TASK_DYING), "Synthesizer task dies"},
  { TEXT_DEFINE(EVT_PLAYER_TASK_START), "Audio player task starts"},
  { TEXT_DEFINE(EVT_PLAYER_TASK_DYING), "Audio player task dies"},
  { TEXT_DEFINE(EVT_PLAYER_MP3_TASK_START), "Codec MP3 task starts"},
  { TEXT_DEFINE(EVT_PLAYER_MP3_TASK_DYING), "Codec MP3 task dies"},
  { TEXT_DEFINE(EVT_AUDIO_RECOGNIZER_TASK_START), "Recognizer task starts"},
  { TEXT_DEFINE(EVT_AUDIO_RECOGNIZER_TASK_DYING), "Recognizer task dies"},
  { TEXT_DEFINE(EVT_CONNECTION_TASK_START), "Connection task starts"},
  { TEXT_DEFINE(EVT_CONNECTION_TASK_DYING), "Connection task dies"},
  { TEXT_DEFINE(EVT_DOWNSTREAM_TASK_START), "Downstream channel task starts"},
  { TEXT_DEFINE(EVT_DOWNSTREAM_TASK_DYING), "Downstream channel task dies"},
  { TEXT_DEFINE(EVT_STATE_TASK_START), "State task starts"},
  { TEXT_DEFINE(EVT_STATE_TASK_DYING), "State task dies"},
  { TEXT_DEFINE(EVT_NETWORK_SIM_TASK_START), "Bad network simulation task starts"},
  { TEXT_DEFINE(EVT_NETWORK_SIM_TASK_DYING), "Bad network simulation task dies"},
  { TEXT_DEFINE(EVT_REFRESH_TOKEN_TASK_START), "Refresh Token task starts"},
  { TEXT_DEFINE(EVT_REFRESH_TOKEN_TASK_DYING), "Refresh Token task dies"},
  { TEXT_DEFINE(EVT_MP3_TASK_START), "MP3 task starts "},
  { TEXT_DEFINE(EVT_MP3_TASK_DYING), "MP3 task dies "},
  { TEXT_DEFINE(EVT_ENDURANCE_TASK_START), "Test task starts "},
  { TEXT_DEFINE(EVT_ENDURANCE_TASK_DYING), "Test task dies"},
  { TEXT_DEFINE(EVT_CHANGE_MP3_FREQUENCY), "Change MP3 frequency"},
  { TEXT_DEFINE(EVT_SYNC_TIME), "Network time synchronized"},
  { TEXT_DEFINE(EVT_WAIT_TOKEN), "Wait a token"},
  { TEXT_DEFINE(EVT_RENEW_ACCESS_TOKEN), "Token renew via the web page"},
  { TEXT_DEFINE(EVT_VALID_TOKEN), "Token Valid !"},
  { TEXT_DEFINE(EVT_HOSTNAME_RESOLVED), "Amazon resolved"},
  { TEXT_DEFINE(EVT_HTTP2_CONNECTED), "HTTP2 Connection State"},
  { TEXT_DEFINE(EVT_START_REC), "Start recording"},
  { TEXT_DEFINE(EVT_STOP_REC), "Stop recording"},
  { TEXT_DEFINE(EVT_START_SPEAK), "Start speaking"},
  { TEXT_DEFINE(EVT_STOP_SPEAK), "Stop speaking"},
  { TEXT_DEFINE(EVT_WAKEUP), "Wakeup"},
  { TEXT_DEFINE(EVT_START_TLS), "Start TLS initialization"},
  { TEXT_DEFINE(EVT_WRITE_TOKEN), "Write persistent token"},
  { TEXT_DEFINE(EVT_READ_TOKEN), "Read  persistent token"},
  { TEXT_DEFINE(EVT_SEND_SYNCHRO_STATE), "Send synchro state!"},
  { TEXT_DEFINE(EVT_SEND_PING), "Send ping!"},
  { TEXT_DEFINE(EVT_ENDURANCE_ALARM_RING_START), "Start alarm ringing"},
  { TEXT_DEFINE(EVT_ENDURANCE_ALARM_RING_STOP), "Stop  alarm ringing"},
  { TEXT_DEFINE(EVT_ENDURANCE_ADD_ALARM), "Add  an alarm"},
  { TEXT_DEFINE(EVT_ENDURANCE_ADD_ALARM), "Del  an alarm"},
  { TEXT_DEFINE(EVT_CHANGE_VOLUME), "Change volume"},
  { TEXT_DEFINE(EVT_CHANGE_MUTE), "Change mute"},
  { TEXT_DEFINE(EVT_PLAYER_START), "Player starts"},
  { TEXT_DEFINE(EVT_PLAYER_BUFFERING), "Player is buffering"},
  { TEXT_DEFINE(EVT_PLAYER_PAUSED), "Player paused"},
  { TEXT_DEFINE(EVT_PLAYER_RESUMED), "Player resumed"},
  { TEXT_DEFINE(EVT_PLAYER_OPEN_STREAM), "Player open https stream"},
  { TEXT_DEFINE(EVT_PLAYER_CLOSE_STREAM), "Player close https stream"},
  { TEXT_DEFINE(EVT_OPEN_TLS), "Open a TLS Connection"},
  { TEXT_DEFINE(EVT_TLS_CERT_VERIFY_FAILED), " TLS certification error, check the Root CA"},
  { TEXT_DEFINE(EVT_WIFI_CONNECTED), "WIFI connected"},
  {0, 0,0}
};

static int32_t findTextEntry(struct service_serial_entry *pList, uint32_t code);
static int32_t findTextEntry(struct service_serial_entry *pList, uint32_t code)
{

  for(int32_t a = 0; pList[a].pText ; a++)
  {
    if(pList[a].evt_code == code) 
    {
      return a;
    }
  }
  return -1;
}

AVS_Result  service_serial_trace_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
AVS_Result  service_serial_trace_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  time_t curTime;
  static char_t strTime[20];
  AVS_Get_Sync_Time(handle, (AVS_TIME *)&curTime);
  struct tm *ptm = localtime((const time_t *)(uint32_t)&curTime);
  if(ptm)
  {
    strftime(strTime, sizeof(strTime), "%T", ptm);
  }


  if(evt == EVT_ENDURANCE_MSG)
  {
    AVS_TRACE_INFO("%10s :  %s ", strTime, (char_t *)pparam);
    return AVS_OK;
  }
  int32_t index = findTextEntry(tTracetList, evt);
  if(index  != -1)
  {
    if(tTracetList[index].pDetail)
    {
      AVS_TRACE_INFO("%10s :  %-30s : %s(%d)", strTime, tTracetList[index].pDetail, tTracetList[index].pText, pparam);
    }
    else
    {
      AVS_TRACE_INFO("%10s : %-30s  : %s(%d)", strTime, "", tTracetList[index].pText, pparam);
    }

  }



  return AVS_OK;
}

