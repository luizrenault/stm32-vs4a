/*******************************************************************************
* @file    avs_porting.h
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

#ifndef _avs_porting_
#define _avs_porting_

#ifdef __cplusplus
extern "C" {
#endif
#include "../avs_board_f7.h"

/* Porting layer for platform operation */
#define PORTING_LAYER_BSP


typedef enum bsp_options
{
  BSP_NO_OPTION = 0,            /* Selection the microphone sample feed , use the optional parameter as AFE_Feed_Options returns AVS_OK or AVS_ERROR */
} BSP_Options;





typedef enum st_bsp_configs
{
  AVS_DEFAULT_CONFIG = 0,
  ST_BSP_CONFIG_NUM_MIC_1 = AVS_DEFAULT_CONFIG

} ST_BSP_Configs;

typedef enum st_bsp_profiles
{
  AVS_DEFAULT_PROFILE = 0,
  ST_BSP_PROFILE_NO_POSTPROCESSING = AVS_DEFAULT_PROFILE

} ST_BSP_Profiles;


typedef struct oavs_Audio_Factory_Platform
{
  uint32_t numConfig;
  uint32_t numProfile;
} AVS_Audio_Factory_Platform;

typedef AVS_Audio_Factory_Platform AVS_Audio_Platform;
typedef BSP_Options                AVS_Audio_Options;


#ifdef __cplusplus
};
#endif
#endif /* _avs_porting_ */
