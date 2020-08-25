
/**
******************************************************************************
* @file    service_endurance_test.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   test some functions and log events
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


#include "service.h"


#define TASK_NAME_TEST_ENDURANCE              "AVS:Test endurance"
#define TASK_PRIORITY_ENDURANCE               (osPriorityIdle)
#define TASK_NAME_NETWORK_ENDURANCE            "AVS: Bad Network Sim"
#define TASK_PRIORITY_NETWORK_ENDURANCE        (osPriorityIdle)
#define TASK_TEST_STACK                         500

static task_t *        hTestTask;
static task_t *        hNetworkTask;
static mutex_t   lock  ;
static uint32_t             bTestActif = 0;
static uint32_t           gTestRunning;
static uint32_t           gNetworkRunning = 0;          /* If this flag is activated, the system will simulate disconnection every N seconds */
       uint32_t           bSimulateBadNetwork = 0;
static uint32_t         iIndexTest = 0;
static uint32_t         bConnected = 0;
static evt_keeper_t     tEvtStack[MAX_STACK];
static uint32_t         iEvtStack = 0;




__STATIC_INLINE  void service_endurance_lock(void);
__STATIC_INLINE  void service_endurance_lock(void)
{
  mutex_Lock(&lock);

}
__STATIC_INLINE  void service_endurance_unlock(void);
__STATIC_INLINE  void service_endurance_unlock(void)
{
  mutex_Unlock(&lock);
}





/*

 Due to the fact we can't block an event,we need to manage events in a task and not within  the callback event
 so, we create a stack of events that will be processed in the main test task


*/
void  service_endurance_evt_add(uint32_t evt,   uint32_t pparam);
void  service_endurance_evt_add(uint32_t evt,   uint32_t pparam)
{
  /* We can't use a lock, otherwise we can fall in a dead lock */
  vTaskSuspendAll();

  if(iEvtStack + 1 < MAX_STACK)
  {
    tEvtStack[iEvtStack].evt = evt;
    tEvtStack[iEvtStack].pparam = pparam;
    iEvtStack++;
  }
  xTaskResumeAll();

}

/* Get and remove the event from the task */
uint32_t  service_endurance_evt_remove(uint32_t *evt,   uint32_t *pparam);
uint32_t  service_endurance_evt_remove(uint32_t *evt,   uint32_t *pparam)
{
  if(iEvtStack == 0) 
  {
    return FALSE;
  }

  service_endurance_lock();
  *evt    = tEvtStack[0].evt ;
  *pparam = tEvtStack[0].pparam ;
  for(int32_t a = 0; a < iEvtStack - 1 ; a++)
  {
    tEvtStack[a] = tEvtStack[a + 1];
  }
  iEvtStack--;
  service_endurance_unlock();
  return TRUE;
}



/* Check the time-out for the item */
uint32_t service_endurance_check_timeout(AVS_Handle handle, Test_item_t *pItem)
{

  AVS_TIME  curTime;
  AVS_Get_Sync_Time(handle, &curTime);
  uint32_t delta = curTime - pItem->start_time;
  if(delta < pItem->timeout) 
  {
    return 0;
  }
  return 1;
}

/* We there is no word spotting, start the capture manually */

void service_endurance_start_capture(AVS_Handle handle)
{
  AVS_Result ret;
  uint32_t timeout = 3000 / 100;
  do
  {
    ret = AVS_Set_State(hInstance, AVS_STATE_START_CAPTURE);
    osDelay(100);
    timeout--;
  }
  while((ret  != AVS_OK) && (timeout != 0));
  if(timeout == 0)
  {
    AVS_Set_State(hInstance, AVS_STATE_RESTART);
    service_endurance_send_msg(handle, NULL, AVS_OK, "Start ********* Force Restart *********");

  }

  AVS_Send_Evt(handle, EVT_ENDURANCE_MSG, (uint32_t)"Wakeup OK");
}

/* Send a formatted message to the serial service */

AVS_Result service_endurance_send_msg(AVS_Handle handle, Test_item_t *pItem, AVS_Result retCode, const char_t *pFormat, ...)
{
  va_list args;
  va_start(args, pFormat);
  char_t msg[100];
  vsnprintf(msg, sizeof(msg), pFormat, args);
  AVS_Send_Evt(handle, EVT_ENDURANCE_MSG, (uint32_t)msg);
  va_end(args);
  return retCode;
}

/* Say a recorded sentence to ALEXA */

void service_endurance_say(AVS_Handle handle, const void *pText, uint32_t mode, const char_t *pText2Say)
{
  AVS_Result ret = AVS_BUSY;
  char_t msg[100];
  if(pText2Say != 0)
  {
    snprintf(msg, sizeof(msg), "Say \"%s\"", pText2Say);
    AVS_Send_Evt(handle, EVT_ENDURANCE_MSG, (uint32_t)msg);
  }

  while(ret != AVS_OK)
  {
    ret = (AVS_Result )AVS_Ioctl(hInstance, AVS_IOCTL_INJECT_MICROPHONE, FALSE, (uint32_t)pText);
    if(ret == AVS_BUSY)
    {
      AVS_Send_Evt(handle, EVT_ENDURANCE_MSG, (uint32_t)"Microphone Busy");
      osDelay(10); /* Wait the end of the previous speech */
    }
    AVS_ASSERT(ret != AVS_ERROR);
  }
}



void   service_endurance_get_current_test_name(char_t *pName, uint32_t maxlen)
{
  if(bTestActif)
  {
    service_endurance_lock();
    strncpy(pName, tUnitTest[iIndexTest].pTestName, maxlen - 1);
    service_endurance_unlock();
  }
  else
  {
    strncpy(pName, "Stopped", maxlen);
  }

}


static void service_endurance_next(AVS_Handle handle, AVS_Result err);
static void service_endurance_next(AVS_Handle handle, AVS_Result err)
{
  service_endurance_lock();

  if(err != AVS_RETRY )
  {
    iIndexTest++;
    if(tUnitTest[iIndexTest].pTestName == 0)
    {
      iIndexTest = 0;
      service_endurance_send_msg(handle, NULL, AVS_OK, "Start ********* Loop %d *********", gAppState.testLoops);
      gAppState.testLoops++;
    }
  }
  tUnitTest[iIndexTest].test(hInstance, &tUnitTest[iIndexTest], STOP_TEST, 0, 0);
  service_endurance_unlock();


  service_endurance_lock();
  tUnitTest[iIndexTest].test(hInstance, &tUnitTest[iIndexTest], START_TEST, 0, 0);
  service_endurance_unlock();

}

static void service_endurance_evt(AVS_Handle handle, uint32_t evt, uint32_t param);
static void service_endurance_evt(AVS_Handle handle, uint32_t evt, uint32_t param)
{
  tUnitTest[iIndexTest].test(hInstance, &tUnitTest[iIndexTest], EVENT_TEST, evt, param);
}

static void service_endurance_network_simulatation(const void *pCookie);
static void service_endurance_network_simulatation(const void *pCookie)
{
  uint32_t  countDelay = 0;
  while(!bConnected)  
  {
    osDelay(100);
  }
  AVS_Send_Evt(hInstance, EVT_NETWORK_SIM_TASK_START, 0);
  while(gNetworkRunning)
  {
    osDelay(500);
    countDelay += 500;
    if(AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, 3, 0) == FALSE)
    {
      /* We are connected */
      if(countDelay > ENDURANCE_NETWORK_DELAY_CONNECTED)
      {
        /* Network disconnection */
        countDelay = 0;
        /* Disconnect the network */
        AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, TRUE, 0);
        /* Notify */
        service_endurance_send_msg(hInstance, NULL, AVS_OK, "Stops the Network");
      }

    }
    else
    {
      /* We are disconnected */
      if(countDelay > ENDURANCE_NETWORK_DELAY_DISCONNECTED)
      {
        /* Network disconnection */
        countDelay = 0;
        /* Reconnect the network */
        AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, FALSE, 0);
        /* Notify */
        service_endurance_send_msg(hInstance, NULL, AVS_OK, "Restarts the Network");
      }
    }
  }
  /* Restore the network */
  AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, FALSE, 0);
  /* Notify end */
  AVS_Send_Evt(hInstance, EVT_NETWORK_SIM_TASK_DYING, 0);

}

static void service_endurance_test_task(const void *pCookie);
static void service_endurance_test_task(const void *pCookie)
{
  while(bConnected==0)  
  {
    osDelay(100);
  }
  /* Make sure we are in english */
  
  service_langage_apply(hInstance,"en-US");


  AVS_Send_Evt(hInstance, EVT_ENDURANCE_TASK_START, 0);
  AVS_Ioctl(hInstance, AVS_IOCTL_MUTE_MICROPHONE, TRUE, 0);

  tUnitTest[iIndexTest].test(hInstance, &tUnitTest[iIndexTest], START_TEST, 0, 0);
  gTestRunning = TRUE;

  while(gTestRunning)
  {
    service_endurance_lock();
    AVS_Result result = tUnitTest[iIndexTest].test(hInstance, &tUnitTest[iIndexTest], CHECK_TEST, 0, 0);
    service_endurance_unlock();
    if(result  != AVS_BUSY )
    {
      if(result  == AVS_OK)
      {
        gAppState.testPassedCount++;
      }
      if(result  == AVS_ERROR)
      {
        gAppState.testErrorCount++;
      }
      if(result  == AVS_TIMEOUT)
      {
        gAppState.testTimeoutCount++;
      }
      if(result  == AVS_RETRY)
      {
        gAppState.testRetryCount++;
      }

      service_endurance_next(hInstance, result);
    }
    uint32_t evt;
    uint32_t pparam;
    if(service_endurance_evt_remove(&evt, &pparam))
    {
      service_endurance_evt(hInstance, evt, pparam);
      continue;
    }

    osDelay(100);
  }
  AVS_Ioctl(hInstance, AVS_IOCTL_MUTE_MICROPHONE, FALSE, 0);
  AVS_Send_Evt(hInstance, EVT_ENDURANCE_TASK_DYING, 0);
  osThreadTerminate(NULL);
}

/* Create the endurance  service */
AVS_Result service_endurance_test_create(AVS_Handle instance);
AVS_Result service_endurance_test_create(AVS_Handle instance)
{
  gTestRunning = TRUE;
  gNetworkRunning = TRUE;
  gAppState.testErrorCount = 0;
  gAppState.testTimeoutCount = 0;
  gAppState.testPassedCount  = 0;
  gAppState.testRetryCount = 0;
  
  if (service_assets_check_integrity() != AVS_OK)
    {
     AVS_TRACE_ERROR("No valid Assets found in QSPI, please check how to flash them before running endurance tests");
     return AVS_ERROR;
    }

  AVS_Get_Sync_Time(instance, &gAppState.testTimeStart);
  AVS_VERIFY(mutex_Create(&lock,"Endurance Lock"));

  hTestTask = task_Create(TASK_NAME_TEST_ENDURANCE,service_endurance_test_task,instance,TASK_TEST_STACK,TASK_PRIORITY_ENDURANCE);
  if(hTestTask ==  0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_TEST_ENDURANCE);
    return AVS_ERROR;
  }
  if(bSimulateBadNetwork)
  {
    hNetworkTask = task_Create(TASK_NAME_NETWORK_ENDURANCE,service_endurance_network_simulatation,instance,TASK_TEST_STACK,TASK_PRIORITY_NETWORK_ENDURANCE);
    if(hNetworkTask ==0)
    {
      AVS_TRACE_ERROR("Create task %s", TASK_NAME_NETWORK_ENDURANCE);
      return AVS_ERROR;
    }
  }

  return AVS_OK;
}



/* Delete the alarm service */
void  service_endurance_test_delete(AVS_Handle hHandle);
void  service_endurance_test_delete(AVS_Handle hHandle)
{
  gTestRunning = FALSE;
  gNetworkRunning = FALSE;
  osDelay(1000);
  mutex_Delete(&lock);
  task_Delete(hTestTask);
  if(hNetworkTask) 
  {
    task_Delete(hNetworkTask);
  }
  hNetworkTask = 0;
  hTestTask = 0;
  



}



/* Event callback for alarm */
AVS_Result service_endurance_test_event_cb(AVS_Handle hHandle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
AVS_Result service_endurance_test_event_cb(AVS_Handle hHandle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  uint32_t extendedEvent = (uint32_t)evt; /* Avoid compiler complain when handling extended events ie non defined inside AVS_Event enum*/

  switch(extendedEvent)
  {
    case EVT_HTTP2_CONNECTED:
    {
      bConnected = TRUE;
      if(gTestRunning)
      {
        service_endurance_evt_add(evt, pparam);
      }
      break;
    }
    case EVT_ENDURANCE_MSG:
    {
      break;
    }


    default:
      if(gTestRunning)
      {
        service_endurance_evt_add(evt, pparam);
      }
      break;
  }
  return AVS_OK;
}



uint32_t service_endurance_get_state(void)
{
  return bTestActif;
}
void   service_endurance_stop(void)
{
  if(bTestActif)
  {
    if(strlen(gAppState.tInfoTest) != 0)
    {
      AVS_TRACE_INFO(gAppState.tInfoTest);
    }

    service_endurance_test_delete(hInstance);
  }
  bTestActif = 0;


}
void   service_endurance_start(void)
{
  if((bTestActif==0) && (sInstanceFactory.initiator != AVS_INITIATOR_PUSH_TO_TALK))
  {
    if (service_endurance_test_create(hInstance) == AVS_OK)
    {
    bTestActif = 1;  
    }
  }
}


uint32_t  service_endurance_enable_network_sim( uint32_t state)
{
  if(state == 0)
  {
    bSimulateBadNetwork = 0;
  }

  if(state == 1)
  {
    bSimulateBadNetwork = 1;
  }

  return bSimulateBadNetwork;

}


