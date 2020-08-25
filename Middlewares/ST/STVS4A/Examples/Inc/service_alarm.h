/**
******************************************************************************
* @file    service_alarm.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   service_alarm.c
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
#ifndef _service_alarm_
#define _service_alarm_


int32_t     service_alarm_add_item(AVS_TIME reftime, const char_t *pToken);
int32_t     service_alarm_find_item(const char_t *pToken);
char_t *      service_alarm_get_string(void);
void        service_alarm_set_time_zone (int32_t zone);
int32_t     service_alarm_get_time_zone (void);
AVS_Result  service_alarm_enable_item(int32_t  index);
AVS_Result  service_alarm_disable_item(int32_t  index);
AVS_Result  service_alarm_create(AVS_Handle hHandle);
void        service_alarm_delete(AVS_Handle hHandle);
void        service_alarm_snooze(void);
AVS_Result service_alarm_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
#endif /* _service_alarm_ */


