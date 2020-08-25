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
#include "avs_private.h"
#include "avs_wifi_private.h"

extern int8_t ipFound ;
/*

mutex overload for tls ( create)


*/


void mutex_init (mbedtls_threading_mutex_t  * mutex )
{
  *mutex = xSemaphoreCreateRecursiveMutex();
  AVS_ASSERT(*mutex);
}

/*

mutex overload for tls ( delete)


*/

void mutex_free (mbedtls_threading_mutex_t  * mutex )
{
  AVS_ASSERT(*mutex);
  vSemaphoreDelete(*mutex);
}


/**
* @brief Initializes lwip network
* @params[in] AVS Handle
* @return error code
**/
AVS_Result avs_network_config(AVS_instance_handle *pHandle)
{
  AVS_TRACE_DEBUG("Start the Network ...");
  if(drv_sys.platform_Network_init(pHandle) != AVS_OK)
  {
    AVS_TRACE_ERROR("Network stack init");
    return AVS_ERROR;
  }

 
  AVS_VERIFY(avs_tls_instance_init());

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
  sntp_setservername(0, pInstance->pFactory->urlNtpServer);

  sntp_init();
  while ((ntpTime == 0) && (timeout != 0))   /* 60 * 1000 ms = 60  seconds maximum */
  {
    avs_core_task_delay(1000);
    timeout--;
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
  if((pHandle->syncTime == 0) && (pHandle->tickBase == 0))
  {
    return AVS_NOT_SYNC;
  }

  if(pEpoch != 0)
  {
    uint64_t curTicks = xTaskGetTickCount();
    *pEpoch = AVS_TIME_MS_TO_EPOCH(pHandle->syncTime +  (curTicks - pHandle->tickBase));
  }
  return AVS_OK;
}


void avs_set_sytem_time(time_t t)
{
  ntpTime = t;
  return;
}



/**
* @brief Signals when the IP adress is resolved
* @params[in] AVS Handle
* @return AVS_OK if IP address is known else AVS_ERROR
**/
AVS_Result avs_network_check_ip_available(AVS_instance_handle *pHandle)
{
  return ipFound ? AVS_OK : AVS_ERROR;
}



/*
lwip  set system time hook
*/

void lwip_set_sytem_time(time_t time)
{
  avs_set_sytem_time(time);
}
