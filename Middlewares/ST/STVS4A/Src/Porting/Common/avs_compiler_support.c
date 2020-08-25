/**
  ******************************************************************************
  * @file    avs_compiler_support.c
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    20-02-2018
  * @brief   HAL time base based on the hardware TIM Template.
  *
  *          This file overrides the native HAL time base functions (defined as weak)
  *          the TIM time base:
  *           + Intializes the TIM peripheral generate a Period elapsed Event each 1ms
  *           + HAL_IncTick is called inside HAL_TIM_PeriodElapsedCallback ie each 1ms
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




#if  defined ( __GNUC__  )
#include <malloc.h>

extern char __heap_start;
extern char __heap_limit;

/*
 * GCC manages mallinfo  differently, arena represents the current maximum heap and not the system linker size allocated for the heap
 * so , we need to extract the heap len from link script info
 */
void   avs_complier_get_heap_info(uint32_t *memTotalSpace,uint32_t *memFreeSpace)
{
  struct mallinfo mi;
  mi = mallinfo();
  uint32_t maxHeap = &__heap_limit - &__heap_start;
  if(memTotalSpace) 
  {
    *memTotalSpace= maxHeap;
  }
  if(memFreeSpace) 
  {
    *memFreeSpace = maxHeap  - mi.uordblks;
  }
}

#endif

#if  defined ( __CC_ARM )
void   avs_complier_get_heap_info(uint32_t *memTotalSpace,uint32_t *memFreeSpace)
{
  if(memTotalSpace) 
  {
    *memTotalSpace= 0;
  }
  if(memFreeSpace) 
  {
    *memFreeSpace = 0;
  }
}
#endif



#if defined ( __ICCARM__ ) /*!< IAR Compiler */




#define MALLINFO_FIELD_TYPE size_t
struct mallinfo
{
  MALLINFO_FIELD_TYPE arena;    /* non-mapped space allocated from system */
  MALLINFO_FIELD_TYPE ordblks;  /* number of free chunks */
  MALLINFO_FIELD_TYPE smblks;   /* always 0 */
  MALLINFO_FIELD_TYPE hblks;    /* always 0 */
  MALLINFO_FIELD_TYPE hblkhd;   /* space in mapped regions */
  MALLINFO_FIELD_TYPE usmblks;  /* maximum total allocated space */
  MALLINFO_FIELD_TYPE fsmblks;  /* always 0 */
  MALLINFO_FIELD_TYPE uordblks; /* total allocated space */
  MALLINFO_FIELD_TYPE fordblks; /* total free space */
  MALLINFO_FIELD_TYPE keepcost; /* releasable (via malloc_trim) space */
};
#if defined (__ICCARM__)
struct mallinfo __iar_dlmallinfo(void);
#endif



/*
IAR STDLIB doesn't support threads  by defaulr
 use --threaded_lib to switch the lib
 the following lines implements the Mutex management

*/
#pragma section = "HEAP"

void   avs_complier_get_heap_info(uint32_t *memTotalSpace,uint32_t *memFreeSpace)
{
char * heap_start = __section_begin("HEAP");
 char * heap_limit = __section_end("HEAP");

  struct mallinfo mi = __iar_dlmallinfo();
  uint32_t maxHeap = heap_limit - heap_start;
  if(memTotalSpace) 
  {
    *memTotalSpace= maxHeap;
  }
  if(memFreeSpace) 
  {
    *memFreeSpace = maxHeap  - mi.uordblks;
  }
}




void __iar_system_Mtxinit(__iar_Rmtx *m)
{
}

void __iar_system_Mtxdst(__iar_Rmtx *m)
{
}

void __iar_system_Mtxlock(__iar_Rmtx *m)
{
  avs_core_lock_tasks();
}

void __iar_system_Mtxunlock(__iar_Rmtx *m)
{
  avs_core_unlock_tasks();
}

void __iar_file_Mtxinit(__iar_Rmtx *p)
{
}
void __iar_file_Mtxdst(__iar_Rmtx *p)
{
}
void __iar_file_Mtxlock(__iar_Rmtx *p)
{
  avs_core_lock_tasks();
}
void __iar_file_Mtxunlock(__iar_Rmtx *p)
{
  avs_core_unlock_tasks();

}

/*
* IAR V8  with threaded_lib
* need to provide support for Task Local STorage (aka tls)
* this is requried for errno handling, used in jason libs for example
*/
#if (__IAR_SYSTEMS_LIB__ > 5)
#include <DLib_Threads.h>

#pragma build_attribute vfpcc_compatible
#pragma build_attribute arm_thumb_compatible
__ATTRIBUTES void *__aeabi_read_tp(void);
#pragma section="__iar_tls$$DATA"
void *__aeabi_read_tp(void)
{
  TaskHandle_t xHandle;
  TaskStatus_t xTaskDetails;

  /* Obtain the handle of a task from its name. */
  xHandle = xTaskGetCurrentTaskHandle();

  /* Check the handle is not NULL. */
  configASSERT( xHandle );

  /* Use the handle to obtain further information about the task. */
  vTaskGetInfo( /* The handle of the task being queried. */
                xHandle,
                /* The TaskStatus_t structure to complete with information
                on xTask. */
                &xTaskDetails,
                /* Include the stack high water mark value in the
                TaskStatus_t structure. */
                pdTRUE,
                /* Include the task state in the TaskStatus_t structure. */
                eInvalid );
  
  return (void *)xTaskDetails.pxStackBase;
}


void avs_core_task_create_prolog(avs_task *pTask)
{
  /* Avoid the thread from being sceduled before init is completed */
    vTaskSuspendAll();
}

void avs_core_task_create_epilog(avs_task *pTask)
{
  TaskHandle_t xHandle;
  TaskStatus_t xTaskDetails;

  /* Obtain the handle of a task from its name. */
  xHandle = (TaskHandle_t)pTask->id;

 /* Use the handle to obtain further information about the task. */
  vTaskGetInfo( /* The handle of the task being queried. */
                xHandle,
                /* The TaskStatus_t structure to complete with information
                on xTask. */
                &xTaskDetails,
                /* Include the stack high water mark value in the
                TaskStatus_t structure. */
                pdTRUE,
                /* Include the task state in the TaskStatus_t structure. */
                eInvalid );


  __iar_tls_init((void *)xTaskDetails.pxStackBase);
  

 /* Init completed : thread can be sceduled. */
 xTaskResumeAll();
}
#endif


/* Provides a clock disconnected from the debugger */
clock_t clock (void)
{
  return xTaskGetTickCount();
}

/* Provides a time disconnected from the debugger */
time_t __time32 (time_t * p)
{
  time_t time;
  avs_network_get_time(avs_core_get_instance(), &time);
  if( p != 0)
  {
    *p = time;
  }
  return time;
}
#elif defined (__CC_ARM)

int _mutex_initialize()
{
                return(0);
}
void _mutex_acquire()
{
  avs_core_lock_tasks();
}
void _mutex_release()
{
  avs_core_unlock_tasks();
}
void _mutex_free()
{
  avs_core_unlock_tasks();
}

time_t time(time_t *p)
{
  time_t time;
  AVS_Get_Sync_Time(avs_core_get_instance(), &time);
  if( p) *p = time;
  return time;
}

#else
/* Make sure time() is defined, it is used by JANSSON */
#endif

