/**
  ******************************************************************************
  * @file    network.h
  * @author  MCD Application Team
  * @version V1.0.1
  * @date    20-02-2018
  * @brief   Network API.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "mbedtls/ssl.h"  /* Some return codes are defined in ssl.h, so that the functions below may be used as mbedtls callbacks. */

int  network_init(void);
void network_credential(void);
int  network_get_ip_address(uint8_t  *ipAddress);
int  network_connect( void *ctx, const char *hostname, int dstport);
int  network_socket_recv(void *ctx, unsigned char *buf, size_t len);
int  network_socket_send(void *ctx, const unsigned char *buf, size_t len);
int  network_socket_close(uint32_t Socket);

#endif /* _NETWORK_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
