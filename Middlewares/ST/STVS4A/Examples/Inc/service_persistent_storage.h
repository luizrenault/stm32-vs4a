/**
******************************************************************************
* @file    service_persistent.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   persistent storage
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

#ifndef __service_persistent__
#define __service_persistent__

#define MAX_TOKEN_STORAGE_SIZE              1024
#define MAX_CA_ROOT_SIZE                    (10*1024)
#define MAX_WIFI_CNTX_SIZE                  (256)
#define PERSISTENT_SIGNATURE                0x23457
typedef struct t_device_config
{
    char_t      lang[10];
    char_t      endpoint[128];
    uint8_t     padding[256-10-128];
}device_config_t;


#define SERVICE_PRIVATE_ITEM_CONFIG       ((Avs_Persist_Item)(((uint32_t)AVS_PERSIST_ITEM_END)+1))



typedef struct t_persistent
{
  uint32_t     signature;
  char_t       tokenStore[MAX_TOKEN_STORAGE_SIZE];
  char_t       root_ca_Store[MAX_CA_ROOT_SIZE];
  char_t       wifi_cntx_Store[MAX_WIFI_CNTX_SIZE];
  device_config_t config;
  uint8_t      padding[28]; /* Insure structure lenght is a multiple of 256 bits for writing to flash)*/ 
}Persistant_t;

 /* check stucture size*/
#ifdef static_assert
static_assert(sizeof(struct t_persistent) % 32 == 0, "struct t_persistent not a multiple of  256 bits");
#endif

uint32_t service_persistent_storage(AVS_Persist_Action action,Avs_Persist_Item item ,char_t *pItem,uint32_t itemSize);
AVS_Result service_persistent_init_default(void);


#endif 
