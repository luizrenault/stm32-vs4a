/**
******************************************************************************
* @file    service_gui_page_info.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   graphic  user interface
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2018 STMicroelectronics International N.V.
* All rights reserved.</center></h2>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted, provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. Neither the name of STMicroelectronics nor the names of other
*    contributors to this software may be used to endorse or promote products
*    derived from this software without specific written permission.
* 4. This software, including modifications and/or derivative works of this
*    software, must execute solely and exclusively on microcontroller or
*    microor devices manufactured by or for STMicroelectronics.
* 5. Redistribution and use of this software other than as permitted under
*    this license is void and will automatically terminate your rights under
*    this license.
*
* THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
* RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
* SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

/******************************************************************************
 *
 * this code is provided as an example , It is not a production code
 *
 ******************************************************************************/
#include "service.h"


void gui_change_avs_initiator_type(void);
void gui_change_avs_initiator_type(void)
{
  if (sInstanceFactory.initiator == AVS_INITIATOR_PUSH_TO_TALK )
  {
    sInstanceFactory.initiator = AVS_INITIATOR_TAP_TO_TALK;
    sInstanceFactory.profile      = AVS_PROFILE_NEAR_FIELD;
  }
  else if (sInstanceFactory.initiator == AVS_INITIATOR_TAP_TO_TALK )
  {
    sInstanceFactory.initiator = AVS_INITIATOR_VOICE_INITIATED;
    sInstanceFactory.profile      = AVS_PROFILE_NEAR_FIELD;
  }
  else
  {
    sInstanceFactory.initiator = AVS_INITIATOR_PUSH_TO_TALK;
    sInstanceFactory.profile      = AVS_PROFILE_CLOSE_TALK;
  }
}



static uint32_t  gui_proc_initiator(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t   gui_proc_initiator(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  static char_t *tInitiatorStr[] = {"DEFAULT", "Push To Talk", "Tap To Talk", "Voice Initiated\rTap To Talk"};

  if(message== GUI_MSG_BUTTON_CLICK)
  {
    gui_change_avs_initiator_type();
  }


  if(message == GUI_MSG_INIT)
  {
    pItem->format |= DRT_FRAME;
    pItem->format |= DRT_TS_ITEM;
  }
  if(message == GUI_MSG_RENDER)
  {
    int32_t index = sInstanceFactory.initiator - AVS_INITIATOR_DEFAULT;
    if(index > AVS_INITIATOR_VOICE_INITIATED)
    {
      index = 0;
    }
    pItem->pText = tInitiatorStr[index];
  }
 return gui_proc_default(pItem,message,lparam);
}


/*

 render all alarms


*/
static uint32_t gui_proc_alarm(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t gui_proc_alarm(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  if(message == GUI_MSG_INIT)
  {
    pItem->offX     = 20;
    pItem->format  &= ~(uint32_t)0xFF;
    pItem->format  |= DRT_HCENTER | DRT_VLF ;
    pItem->format  |= DRT_FRAME ;

  }
  if(message == GUI_MSG_RENDER)
  {
    pItem->pText = service_alarm_get_string();
    if(pItem->pText == 0)
    {
      pItem->pText = "";
    }
  }
  return gui_proc_default(pItem,message,lparam);
}


/*

 proc player state


*/
static uint32_t gui_proc_player(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t gui_proc_player(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  if(message == GUI_MSG_INIT)
  {
    pItem->offX     = 20;
    pItem->format  &= ~(uint32_t)0xFF;
    pItem->format  |= DRT_HCENTER | DRT_VLF ;
    pItem->format  |= DRT_FRAME ;
  }
  if(message == GUI_MSG_RENDER)
  {
    pItem->pText = service_player_get_string();
    if(pItem->pText == 0)
    {
      pItem->pText = "";
    }
  }
  return gui_proc_default(pItem,message,lparam);
}


AVS_Result service_gui_event_page_info_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
AVS_Result service_gui_event_page_info_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
void gui_add_page_info(void);
void gui_add_page_info(void)
{
  gui_add_item_res(GUI_ITEM_ALARM, gui_proc_alarm, 0);
  gui_add_item_res(GUI_ITEM_PLAYER, gui_proc_player, 0);
  gui_add_item_res(GUI_ITEM_INITIATOR, gui_proc_initiator, 0);
}
