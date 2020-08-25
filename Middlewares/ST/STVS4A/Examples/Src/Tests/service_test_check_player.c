#ifndef __service_test_player__
#define __service_test_player__

/* Test Waiting for a player finished */

extern uint32_t service_player_is_active(void);

static AVS_Result service_check_player(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param);
static AVS_Result service_check_player(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param)

{
  if(action == START_TEST )
  {
    /* Start the first dialogue */
    pItem->iIndexResult = 0;
    AVS_Get_Sync_Time(hInstance, &pItem->start_time);
    pItem->iIndexResult++;
    return service_endurance_send_msg(handle, pItem, AVS_OK, "Start :   %s", pItem->pTestName);
  }
  if(action == EVENT_TEST)
  {
    return AVS_BUSY;
  }

  if(action == CHECK_TEST)
  {
    if(service_endurance_check_timeout(handle, pItem))   
    {
      return service_endurance_send_msg(handle, pItem, AVS_TIMEOUT, "Time-out : %s", pItem->pTestName);
    }
    if(service_player_is_active() == 0) 
    {
      return AVS_OK;
    }
    return AVS_BUSY;
  }

  if(action == STOP_TEST )
  {
    return AVS_OK;
  }



  return AVS_ERROR;
}

#endif /* __service_test_player__ */

