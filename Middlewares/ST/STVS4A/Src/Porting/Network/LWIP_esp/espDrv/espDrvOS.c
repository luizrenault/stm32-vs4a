/**
******************************************************************************
* @file    espDrvOS.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   OS abstraction
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2018 STMicroelectronics International N.V.
* All rights reserved.</center></h2>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted, provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. Neither the name of STMicroelectronics nor the names of other
*    contributors to this software may be used to endorse or promote products
*    derived from this software without specific written permission.
* 4. This software, including modifications and/or derivative works of this
*    software, must execute solely and exclusively on microcontroller or
*    microprocessor devices manufactured by or for STMicroelectronics.
* 5. Redistribution and use of this software other than as permitted under
*    this license is void and will automatically terminate your rights under
*    this license.
*
* THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
* RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
* SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/
/*
  Implement an OS abstraction to make this module more independent

*/
#include "espDrvCore.h"
#include "cmsis_os.h"



/* Determine whether we are in thread mode or handler mode. */
static int osInHandlerMode (void)
{
  return __get_IPSR() != 0;
}



/**
 * @brief Create a thread 
 * 
 * @param pName Thread name 
 * @param pCb  Thread callback 
 * @param pCookie Thread cookie 
 * @param stackSize Thread stack size 
 * @param priority  Thread priority 
 * @return osTask_t* Thread handle created or NULL
 */
osTask_t *  osTask_Create(const char_t *pName, osTask_CB  pCb, void *pCookie, uint32_t stackSize, uint32_t  priority)
{
  xTaskHandle hTask = NULL;
  uint32_t status = xTaskCreate((pdTASK_CODE)pCb, pName, stackSize, pCookie, priority, &hTask);
  if (status != pdPASS)
  {
    hTask = NULL;
  }
  return hTask;
}

/**
 * @brief  Delete a thread
 * 
 * @param task handle
 */
void  osTask_Delete(osTask_t *task)
{
  vTaskDelete(task);

}

/**
 * @brief Create a mutex 
 * 
 */
int8_t  osMutex_Create(osMutex_t *mutex)
{
  mutex->h = xSemaphoreCreateMutex();
  return (int8_t)(mutex->h ? TRUE : FALSE);
}

/**
 * @brief Delete a mutex 
 * 
 * @param mutex 
 */
void  osMutex_Delete(osMutex_t *mutex)
{
  vSemaphoreDelete(mutex->h);
  mutex->h = 0;
}

/**
 * @brief Lock a mutex 
 * 
 * @param mutex 
 * @return uint32_t 
 */

uint32_t  osMutex_Lock(osMutex_t  *mutex)
{
  xSemaphoreTake(mutex->h, portMAX_DELAY);
  return 0;

}

/**
 * @brief Unlock a mutex 
 * 
 * @param mutex 
 */

void  osMutex_Unlock(osMutex_t *mutex)
{
  xSemaphoreGive(mutex->h);
}

/**
 * @brief Create an event 
 * 
 * @param event 
 * @return int8_t 
 */

int8_t  osEvent_Create(osEvent_t *event)
{
  vSemaphoreCreateBinary(event->h);
  xSemaphoreTake(event->h, 0);
  return TRUE;

}

/**
 * @brief Delete an event 
 * 
 * @param event 
 */

void  osEvent_Delete(osEvent_t *event)
{
  vSemaphoreDelete(event->h);
  event->h = 0;
}

/**
 * @brief Signal an event 
 * 
 * @param event 
 * @param fromISR TRUE if called from an ISR 
 */

void  osEvent_Set(osEvent_t  *event)
{
  if (osInHandlerMode())
  {
    portBASE_TYPE taskWoken = pdFALSE;
    xSemaphoreGiveFromISR(event->h, &taskWoken);
    portEND_SWITCHING_ISR(taskWoken);
  }
  else
  {
    xSemaphoreGive(event->h);
  }

}

/**
 * @brief Wait an Event 
 * 
 * @param event 
 * @param timeout 
 * @return uint8_t 
 */

uint8_t  osEvent_Wait(osEvent_t *event, uint32_t timeout)
{
  if (xSemaphoreTake(event->h, timeout) != pdTRUE)
  {
    return FALSE;
  }
  return TRUE;
}

/**
 * @brief Allocs memory 
 * 
 * @param size 
 * @return void* 
 */
void	*  osMalloc(uint32_t size)
{
  return pvPortMalloc(size);
}

/**
 * @brief Free memory 
 * 
 * @param pMalloc 
 */
void	osFree(void *pMalloc)
{
  vPortFree(pMalloc);

}

/**
 * @brief Task delay 
 * 
 * @param delay 
 */

void osTaskDelay(uint32_t delay)
{
  vTaskDelay(delay);
}





