/**
  ******************************************************************************
  * @file    audio_devices.h
  * @author  MCD Application Team
  * @version V1.2.0RC2
  * @date    20-02-2018
  * @brief   Abstraction of boared specific devices.
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
#ifndef __AUDIO_DEVICES_H
#define __AUDIO_DEVICES_H

#ifdef __cplusplus
extern "C" {
#endif

#define USE_USB_AUDIO_RECORDING
#define USE_USB_AUDIO_CLASS_10
#define USE_USB_FS_INTO_HS
#define USE_USB_FS
#define USE_AUDIO_RECORDING_VOLUME_CONTROL


/* list of frequencies*/
/* #define USB_AUDIO_CONFIG_FREQ_16_K   16000 */


#define USB_IRQ_PREPRIO 0xA
#define USB_FIFO_WORD_SIZE  320
#define USBD_AUDIO_CONFIG_PLAY_RES_BIT                0x10 /* 16 bit per sample */
#define USBD_AUDIO_CONFIG_PLAY_RES_BYTE               0x02 /* 2 bytes */


#define  USBD_AUDIO_CONFIG_PLAY_BUFFER_SIZE (1024 * 2)



/*record session : list of terminal and unit id for audio function */
/* must be greater than the highest interface number(to avoid request destination confusion */
#define USB_AUDIO_CONFIG_RECORD_TERMINAL_INPUT_ID     0x011
#define USB_AUDIO_CONFIG_RECORD_UNIT_FEATURE_ID       0x015
#define USB_AUDIO_CONFIG_RECORD_TERMINAL_OUTPUT_ID    0x013

#ifdef USB_AUDIO_DEEPER_RECORD_BUFFER_SIZE
#define  USBD_AUDIO_CONFIG_RECORD_BUFFER_SIZE         (1024 * 2 * 8)
#else
#define  USBD_AUDIO_CONFIG_RECORD_BUFFER_SIZE         (1024 * 2)
#endif

/*record session : audio description */
#define USBD_AUDIO_CONFIG_RECORD_CHANNEL_COUNT        0x02 /* stereo */
#define USBD_AUDIO_CONFIG_RECORD_CHANNEL_MAP          0x03 /* (USBD_AUDIO_CONFIG_CHANNEL_LEFT_FRONT|USBD_AUDIO_CONFIG_CHANNEL_RIGHT_FRONT) */


#define USBD_AUDIO_CONFIG_RECORD_RES_BIT              0x10 /* 16 bit per sample */
#define USBD_AUDIO_CONFIG_RECORD_RES_BYTE             0x02 /* 2 bytes */


#define USB_AUDIO_CONFIG_RECORD_FREQ_COUNT           1 /* 1 frequence */
#define USB_AUDIO_CONFIG_RECORD_FREQ_MAX             USB_AUDIO_CONFIG_RECORD_DEF_FREQ
#define USB_AUDIO_CONFIG_RECORD_DEF_FREQ             USB_AUDIO_CONFIG_FREQ_16_K /* to set by user */


#define USBD_AUDIO_CONFIG_RECORD_MAX_PACKET_SIZE ((uint16_t)(AUDIO_MS_MAX_PACKET_SIZE(USB_AUDIO_CONFIG_RECORD_FREQ_MAX,\
    USBD_AUDIO_CONFIG_RECORD_CHANNEL_COUNT,\
    USBD_AUDIO_CONFIG_RECORD_RES_BYTE)))


#define USBD_AUDIO_CONFIG_RECORD_SA_INTERFACE            0x01 /* AUDIO STREAMING INTERFACE NUMBER FOR RECORD SESSION */
#define USB_AUDIO_CONFIG_RECORD_EP_IN                    0x81

#define   AUDIO_MS_PACKET_SIZE(freq,channel_count,res_byte) (((uint32_t)((freq) /1000))* (channel_count) * (res_byte))
#define   AUDIO_MS_MAX_PACKET_SIZE(freq,channel_count,res_byte) AUDIO_MS_PACKET_SIZE((freq)+999,(channel_count),(res_byte))


/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif  /* __AUDIO_USER_DEVICES_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
