/**
******************************************************************************
* @file    service_assets.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   Manage QSPI assets
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
 * this code is provided as an example , It is not a production code
 *
 ******************************************************************************/


/******************************************************************************
 *
 * Assets are managed as a primitive file system, the user ask for a name and receive
 * a pointer and a size on the asset
 * asset building and flashing takes time and takes large place in the flash
 * so, there are several option to store assets according the flags AVS_USE_QSPI_FLASHED and AVS_USE_QSPI
 * AVS_USE_QSPI_FLASHED and AVS_USE_QSPI are not defined,
 *     all assets will be stored in flash ( if there is enought rom)
 * AVS_USE_QSPI  is defined , assets will be stored in the Qspi, notice the qspi can't be flashed directely by  IAR,
 *   you have to flash .hex at least once using ST-Link utility
  * AVS_USE_QSPI_FLASHED defined
 *   assets won't be builded and flashed during the development run loop , and the manager will assume that assets are already stored in the qspi ( development  mode)s
 ******************************************************************************/
#include "service.h"
#include "assetConfig.h"
#define  ASSETS_SIGNATURE       0xAA552211U


typedef struct assets_entry_t
{
  char_t pName[50];
  const void *pData;
  const uint32_t size;
} Assets_entry_t;


typedef struct assets_header_t
{
  uint32_t signature;
  /* Assets_entry_t pData[];  Modified because of Keil : Assets.c(100): error:  #1077: an initializer cannot be specified for a flexible array member */
  Assets_entry_t pData[20];
} Assets_header_t;

/* Mandatory assets */

#include "logoSensory_bmp_bsp16.c"
#include "logoST_bmp_bsp16.c"
#include "alarm_active_wav_raw.c"
#include "drip_echo_mono_16K_wav_raw.c"
#include "ding_notification_sound_wav_raw.c"
#include "beanforming_bmp_bsp16.c"
#include "Heart_bmp_bsp16.c"
#include "default_amazon_root_ca_raw.c"
#include "esp8266_bmp_raw.c"


Assets_header_t assets_header_flash_base =
{
  ASSETS_SIGNATURE,
  {
//    {"logoST_bmp", logoST_bmp, sizeof(logoST_bmp) },
//    {"logoSensory_bmp", logoSensory_bmp, sizeof(logoSensory_bmp) },
    {"alarm_active_wav", alarm_active_wav, sizeof(alarm_active_wav) },
    {"drip_echo_mono_16K_wav", drip_echo_mono_16K_wav, sizeof(drip_echo_mono_16K_wav)},
    {"ding_notification_sound_wav", ding_notification_sound_wav, sizeof(ding_notification_sound_wav)},
//    {"beanforming_bmp", beanforming_bmp, sizeof(beanforming_bmp)},
//    {"Heart_bmp", Heart_bmp, sizeof(Heart_bmp)},
//    {"esp8266_bmp", esp8266_bmp, sizeof(esp8266_bmp)},

    {"default_amazon_root_ca", default_amazon_root_ca, sizeof(default_amazon_root_ca)},
    
    
    {"", 0, 0},
    {"", 0, 0},
    {"", 0, 0},
    {"", 0, 0},
    {"", 0, 0},
    {"", 0, 0},
    {"", 0, 0},
    {"", 0, 0},
  }

};


#ifndef AVS_USE_QSPI_FLASHED

/* Assets for tests */
#define  FS_PREFIX_F  const static

#include "tell_me_a_joke_wav_raw.c"
#include "cancel_alarm_2PM_wav_raw.c"
#include "Alexa_wav_raw.c"
#include "set_alarm_2PM_wav_raw.c"
#include "six_sec_wav_raw.c"
#include "set_timer_wav_raw.c"
#include "sing_song_wav_raw.c"
#include "nina_128kb_mp3_raw.c"
#include "unmute_wav_raw.c"
#include "mute_wav_raw.c"
#include "vol_up_wav_raw.c"
#include "vol_dn_wav_raw.c"
#include "mb1388A_bmp_bsp16.c"
#include "omni_mic_bmp_bsp16.c"
#include "mb1337A_bmp_bsp16.c"
#include "shut_up_wav_raw.c"





/*
External assets are used only for test & debug purpose
    so there are no built for the release code
    we manage them optionality and in another partition

*/


/*
Speech are done using these info
  http://www.fromtexttospeech.com/
  Alexa
  Sing me a song.
  Mute.
  UnMute.
  Set Alarm at 2 PM.
  Cancel Alarm at 2 PM..
  Set Timer.
  Sing me a Song.
  Tell me a Joke.
  Volume Down.
  Volume Up.
  Six Seconds.
  shut up

*/




#ifdef AVS_USE_QSPI
#if defined ( __ICCARM__ )
#pragma location="ExtQSPIFlashHeader"
#else
__attribute__((section(".ExtQSPIFlashHeader")))
#endif
#endif
const static Assets_header_t assets_header_flash_ext=
{
  ASSETS_SIGNATURE,
  {
//    {"tell_me_a_joke_wav", tell_me_a_joke_wav, sizeof(tell_me_a_joke_wav) },
//    {"set_alarm_2pm_wav", set_alarm_2PM_wav, sizeof(set_alarm_2PM_wav) },
//    {"Alexa_wav", Alexa_wav, sizeof(Alexa_wav) },
//    {"cancel_alarm_2PM_wav", cancel_alarm_2PM_wav, sizeof(cancel_alarm_2PM_wav)},
//    {"six_sec_wav", six_sec_wav, sizeof(six_sec_wav) },
//    {"set_timer_wav", set_timer_wav, sizeof(set_timer_wav) },
//    {"sing_song_wav", sing_song_wav, sizeof(sing_song_wav)},
//    {"nina_128kb_mp3", nina_128kb_mp3, sizeof(nina_128kb_mp3)},
//    {"unmute_wav", unmute_wav, sizeof(unmute_wav)},
//    {"mute_wav", mute_wav, sizeof(mute_wav)},
//    {"vol_up_wav", vol_up_wav, sizeof(vol_up_wav)},
//    {"vol_dn_wav", vol_dn_wav, sizeof(vol_dn_wav)},
//    {"mb1388A_bmp", mb1388A_bmp, sizeof(mb1388A_bmp)},
//    {"omni_mic_bmp", omni_mic_bmp, sizeof(omni_mic_bmp)},
//    {"mb1337A_bmp", mb1337A_bmp, sizeof(mb1337A_bmp)},
//    {"shut_up_wav", shut_up_wav, sizeof(shut_up_wav)},
    {"", 0,0},
    {"", 0,0},
    {"", 0,0},
    {"", 0,0}
  }

};

#endif


#if !defined(AVS_USE_QSPI_FLASHED)
__STATIC_INLINE const Assets_header_t *get_ext_header(void);
__STATIC_INLINE const Assets_header_t *get_ext_header(void)
{
  return &assets_header_flash_ext;
}
#else
__STATIC_INLINE const Assets_header_t *get_ext_header(void);
__STATIC_INLINE const Assets_header_t *get_ext_header(void)
{
  return  ( const Assets_header_t *)0x90000000;
}
#endif


/*

 check that assets are presents in QSPI


*/
AVS_Result service_assets_check_integrity(void)
{
  const Assets_header_t * hdr =  get_ext_header();
  if(hdr->signature != ASSETS_SIGNATURE)
  {
    return AVS_ERROR;
  }
 return AVS_OK;
}

/*

 look for an entry in a specific partition


*/
static const Assets_entry_t *asset_find_entry(const char_t *pName, uint32_t flags, const Assets_header_t *pHeader);
static const Assets_entry_t *asset_find_entry(const char_t *pName, uint32_t flags, const Assets_header_t *pHeader)
{
  if(pHeader->signature != ASSETS_SIGNATURE)
  {
    return 0;
  }
  const Assets_entry_t *pEntries = pHeader->pData;
  while(pEntries->pData)
  {
    if(strcmp(pEntries->pName, pName) == 0)
    {
      return pEntries;
    }
    pEntries++;
  }
  return 0;
}


/* Look for an entry in all partitions */
static const Assets_entry_t *asset_find_entries(const char_t *pName, uint32_t flags);
static const Assets_entry_t *asset_find_entries(const char_t *pName, uint32_t flags)
{
  const Assets_entry_t *pRes = asset_find_entry( pName, flags, &assets_header_flash_base);
  if(pRes )
  {
    return pRes;
  }
  pRes = asset_find_entry( pName, flags, get_ext_header());
  if(pRes )
  {
    return pRes;
  }

  return 0;
}



/*

 Load an asset


*/
const void *service_assets_load(const char_t *pName, uint32_t *pSize, uint32_t flags)
{
  const Assets_entry_t *pRes = asset_find_entries( pName, flags);
  if(pRes )
  {
    if(pSize)
    {
      *pSize = pRes->size;
    }
    return pRes->pData;
  }
  return NULL;
}

/*

 free an assets ( future use, not implemented )


*/
void service_assets_free(const void *pRes)
{
}








