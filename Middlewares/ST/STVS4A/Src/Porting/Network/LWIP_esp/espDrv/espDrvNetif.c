/**
******************************************************************************
* @file    espDrvNetif.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Manage LWIP interface
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
/*
  Implement a LWIP  NETIF using the UART LWIP packets 

*/


#include "espDrvCore.h"
#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"

#define IFNAME0 's'
#define IFNAME1 't'

/* MAC ADDRESS: MAC_ADDR0:MAC_ADDR1:MAC_ADDR2:MAC_ADDR3:MAC_ADDR4:MAC_ADDR5 */

#define MAC_ADDR0   2U
#define MAC_ADDR1   0U
#define MAC_ADDR2   0U
#define MAC_ADDR3   0U
#define MAC_ADDR4   0U
#define MAC_ADDR5   0U

#define ESP_MTU         1300  /* Strangely, if we use a standard MTU of 1500, the esp module crashes */

static struct pbuf *   netif_create_pbuff(uint8_t *pBuffer, uint32_t len);
struct netif           *gInstance_netif;

/**
 * @brief  called from the Event handler, inject a EThernet frame in LWIP 
 * 
 * @param pBuffer 
 * @param len 
 * @return uint32_t 
 */
uint32_t  netif_inject_input_frame(uint8_t *pBuffer, uint32_t len)
{

  struct  netif *netif = gInstance_netif;
  if(netif  == 0) return FALSE;
  if(bSimulateDisconnect) return FALSE;

  err_t err = ERR_BUF;
  LOCK_TCPIP_CORE();
  struct pbuf *p = netif_create_pbuff(pBuffer,len);
  WIFI_ASSERT(p);
  if (p != NULL)
  {
    err = netif->input(p, netif);
    if ( err != ERR_OK)
    {
      WIFI_TRACE_ERROR("Input Frame parsing error");
      pbuf_free(p);
    }
  }
  UNLOCK_TCPIP_CORE();
  iEthRxCount += len;
  return err== ERR_OK ?TRUE : FALSE;
}

/**
 * @brief Called from LWIP, Send an Ethernet packet to the module 
 * 
 * @param netif   netif handle 
 * @param p       buffer
 * @return err_t  error code 
 */

static err_t
netif_linkoutput(struct netif *netif, struct pbuf *p)
{
  if(bSimulateDisconnect) return ERR_TIMEOUT;
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  LWIP_ASSERT("p != NULL", (p != NULL));
#ifndef DISABLE_NETWORK_COM
   espDrv_send_ack_pbuff(MSG_LINKOUTPUT_FRAME_FROM_HOST ,p,0);
#endif
  iEthTxCount += p->tot_len;
  return ERR_OK;
}


/**
 * @brief  netif initialization
 * 
 * @param netif  netif handle 
 * @return err_t error code 
 */

err_t netif_espDrv_init(struct netif *netif)
{

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->linkoutput = netif_linkoutput;
  netif->output =  etharp_output;
  netif->mtu = ESP_MTU;

  gInstance_netif = netif;
  /* If no Mac address defined, Set default values*/
  if (!netif->hwaddr_len)
                                                                                                                                                   {
    /* set netif MAC hardware address length */
    netif->hwaddr_len = 6;
    /* set netif MAC hardware address */
    netif->hwaddr[0] =  MAC_ADDR0;
    netif->hwaddr[1] =  MAC_ADDR1;
    netif->hwaddr[2] =  MAC_ADDR2;
    netif->hwaddr[3] =  MAC_ADDR3;
    netif->hwaddr[4] =  MAC_ADDR4;
    netif->hwaddr[5] =  MAC_ADDR5;
  }
  netif->flags |= NETIF_FLAG_LINK_UP  | NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP       ;

  return ERR_OK;
}
/**
 * @brief Create a LWIP buffer from an aligned buffer
 * 
 * @param pBuffer buffer 
 * @param len buffer size 
 * @return struct pbuf* LWIP pbuff 
 */

static struct pbuf * netif_create_pbuff(uint8_t *pBuffer, uint32_t len)
{
  struct pbuf *p = NULL, *q = NULL;

  if (len > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }
  if(p == 0) return p;
  q = p;
  while(len)
  {
    uint32_t blk =len;
    if(len < q->len) blk = q->len;
    memcpy(q->payload,pBuffer,blk);
    len -= blk;
    pBuffer += blk;
    q = q->next;

  }
  return p;
}






