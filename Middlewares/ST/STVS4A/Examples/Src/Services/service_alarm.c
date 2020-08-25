/**
******************************************************************************
* @file    service_alarm.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   manage  timer and alarm notification
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

/******************************************************************************
 *
 * This code manages a list of alarms or timer, and  manage add / del and fire
 * And reacts to Directive Events
 ******************************************************************************/

#include "service.h"

#define DEFAULT_TIME_ZONE              0                /* +2 for france */
#define HOUR_IN_SEC                    (60*60)
#define MAX_TOKEN_SIZE                 256
#define BASE_YEAR_EPOCH                1900
#define FORMAT_MESSAGE_ID               "Alrm-%lu"
#define VOLUME_ALERT                    10
#define ALARM_DELAY_IN_SEC              10              /* Delay the alarm rings */

static mutex_t                 lockAlarm  ;
typedef struct
{
  unsigned int  timer : 1;
  unsigned int  armed : 1;
  unsigned int  active: 1;
  unsigned int  forground : 1;
  unsigned int  background : 1;
} alarm_flags;


typedef struct   time_slot
{
  AVS_TIME      time;
  int32_t       alarm_delay;
  alarm_flags   flags;
  char_t        sToken[MAX_TOKEN_SIZE];
} AlarmTimeSlot;

#define DELAY_ALARM                     4
#define MAX_ALARM                       6
#define TASK_NAME_ALARM                 "AVS:Alarm Idle"
#define TASK_PRIORITY_ALARM             (osPriorityIdle)
#define TASK_ALARM_STACK                200            /* More than 500 */

#define ALARM_PERIOD_PRECISION          500
static task_t *                      hAlarmTask;
static uint32_t                         gAvsState;
static char_t                             tBufferAlarmString[512];        /* Buffer used to exchange info with the UI */
static AlarmTimeSlot                    tAlarm[MAX_ALARM];              /* Slots for an alarm */
static int32_t                          gTimeZone = DEFAULT_TIME_ZONE;    /* Time zone France = +2 */
static uint32_t                         messageIdCounter;               /* Message id index */
static void                             service_alarm_event(AVS_Handle handle, const char_t *pToken, const char_t *pName);
static const void *                     pAlarm_Active_Wav;

__STATIC_INLINE  void service_alarm_lock(void);
__STATIC_INLINE  void service_alarm_lock(void)
{
  mutex_Lock(&lockAlarm);

}
__STATIC_INLINE  void service_alarm_unlock(void);
__STATIC_INLINE  void service_alarm_unlock(void)
{
  mutex_Unlock(&lockAlarm);
}



/* Disable a specific alarm */
AVS_Result  service_alarm_disable_item(int32_t index)
{
  if((unsigned)index >= MAX_ALARM) 
  {
    return AVS_ERROR;
  }
  tAlarm[index].flags.armed = 0;
  tAlarm[index].flags.active = 0;
  return AVS_OK;
}

/* Enable a specific alarm */
AVS_Result  service_alarm_enable_item(int32_t index)
{
  if((unsigned)index >= MAX_ALARM) 
  {
    return AVS_ERROR;
  }
  tAlarm[index].flags.armed = 1;
  return AVS_OK;
}


/* Return the number item armed */
static uint32_t service_alarm_get_armed(void);
static uint32_t service_alarm_get_armed(void)
{
  uint32_t nbItem = 0;
  service_alarm_lock ();
  for(int32_t index = 0; index  < MAX_ALARM  ; index++)
  {
    if(tAlarm[index].flags.armed)  
    {
      nbItem++;
    }
  }
  service_alarm_unlock ();
  return nbItem;
}


/* Add an alarm  in the first free slot */
int32_t service_alarm_add_item(AVS_TIME reftime, const char_t *pToken)
{
  int32_t item = -1;
  service_alarm_lock ();
  for(int32_t index = 0; index  < MAX_ALARM  ; index++)
  {
    if(!tAlarm[index].flags.armed)
    {
      AVS_ASSERT(strlen(pToken) + 1 < MAX_TOKEN_SIZE);
      strncpy(tAlarm[index].sToken, pToken, MAX_TOKEN_SIZE - 1);
      tAlarm[index].time = reftime;
      tAlarm[index].flags.armed   = 1;
      tAlarm[index].flags.timer   = 0;
      tAlarm[index].flags.forground = 0;
      tAlarm[index].flags.background = 0;

      item = index;
      break;
    }

  }
  service_alarm_unlock ();
  return item;
}

/* Return the number active alarm */
static uint32_t service_get_active(void);
static uint32_t service_get_active(void)
{
  uint32_t nbActive = 0;
  for(int32_t index = 0; index  < MAX_ALARM  ; index++)
  {
    if((tAlarm[index].flags.armed != 0)  && (tAlarm[index].flags.active!= 0) )   
    {
      nbActive++;
    }
  }
  return nbActive ;
}

/* Stops all active alarms */
void  service_alarm_snooze(void)
{
  if(service_get_active() == 0) 
  {
    return;
  }

  for(int32_t index = 0; index  < MAX_ALARM  ; index++)
  {
    if((tAlarm[index].flags.armed!= 0)  && (tAlarm[index].flags.active!= 0)  )
    {
      service_alarm_lock();
      /* Will be stooped by the task in the next loop ( ie send event etc....) */
      tAlarm[index].alarm_delay = 0;
      service_alarm_unlock();
    }
  }
}

/* Find an alarm  for an token and return its index */
int32_t service_alarm_find_item(const char_t *pToken)
{
  for(int32_t index = 0; index  < MAX_ALARM  ; index++)
  {
    int32_t bFound = strcmp(pToken, tAlarm[index].sToken);
    if((tAlarm[index].flags.armed != 0)  && (bFound== 0))   
    {
      return index;
    }
  }
  return -1;
}

/* Return a pointer on a string describing active alarms for the UI */
char_t * service_alarm_get_string(void)
{
  memset(tBufferAlarmString, 0, sizeof(tBufferAlarmString));

  if(service_alarm_get_armed() == 0)
  {
    snprintf(tBufferAlarmString, sizeof(tBufferAlarmString), "Alarm Empty");
    return tBufferAlarmString;

  }

  service_alarm_lock();

  AVS_TIME curTime ;
  AVS_Get_Sync_Time(hInstance, &curTime);
  static char_t localTime1[30];
  static char_t localTime2[30];
  static char_t localLine[60];

  for(int32_t a = 0; a < MAX_ALARM  ; a++)
  {
    if(tAlarm[a].flags.armed)
    {
      if(!tAlarm[a].flags.timer)
      {
        if(tAlarm[a].flags.active)
        {
          snprintf(localLine, sizeof(localLine), "%02ld: (A) Active (%ld)\r", a, tAlarm[a].alarm_delay);
        }
        else
        {
          AVS_TIME itime = tAlarm[a].time;
          itime += service_alarm_get_time_zone() * HOUR_IN_SEC;
          struct tm *ptm = localtime((const time_t *)&itime);
          if(ptm)
          {
            strftime(localTime1, sizeof(localTime1), "%D %T", ptm);
            snprintf(localLine, sizeof(localLine), "%02ld: (A) %s\r", a, localTime1);
          }
        }
        AVS_ASSERT(strlen(localLine) + 1 +  strlen(tBufferAlarmString) + 1 < sizeof(tBufferAlarmString) - 1);
      }
      else
      {
        if(tAlarm[a].flags.active)
        {
          snprintf(localLine, sizeof(localLine), "%02ld: (T) Active (%ld)\r", a, tAlarm[a].alarm_delay);
        }
        else
        {
          AVS_TIME itime = tAlarm[a].time;
          itime += service_alarm_get_time_zone() * HOUR_IN_SEC;
          struct tm *ptm = localtime((const time_t *)&itime);
          if(ptm)
          {
            strftime(localTime1, sizeof(localTime1), "%T", ptm);
            itime -= curTime;
            ptm = localtime((const time_t *)&itime);
            if( ptm )
            {
              strftime(localTime2, sizeof(localTime2), "%T", ptm);
              snprintf(localLine, sizeof(localLine), "%02ld: (T) %s-(%s)\r", a, localTime1, localTime2);
            }
          }
        }

        AVS_ASSERT(strlen(localLine) + 1 +  strlen(tBufferAlarmString) + 1 < sizeof(tBufferAlarmString) - 1);

      }
      strcat(tBufferAlarmString, localLine);
    }
  }
  service_alarm_unlock();
  return tBufferAlarmString;
}



/*
Alarm thread , check periodically  the alarm list and send an event if an alarm occurs

 an alarm is armed when the alarm has been set by Alexa and waiting for it target time
 an alarm is foreground when the alarm is ringing and no activity in the channel dialogue
 an alarm is background when the alarm is ringing/active but muted because the dialogue channel is active.
 the item will become foreground again as soon as the channel dialogue will be closed


*/


static void service_alarm_task(const void *pCookie);
static void service_alarm_task(const void *pCookie)
{
  AVS_Handle instance = (AVS_Handle)(uint32_t)pCookie;
  /* Wait NTP synchronization */
  while(AVS_Get_Sync_Time(instance,NULL) != AVS_OK)
  {
    osDelay(ALARM_PERIOD_PRECISION);
  }
  while(1)
  {
    AVS_TIME curTime ;
    static AVS_TIME  lastTime;
    AVS_Get_Sync_Time(instance, &curTime);
    AVS_TIME  deltaTime = curTime - lastTime;
    lastTime = curTime ;

    for(int32_t a = 0; a < MAX_ALARM  ; a++)
    {
      if((tAlarm[a].flags.armed != 0 )  && (!tAlarm[a].flags.active) && ((uint32_t)curTime >= (uint32_t)tAlarm[a].time) )
      {
        /* Start the alarm */
        /* Send notification to the cloud */
        service_alarm_event(instance, tAlarm[a].sToken, "AlertStarted");
        service_alarm_lock();
        /* Start the active phase */
        tAlarm[a].flags.active  = 1;
        tAlarm[a].alarm_delay = ALARM_DELAY_IN_SEC;
        service_alarm_unlock();
        AVS_Send_Evt(instance, EVT_ENDURANCE_ALARM_RING_START, 0);
        /* Check if the dialogue channel is opened */
        if(gAvsState == AVS_STATE_IDLE)
        {
          /* No dialogu and we can go in foreground and play the sound */
          service_alarm_lock();
          tAlarm[a].flags.forground = 1;
          tAlarm[a].flags.background = 0;
          service_alarm_unlock();
          AVS_Play_Sound(instance, AVS_PLAYSOUND_PLAY_LOOP, (void *)(uint32_t)pAlarm_Active_Wav, VOLUME_ALERT);
        }
        else
        {
          /* The dialogue channel is opened put the alarm in background  and signal it */
          /* The sound will be played at the end of the background stage */
          service_alarm_event(instance, tAlarm[a].sToken, "AlertEnteredBackground");
          service_alarm_lock();
          tAlarm[a].flags.background = 1;
          tAlarm[a].flags.forground = 0;
          service_alarm_unlock();
        }

      }

      if((tAlarm[a].flags.armed != 0)  && (tAlarm[a].flags.active))
      {
        /* The alarm is active but we have to manage the transition foreground background in case of dialogue */

        if((tAlarm[a].flags.background == 1) && (gAvsState == AVS_STATE_IDLE))
        {
          /* Dialogue is finished we have to re-enable sound and state */
          service_alarm_lock();
          tAlarm[a].flags.background = 0;
          tAlarm[a].flags.forground  = 1;
          service_alarm_unlock();
          /* Restore the alarm sound */
          AVS_Play_Sound(instance, AVS_PLAYSOUND_PLAY_LOOP, (void *)(uint32_t)pAlarm_Active_Wav, VOLUME_ALERT);
          /* Signal we restore the foreground */
          service_alarm_event(instance, tAlarm[a].sToken, "AlertEnteredForground");

        }
        if((tAlarm[a].flags.forground == 1) && (gAvsState != AVS_STATE_IDLE))
        {
          /* Dialogue is starting we have to disable the  sound and set the state */
          service_alarm_lock();
          tAlarm[a].flags.background = 1;
          tAlarm[a].flags.forground  = 0;
          service_alarm_unlock();

          /* Disable sound */
          AVS_Play_Sound(instance, AVS_PLAYSOUND_STOP, 0, 0);
          /* Signal we restore the background */
          service_alarm_event(instance, tAlarm[a].sToken, "AlertEnteredBackground");
        }
        /* If we are foreground, we can manage the count down and manage the end of the alarm */
        if(tAlarm[a].flags.forground == 1)
        {
          /* We check if the delay before to stop the alarm */
          service_alarm_lock();
          tAlarm[a].alarm_delay -= deltaTime;
          service_alarm_unlock();

          if(tAlarm[a].alarm_delay < 0)
          {
            /* Stop the alarm and send the notification */
            service_alarm_event(instance, tAlarm[a].sToken, "AlertStopped");
            AVS_Send_Evt(instance, EVT_ENDURANCE_ALARM_RING_STOP, 0);
            service_alarm_lock();
            tAlarm[a].flags.active  = 0;
            tAlarm[a].flags.armed = 0;
            service_alarm_unlock();
            /* Stop the sound alarm */
            AVS_Play_Sound(instance, AVS_PLAYSOUND_STOP, 0, 0);
          }
        }
      }
    }
    osDelay(ALARM_PERIOD_PRECISION);

  }
}


/* Set the time zone , example +2 = french time zone */
void service_alarm_set_time_zone (int32_t zone)
{
  gTimeZone = zone;
}


/* Get the time zone for local clock calculation */
int32_t  service_alarm_get_time_zone (void)
{
  return    gTimeZone;
}


/* Returns an epoch time  from a iso_8601 string */
static AVS_TIME service_alarm_parse_avs_time(const char_t *pTime);
static AVS_TIME service_alarm_parse_avs_time(const char_t *pTime)
{
  if(pTime == 0) 
  {
    return 0;
  }
  int32_t y = -1, m = -1, d = -1, h = -1, min = -1, sec = -1, mil = -1;
  sscanf(pTime, "%ld-%ld-%ldT%ld:%ld:%ld+%ld", &y, &m, &d, &h, &min, &sec, &mil);
  AVS_ASSERT(((y != -1) && (m != -1) && (d != -1) &&  (h != -1) && (min != -1) && (sec != -1) && (mil != -1)));
  AVS_ASSERT(m);
  struct tm t = {0};  /* Initialize to all 0's */
  t.tm_year = y - BASE_YEAR_EPOCH; /* This is year-1900, so 112 = 2012 */
  t.tm_mon  = m - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = min;
  t.tm_sec = sec;
  return mktime(&t) /*+service_alarm_get_time_zone() * HOUR_IN_SEC*/;
}

/* Create a iso_8601 string from and epoch time */
static  void service_alarm_format_iso_8601(char_t *pTime, uint32_t len, AVS_TIME itime);
static  void service_alarm_format_iso_8601(char_t *pTime, uint32_t len, AVS_TIME itime)
{
  struct tm *ptm = localtime((const time_t *)&itime);
  if(ptm)
  {
    snprintf(pTime, len, "%04u-%02u-%02uT%02u:%02u:%02u+0000", ptm->tm_year + BASE_YEAR_EPOCH, ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  }
}

static json_t *service_alarm_synchro_item(AlarmTimeSlot *pAlarm );
static json_t *service_alarm_synchro_item(AlarmTimeSlot *pAlarm )
{
  uint32_t err = 0;
  json_t *object = json_object();
  static char_t tTime[50];
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(object, "token", json_string(pAlarm->sToken));
  }
  if(pAlarm->flags.timer)
  {
    if(!err) 
    {
      err |= (uint32_t)json_object_set_new(object, "type", json_string("TIMER"));
    }
  }
  else
  {
    if(!err) 
    {
      err |= (uint32_t)json_object_set_new(object, "type", json_string("ALARM"));
    }
  }

  service_alarm_format_iso_8601(tTime, sizeof(tTime) - 1, pAlarm->time);
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(object, "scheduledTime", json_string(tTime));
  }
  if(err)
  {
    AVS_TRACE_ERROR("JSon Synchro Alarm");
  }


  return object;
}
json_t *service_alarm_synchro(void);
json_t *service_alarm_synchro(void)
{
  json_t *ctx;
  json_t *header;
  json_t *payload;
  json_t *allAlerts;
  json_t *activeAlerts;
  uint32_t err = 0;

  if(service_alarm_get_armed() == 0) 
  {
    return 0;
  }


  service_alarm_lock();

  allAlerts    = json_array();
  activeAlerts = json_array();
  ctx          = json_object();
  header       = json_object();
  payload      = json_object();

  /* Create the base entries */
  if(!err) 
  {
    err = (uint32_t)json_object_set_new(header, "namespace", json_string("Alerts"));
  }
  if(!err) 
  {
    err = (uint32_t)json_object_set_new(header, "name", json_string("AlertsState"));
  }

  /* Consider all entries */
  for(int32_t cpt = 0; cpt < MAX_ALARM; cpt++)
  {
    AlarmTimeSlot *pAlarm = &tAlarm[cpt];
    if(pAlarm->flags.armed)
    {
      if(pAlarm->flags.active)
      {
        json_array_append(activeAlerts, service_alarm_synchro_item(pAlarm));
      }
      if(!err) 
      {
        err |= (uint32_t)json_array_append_new(allAlerts, service_alarm_synchro_item(pAlarm)); /* To check this bug */
      }
    }
  }

  /* Link to "payload" */
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(payload, "allAlerts", allAlerts);
  }
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(payload, "activeAlerts", activeAlerts);
  }

  /* Link header and payload  to the context */
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(ctx, "header", header);
  }
  if(!err) 
  {
    err |= (uint32_t)json_object_set_new(ctx, "payload", payload);
  }
  service_alarm_unlock();

  if(err)
  {
    json_decref(ctx);
    return NULL;
  }

  return ctx;
}



/*
{
"event": {
    "header": {
        "namespace": "Alerts",
        "name": "SetAlertSucceeded",
        "messageId": "{{STRING}}"
    },
    "payload": {
        "token": "{{STRING}}"
    }
}
 send an alaram event to AVS

*/
static    void service_alarm_event(AVS_Handle handle, const char_t *pToken, const char_t *pName)
{
  uint32_t err = 0;
  json_t *root  = json_object();
  json_t *event = json_object();
  json_t *header = json_object();
  json_t *payload = json_object();
  /* Initialize JSON arrays */
  json_t *context = json_array();

  static char_t msgid[32];
  sprintf(msgid, FORMAT_MESSAGE_ID, messageIdCounter);
  messageIdCounter++;
  err |= (uint32_t)json_object_set_new(header, "namespace", json_string("Alerts"));
  err |= (uint32_t)json_object_set_new(header, "name", json_string(pName));
  err |= (uint32_t)json_object_set_new(header, "messageId", json_string(msgid));
  err |= (uint32_t)json_object_set_new(payload, "token", json_string(pToken));
  /* Links */
  err |= (uint32_t)json_object_set_new(root, "event", event);
  err |= (uint32_t)json_object_set_new(event, "header", header);
  err |= (uint32_t)json_object_set_new(event, "payload", payload);
  AVS_ASSERT(err ==0);
  if(err != 0)
  {
    AVS_TRACE_ERROR("Json returns an error");
  }

  AVS_VERIFY(json_object_set_new(root, "context", context) == 0);
  /* Add the context state to the event */
  AVS_VERIFY(AVS_Json_Add_Context(handle, root));


  const char_t *pJson = json_dumps(root, 0);
  json_decref(root);
  if(AVS_Send_JSon(handle, pJson) != AVS_OK)
  {
    AVS_TRACE_ERROR("Send Json Event");
  }
  jsonp_free((void *)(uint32_t)pJson);

}


/* Event callback delegation, we should watch here for events and catch alarm directive */
/* And add or delete alarms according to the directive */

AVS_Result service_alarm_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  AVS_Result err = AVS_OK;
  switch(evt)
  {


    /*

          when the SDK send a synchro-state ( initiated by the SDK or using AVS_Post_Sychro)
          the synchro must reflect the alarm state in the json context array
          each time the synchro will sbe done, the system will create a json and will send EVT_UPDATE_SYNCHRO
          in order that all apps or user services update the synchro state and reflects changes
          the parameter is the root Json handle  pre-filled by the sdk and read to be send to AVS after the EVT_UPDATE_SYNCHRO update

    */

    case EVT_UPDATE_SYNCHRO:
    {

      json_t *root = (json_t *)pparam;
      json_t *ctx = json_object_get(root, "context");
      json_t *alarm =  service_alarm_synchro();
      if(alarm)
      {
        AVS_VERIFY(json_array_append_new(ctx, alarm) == 0);
      }
      break;
    }


    case EVT_CHANGE_STATE:
    {


      /* If an alarm occurs and the dialogue channel is active, we must set the alarm in forground */
      /* So, we store the change state, foreground / background state will be updated in the next task loop */

      gAvsState = pparam;
    }
    break;

    case EVT_DIRECTIVE_ALERT:
    {
      /* The json is the parameter */
      json_t *root = (json_t *)pparam;
      /* Parse it to extract information */
      json_t *directive = json_object_get(root, "directive");
      json_t *payload    = json_object_get(directive, "payload");
      json_t *header    = json_object_get(directive, "header");
      json_t *scheduledTime = json_object_get(payload, "scheduledTime");
      json_t *token = json_object_get(payload, "token");
      json_t *type = json_object_get(payload, "type");

      json_t *name    = json_object_get(header, "name");
      const char_t *pToken = json_string_value(token);
      const char_t *pTime = json_string_value(scheduledTime);
      const char_t *pType = json_string_value(type);

      /* If setalert we can parse the message to add timers */
      if(strcmp(json_string_value(name), "SetAlert") == 0)
      {
        /* Extract and convert string iso_8601 to epoch */
        AVS_TIME  itime  = service_alarm_parse_avs_time(pTime);
        /* Check if we have a free slot */
        int32_t index = service_alarm_add_item(itime, pToken);
        if(index  != -1)
        {
          /* In this code we consider only timer and alarm, other are ignored */
          if(strcmp(pType, "ALARM") == 0)
          {
            tAlarm[index].flags.timer = 0;
            service_alarm_event(handle, pToken, "SetAlertSucceeded");
            AVS_Send_Evt(handle, EVT_ENDURANCE_ADD_ALARM, 0);
          }
          else if(strcmp(pType, "TIMER") == 0)
          {
            tAlarm[index].flags.timer = 1;
            service_alarm_event(handle, pToken, "SetAlertSucceeded");
            AVS_Send_Evt(handle, EVT_ENDURANCE_ADD_ALARM, 0);
          }
          else
          {
            /* Do nothing */
          }
        }
        else
        {
          /* No slot available, signal failed */
          service_alarm_event(handle, pToken, "SetAlertFailed");

        }

      }
      /* Parse the alarm delete */
      if(strcmp(json_string_value(name), "DeleteAlert") == 0)
      {
        /* Fin the item from the token */
        int32_t index = service_alarm_find_item(pToken);
        if(index != -1)
        {
          /* Detete the entry */
          service_alarm_disable_item(index );
          /* Signal Succeeded */
          service_alarm_event(handle, pToken, "DeleteAlertSucceeded");
          AVS_Send_Evt(handle, EVT_ENDURANCE_DEL_ALARM, 0);

        }
        else
        {
          /* No item found, signal it */
          service_alarm_event(handle, pToken, "DeleteAlertFailed");
        }
      }
      err =  AVS_EVT_HANDLED;
      break;
    }
    default:
    	break;
  }
  return err;
}





/* Create the alarm service */
AVS_Result service_alarm_create(AVS_Handle hHandle)
{

  AVS_VERIFY(mutex_Create(&lockAlarm,"lockAlarm"));
  pAlarm_Active_Wav = service_assets_load("alarm_active_wav", 0, 0);
  AVS_ASSERT(pAlarm_Active_Wav);

  memset(tAlarm, 0, sizeof(tAlarm));
  hAlarmTask = task_Create(TASK_NAME_ALARM,service_alarm_task,hHandle,TASK_ALARM_STACK,TASK_PRIORITY_ALARM);
  if(hAlarmTask == 0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_ALARM);
    return AVS_ERROR;
  }

  return AVS_OK;

}

/* Delete the alarm service */
void  service_alarm_delete(AVS_Handle hHandle)
{
  service_assets_free(pAlarm_Active_Wav);
  task_Delete(hAlarmTask);
  mutex_Delete(&lockAlarm);
}
