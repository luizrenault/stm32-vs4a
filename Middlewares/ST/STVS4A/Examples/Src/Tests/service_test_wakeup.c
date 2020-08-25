#ifndef __service_test_wakeup__
#define __service_test_wakeup__

/*


 test endurance produce a wakeup, the test checks if the wakeup is detected , if not,  it retries the wakeup


*/
static AVS_Result service_endurance_wakeup(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param);
static AVS_Result service_endurance_wakeup(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param)
{
  static uint32_t bEvent = 0;
  static uint32_t cptRetry = 0;
  if(action == START_TEST )
  {
    /* Start the first dialogue */
    pItem->iIndexResult = 0;
    pItem->timeout = 20; /* Time 10 secs */
    bEvent = FALSE;

    /* Get the entry time to compute the timeout */
    AVS_Get_Sync_Time(hInstance, &pItem->start_time);


    if(sInstanceFactory.initiator == AVS_INITIATOR_TAP_TO_TALK)
    {
      pItem->iIndexResult++;
      return service_endurance_send_msg(handle, pItem, AVS_OK, "Start   %s", pItem->pTestName);
    }
    else
    {
      if(pItem->sequence[pItem->iIndexResult].pResName)
      {
        pItem->sequence[pItem->iIndexResult].pData = service_assets_load(pItem->sequence[pItem->iIndexResult].pResName, 0, 0);
        AVS_ASSERT(pItem->sequence[pItem->iIndexResult].pData );
        service_endurance_say(hInstance, pItem->sequence[pItem->iIndexResult].pData, TRUE, pItem->sequence[pItem->iIndexResult].pResName);

        pItem->iIndexResult++;
        return service_endurance_send_msg(handle, pItem, AVS_OK, "Start   %s", pItem->pTestName);
      }
    }
  }
  if(action == EVENT_TEST )
  {
    /* Wait a EVT_WAKEUP to confirm the word is recognized */
    if(evt == EVT_WAKEUP)  /* Wake up occurs ? */
    {
      /* Signal we got the signal */
      bEvent = TRUE;
      return AVS_OK;
    }
    return AVS_BUSY;
  }

  if(action == CHECK_TEST)
  {

    /* Check the time-out */
    if(service_endurance_check_timeout(handle, pItem))
    {

      return service_endurance_send_msg(handle, pItem, AVS_TIMEOUT, "Time-out : %s", pItem->pTestName);
    }
    /* If tap to talk, no event yo signal a wakeup */
    if(sInstanceFactory.initiator == AVS_INITIATOR_TAP_TO_TALK)
    {
      time_t  curTime;
      AVS_Get_Sync_Time(handle, (AVS_TIME *)&curTime);
      uint32_t delta = curTime - pItem->start_time;
      if(delta != 0)
      {
        service_endurance_start_capture(handle);
        bEvent = TRUE;
      }
    }

    /* If the bEvent is true we have got the evt, let's return a OK */
    if(bEvent)
    {
      return AVS_OK;
    }
    /* If the wakeup doesn't occurs after 2 sec , ie the wakeup is wrong */
    time_t  curTime;
    AVS_Get_Sync_Time(handle, (AVS_TIME *)&curTime);
    uint32_t delta = curTime - pItem->start_time;
    if(delta > 10)
    {
      if(cptRetry > 3)
      {
        cptRetry = 0;
        /* 3 retry, the stack is in error, leave it reboot */
        return AVS_ERROR;
      }
      cptRetry++;
      return AVS_RETRY;
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

#endif /* __service_test_wakeup__ */
