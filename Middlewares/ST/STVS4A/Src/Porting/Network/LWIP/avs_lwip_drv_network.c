/*******************************************************************************
* @file    avs_lwip_drv_network.c
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


#define IPV4_HOST_ADDR "xxx.xxx.xxx.xxx"
#define IPV4_SUBNET_MASK "xxx.xxx.xxx.xxx"
#define IPV4_DEFAULT_GATEWAY "xxx.xxx.xxx.xxx"
#define IPV4_PRIMARY_DNS "8.8.8.8"
#define IPV4_SECONDARY_DNS "8.8.4.4"




/*

 replace all zero by the default value in the platform context


*/

AVS_Result platform_Network_init(AVS_instance_handle *pHandle);
AVS_Result platform_Network_init(AVS_instance_handle *pHandle)
{
  avs_init_default_string(&pHandle->pFactory->hostName, "hostName");
  avs_init_default_string(&pHandle->pFactory->ipV4_host_addr, IPV4_HOST_ADDR);
  avs_init_default_string(&pHandle->pFactory->ipV4_subnet_msk, IPV4_SUBNET_MASK);
  avs_init_default_string(&pHandle->pFactory->ipV4_default_gatway, IPV4_DEFAULT_GATEWAY);
  avs_init_default_string(&pHandle->pFactory->ipV4_primary_dns, IPV4_PRIMARY_DNS);
  avs_init_default_string(&pHandle->pFactory->ipV4_secondary_dns, IPV4_SECONDARY_DNS);
  avs_init_default_string(&pHandle->pFactory->netSupportName, "LWIP_eth");
  avs_init_default_interger(&pHandle->pFactory->used_dhcp, 1U);
  if(pHandle->pFactory->used_dhcp==2) 
  {
    pHandle->pFactory->used_dhcp=0;
  }
  avs_init_default_interger(&pHandle->pFactory->use_mdns_responder, 1U);

  return AVS_OK;
}
AVS_Result platform_Network_Solve_macAddr(AVS_instance_handle *pHandle);
AVS_Result platform_Network_Solve_macAddr(AVS_instance_handle *pHandle)
{

  if( (pHandle->pFactory->macAddr[0] == 0) &&
      (pHandle->pFactory->macAddr[1] == 0) &&
      (pHandle->pFactory->macAddr[2] == 0) &&
      (pHandle->pFactory->macAddr[3] == 0) &&
      (pHandle->pFactory->macAddr[4] == 0) &&
      (pHandle->pFactory->macAddr[5] == 0) )
  {
    /* Set host board MAC address */
    pHandle->pFactory->macAddr[0] = 0xE0;
    pHandle->pFactory->macAddr[1] = 0xA0;
    pHandle->pFactory->macAddr[2] = ((char_t *)UID_BASE)[0];
    pHandle->pFactory->macAddr[3] = ((char_t *)UID_BASE)[1];
    pHandle->pFactory->macAddr[4] = ((char_t *)UID_BASE)[2];
    pHandle->pFactory->macAddr[5] = ((char_t *)UID_BASE)[3] | 0x80;

  }
  return AVS_OK;
}

uint32_t bSimulateDisconnect = 0;
uint32_t iEthRxCount = 0;
uint32_t iEthTxCount = 0;

uint32_t  platform_Network_ioctl(AVS_instance_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam, ...);
uint32_t  platform_Network_ioctl(AVS_instance_handle *pHandle, uint32_t code, uint32_t wparam, uint32_t lparam, ...)
{
  if(code == AVS_NETWORK_COUNTS)
  {
    if(wparam)
    {
      *((uint32_t *) wparam) = iEthRxCount;
    }

    if(lparam)
    {
      *((uint32_t *) lparam) = iEthTxCount;
    }


    return AVS_OK;
  }



  if(code == AVS_NETWORK_RENEW_TOKEN)
  {
    /* Check param validity */
    if(((int32_t)wparam < 0) && (wparam > 2))
    {
      AVS_TRACE_ERROR("Parameter Invalid");
      return AVS_ERROR;
    }

    /* Wparam = 0 : return the delay before renew */
    if(wparam == 0)
    {
      /* If time is 0 : the Renew is pending */
      if(pHandle->tokenRenewTimeStamp == 0)
      {
        return 0;
      }
      uint32_t timeElapsed = osKernelSysTick() - pHandle->tokenRenewTimeStamp;
      uint32_t *p32 = (uint32_t *)lparam;
      AVS_ASSERT(p32);
      *p32 = (AVS_TOKEN_RENEW_MAX - timeElapsed);

      /* Return the delay */
      return AVS_OK;
    }

    /* Wparam = 1 : Force the renew */
    if(wparam == 1)
    {
      /* Set renew state */
      pHandle->tokenRenewTimeStamp = 0;
      /* Force the renew */
      avs_core_event_set(&pHandle->hRenewToken);
      uint32_t  timeout = (20 * 1000) / DELAY_SLEEP ; /* Wait 20 sec max */
      while(timeout != 0)
      {
        if(pHandle->tokenRenewTimeStamp  != 0)
        {
          return 1;
        }
        avs_core_task_delay(DELAY_SLEEP);
        timeout--;
      }

      return 0;
    }


  }

  if(code == AVS_NETWORK_SIMULATE_DISCONNECT)
  {
    /* Simulate a issue on the network by parching a nic fn that returns fails */
    if((((int32_t)wparam) < 0 ) && (wparam > 3))
    {
      AVS_TRACE_ERROR("Parameter Invalid");
      return AVS_ERROR;
    }

    if(wparam == 0)
    {
      /* Set link up */
      avs_core_atomic_write(&bSimulateDisconnect, 0);


    }
    if(wparam == 1)
    {
      /* Set link down */
      avs_core_atomic_write(&bSimulateDisconnect, 1);

    }

    /* Return the link state */
    return bSimulateDisconnect == 0 ? FALSE : TRUE;
  }

  return AVS_NOT_IMPL;
}

AVS_Result  platform_Network_term(AVS_instance_handle *pHandle);
AVS_Result  platform_Network_term(AVS_instance_handle *pHandle)
{
  return AVS_OK;
}

