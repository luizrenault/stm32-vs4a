/**
  ******************************************************************************
  * @file    USB_Device/AUDIO_EXT_Advanced_Player_Recorder/Inc/usbd_conf.h
  * @author  MCD Application Team
  * @version V1.2.0RC2
  * @date    20-02-2018
  * @brief   General low level driver configuration
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CONF_H
#define __USBD_CONF_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "hal_usb_ex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Exported constants --------------------------------------------------------*/
/* Common Config */
#define USBD_MAX_NUM_INTERFACES               2
#define USBD_MAX_NUM_CONFIGURATION            1
#define USBD_MAX_STR_DESC_SIZ                 0x100
#define USBD_SUPPORT_USER_STRING              0
#define USBD_SELF_POWERED                     1
#define USBD_DEBUG_LEVEL                      0
#define USBD_SUPPORT_AUDIO_MULTI_FREQUENCES   1



/* AUDIO Class Config */
/* Exported types ------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Memory management macros */
#define USBD_malloc               malloc
#define USBD_free                 free
#define USBD_memset               memset
#define USBD_memcpy               memcpy
#define USBD_assert(a)            if(!(a)){printf("USB Assert %s:%d \r\n",__FILE__,__LINE__);while(1);}

/* DEBUG macros */
#if (USBD_DEBUG_LEVEL > 0)
#define  USBD_UsrLog(...)   printf(__VA_ARGS__);\
  printf("\n");
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1)

#define  USBD_ErrLog(...)   printf("ERROR: ") ;\
  printf(__VA_ARGS__);\
  printf("\n");
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define  USBD_DbgLog(...)   printf("DEBUG : ") ;\
  printf(__VA_ARGS__);\
  printf("\n");
#else
#define USBD_DbgLog(...)
#endif

/* Exported functions ------------------------------------------------------- */
void USBD_error_handler(void);
#endif /* __USBD_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
