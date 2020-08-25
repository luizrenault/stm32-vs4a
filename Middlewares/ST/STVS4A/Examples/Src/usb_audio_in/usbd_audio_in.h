/**
  ******************************************************************************
  * @file    usbd_audio_in.h
  * @author  MCD Application Team
  * @version V1.1.0RC2
  * @date    20-02-2018
  * @brief   This file
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
#ifndef __USB_AUDIO_IN
#define __USB_AUDIO_IN
#include "usbd_audio_ex.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "usbd_devices.h"
#include "usbd_audio_ex.h"


#define   VOLUME_USB_TO_DB_256(v_db, v_usb)                                     (v_db)=((v_usb <= 0x7FFF)? v_usb:  - (((int32_t)0xFFFF - v_usb)+1))
#define   VOLUME_DB_256_TO_USB(v_usb, v_db)                                     (v_usb) = (v_db >= 0)? v_db : ((int32_t)0xFFFF+v_db) +1
#define   AUDIO_USB_PACKET_SIZE(freq,channel_count,res_byte)                    (((uint32_t)((freq) /1000))* (channel_count) * (res_byte))
#define   AUDIO_USB_MAX_PACKET_SIZE(freq,channel_count,res_byte)                AUDIO_USB_PACKET_SIZE(freq+999,channel_count,res_byte)



typedef enum t_usbd_audio_in_evt
{

  AUDIO_INIT,          /* wparam = 0 ,lparam =0 , return void  */
  AUDIO_TERM,          /* wparam = 0 ,lparam =0 , return void  */
  AUDIO_START,          /* wparam = 0 ,lparam =0 , return void  */
  AUDIO_STOP,           /* wparam = 0 ,lparam =0 ,  return void   */
  AUDIO_GET_MUTE,       /* wparam = int16_t *pMunte,lparam =0 , return void    */
  AUDIO_SET_MUTE,        /* wparam = O or 1  ,lparam =0 ,  return void */
  AUDIO_GET_VOLUME,      /* wparam = int16_t *pVolume ,lparam =0 ,  return 0*/
  AUDIO_SET_VOLUME,        /* wparam = the volume ,lparam =0 ,  return void */
  AUDIO_GET_FREQ,        /* wparam = uint32_t *pFreq,lparam =0 ,  return 0*/
  AUDIO_SET_FREQ,        /* wparam = uint32_t**pFreq,lparam =0 ,  return void */

  AUDIO_UNDERRUN,       /* wparam = 0 ,lparam =0 , return void  */
  AUDIO_OVERRRUN,       /* wparam = 0 ,lparam =0 , return void  */
} usbd_audio_in_evt_t;



typedef struct t_record_instance
{
  uint32_t       hValid;                        /* True if the handle is valid */
  struct t_usbd_audio_in_if *pInterface;        /* Interface pointer */
  uint32_t       state;                         /* Internal state */
  uint32_t       alternate;
  uint32_t       recState;                      /* True if the record state is started */
  uint32_t       recFlags;                      /* Internal state */
  uint32_t       packet_length;                 /* Normal packet len according to the freq */
  uint32_t       max_packet_length;             /* Max packet len according to the freq */
  uint32_t       iFreq;                         /* Cur freq */
  uint32_t       iChan;                         /* Cur channel */
  uint8_t        buff[200];                     /* Max 48000 */
  uint32_t       tFreq[5];                      /* Frequ List */
  uint32_t       nbFreq;                         /* nbFreq */
  USBD_AUDIO_FeatureControlCallbacksTypeDef control;

} record_instance_t;


typedef struct t_usbd_audio_in_if
{
  void *pCookie;
  uint32_t  (*notifyEvt)(record_instance_t *pInstance, usbd_audio_in_evt_t evt, uint32_t wparam, uint32_t lparam);    /*  notify USB in state */
  uint8_t  *(*getBuffer)(record_instance_t *pInstance, uint16_t* packet_length);     /* return the raw buffer */

} usbd_audio_in_if_t;

extern USBD_AUDIO_InterfaceCallbacksfTypeDef audio_class_interface;
uint16_t USB_AUDIO_GetConfigDescriptor(uint8_t **desc);
uint16_t USB_AUDIO_GetMaxPacketLen(void);
record_instance_t*  USBD_Audio_Input_Create(uint32_t *pFreq,uint32_t nbFreq,uint32_t ch, usbd_audio_in_if_t *pInterface);
#ifdef __cplusplus
}
#endif

#endif  /* __USB_AUDIO_IN*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/



