/**
******************************************************************************
* @file    avs.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   startup example STVS4A
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

#ifndef  MY_CLIENT_ID
#define  MY_CLIENT_ID            "amzn1.application-oa2-client.000fb34bf7ec406b97747e71d8ea33d7"    /* Identifier of a test device example created on developper.amazon.com */
#endif
#ifndef  MY_CLIENT_SECRET_ID
#define  MY_CLIENT_SECRET_ID        "8c7b9d401528650c815df036cd7cff17bf8bec56a3191498e3b791691cb5c4f1" /* Identifier of a test device example created on developper.amazon.com */
#endif
#ifndef MY_CLIENT_PRODUCT_ID
#define MY_CLIENT_PRODUCT_ID           "TEST_STVS4A" /* Identifier of a test device example created on developper.amazon.com */
#endif


#define TASK_NAME_MAIN                   "AVS:Main"
#define TASK_PRIORITY_MAIN               (osPriorityIdle)
#define TASK_STACK_MAIN                  1000
#define CALCULATION_PERIOD               1000


AVS_Handle               hInstance;
AVS_Instance_Factory   sInstanceFactory;
Application_State        gAppState;
task_t *              hMainTask;

static task_t *    xIdleHandle = NULL;
static __IO uint32_t  osCPU_Usage = 0U; 
static  uint32_t      osCPU_IdleStartTime = 0U; 
static  uint32_t      osCPU_IdleSpentTime = 0U; 
static  uint32_t      osCPU_TotalIdleTime = 0U; 


__weak AVS_Result      service_langage_apply(AVS_Handle handle,char_t *pLang)
{
  return AVS_OK;
}




/* Weak functions to disable optional features services */

__weak AVS_Result service_player_create(AVS_Handle hHandle);
__weak AVS_Result service_player_create(AVS_Handle hHandle)
{
  return AVS_OK;
}
__weak AVS_Result service_player_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
__weak AVS_Result service_player_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak char_t * service_player_get_string(void)
{
  return (char_t*)0;
}
__weak void   service_player_volume_max(AVS_Handle handle, uint32_t max)
{
}
__weak uint32_t   service_player_get_buffer_percent(AVS_Handle handle)
{
  return 0;
}
__weak uint32_t  service_player_is_active(void);
__weak uint32_t  service_player_is_active(void)
{
  return 0;
}
__weak AVS_Result service_alarm_create(AVS_Handle hHandle)
{
  return AVS_OK;
}
__weak AVS_Result service_alarm_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak AVS_Result service_wakeup_create(AVS_Handle hHandle)
{
  return AVS_OK;
}
__weak AVS_Result service_wakeup_event_cb(AVS_Handle hInst, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak void  service_alarm_snooze(void)
{
}
__weak AVS_Result service_gui_create(void)
{
  return AVS_OK;
}
__weak AVS_Result service_gui_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak AVS_Result service_serial_trace_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
__weak AVS_Result service_serial_trace_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak AVS_Result service_endurance_test_event_cb(AVS_Handle hInst, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
__weak AVS_Result service_endurance_test_event_cb(AVS_Handle hInst, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak AVS_Result service_endurance_test_create(AVS_Handle instance);
__weak AVS_Result service_endurance_test_create(AVS_Handle instance)
{
  return AVS_OK;
}
__weak void       service_endurance_stop(void)
{
}

__weak void       service_endurance_start(void)
{
}
__weak uint32_t   service_endurance_get_state(void)
{
  return 0;
}
__weak void       service_endurance_get_current_test_name(char_t *pName, uint32_t maxlen)
{
}
__weak AVS_Result service_audio_feed_usb_create(AVS_Handle handle);
__weak AVS_Result service_audio_feed_usb_create(AVS_Handle handle)
{
  return AVS_OK;
}

__weak AVS_Result service_authentication_create(AVS_Handle  hHandle, AVS_Instance_Factory *pFactory)
{
  return AVS_OK;
}

/*


Event Callback


*/
static AVS_Result appEventHandler(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
static AVS_Result appEventHandler(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  service_endurance_test_event_cb(handle, pCookie, evt, pparam);
  if(service_serial_trace_cb(handle, pCookie, evt, pparam) == AVS_EVT_HANDLED)
  {
    return AVS_EVT_HANDLED; /* Delegation for the serial */
  }
  if(service_gui_event_cb(handle, pCookie, evt, pparam) == AVS_EVT_HANDLED)
  {
    return AVS_EVT_HANDLED; /* Delegation for the UI */
  }
  if(service_wakeup_event_cb(handle, pCookie, evt, pparam) == AVS_EVT_HANDLED)
  {
    return AVS_EVT_HANDLED; /* Delegation for the wakeup */
  }
  if(service_alarm_event_cb(handle, pCookie, evt, pparam) == AVS_EVT_HANDLED)
  {
    return AVS_EVT_HANDLED; /* Delegation for the alarm */
  }
  if(service_player_event_cb(handle, pCookie, evt, pparam) == AVS_EVT_HANDLED)
  {
    return AVS_EVT_HANDLED; /* Delegation for the player */
  }

  return AVS_OK;
}

static uint32_t appPersistCB(AVS_Persist_Action action,Avs_Persist_Item item ,char_t *pItem,uint32_t itemSize);
static uint32_t appPersistCB(AVS_Persist_Action action,Avs_Persist_Item item ,char_t *pItem,uint32_t itemSize)
{
  uint32_t ret = AVS_NOT_IMPL;
  ret  = service_persistent_storage(action,item ,pItem,itemSize);
  if(ret !=  AVS_NOT_IMPL)  
  {
    return ret;
  }
  return AVS_NOT_IMPL;
}
  


void services_Init(void)
{
  AVS_VERIFY(AVS_Init());
  AVS_Set_Debug_Level( AVS_TRACE_LVL_DEFAULT );

  AVS_TRACE_INFO("");
  AVS_TRACE_INFO("Hello STVS4A");
  AVS_TRACE_INFO("Sdk : Version %s ", AVS_VERSION);
  AVS_TRACE_INFO("App : Version %s ", APP_VERSION);

  memset(&sInstanceFactory, 0, sizeof(sInstanceFactory));
  memset(&gAppState, 0, sizeof(gAppState));
  
  service_persistent_init_default();

  sInstanceFactory.clientId     = MY_CLIENT_ID;
  sInstanceFactory.clientSecret = MY_CLIENT_SECRET_ID;
  sInstanceFactory.productId    = MY_CLIENT_PRODUCT_ID;
  sInstanceFactory.eventCB      = appEventHandler;
//  sInstanceFactory.persistentCB = appPersistCB;

  
  
  AVS_VERIFY(service_gui_create());
  AVS_VERIFY(service_player_create(hInstance));

  



  hInstance          = AVS_Create_Instance(&sInstanceFactory);
  AVS_ASSERT(hInstance != 0);


  AVS_VERIFY(service_wakeup_create(hInstance));
  AVS_VERIFY(service_alarm_create(hInstance));
  AVS_VERIFY(service_authentication_create(hInstance, &sInstanceFactory));
  AVS_VERIFY(service_audio_feed_usb_create(hInstance));

  AVS_Show_Config(hInstance);
  AVS_TRACE_INFO("\r************ Start ***************\r");
}




/**
  * @brief  Application Idle Hook
  * @param  None 
  * @retval None
  */

void vApplicationIdleHook(void);
void vApplicationIdleHook(void) 
{
  if( xIdleHandle == NULL )
  {
    /* Store the handle to the idle task. */
    xIdleHandle = xTaskGetCurrentTaskHandle();
  }
  /* Enter SLEEP mode => Systick timer is still running */

#ifndef STM32H7 /* Not working on H7 / to be investigated why*/
  __WFI();
#endif
}

/**
  * @brief  Application Idle Hook
  * @param  None 
  * @retval None
  */
void vApplicationTickHook (void);
void vApplicationTickHook (void)
{
  static int32_t tick = 0;
  
  if(tick> CALCULATION_PERIOD)
  {
    tick = 0;
    
    if(osCPU_TotalIdleTime > 1000)
    {
      osCPU_TotalIdleTime = 1000;
    }
    osCPU_Usage = (100 - (osCPU_TotalIdleTime * 100) / CALCULATION_PERIOD);
    osCPU_TotalIdleTime = 0;
  }
  tick++;
}

/**
  * @brief  Start Idle monitor
  * @param  None 
  * @retval None
  */
void StartIdleMonitor (void);
void StartIdleMonitor (void)
{
  if( xTaskGetCurrentTaskHandle() == xIdleHandle ) 
  {
    osCPU_IdleStartTime = xTaskGetTickCountFromISR();
  }
}

/**
  * @brief  Stop Idle monitor
  * @param  None 
  * @retval None
  */
void EndIdleMonitor (void);
void EndIdleMonitor (void)
{
  if( xTaskGetCurrentTaskHandle() == xIdleHandle )
  {
    /* Store the handle to the idle task. */
    osCPU_IdleSpentTime = xTaskGetTickCountFromISR() - osCPU_IdleStartTime;
    osCPU_TotalIdleTime += osCPU_IdleSpentTime; 
  }
}

/**
  * @brief  Stop Idle monitor
  * @param  None 
  * @retval None
  */
uint16_t service_GetCPUUsage (void)
{
  return (uint16_t)osCPU_Usage;
}
