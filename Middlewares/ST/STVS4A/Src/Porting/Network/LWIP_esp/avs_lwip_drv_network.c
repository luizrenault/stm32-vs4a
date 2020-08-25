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
* Redistribution and use in source and binary forms, with or without
* modification, are permitted, provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. Neither the name of STMicroelectronics nor the names of other
*    contributors to this software may be used to endorse or promote products
*    derived from this software without specific written permission.
* 4. This software, including modifications and/or derivative works of this
*    software, must execute solely and exclusively on microcontroller or
*    microprocessor devices manufactured by or for STMicroelectronics.
* 5. Redistribution and use of this software other than as permitted under
*    this license is void and will automatically terminate your rights under
*    this license.
*
* THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
* RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
* SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

#include "avs_private.h"
#include "avs_wifi_private.h"

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
  avs_init_default_string(&pHandle->pFactory->netSupportName, "LWIP_esp");
  avs_init_default_interger(&pHandle->pFactory->used_dhcp, 1U);
  avs_init_default_interger(&pHandle->pFactory->use_mdns_responder, 1U);
  return (AVS_Result)wifi_network_init();
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

  /* to do remove me, just for debug */
  if(code == AVS_TEST_IOCTL)
  {
#if 0    
    /*
    esp_com_Tx_stress_start();
    esp_send_packet(MSG_TEST_TRANSPORT,comm_create_sequence_number(),0,"Test",strlen("Test")+1);
    esp_send_packet(MSG_RX_STRESS_START,comm_create_sequence_number(),0,0,0);
    */
#endif


    return AVS_OK;
  }


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

  if(code == AVS_IOCTL_WIFI_POST_SCAN)
  {
    return wifi_post_scan((char_t*)lparam,wparam);
  }

  if(code == AVS_IOCTL_WIFI_CONNECTED_SPOT)
  {
    /* Scan spots and return the result in a string */
    AVS_ASSERT(wparam);
    AVS_ASSERT(lparam);
    return wifi_get_connected_spot((char_t*)lparam,wparam);

  }
  if(code == AVS_IOCTL_WIFI_CONNECT)
  {
    if(wparam == TRUE)
    {
        return wifi_connect(TRUE,(wifi_mode_e)lparam);
    }
    if(wparam == FALSE)
    {
        return wifi_connect(FALSE,(wifi_mode_e)0);
    }

    return AVS_PARAM_INCORECT;
  }
  return AVS_NOT_IMPL;
}

AVS_Result  platform_Network_term(AVS_instance_handle *pHandle);
AVS_Result  platform_Network_term(AVS_instance_handle *pHandle)
{
  return (AVS_Result)wifi_network_term();

}

