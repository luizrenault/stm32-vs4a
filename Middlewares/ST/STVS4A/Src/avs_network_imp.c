/**
******************************************************************************
* @file    avs_network_imp_lwip.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This module implements init of the  lwip network
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

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <string.h>
#include <stdint.h>


#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#endif

#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/apps/sntp.h"

#include "ethernetif.h"
#include "avs_private.h"

uint32_t HAL_GetTick(void);
static int8_t ipFound = FALSE;

/**
 * @brief Convert ip adress from string to 32 bits format
 * @params[in] ip adress as string
 * @return Error ip adress as 32 bits
 **/
uint32_t parseIPV4string(char const * ipAddress)
{
  uint32_t ip[4];

  if ( 4 != sscanf(ipAddress, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]) )
    return 0;

  return ((ip[0] & 0xff) | ((ip[1] & 0xff) << 8) |  ((ip[2] & 0xff) << 16)   + ((ip[3] & 0xff) << 24));
}

/**
 * @brief Convert IP address from 32 bits  to string format
 * @params[in] IP address as 32 bits
 * @params[out] IP address as string
 * @return Error IP address as 32 bits
 **/
void buildIPV4string(uint32_t ipv4, char  * ipAddress)
{
  uint8_t  *ip = (uint8_t *) &ipv4;

  sprintf(ipAddress, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

/* Function called by LwIP when like status change*/
void avs_netif_status_callback_fn (struct netif *netif)
{
  /* Retrieve pHandle from LwIP opaque structure*/
  AVS_instance_handle *pHandle = (AVS_instance_handle *)netif->state;

  /* Check New IP address (in case of DHCP*/
  if (netif->ip_addr.addr)
  {
    sprintf( pHandle->stringIpAdress,
             "%lu.%lu.%lu.%lu",
             pHandle->network.netContext.ip_addr.addr & 0xff,
             (pHandle->network.netContext.ip_addr.addr >> 8)  & 0xff,
             (pHandle->network.netContext.ip_addr.addr >> 16) & 0xff,
             (pHandle->network.netContext.ip_addr.addr >> 24) & 0xff);

    avs_atomic_write_charptr(&pHandle->pFactory->ipV4_host_addr, pHandle->stringIpAdress);

    AVS_TRACE_DEBUG("IpAddress = %s", pHandle->stringIpAdress);
    ipFound = TRUE;
  }
  return;
}

/**
 * @brief Initializes lwip network
 * @params[in] AVS Handle
 * @return error code
 **/
AVS_Result avs_network_config(AVS_instance_handle *pHandle)
{
  AVS_Result error;
  ip4_addr_t ipAddr;
  ip4_addr_t netmaskAddr;
  ip4_addr_t gwAddr;
  uint32_t   startTick;
  uint8_t i;

  ipFound = FALSE;

  AVS_TRACE_DEBUG("Start the Network ...");
  AVS_VERIFY(drv_sys.platform_Network_init(pHandle));


  /* Set Mac address*/
  AVS_VERIFY(drv_sys.platform_Network_Solve_macAddr(pHandle));
  pHandle->network.netContext.hwaddr_len = 6;
  for (i = 0; i < pHandle->network.netContext.hwaddr_len; i++)
  {
    pHandle->network.netContext.hwaddr[i] =  pHandle->pFactory->macAddr[i];
  }


  /* Configure tcp stack */
  tcpip_init(NULL, NULL);

  if(pHandle->pFactory->used_dhcp) /* dhcp ?  Set default  adresses */
  {
    ipAddr.addr = netmaskAddr.addr = gwAddr.addr = 0;
  }
  else /* Static addresses */
  {
    /* Set IPv4 host address */
    ipAddr.addr       = parseIPV4string(pHandle->pFactory->ipV4_host_addr);
    /* Set subnet mask */
    netmaskAddr.addr  = parseIPV4string(pHandle->pFactory->ipV4_subnet_msk);
    /* Set default gateway */
    gwAddr.addr       = parseIPV4string(pHandle->pFactory->ipV4_default_gatway);
  }

  /* add the network interface */
  netif_add(&pHandle->network.netContext, &ipAddr, &netmaskAddr, &gwAddr, NULL, &ethernetif_init, &ethernet_input);

  /* register the default network interface */
  netif_set_default(&pHandle->network.netContext);

  netif_set_status_callback(&pHandle->network.netContext, avs_netif_status_callback_fn);

  netif_set_up(&pHandle->network.netContext);

  /* Store AVS handle in the opaque LwIP structure*/
  pHandle->network.netContext.state = (void *)pHandle;

  /* DHCP ?  Retrieves dynamic ip addresses */
  if(pHandle->pFactory->used_dhcp)
  {
    dhcp_start(&pHandle->network.netContext);
  }

  return AVS_OK;
}


static time_t ntpTime = 0;

/**
* @brief synchronise the internal clock with a SNTP sever
**/
AVS_Result  avs_network_synchronize_time(AVS_instance_handle *pInstance)
{
  AVS_Result err = AVS_OK;
  uint8_t timeout =  60; /* 60 iterations */
  ntpTime = 0;


  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");


  sntp_init();
  while (ntpTime == 0 && timeout--)   /* 60 * 1000 ms = 60  seconds maximum */
  {
    avs_core_task_delay(1000);
  }
  if (ntpTime)
  {
    pInstance->syncTime    =  ntpTime  * 1000ULL;      /* Store as Milliseconds */
    pInstance->tickBase    = xTaskGetTickCount();    /* Tick reference */

    avs_core_message_send(pInstance, EVT_SYNC_TIME, AVS_OK);
    err = AVS_OK;
  }
  else
  {
    AVS_TRACE_ERROR("Unable to get ntp time");
    err = AVS_TIMEOUT;
  }
  sntp_stop();
  return err;
}



AVS_Result  avs_network_get_time(AVS_instance_handle *pHandle, time_t *pEpoch)
{
  if(pHandle->syncTime == 0 && pHandle->tickBase == 0) return AVS_NOT_SYNC;

  uint64_t curTicks = xTaskGetTickCount();
  *pEpoch = AVS_TIME_MS_TO_EPOCH(pHandle->syncTime +  (curTicks - pHandle->tickBase));
  return AVS_OK;
}


void avs_set_sytem_time(time_t t)
{
  ntpTime = t;
  return;
}



/**
 * @brief Signals when the IP address is resolved
 * @params[in] AVS Handle
 * @return AVS_OK if IP address is known else AVS_ERROR
 **/
AVS_Result avs_network_check_ip_available(AVS_instance_handle *pHandle)
{
  return ipFound ? AVS_OK : AVS_ERROR;
}



