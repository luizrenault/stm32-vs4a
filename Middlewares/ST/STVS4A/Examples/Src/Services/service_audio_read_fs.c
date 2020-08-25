/**
******************************************************************************
* @file    service_audio_read_fs.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   manage the sample reading from file system
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

#include "service.h"


static uint8_t  *pAsset;
static uint32_t  iAssetSize;
static uint32_t  iAssetCur;


/*

 Force to close the audio stream


*/
static AVS_Result service_audio_read_fs_close(struct t_player_context *pHandle);
static AVS_Result service_audio_read_fs_close(struct t_player_context *pHandle)
{
  return AVS_OK;
}


/* Open an audio stream */
static AVS_Result service_audio_read_fs_open(struct t_player_context *phandle, const char *pURL);
static AVS_Result service_audio_read_fs_open(struct t_player_context *phandle, const char *pURL)
{
  if(strlen(pURL) > strlen("cid:")) 
  {
    pURL += strlen("cid:");
  }

  pAsset =      (uint8_t  *)(uint32_t)service_assets_load(pURL, &iAssetSize, 0);
  if(pAsset  == 0)
  {
    AVS_TRACE_ERROR("Can't find asset %s", pURL);
    return AVS_ERROR;
  }
  iAssetCur  = 0;



  return AVS_OK ;
}


/*

 pull some data


*/
static AVS_Result service_audio_read_fs_pull(player_context_t *pHandle, void *pBuffer, uint32_t szInSByte, uint32_t *retSize);
static AVS_Result service_audio_read_fs_pull(player_context_t *pHandle, void *pBuffer, uint32_t szInSByte, uint32_t *retSize)
{
  uint32_t   delta = iAssetSize - iAssetCur;
  if(delta == 0) 
  {
    return AVS_EOF;
  }
  if(szInSByte >= delta) 
  {
    szInSByte = delta;
  }
  if(retSize) 
  {
    *retSize = szInSByte;
  }
  memcpy(pBuffer, &pAsset[iAssetCur], szInSByte);
  iAssetCur += szInSByte;
  return AVS_OK;
}

/* Create the FS reader instance */
void service_audio_read_fs_create(player_reader_t *pReader)
{

  /* Init a network stream support */
  pReader->open  = service_audio_read_fs_open;
  pReader->pull  = service_audio_read_fs_pull;
  pReader->close  = service_audio_read_fs_close;
}







