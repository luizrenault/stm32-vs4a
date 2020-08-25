#ifndef __service__test_sequence__
#define __service__test_sequence__
/*

 Allows to produce a sequence of speech and wait for specific events


*/
static AVS_Result service_endurance_sequence(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param);
static AVS_Result service_endurance_sequence(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param)
{
  if(action == START_TEST )
  {
    /* Start the first dialogue */
    pItem->iIndexResult = 0;
    AVS_Get_Sync_Time(hInstance, &pItem->start_time);
    pItem->iIndexResult++;
    return service_endurance_send_msg(handle, pItem, AVS_OK, "Start %s", pItem->pTestName);
  }
  if(action == EVENT_TEST )
  {
    if(pItem->sequence[pItem->iIndexResult].evt == 0)  
    {
      return AVS_OK;
    }
    if(pItem->sequence[pItem->iIndexResult].evt != evt) 
    {
      return AVS_BUSY; /* Not the expected msg */
    }
    if(pItem->sequence[pItem->iIndexResult].pResName)
    {
      pItem->sequence[pItem->iIndexResult].pData = service_assets_load(pItem->sequence[pItem->iIndexResult].pResName, 0, 0);
      AVS_ASSERT(pItem->sequence[pItem->iIndexResult].pData );
      service_endurance_say(hInstance, pItem->sequence[pItem->iIndexResult].pData, FALSE, pItem->sequence[pItem->iIndexResult].pResName);
      service_assets_free(pItem->sequence[pItem->iIndexResult].pData);
    }

    if(pItem->iIndexResult >= MAX_SEQUENCE)
    {
      return service_endurance_send_msg(handle, pItem, AVS_ERROR, "Failed : %s", pItem->pTestName);
    }
    pItem->iIndexResult++;
    return AVS_BUSY;
  }

  if(action == CHECK_TEST)
  {
    if(service_endurance_check_timeout(handle, pItem))
    {
      return service_endurance_send_msg(handle, pItem, AVS_TIMEOUT, "Timeout : %s", pItem->pTestName);
    }
    if(pItem->sequence[pItem->iIndexResult].evt == 0)
    {
      return service_endurance_send_msg(handle, pItem, AVS_OK, "Success : %s", pItem->pTestName);
    }
    if(pItem->iIndexResult >= MAX_SEQUENCE)
    {
      return service_endurance_send_msg(handle, pItem, AVS_ERROR, "Error : %s", pItem->pTestName);
    }
    return AVS_BUSY;
  }

  if(action == STOP_TEST )
  {
    service_assets_free(pItem->sequence[pItem->iIndexResult].pData);
    return AVS_OK;
  }



  return AVS_ERROR;
}

#endif /* __service__test_sequence__ */


