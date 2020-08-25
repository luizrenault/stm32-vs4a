/**
******************************************************************************
* @file    service_os.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Abstraction OS using CMSIS
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
#ifndef _service_os_
#define _service_os_


/* Abstraction OS using CMSIS */

typedef  struct 
{
  osThreadDef_t osThread;
  osThreadId     id;
}task_t;

typedef struct
{
  osMutexDef_t     m;
  osMutexId    id;
} mutex_t;


task_t *task_Create(const char_t *pName, os_pthread  pCb, void *pCookie, size_t stackSize, osPriority  priority);
void task_Delete(task_t *task);
int8_t mutex_Create(mutex_t *mutex, const char *pName);
void mutex_Delete(mutex_t *mutex);
void mutex_Lock(mutex_t  *mutex);
void mutex_Unlock(mutex_t *mutex);

#endif /*_service_os_*/
