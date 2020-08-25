#ifndef __service_test_disconnection__
#define __service_test_disconnection__
#define DISCONNECT_TEST_NORMAL_DELAY       (2*60*1000) /* Max time 60 secs for a normal test, let's take the double */

/*


 test endurance simulate disconnection and wait for a clean restart


*/
static AVS_Result service_endurance_disconnection(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param);
static AVS_Result service_endurance_disconnection(AVS_Handle handle, struct test_item_t *pItem, Action_t action, uint32_t evt, uint32_t param)
{
  if(action == START_TEST )
  {
    /* Start the first dialogue */
    pItem->iIndexResult = 0;

    /* If we use the Bad newtork simulation, this test is useless */
    if(bSimulateBadNetwork) 
    {
      return AVS_OK;
    }

    /* Because we could have a dead lock when we simulate the disconnection */
    /* If  we have in same time a token renew. The  app will never re-connect */
    /* So, we check if the renew token will be renewed during the disconnect time */
    /* And if it is the case, we force the token renew before to enter in  the test */

    /* Get the remaning time before the renew */
    uint32_t remainingTime=0 ;
    AVS_Ioctl(hInstance, AVS_NETWORK_RENEW_TOKEN, 0, (uint32_t)&remainingTime);
    if(remainingTime  < DISCONNECT_TEST_NORMAL_DELAY)
    {
      service_endurance_send_msg(hInstance, pItem, AVS_OK, "Forcing Renew token");
      /* Force and wait the renew */
      if(!AVS_Ioctl(hInstance, AVS_NETWORK_RENEW_TOKEN, 1, 0))
      {
        service_endurance_send_msg(hInstance, pItem, AVS_OK, "Renew token failed");
      }
    }

    AVS_Get_Sync_Time(hInstance, &pItem->start_time);


    Dialogue_t model[] = {{EVT_ENDURANCE_TEST_START,0,0,0}, {EVT_START_REC,0,0,0}, {EVT_CHANGE_STATE, 0, AVS_STATE_RESTART,0}, {EVT_HTTP2_CONNECTED,0,0,0}};
    memcpy(pItem->sequence, model, sizeof(model));

    pItem->iIndexResult++;
    return service_endurance_send_msg(handle, pItem, AVS_OK, "Start :   %s", pItem->pTestName);
  }
  if(action == EVENT_TEST)
  {
    /* If we use the Bad newtork simulation, this test is useless */
    if(bSimulateBadNetwork) 
    {
      return AVS_OK;
    }
    

    if(pItem->sequence[pItem->iIndexResult].evt == 0)  
    {
      return AVS_OK;
    }
    if(pItem->sequence[pItem->iIndexResult].evt != evt ) 
    {
      return AVS_BUSY;
    }
    if(pItem->sequence[pItem->iIndexResult].evt == EVT_START_REC)
    {
      /* Disconnect the network */
      AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, TRUE, 0);
    }
    /* If EVT_CHANGE_STATE && AVS_STATE_RESTART ... reconnect the network */
    if(pItem->sequence[pItem->iIndexResult].evt == EVT_CHANGE_STATE)
    {
      if(pItem->sequence[pItem->iIndexResult].param == param)
      {
        /* Reconnect  the network because we get a restart */
        AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, FALSE, 0);
      }
      else
      {
        /* Stay on the same index */
        pItem->iIndexResult--;
      }

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
    /* If we use the Bad newtork simulation, this test is useless */
    if(bSimulateBadNetwork) 
    {
      return AVS_OK;
    }
    
    if((pItem->iIndexResult != 0) && (evt == EVT_OPEN_TLS) && (param==AVS_ERROR) )
    {
      /*
        if the for some reason we have an error when we open a TLS stream 
        it is the case  for a Token renew. we can fall in an hang situation 
        because we will never receive the EVT_HTTP2_CONNECTED due to the fact that 
        there is no token valid and no posibility to renew it 
        so we re-enable the network and we quit the test to solve this situation
      */
      AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, FALSE, 0);
      return AVS_OK;
    }
    

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
    /* If we use the Bad newtork simulation, this test is useless */
    if(bSimulateBadNetwork) 
    {
      return AVS_OK;
    }

    /* Make sure we are reconnected */
    if((pItem->sequence[pItem->iIndexResult].evt == EVT_CHANGE_STATE) && (pItem->sequence[pItem->iIndexResult].param == AVS_STATE_RESTART))
    {
      /* Disconnect the network */
      AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, FALSE, 0);
    }
    service_assets_free(pItem->sequence[pItem->iIndexResult].pData);
    return AVS_OK;
  }



  return AVS_ERROR;
}

#endif /* __service_test_disconnection__ */

