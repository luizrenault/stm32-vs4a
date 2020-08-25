/*******************************************************************************
* @file    avs_porting_bsp_F7_audio.c
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
* this module is in charge to manage audio configuration and management
*
*
*
*/


#include "avs_private.h"
#include "avs_porting_bsp_F7.h"



__weak  AVS_Result init_bsp_audio(AVS_audio_handle *pHandle);
__weak  AVS_Result init_bsp_audio(AVS_audio_handle *pHandle)
{
  return AVS_ERROR;
}
__weak AVS_Result term_bsp_audio(AVS_audio_handle *pHandle);
__weak AVS_Result term_bsp_audio(AVS_audio_handle *pHandle)
{
  return AVS_OK;
}


AVS_Result  platform_Audio_default(AVS_audio_handle *pHandle)
{
  avs_init_default_interger(&pHandle->pFactory->platform.numConfig, ST_BSP_CONFIG_NUM_MIC_1);
  avs_init_default_interger(&pHandle->pFactory->platform.numProfile, ST_BSP_PROFILE_NO_POSTPROCESSING);
  avs_init_default_interger(&pHandle->pFactory->initVolume, 60);
  avs_init_default_interger(&pHandle->pFactory->freqenceOut, 48000);
  avs_init_default_interger(&pHandle->pFactory->freqenceIn, 16000);
  avs_init_default_interger(&pHandle->pFactory->chOut, 2);
  avs_init_default_interger(&pHandle->pFactory->chIn, 1);



  return AVS_OK;
}


AVS_Result  platform_Audio_init(AVS_audio_handle *pHandle)
{
  AVS_ASSERT(pHandle->pFactory != 0);
  /* For BSP debug audio  prorting layer ,we support only default profile and config - ie 0) */
  if( (pHandle->pFactory->platform.numConfig != 0) || (pHandle->pFactory->platform.numProfile != 0))
  {
    AVS_TRACE_ERROR("For BSP debug audio  prorting layer ,we support only default profile and config - ie 0)");
    return AVS_ERROR;
  }

  return init_bsp_audio(pHandle);
}

AVS_Result  platform_Audio_term(AVS_audio_handle *pHandle)
{
  return term_bsp_audio(pHandle);
}

