/**
******************************************************************************
* @file    service_os.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Service Os abstraction
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


task_t *task_Create(const char_t *pName, os_pthread  pCb, void *pCookie, size_t stackSize, osPriority  priority)
{
  
  task_t *pTask = (task_t *)pvPortMalloc(sizeof(task_t)) ;
  pTask->osThread.name = (char_t *)(uint32_t)pName;
  pTask->osThread.pthread = pCb;
  pTask->osThread.tpriority = priority;
  pTask->osThread.stacksize = stackSize;
  pTask->osThread.instances = 0;
  pTask->id = osThreadCreate(&pTask->osThread,pCookie);
  if(pTask->id ==0)
  {
    vPortFree(pTask);
    pTask = NULL;
  }
  AVS_ASSERT(pTask != 0);
  return pTask;
}
/* Delete a task */
 
 void task_Delete(task_t *task)
{
  if((task != 0) && (task->id != 0)) 
  {
    osThreadTerminate(task->id);
    task->id = 0;
  }
  vPortFree(task);
}

int8_t mutex_Create(mutex_t *mutex, const char *pName)
{
  mutex->id     = osRecursiveMutexCreate(&mutex->m);  
  if((mutex->id!= 0)  && (pName != 0))
  {
    vQueueAddToRegistry (mutex->id, pName);
  }
  
  AVS_ASSERT(mutex->id!= 0);
  return (int8_t)  ((mutex->id != 0) ? TRUE : FALSE);

}


void mutex_Delete(mutex_t *mutex)
{
  AVS_ASSERT(mutex != 0);
  AVS_ASSERT(mutex->id != 0);
  if(mutex->id != 0)
  {
    vQueueUnregisterQueue(mutex->id);
  }
  osMutexDelete(mutex->id);

}

 
 void mutex_Lock(mutex_t  *mutex)
{
  
  AVS_ASSERT(mutex != 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    osRecursiveMutexWait(mutex->id,  osWaitForever);
  }
}

/* Unlock  a recursive mutex */

void mutex_Unlock(mutex_t *mutex)
{
  AVS_ASSERT(mutex != 0);
  if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
  {
    osRecursiveMutexRelease(mutex->id);
  }
  
}
