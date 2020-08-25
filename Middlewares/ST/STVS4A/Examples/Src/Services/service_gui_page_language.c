/**
******************************************************************************
* @file    service_gui_page_language.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   flash esp8266 user interface
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

#include "service.h"


#define GUI_MSG_SYNC_LISTBOX                    (GUI_MSG_USER+3)
#define AVS_MAX_LANG                            8         

static char_t                    *tlistLangue[AVS_MAX_LANG]=
{
  "en-US",
  "fr-FR",
  "ja-JP",
  "de-DE",
  "en-AU",
  "en-CA",
  "en-GB",
  "en-IN"
};





static listBox_instance_t         gLangageListInstance={AVS_MAX_LANG,0,0,tlistLangue,"No language",100};
static char_t                     gCurLang[10];
static uint32_t                   gConnected;

AVS_Result  service_langage_apply(AVS_Handle handle,char_t *pLang)
{
  uint32_t err = 0;
  json_t *root  = json_object();
  json_t *event = json_object();
  json_t *header = json_object();
  json_t *payload = json_object();
  json_t *locale_settings =  json_object();
  /* Initialize JSON arrays */
  json_t *context = json_array();
  json_t *settings = json_array();
  
  if(!gConnected ) 
  {
    return AVS_ERROR;
  }
  static char_t msgid[32];
  static uint32_t   messageIdCounter;               /* Message id index */
  
  snprintf(msgid,sizeof(msgid), "lang%lu", messageIdCounter);
  messageIdCounter++;
  err |= (uint32_t)json_object_set_new(header, "namespace", json_string("Settings"));
  err |= (uint32_t)json_object_set_new(header, "name", json_string("SettingsUpdated"));
  err |= (uint32_t)json_object_set_new(header, "messageId", json_string(msgid));
  /* Links */
  err |= (uint32_t)json_object_set_new(root, "event", event);
  err |= (uint32_t)json_object_set_new(event, "header", header);
  err |= (uint32_t)json_object_set_new(locale_settings, "key", json_string("locale"));
  err |= (uint32_t)json_object_set_new(locale_settings, "value", json_string(pLang));
  err |= (uint32_t)json_array_append_new(settings, locale_settings);
  err |= (uint32_t)json_object_set_new(payload, "settings", settings);
  
  
  err |= (uint32_t)json_object_set_new(event, "payload", payload);
  AVS_ASSERT(err ==0);
  if(err != 0)
  {
    AVS_TRACE_ERROR("Json returns an error");
  }
  AVS_VERIFY(json_object_set_new(root, "context", context) == 0);
  
  const char_t *pJson = json_dumps(root, 0);
  json_decref(root);
  if(AVS_Send_JSon(handle, pJson) != AVS_OK)
  {
    AVS_TRACE_ERROR("Send Json Event");
  }
  jsonp_free((void *)(uint32_t)pJson);
  return AVS_OK;
}



/*

Language Page management 

*/



static uint32_t  gui_proc_language(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t  gui_proc_language(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  static gui_item_t* pListLang;
  static gui_item_t* pSelect;
  static gui_item_t* pTitle;
  
  if(message == GUI_MSG_INIT)
  {
    pItem->page   = PAGE_LANGUAGE;
    pItem->format = DRT_RENDER_ITEM;
    
    /* page title */
    pTitle = gui_add_item_res(GUI_ITEM_LANGUAGE_TITLE, gui_proc_default,"Language selection");
    pTitle->format |= DRT_TS_ITEM;
    pTitle->page   = PAGE_LANGUAGE;
    pTitle->parent =     pItem;
    
    
    /* List Lang*/
    pListLang  = gui_add_item_res(GUI_ITEM_LIST_LANG, gui_proc_listBox,NULL);
    pListLang->format |= DRT_FRAME;
    pListLang->pInstance = &gLangageListInstance;
    pListLang->parent    = pItem;
    pListLang->page   = PAGE_LANGUAGE;
    
    /* button Select */
    pSelect =  gui_add_item_res(GUI_ITEM_APPLY_LANG, gui_proc_default, "Apply");
    pSelect->format |= DRT_FRAME | DRT_TS_ITEM;
    pSelect->page   = PAGE_LANGUAGE;
    pSelect->parent =     pItem;
    
    
    /* Load the current lang */
    uint32_t err = sInstanceFactory.persistentCB(AVS_PERSIST_GET_POINTER,SERVICE_PRIVATE_ITEM_CONFIG,0,0);
    if(err  > AVS_END_RESULT)
    {
      
      device_config_t  *pConfig = (device_config_t*) err;
      if(pConfig->lang[0] != 0xFF)
      {
        strncpy(gCurLang,pConfig->lang, sizeof(gCurLang)-1); 
        gCurLang[sizeof(gCurLang)-1] = '\0';
      }
    }
    pItem->itemCB(pItem,GUI_MSG_SYNC_LISTBOX,0);
    
  }
  
  
  
  
  if(message == GUI_MSG_SYNC_LISTBOX)
  {
    /* we got the station , we should find this station in the list */
    uint32_t index=0;
    uint8_t bFound = FALSE;
    while(index < gLangageListInstance.iListNb)
    {
      if(strncmp(gLangageListInstance.plistItem[index],gCurLang,strlen(gCurLang)) ==0)
      {
        /* we have found the item in the list */
        /* select it */
        bFound = TRUE;
        break;
      }
      index++;
    }
    if(bFound) 
    {
      pListLang->itemCB(pListLang,GUI_MSG_LISTBOX_SET_CUR_SEL,index);
    }
  }
  
  
  if(message == GUI_MSG_RENDER)
  {
    pSelect->txtColor = GUI_RGB(192U,192U,192U); /* disabled */
    pSelect->format  &= ~DRT_TS_ITEM;
    
    if(gConnected) 
    {
      pSelect->txtColor = GUI_COLOR_BLACK;    /* active */
      pSelect->format  |= DRT_TS_ITEM;
    }
  }
  
  if(message == GUI_MSG_BUTTON_CLICK_NOTIFY)
  {
    if((gui_item_t*)lparam == pSelect)
    {
      strcpy(gCurLang,gLangageListInstance.tCurSel);
      service_langage_apply(hInstance,gCurLang);
      uint32_t err = sInstanceFactory.persistentCB(AVS_PERSIST_GET_POINTER,SERVICE_PRIVATE_ITEM_CONFIG,0,0);;
      if(err  > AVS_END_RESULT)
      {
        device_config_t  *pConfig = (device_config_t*) err;
        device_config_t  config = *pConfig;
        strcpy(config.lang,gCurLang);
        sInstanceFactory.persistentCB(AVS_PERSIST_SET,SERVICE_PRIVATE_ITEM_CONFIG,(char_t *)&config,sizeof(config));
      }
    }
  }
  
  
  return 0;
}

gui_item_t*  gui_add_page_language(void);
gui_item_t*  gui_add_page_language(void)
{
  return gui_add_item(0, 0, 1, 1, gui_proc_language,NULL);
}

AVS_Result service_gui_event_page_language_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
AVS_Result service_gui_event_page_language_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  if(evt == EVT_HTTP2_CONNECTED)
  {
    gConnected = pparam;
  }
  
  if(evt == EVT_DIRECTIVE_SYSTEM)
  {
    /* Manage the endpoint change */
    json_t *root = (json_t *)pparam;
    /* Extract information */
    json_t *directive = json_object_get(root, "directive");
    if(directive == 0)  
    {
      return AVS_OK;
    }
    /* "system" key is assumed by EVT_DIRECTIVE_SYSTEM*/
    json_t *header    = json_object_get(directive, "header");
    if(header == 0) 
    {
      return AVS_OK;
    }
    json_t *name    = json_object_get(header, "name");
    if(name == 0)  
    {
      return AVS_OK;
    }
    /* check the API */
    if(strcmp(json_string_value(name),"SetEndpoint")==0)
    {
      /* Extract the new endpoint string */
      json_t *payload    = json_object_get(directive, "payload");
      if(payload == 0)  
      {
        return AVS_OK;
      }
      json_t *endpoint  = json_object_get(payload, "endpoint");
      if(endpoint)
      {
        /* we got it, record it */
        strncpy(gAppState.gEndPoint,json_string_value(endpoint)+strlen("https://"),sizeof(gAppState.gEndPoint));
        if(strcmp(gAppState.gEndPoint,sInstanceFactory.urlEndPoint ) != 0)
        {
          /* store the currend end point in cas of disconnection */
          sInstanceFactory.urlEndPoint = gAppState.gEndPoint;
          /* store it in the persistent for the next reboot */
          uint32_t err = sInstanceFactory.persistentCB(AVS_PERSIST_GET_POINTER,SERVICE_PRIVATE_ITEM_CONFIG,0,0);;
          if(err  > AVS_END_RESULT)
          {
            device_config_t  config = *((device_config_t*) err);
            strcpy(config.endpoint,gAppState.gEndPoint);
            if(config.lang[0] == 0xFF) 
            {
              strcpy(config.lang,"en-US");
              service_langage_apply(hInstance,"en-US");
            }
            sInstanceFactory.persistentCB(AVS_PERSIST_SET,SERVICE_PRIVATE_ITEM_CONFIG,(char_t *)&config,sizeof(config));
            AVS_TRACE_DEBUG("Endpoint change : %s ",gAppState.gEndPoint);
          }
        }
        
      }
    }
  }
  return AVS_OK;
}
