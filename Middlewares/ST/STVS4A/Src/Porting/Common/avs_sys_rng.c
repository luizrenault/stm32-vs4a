/**
******************************************************************************
* @file    avs_sys_rng.c
* @author  MCD Application Team
* @version V1.2.1
* @date    20-02-2018
* @brief   mbedtls alternate entropy data function.
*          the mbedtls_hardware_poll() is customized to use the STM32 RNG
*          to generate random data, required for TLS encryption algorithms.
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


static RNG_HandleTypeDef RngHandle;
static uint32_t          rng_initalized = 0;

static void ensure_rng_initialized(void);
static void ensure_rng_initialized(void)
{
  if(rng_initalized == 0)
  {
    rng_initalized = 1;
    /* Windows-specific initialization */
    /* Enable RNG peripheral clock */
    __HAL_RCC_RNG_CLK_ENABLE();
    
    /* Clock the IP */
    RngHandle.Instance = RNG;
    __HAL_RNG_ENABLE(&RngHandle);
    
    
    /* DeInitialize the RNG peripheral */
    if (HAL_RNG_DeInit(&RngHandle) != HAL_OK)
    {
      /* DeInitialization Error */
      AVS_Signal_Exeception(AVS_NULL, AVS_SIGNAL_EXCEPTION_GENERAL_ERROR);
    }
    
    /* Initialize the RNG peripheral */
    if (HAL_RNG_Init(&RngHandle) != HAL_OK)
    {
      /* Initialization Error */
      AVS_Signal_Exeception(AVS_NULL, AVS_SIGNAL_EXCEPTION_GENERAL_ERROR);
    }
  }
}


AVS_Result  avs_sys_get_rng(uint32_t *randomValue)
{
  ensure_rng_initialized();
  if(HAL_RNG_GenerateRandomNumber(&RngHandle, randomValue) != HAL_OK) 
  {
    return AVS_ERROR;
  }
  return AVS_OK;
}




