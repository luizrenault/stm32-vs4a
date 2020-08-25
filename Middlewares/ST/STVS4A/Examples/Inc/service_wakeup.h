/**
******************************************************************************
* @file    service_wakeup.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   wakeup and button management
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



#ifndef _service_wakeup_
#define _service_wakeup_

AVS_Result service_wakeup_create(AVS_Handle hHandle);
AVS_Result service_wakeup_event_cb(AVS_Handle hInst, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);

#endif /* _service_wakeup_ */

