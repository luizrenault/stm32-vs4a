/**
******************************************************************************
* @file    espDrvGlueAvs.h
* @author  MCD Application Team
* @brief   Makes the connection with STAVS API
*
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
* All rights reserved.</center></h2>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted, provided that the following conditions are met:
*
* 1. Redistribution of source code must retain the above copyright notice,
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

/*
  Implement the connection with STVS4A for the environment control

*/

#include "espDrvGlueAvs.h"
#include "avs_private.h"
#include "avs_network_private.h"
#include "avs_wifi_private.h"

int8_t ipFound = FALSE;


void espDrv_mem_dump(uint8_t *pPayload,uint32_t szpPayload)
{
    AVS_Dump((uint32_t)-1,"",pPayload,szpPayload);
}



/**
* @brief Convert ip address from string to 32 bits format
* @params[in] ip address as string
* @return Error ip address as 32 bits
**/
uint32_t parseIPV4string(const char_t* ipAddress);
uint32_t parseIPV4string(const char_t* ipAddress)
{
  uint32_t ip[4];

  if ( 4 != sscanf(ipAddress, "%lu.%lu.%lu.%lu", &ip[0], &ip[1], &ip[2], &ip[3]) )
  {
    return 0;
  }

  return ((ip[0] & 0xff) | ((ip[1] & 0xff) << 8) |  ((ip[2] & 0xff) << 16)   |  ((ip[3] & 0xff) << 24));
}

/**
* @brief Convert ip address from 32 bits  to string format
* @params[in] ip address as 32 bits
* @params[out] ip address as string
* @return Error ip address as 32 bits
**/
void buildIPV4string(uint32_t ipv4, char  * ipAddress);
void buildIPV4string(uint32_t ipv4, char  * ipAddress)
{
  uint8_t  *ip = (uint8_t *)(uint32_t) &ipv4;
  sprintf(ipAddress, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

/**
 * @brief Notify a Scan Result 
 * 
 * @param pResult a string with the scan result
 */
void    espDrv_notify_scan_result(char_t *pResult)
{
  AVS_instance_handle *pHandle = avs_core_get_instance();
  if(pResult)
  {
    avs_core_message_send(pHandle, EVT_WIFI_SCAN_RESULT, (uint32_t)pResult);
  }
  else
  {
    avs_core_message_send(pHandle, EVT_WIFI_SCAN_RESULT, 0);
  }
}

/**
 * @brief Notify the Wifi spot is connected 
 * 
 * @param state True or false
 */
void   espDrv_notify_wifi_connected(uint32_t state)
{
  AVS_instance_handle *pHandle = avs_core_get_instance();
  if(state == TRUE)
  {
    /* Signal station connected */
    avs_core_message_post(pHandle, EVT_WIFI_CONNECTED, AVS_OK);
    wifi_network_start_dhcp();
  }
  else
  {
    avs_core_message_post(pHandle, EVT_WIFI_CONNECTED, AVS_ERROR);
  }
}

/**
 * @brief Return the connection info string 
 * 
 * @return char_t* The info string 
 */

char_t * wifi_get_connection_info()
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)avs_core_get_instance();
  uint32_t  ret = pHandle->pFactory->persistentCB(AVS_PERSIST_GET_POINTER,AVS_PERSIST_ITEM_WIFI,0,0);
  if(ret < AVS_END_RESULT)
  {
    return NULL;
  }
  return (char_t *)ret;
}

/**
 * @brief Set the Mac address in LWIP 
 * 
 * @param pMac 
 */
void wifi_set_mac_adress(uint8_t *pMac)
{
  AVS_instance_handle *pHandle = (AVS_instance_handle *)avs_core_get_instance();
  memcpy(& pHandle->network.netContext.hwaddr,pMac,sizeof(pHandle->network.netContext.hwaddr));
}

/**
 * @brief Start the LWIP DHCP
 * 
 * @return uint32_t 
 */


uint32_t   wifi_network_start_dhcp()
{
  /* Dhcp ?  Retrieves dynamic IP addresses */
  if(avs_core_get_instance()->pFactory->used_dhcp)
  {
    ipFound = FALSE;
    dhcp_start(&avs_core_get_instance()->network.netContext);
  }
  return TRUE;
}

/**
 * @brief Produces a formatted trace on the console according to the lvl
 * 
 * @param lvl  bit field lvl
 * @param cr    true if the message add a cr at the end 
 * @param pFormat format 
 * @param ... 
 */
void  wifi_trace(uint32_t lvl, uint32_t cr, const char *pFormat, ...)
{
  if ((lvl & wifiTraceLevel) != 0)
  {
    static char tLocal[100];
    va_list args;
    va_start(args, pFormat);
    vsnprintf(tLocal, sizeof(tLocal) - 1, pFormat, args);
    if (cr) strcat(tLocal, "\r");
    AVS_Printf((uint32_t)-1,"Wifi:%s",tLocal);
    va_end(args);
  }
}

/**
 * @brief LWIP callback NETIF
 * 
 * @param pnetif 
 */


void avs_netif_link_callback_fn (struct netif *pnetif)
{
  /* Retrieve pHandle from LwIP opaque structure*/
  AVS_instance_handle *pHandle = (AVS_instance_handle *)avs_core_get_instance();

  if(netif_is_link_up(pnetif)==TRUE && (ipFound==FALSE) )
  {
    /* Check New IP address (in case of dhcp*/
    if (pHandle->network.netContext.ip_addr.addr)
    {
      uint8_t *pIp = (uint8_t *)&pHandle->network.netContext.ip_addr;
      snprintf( (char *)pHandle->stringIpAdress,sizeof(pHandle->stringIpAdress),
               "%u.%u.%u.%u",
               pIp [0],
               pIp [1],
               pIp [2],
               pIp [3]
                 );
      avs_atomic_write_charptr(&pHandle->pFactory->ipV4_host_addr, pHandle->stringIpAdress);
      ipFound = TRUE;

    }
    if (pHandle->network.netContext.netmask.addr)
    {
      static char_t strNetmask[4*4+1];
      uint8_t *pIp = (uint8_t *)&pHandle->network.netContext.netmask;
      snprintf( (char *)strNetmask,sizeof(strNetmask),
               "%u.%u.%u.%u",
               pIp [0],
               pIp [1],
               pIp [2],
               pIp [3]);

      avs_atomic_write_charptr(&pHandle->pFactory->ipV4_subnet_msk, strNetmask);
    }
    if (pHandle->network.netContext.gw.addr)
    {
      static char_t strGw[4*4+1];
      uint8_t *pIp = (uint8_t *)&pHandle->network.netContext.gw;

      snprintf( (char *)strGw,sizeof(strGw),
               "%u.%u.%u.%u",
               pIp [0],
               pIp [1],
               pIp [2],
               pIp [3]);
      avs_atomic_write_charptr(&pHandle->pFactory->ipV4_default_gatway, strGw);
    }
    if(ipFound)
    {
      WIFI_TRACE_VERBOSE("IpAdress = %s", pHandle->stringIpAdress);
      avs_core_message_send(pHandle, EVT_NETWORK_IP_OK, 0);

    }
  }
  if(netif_is_link_up(pnetif)==FALSE)
  {
    avs_atomic_write_charptr(&pHandle->pFactory->ipV4_host_addr, "xxx.xxx.xxx.xxx");
    ipFound = FALSE;
  }

  return;
}

/**
 * @brief Init the Network stacks
 * 
 * @param wifistate spot connected 
 * @return uint32_t 
 */


uint32_t     wifi_network_lwip_init(uint32_t wifistate)
{
  ip4_addr_t ipAddr;
  ip4_addr_t netmaskAddr;
  ip4_addr_t gwAddr;
  uint8_t i;
  AVS_instance_handle *pHandle = avs_core_get_instance();
  pHandle->network.esp8266Online = wifistate;

  /* Set Mac address*/
  WIFI_VERIFY(drv_sys.platform_Network_Solve_macAddr(pHandle));
  pHandle->network.netContext.hwaddr_len = 6;
  for (i = 0; i < pHandle->network.netContext.hwaddr_len; i++)
  {
    pHandle->pFactory->macAddr[i] = pHandle->network.netContext.hwaddr[i];
  }
  ipFound = FALSE;
  /* Configure TCP stack */
  tcpip_init(NULL, NULL);

  if(pHandle->pFactory->used_dhcp) /* DHCP ?  Set default  address */
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

  /* Add the network interface */
  netif_add(&pHandle->network.netContext, &ipAddr, &netmaskAddr, &gwAddr, NULL, &netif_espDrv_init, &ethernet_input);

  /* Register the default network interface */
  netif_set_default(&pHandle->network.netContext);

  netif_set_status_callback(&pHandle->network.netContext, avs_netif_link_callback_fn);
  netif_set_link_callback(&pHandle->network.netContext, avs_netif_link_callback_fn);

  netif_set_up(&pHandle->network.netContext);

  /* Store AVS handle in the opaque LwIP structure*/
  pHandle->network.netContext.state = (void *)pHandle;
  wifi_network_start_dhcp();

  return WIFI_OK;
}

/**
 * @brief Terminate network wifi session
 * 
 * @param pHandle 
 * @return uint32_t 
 */

uint32_t wifi_network_lwip_term(void)
{
  return WIFI_OK;
}
