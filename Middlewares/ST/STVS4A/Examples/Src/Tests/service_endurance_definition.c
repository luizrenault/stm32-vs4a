
/**
******************************************************************************
* @file    service_endurance_definition.c.c
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

#include "service.h"




/* Include test item objects */
#include "service_test_delay.c"
#include "service_test_sequence.c"
#include "service_test_disconnection.c"
#include "service_test_wakeup.c"
#include "service_test_check_player.c"



#define TEST_WAKEUP              {"Wakeup",1,service_endurance_wakeup,{{EVT_ENDURANCE_TEST_START,"Alexa_wav",0,0},},1,0},
#define TEST_DELAY(Label,nbSec)  {Label,nbSec,service_endurance_delay,{{0,0,0,0},},0,0},


/* Test list for the test session */

Test_item_t tUnitTest[30] =
{
  TEST_WAKEUP
  {
    "Media Player", 60, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "sing_song_wav",0,0},
      {EVT_PLAYER_START,0,0,0},
    }, 1,0},

  TEST_DELAY("Wait some time", 1)
  TEST_WAKEUP
  {
    "Volume up", 10, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "vol_up_wav",0,0},
      {EVT_CHANGE_VOLUME,0,0,0},

    }, 1,0},


  TEST_DELAY("Wait some time", 1)
  TEST_WAKEUP
  {
    "Volume down", 10, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "vol_dn_wav",0,0},
      {EVT_CHANGE_VOLUME,0,0,0},

    }, 1,0},

  TEST_DELAY("Wait some time", 1)
  TEST_WAKEUP
  {
    "Mute", 10, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "mute_wav",0,0},
      {EVT_CHANGE_MUTE,0,0,0},

    }, 1,0},

  TEST_DELAY("Wait some time", 1)
  TEST_WAKEUP

  {
    "unMute", 10, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "unmute_wav",0,0},
      {EVT_CHANGE_MUTE,0,0,0},
    }, 1,0},

  {"Wait Player end", 5 * 60, service_check_player, {{0}}, 1},
  TEST_WAKEUP
  {
    "Multi-turn-timer", 1 * 60, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "set_timer_wav",0,0}, {EVT_STOP_REC,0,0,0},
      {EVT_START_SPEAK,0,0,0}, {EVT_STOP_SPEAK,0,0,0},
      {EVT_START_REC, "six_sec_wav",0,0},
      {EVT_ENDURANCE_ADD_ALARM,0,0,0},
      {EVT_ENDURANCE_ALARM_RING_START,0,0,0},
      {EVT_ENDURANCE_ALARM_RING_STOP,0,0,0}
    }, 1,0},

  TEST_WAKEUP
  {
    "Set Alarm", 20, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "set_alarm_2pm_wav",0,0},
      {EVT_ENDURANCE_ADD_ALARM,0,0,0},
      {EVT_STOP_SPEAK,0,0,0}

    }, 1,0},

  TEST_DELAY("Wait some time", 10)
  TEST_WAKEUP

  {
    "Cancel Alarm", 20, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "cancel_alarm_2PM_wav",0,0},
      {EVT_ENDURANCE_DEL_ALARM,0,0,0},
    }, 1,0},

  TEST_DELAY("Wait some time", 10)
  TEST_WAKEUP
  {
    "Joke", 50, service_endurance_sequence,
    {
      {EVT_ENDURANCE_TEST_START,0,0,0},
      {EVT_START_REC, "tell_me_a_joke_wav",0,0},
      {EVT_STOP_REC,0,0,0},
      {EVT_STOP_SPEAK,0,0,0}
    }, 3,0},

  TEST_WAKEUP
  {"Net Disconnection", 5 * 60, service_endurance_disconnection, {{0,0,0,0}}, 1,0},
  {0}

};

