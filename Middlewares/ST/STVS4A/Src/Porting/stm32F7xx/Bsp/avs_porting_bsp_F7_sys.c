/*******************************************************************************
* @file    avs_porting_bsp_sys.c
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
#include "avs_private.h"
#include "avs_porting_bsp_F7.h"

/*


     We assume that the HAL_Init is done and we have to initialize  the system only for
     IP used by the STVS4A system, audio and instance will be initialied later


*/

AVS_Result      platform_Sys_init(void)
{
  HAL_InitTick(0); /* [DSe] TODO -  dafault tick priority wont work (handler never called) */
  HAL_ResumeTick();
  return AVS_OK;
}


/*

 returns a random uint32_t

*/
uint32_t platform_Sys_rnd(void)
{
  uint32_t value;
  if(avs_sys_get_rng(&value) != AVS_OK)
  {
      AVS_ASSERT(0);
      return 0;
  }
  return value;
}



uint32_t   platform_Sys_ioctl(AVS_instance_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam)
{
  if(code == AVS_SYS_SET_LED)
  {
//    Led_TypeDef tLed[] = {LED1, LED2};
    if(wparam >= 2 )
    {
      wparam = 1;
    }
    if(lparam == 0)
    {
//      HAL_GPIO_WritePin(LED;
    }
    else
    {
//      BSP_LED_On(tLed[wparam]);
    }
    return AVS_OK;
  }

  return AVS_NOT_IMPL;
}








