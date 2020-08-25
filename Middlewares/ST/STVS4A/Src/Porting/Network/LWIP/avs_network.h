/**
******************************************************************************
* @file    avs_network.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   This file provides the network abstraction and porting glue
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
#ifndef _avs_network_
#define _avs_network_

#define AVS_USE_NETWORK_LWIP
MISRAC_DISABLE
#include "lwip/sockets.h"
MISRAC_ENABLE
#ifndef htons
#define htons(value) ((((uint16_t)(value) & 0x00FFU) << 8) | (((uint16_t)(value) & 0xFF00U) >> 8))
#define ntohl(value) ((((uint32_t)(x) & 0x000000FFUL) << 24) | (((uint32_t)(x) & 0x0000FF00UL) << 8) | (((uint32_t)(x) & 0x00FF0000UL) >> 8) | (((uint32_t)(x) & 0xFF000000UL) >> 24))



#endif


#endif /* _avs_network_ */

