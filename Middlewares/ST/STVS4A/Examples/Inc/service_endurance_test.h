
/**
******************************************************************************
* @file    service_endurance_test.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   test some functions and log events
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

#ifndef _service_endurance_test_
#define _service_endurance_test_
#define ENDURANCE_NETWORK_DELAY_DISCONNECTED     (30*1000)                        /* Bad Network simulation, stays disconnected 30 secs */
#define ENDURANCE_NETWORK_DELAY_CONNECTED        (30*1000)                        /* Bad Network simulation, stays connected 30 secs */

#define MAX_SEQUENCE                           30
#define MAX_STACK                              20


typedef enum action_t
{
  START_TEST,
  EVENT_TEST,
  CHECK_TEST,
  STOP_TEST
} Action_t;

typedef struct t_evt_keeper
{
  uint32_t   evt;
  uint32_t   pparam;
} evt_keeper_t;

typedef struct dialogue_t
{
  uint32_t evt;
  const char_t *pResName;
  uint32_t param;
  const void     *pData;
} Dialogue_t;


typedef struct  test_item_t
{

  char_t    *pTestName;
  uint32_t timeout;
  AVS_Result    (*test)(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param);
  Dialogue_t     sequence[MAX_SEQUENCE];
  uint32_t      iIndexResult;
  AVS_TIME        start_time;
} Test_item_t;

extern uint32_t bSimulateBadNetwork;
extern Test_item_t tUnitTest[30];


uint32_t        service_endurance_get_state(void);
void            service_endurance_stop(void);
void            service_endurance_start(void);
uint32_t        service_endurance_enable_network_sim( uint32_t state);
void            service_endurance_get_current_test_name(char_t *pName, uint32_t maxlen);
void            service_endurance_say(AVS_Handle handle, const void *pText, uint32_t mode, const char_t *pText2Say);
uint32_t        service_endurance_check_timeout(AVS_Handle handle, Test_item_t *pItem);
AVS_Result      service_endurance_send_msg(AVS_Handle handle, Test_item_t *pItem, AVS_Result retCode, const char_t *pFormat, ...);
void            service_endurance_start_capture(AVS_Handle handle);
#endif /* _service_endurance_test_ */

