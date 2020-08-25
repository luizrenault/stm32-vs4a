#ifndef __service_test_delay__
#define __service_test_delay__

/* Just produce a delay and wait for the idle state in order to start on a clean state */
static AVS_Result service_endurance_delay(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param);
static AVS_Result service_endurance_delay(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param)
{
  if(action == START_TEST)
  {
    pItem->iIndexResult = 0;
    AVS_Get_Sync_Time(hInstance, &pItem->start_time);
    return service_endurance_send_msg(handle, pItem, AVS_OK, "%s : %d secs", pItem->pTestName, pItem->timeout);
  }
  if(action == EVENT_TEST)
  {
    return AVS_BUSY;
  }

  if(action == CHECK_TEST)
  {
    if(service_endurance_check_timeout(handle, pItem)) 
    {
      return AVS_OK;
    }
    return AVS_BUSY;
  }
  if(action == STOP_TEST)
  {
    return AVS_OK;
  }

  return AVS_ERROR;
}
#endif /* __service_test_delay__ */
