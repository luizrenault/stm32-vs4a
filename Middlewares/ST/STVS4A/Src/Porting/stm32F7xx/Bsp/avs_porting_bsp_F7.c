/*******************************************************************************
* @file    avs_porting_bsp_F7.c
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

/*
*
*
* this module to initalise the driver according to the porting layer platform
*
*
*
*/



#include "avs_private.h"
#include "avs_porting_bsp_F7.h"


/* Replace all zero by the default value in the platform context */

static   AVS_Result platform_init(AVS_instance_handle *pHandle);
static   AVS_Result platform_init(AVS_instance_handle *pHandle)
{
  static char_t tCpuID[30];
  /* Generate a default serial number if not set */
  static char_t serialNumber[20];
  sprintf( serialNumber, "S/N_%lu", *(uint32_t *)UID_BASE);
  avs_init_default_string(&pHandle->pFactory->serialNumber, serialNumber);
  pHandle->pFactory->portingAudioName = "bsp-audio";
  uint32_t var  = (SCB->CPUID & SCB_CPUID_VARIANT_Msk) >> SCB_CPUID_VARIANT_Pos;
  uint32_t rev  = (SCB->CPUID & SCB_CPUID_REVISION_Msk) >> SCB_CPUID_REVISION_Pos;
  snprintf(tCpuID, sizeof(tCpuID), "STM32F769I cut %lu.%lu", rev + 1, var); /* Revision field start to 0 but knwon as version 1, so +1 :) */
  pHandle->pFactory->cpuID = tCpuID;
  avs_init_default_interger(&pHandle->pFactory->profile, AVS_PROFILE_NEAR_FIELD);
  avs_init_default_interger(&pHandle->pFactory->initiator, AVS_INITIATOR_TAP_TO_TALK);
  return AVS_OK;
}


AVS_Result drv_platform_Init(void)
{

  drv_sys.platform_Sys_init =     platform_Sys_init;
  drv_sys.platform_Sys_rnd  =     platform_Sys_rnd;
  drv_sys.platform_Sys_ioctl =     platform_Sys_ioctl;

  drv_instance.platform_init =     platform_init;

  drv_audio.platform_Audio_init  = platform_Audio_init;
  drv_audio.platform_Audio_term  = platform_Audio_term;
  drv_audio.platform_Audio_ioctl = platform_Audio_ioctl;
  drv_audio.platform_Audio_default = platform_Audio_default;

  /* Init the audio mp3 SW decoder */
  drv_audio.platform_MP3_decoder_init = platform_MP3_decoder_init;
  drv_audio.platform_MP3_decoder_term = platform_MP3_decoder_term ;
  drv_audio.platform_MP3_decoder_ioctl = platform_MP3_decoder_ioctl;
  drv_audio.platform_MP3_decoder_inject = platform_MP3_decoder_inject;


  /* Init the network support phy */
  drv_sys.platform_Network_init = platform_Network_init;
  drv_sys.platform_Network_term = platform_Network_term;
  drv_sys.platform_Network_ioctl = platform_Network_ioctl;

  drv_sys.platform_Network_Solve_macAddr = platform_Network_Solve_macAddr;
  return AVS_OK;

}



