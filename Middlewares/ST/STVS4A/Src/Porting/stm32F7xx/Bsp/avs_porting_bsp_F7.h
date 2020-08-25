/*******************************************************************************
* @file    avs_porting_bsp_F7.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   porting layer file
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

#ifndef _avs_porting_bsp_F7_
#define _avs_porting_bsp_F7_
#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <../avs_board_f7.h>



AVS_Result  platform_Audio_init(AVS_audio_handle *pHandle);
AVS_Result  platform_Audio_term(AVS_audio_handle *pHandle);
AVS_Result  platform_Audio_default(AVS_audio_handle *pHandle);
uint32_t    platform_Audio_ioctl(AVS_audio_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);
AVS_Result  platform_Network_init(AVS_instance_handle *pHandle);
AVS_Result  platform_Network_term(AVS_instance_handle *pHandle);
uint32_t    platform_Network_ioctl(AVS_instance_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);
AVS_Result  platform_Network_Solve_macAddr(AVS_instance_handle *pHandle);
AVS_Result  platform_MP3_decoder_init(AVS_audio_handle *pHandle);
AVS_Result  platform_MP3_decoder_term(AVS_audio_handle *pHandle);
uint32_t    platform_MP3_decoder_ioctl(AVS_audio_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);
AVS_Result  platform_MP3_decoder_inject(AVS_audio_handle *pAHandle, uint8_t *pMp3Payload, uint32_t MP3PayloadSize);
AVS_Result  platform_Sys_init(void);
uint32_t    platform_Sys_rnd(void);
uint32_t    platform_Sys_ioctl(AVS_instance_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam);



#ifdef __cplusplus
};
#endif

#endif /* _avs_porting_bsp_F7_ */

