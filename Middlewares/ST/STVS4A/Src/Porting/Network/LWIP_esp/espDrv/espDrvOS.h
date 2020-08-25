/**
******************************************************************************
* @file    espDrvOS.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Abstraction OS
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
#ifndef  _espDrvOS_
#define _espDrvOS_
#include "espDrvCore.h"

/*
 * ARM Compiler 4/5
 */
#if   defined ( __CC_ARM )

  #define __ASM            __asm                                      
  #define __INLINE         __inline                                     
  #define __STATIC_INLINE  static __inline

  #include "cmsis_armcc.h"

/*
 * GNU Compiler
 */
#elif defined ( __GNUC__ )

  #define __ASM            __asm                                      /*!< asm keyword for GNU Compiler          */
  #define __INLINE         inline                                     /*!< inline keyword for GNU Compiler       */
  #define __STATIC_INLINE  static inline

  #include "cmsis_gcc.h"


/*
 * IAR Compiler
 */
#elif defined ( __ICCARM__ )

  #ifndef   __ASM
    #define __ASM                     __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                  inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE           static inline
  #endif

  #include <cmsis_iar.h>
#endif



/* Rtos Objects */
typedef xTaskHandle osTask_t;

typedef struct
{
	xSemaphoreHandle h;
} osMutex_t;

typedef struct
{
	xSemaphoreHandle  h;
} osEvent_t;

#define osTaskPriorityIdle        (tskIDLE_PRIORITY)
#define osTaskPriorityNormal      (tskIDLE_PRIORITY+5)
#define osTaskPriorityAboveNormal (tskIDLE_PRIORITY+10)

/* Force Keil compiler to inline critical functions. */
#ifndef __OS_INLINE
#if defined ( __GNUC__  ) || defined ( __ICCARM__   ) /* GCC + IAR */
#define __OS_INLINE  static inline
#elif defined (__CC_ARM) /* Keil */
#define __OS_INLINE __attribute__((always_inline))
#else
#warning "__ALWAYS_INLINE undefined "
#define __OS_INLINE
#endif
#endif



/* ARM ARM says that Load and Store instructions are atomic and it's execution is guaranteed to be complete before interrupt handler executes. */
/* Verified by looking at arch/arm/include/asm/atomic.h */

__OS_INLINE uint32_t osAtomicRead(uint32_t *p32);
__OS_INLINE uint32_t osAtomicRead(uint32_t *p32)
{
  return ((*(volatile uint32_t *)p32));
}
__OS_INLINE void osAtomicWrite(uint32_t *p32, uint32_t val );
__OS_INLINE void osAtomicWrite(uint32_t *p32, uint32_t val )
{
  (*(volatile uint32_t *)p32) = val;
}




typedef void(*osTask_CB)(const void * argument);

osTask_t *osTask_Create(const char *pName, osTask_CB pCb, void *pCookie, uint32_t stackSize, uint32_t priority);
void osTask_Delete(osTask_t *task);
int8_t osMutex_Create(osMutex_t *mutex);
void osMutex_Delete(osMutex_t *mutex);
uint32_t osMutex_Lock(osMutex_t *mutex);
void osMutex_Unlock(osMutex_t *mutex);

int8_t osEvent_Create(osEvent_t *event);
void osEvent_Set(osEvent_t *event);
uint8_t osEvent_Wait(osEvent_t *event, uint32_t timeout);
void osEvent_Delete(osEvent_t *event);
void *osMalloc(uint32_t size);
void osFree(void *pMalloc);
void osTaskDelay(uint32_t delay);

#endif /*_service_os_*/
