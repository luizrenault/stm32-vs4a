/**
******************************************************************************
* @file    service_persistent.c
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

/******************************************************************************
 *
 * this code is provided as an example , is is not a production code
 *
 ******************************************************************************/

/******************************************************************************
 *
 * This code manages the persistent stoarage of the token
 * this code store the token if flash
 ******************************************************************************/



#include "service.h"

#define       TOKEN_FIRST_CHARS "Atzr|"

static uint32_t     flash_base_adress ;
static uint32_t     flash_base_sector ;
static uint32_t     flash_sector_size ;
static mutex_t      mutexStorage;

/*
  returns the base address sector and size of the flash storage 
  the default implementation is done for a stm32F769I-disco
  for other board it should be overloaded in init_platform.c

*/

__weak void Get_Board_Flash_Storage(uint32_t *pOffset,uint32_t *pSector,uint32_t *pSectorSize);
__weak void Get_Board_Flash_Storage(uint32_t *pOffset,uint32_t *pSector,uint32_t *pSectorSize)
{

/* Other boards  may need to override this function*/
static const pstorage[128];
if(pOffset != 0)
 {
    *pOffset = pstorage;
 }
 if(pSector != 0)
 {
    *pSector = pstorage;
 }
 if(pSectorSize != 0)
 {
    *pSectorSize = 128;
 }


#ifdef STM32F769xx
 if(pOffset != 0)
  {
     *pOffset = 0x081E0000;
  }
  if(pSector != 0)
  {
     *pSector = FLASH_SECTOR_23;
  }
  if(pSectorSize != 0)
  {
     *pSectorSize = 128;
  }
#endif
}



   
Persistant_t *service_persistent_get_storage(void);
Persistant_t *service_persistent_get_storage(void)
{
  return (Persistant_t*)flash_base_adress;
}



__weak AVS_Result  service_persistent_erase_storage(void);
__weak AVS_Result  service_persistent_erase_storage(void)
{
  AVS_TRACE_ERROR("service_persistent_erase_storage is not overloaded, Persistant disabled");
  return AVS_ERROR;

}


/**
 * @brief Write token from flash
 * @note  BE CAREFULL :the sector number is hard coded below
 *        and this address must be in lin with scatter file content
 * @param[in] context Pointer to the AVS context
 * @return Error code
 **/



__weak AVS_Result service_persistent_write_storage(Persistant_t *pPersistent);
__weak AVS_Result service_persistent_write_storage(Persistant_t *pPersistent)
{
  AVS_TRACE_ERROR("service_persistent_write_storage is not overloaded, Persistant disabled");
  return AVS_ERROR;

}


AVS_Result service_persistent_init_default(void)
{
  Get_Board_Flash_Storage(&flash_base_adress,&flash_base_sector,&flash_sector_size);
  AVS_ASSERT(flash_base_adress != 0);
  AVS_ASSERT(flash_base_sector != 0);
  
  if(mutexStorage.id==0)
  {
    mutex_Create(&mutexStorage,"Storage");
  }

  /* Check if the storage is corrupted, if yes erase  it to start in a normal situation */
  if(service_persistent_get_storage()->signature != PERSISTENT_SIGNATURE)
  {
    service_persistent_erase_storage();
  }
  if(service_persistent_get_storage()->config.endpoint[0] != 0xFF)
  {
    strcpy(gAppState.gEndPoint,service_persistent_get_storage()->config.endpoint);
    sInstanceFactory.urlEndPoint = gAppState.gEndPoint;
  }
  if(service_persistent_get_storage()->root_ca_Store[0] == 0xFF)
  {
    uint32_t assetSize;
    
    const void *pData = service_assets_load("default_amazon_root_ca",&assetSize,0);
    if(pData)
    {
      service_persistent_storage(AVS_PERSIST_SET,AVS_PERSIST_ITEM_CA_ROOT,(char_t *)(uint32_t)pData,assetSize);
    }
    service_assets_free(pData);
  }
  return AVS_OK;
}


uint32_t service_persistent_storage(AVS_Persist_Action action,Avs_Persist_Item item ,char_t *pItem,uint32_t itemSize)
{
  
  if((action == AVS_PERSIST_GET) && (item == AVS_PERSIST_ITEM_TOKEN))
  {
    Persistant_t *pPersistent = service_persistent_get_storage();
    if(pPersistent->signature != PERSISTENT_SIGNATURE)
    {
      AVS_TRACE_DEBUG("Flash storage empty");
      return AVS_ERROR;
    }
    AVS_TRACE_DEBUG("Persistent: Looking from token in Flash");
    /* Check that a token is stored in flash */
    if (strncmp((char_t *) pPersistent->tokenStore, TOKEN_FIRST_CHARS, sizeof(TOKEN_FIRST_CHARS) - 1) )
    {
      AVS_TRACE_DEBUG("Flash doesn't contain a valid token yet");
      return AVS_ERROR;
    }
    int32_t rlen = strlen((char_t *) pPersistent->tokenStore);
    if ( (pItem == NULL) || (itemSize <= rlen ) )
    {
      AVS_TRACE_DEBUG("null buffer, or buffer too small");
      return AVS_ERROR;
    }
    /* Read the token from flash */
    strcpy(pItem, (void *) pPersistent->tokenStore);
    AVS_TRACE_DEBUG("Token read from Flash : %s", pItem);
    return AVS_OK;
  }
  
  if((action == AVS_PERSIST_SET) && (item == AVS_PERSIST_ITEM_TOKEN))
  {
    AVS_TRACE_DEBUG("Persistent: Write token in Flash");
    
    /* Save the storage in memory */
    Persistant_t *pPersistent = pvPortMalloc(sizeof(Persistant_t ));
    AVS_ASSERT(pPersistent);
    memcpy(pPersistent,service_persistent_get_storage(),sizeof(Persistant_t ));
    /* copy update the storage  */
    memcpy(pPersistent->tokenStore,pItem,itemSize);
    AVS_ASSERT(itemSize < sizeof(pPersistent->tokenStore));
    /* Re-write the storage */
    mutex_Lock(&mutexStorage);
    AVS_Result ret = service_persistent_write_storage(pPersistent);
    mutex_Unlock(&mutexStorage);
    /* clean-up */
    vPortFree(pPersistent);
    return ret;
  }
  
  
  if((action == AVS_PERSIST_GET_POINTER) && (item == AVS_PERSIST_ITEM_CA_ROOT))
  {
    AVS_TRACE_DEBUG("Read CA Root");
    
    /* Save the storage in memory */
    Persistant_t *pPersistent = service_persistent_get_storage();
    if(pPersistent->signature != PERSISTENT_SIGNATURE)
    {
      AVS_TRACE_DEBUG("Flash storage empty");
      return AVS_ERROR;
    }
    if(pPersistent->root_ca_Store[0] == 0xFF) 
    {
      AVS_TRACE_DEBUG("Flash Store empty");
      return AVS_ERROR;
    }
    return (uint32_t)pPersistent->root_ca_Store;
  }
  
  if((action == AVS_PERSIST_SET) && (item == AVS_PERSIST_ITEM_CA_ROOT))
  {
    AVS_TRACE_DEBUG("Persistent: Write Item in Flash");
    
    /* Save the storage in memory */
    Persistant_t *pPersistent = pvPortMalloc(sizeof(Persistant_t ));
    AVS_ASSERT(pPersistent);
    memcpy(pPersistent,service_persistent_get_storage(),sizeof(Persistant_t ));
    /* copy update the storage  */
    memcpy(pPersistent->root_ca_Store,pItem,itemSize);
    pPersistent->root_ca_Store[itemSize] = 0;
    itemSize++;
    AVS_ASSERT(itemSize < sizeof(pPersistent->root_ca_Store));
    /* Re-write the storage */
    mutex_Lock(&mutexStorage);
    AVS_Result ret = service_persistent_write_storage(pPersistent);
    mutex_Unlock(&mutexStorage);
    /* clean-up */
    vPortFree(pPersistent);
    return ret;
  }


  if((action == AVS_PERSIST_SET) && (item == AVS_PERSIST_ITEM_WIFI))
  {
    AVS_TRACE_DEBUG("Persistent: Write Item in Flash");
    
    /* Save the storage in memory */
    Persistant_t *pPersistent = pvPortMalloc(sizeof(Persistant_t ));
    AVS_ASSERT(pPersistent);
    memcpy(pPersistent,service_persistent_get_storage(),sizeof(Persistant_t ));
    /* copy update the storage  */
    memcpy(pPersistent->wifi_cntx_Store,pItem,itemSize);
    pPersistent->wifi_cntx_Store[itemSize] = 0;
    itemSize++;
    AVS_ASSERT(itemSize < sizeof(pPersistent->wifi_cntx_Store));
    /* Re-write the storage */
    mutex_Lock(&mutexStorage);
    AVS_Result ret = service_persistent_write_storage(pPersistent);
    mutex_Unlock(&mutexStorage);
    /* clean-up */
    vPortFree(pPersistent);
    return ret;
  }

  if((action == AVS_PERSIST_GET_POINTER) && (item == AVS_PERSIST_ITEM_WIFI))
  {
    AVS_TRACE_DEBUG("Read Wifi Cntx");
    
    /* Save the storage in memory */
    Persistant_t *pPersistent = service_persistent_get_storage();
    if(pPersistent->signature != PERSISTENT_SIGNATURE)
    {
      AVS_TRACE_DEBUG("Flash storage empty");
      return AVS_ERROR;
    }
    if(pPersistent->wifi_cntx_Store[0] == 0xFF) 
    {
      AVS_TRACE_DEBUG("Flash Store empty");
      return AVS_ERROR;
    }
    return (uint32_t)pPersistent->wifi_cntx_Store;
  }
  
  if((action == AVS_PERSIST_GET_POINTER) && (item == SERVICE_PRIVATE_ITEM_CONFIG))
  {
    AVS_TRACE_DEBUG("Read Private config");
    
    /* Save the storage in memory */
    Persistant_t *pPersistent = service_persistent_get_storage();
    if(pPersistent->signature != PERSISTENT_SIGNATURE)
    {
      AVS_TRACE_DEBUG("Flash storage empty");
      return AVS_ERROR;
    }
    return (uint32_t)&pPersistent->config;
  }
  
  if((action == AVS_PERSIST_SET) && (item == SERVICE_PRIVATE_ITEM_CONFIG))
  {
    AVS_TRACE_DEBUG("Write Private config");
    
    /* Save the storage in memory */
    Persistant_t *pPersistent = pvPortMalloc(sizeof(Persistant_t ));
    AVS_ASSERT(pPersistent);
    memcpy(pPersistent,service_persistent_get_storage(),sizeof(Persistant_t ));
    /* copy update the storage  */
    memcpy(&pPersistent->config,pItem,itemSize);
    /* Re-write the storage */
    mutex_Lock(&mutexStorage);
    AVS_Result ret = service_persistent_write_storage(pPersistent);
    mutex_Unlock(&mutexStorage);
    /* clean-up */
    vPortFree(pPersistent);
    return ret;
  }
  
  
  
  

    
  
  return AVS_NOT_IMPL;
}



