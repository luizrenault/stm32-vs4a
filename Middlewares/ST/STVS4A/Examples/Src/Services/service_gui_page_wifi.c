/**
******************************************************************************
* @file    service_gui_wifi.c
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


#include "service.h"
#ifdef AVS_USE_NETWORK_WIFI


#define GUI_PRIVATE_MSG_WIFI_CONNECTED          (GUI_MSG_USER+1)
#define GUI_PRIVATE_MSG_LISTBOX                 (GUI_MSG_USER+3)
#define GUI_PRIVATE_MSG_WIFI_SCAN_RESULT        (GUI_MSG_USER+4)

static gui_item_t* gFlashPage=0;
static gui_item_t* gMainPage=0;
__weak gui_item_t*   gui_add_page_wifi_flash_esp(void);
__weak gui_item_t*   gui_add_page_wifi_flash_esp(void)
{
  return NULL;
}


/*

fill keyboard instance

*/


static char_t                    *tlistWifiSSID[MAX_WIFI_SSID];
static char_t                     tPassEditWifi[MAX_PASS_NAME_STRING]="Password";
static char_t                     tSSIDEditWifi[MAX_SSID_NAME_STRING]="Login";

static keyboard_instance_t        gWifiKeyboardInstance={{0},tPassEditWifi,MAX_PASS_NAME_STRING,"Password"};
static listBox_instance_t         gWifiListSSIDInstance={0,0,0,tlistWifiSSID,"Please Scan",100};
static uint32_t                   gSzScanResult=2000;
static char_t *                   gScanResult = 0;

static void gui_get_quoted_string(char_t quote, char_t *pQuote, char_t *pString, uint32_t maxSize);
static void gui_get_quoted_string(char_t quote, char_t *pQuote, char_t *pString, uint32_t maxSize)
{
  while (*pQuote != quote)
  {
    pQuote++;
  }
  pQuote++;
  while (maxSize>1)
  {
    if (*pQuote == quote)
    {
      break;
    }
    *pString++ = *pQuote++;
  }
  *pString = 0;
  
}

static uint32_t gui_proc_pass_quoted(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t gui_proc_pass_quoted(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  if (message == GUI_MSG_RENDER)
  {
    char_t tText[100];
    if ((pItem->format & DRT_FRAME) != 0)
    {
      gui_rect_t r = pItem->rect;
      gui_set_full_clip();
      gui_inflat_rect(2, 2, &r);
      gui_render_rect(&r, GUI_COLOR_FRAME);
    }
    
    snprintf(tText,sizeof(tText),"'%s'",pItem->pText);
    gui_render_text(pItem, tText, pItem->txtColor, pItem->bkColor);
    return 0;
  }
  
  return gui_proc_default(pItem,message,lparam);
}



/*

This function manages a group of items only and has no rendering


*/

static uint32_t  gui_proc_wifi_main(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t  gui_proc_wifi_main(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  static gui_item_t* pListSSID;
  static gui_item_t* pButtonSSID;
  static gui_item_t* pButtonPass;
  static gui_item_t* pConnect;
  static gui_item_t* pScan;
  static gui_item_t* pFlash;
  static gui_item_t* pWifiState;
  
  if(message == GUI_MSG_INIT)
  {
    /* this item is hiden and just allow to manager sub item*/
    pItem->page   = PAGE_WIFI;
    pItem->format = 0;
    
    /* List wifi SSID*/
    pListSSID  = gui_add_item_res(GUI_ITEM_WIFI_LIST_SSID, gui_proc_listBox,NULL);
    pListSSID->format |= DRT_FRAME;
    pListSSID->pInstance = &gWifiListSSIDInstance;
    pListSSID->parent    = pItem;
    pListSSID->page   = PAGE_WIFI;
    
    
    /* static message */
    gui_add_item_res(GUI_ITEM_WIFI_SSID_NAME_STATIC, gui_proc_default,"Ssid")->page = PAGE_WIFI;
    gui_add_item_res(GUI_ITEM_WIFI_SSID_PASS_STATIC, gui_proc_default,"Pass")->page = PAGE_WIFI;
    
    pButtonSSID = gui_add_item_res(GUI_ITEM_WIFI_SSID_NAME, gui_proc_default,tSSIDEditWifi);
    pButtonSSID->page    = PAGE_WIFI;
    pButtonSSID->format |= DRT_FRAME;
    pButtonSSID->parent  = pItem;
    
    
    /* button pass */
    pButtonPass = gui_add_item_res(GUI_ITEM_WIFI_SSID_PASS, gui_proc_pass_quoted,tPassEditWifi);
    pButtonPass->format |= DRT_FRAME | DRT_TS_ITEM;
    pButtonPass->page   = PAGE_WIFI;
    pButtonPass->parent  = pItem;
    
    
    /* button connect */
    pConnect =  gui_add_item_res(GUI_ITEM_WIFI_RECONNECT, gui_proc_default, "Re-Connect");
    pConnect->format |= DRT_FRAME | DRT_TS_ITEM;
    pConnect->page   = PAGE_WIFI;
    pConnect->parent =     pItem;
    
    
    /* button scan  */
    pScan =  gui_add_item_res(GUI_ITEM_WIFI_SCAN, gui_proc_default, "Wifi Scan");
    pScan->format |= DRT_FRAME | DRT_TS_ITEM;
    pScan->page   = PAGE_WIFI;
    pScan->parent =     pItem;
    if(gFlashPage)
    {
      /* button flash */
      pFlash =  gui_add_item_res(GUI_ITEM_WIFI_FLASH_ESP, gui_proc_default, "Flash esp");
      pFlash->format |= DRT_FRAME | DRT_TS_ITEM;
      pFlash->parent =     pItem;
      pFlash->page    = PAGE_WIFI;
      
    }
    pWifiState = gui_add_item_res(GUI_ITEM_WIFI_CONNECT_STATUS, gui_proc_default, "Disconnected");
    pWifiState->parent =     pItem;
    pWifiState->page   = PAGE_WIFI;
    
    
    pFlash->page   = PAGE_WIFI;
    /* keyboard */
    gui_item_t*pKeyboard = gui_add_item(5, 15, 800-10, 480-5, gui_proc_keyboard,NULL);
    
    pKeyboard->pInstance = &gWifiKeyboardInstance;
    pKeyboard->parent    = pItem;
  }
  
  
  if(message == GUI_MSG_PAGE_ACTIVATE)
  {
    /* Init edit box with peristant values */
    memset(tSSIDEditWifi,0,sizeof(tSSIDEditWifi));
    memset(tPassEditWifi,0,sizeof(tPassEditWifi));
    
    uint32_t ctx = sInstanceFactory.persistentCB(AVS_PERSIST_GET_POINTER,AVS_PERSIST_ITEM_WIFI,0,0);
    if(ctx > AVS_END_RESULT)
    {
      char_t  *pPersist = (char_t  *) ctx;
      char_t *pKey = strstr(pPersist, "ssid:");
      if (pKey)
      {
        gui_get_quoted_string('\'', pKey +strlen("ssid:"), tSSIDEditWifi, sizeof(tSSIDEditWifi));
      }
      pKey = strstr(pPersist, "pass:");
      if (pKey)
      {
        gui_get_quoted_string('\'', pKey + strlen("pass:"), tPassEditWifi, sizeof(tPassEditWifi));
      }
      pItem->itemCB(pItem,GUI_PRIVATE_MSG_LISTBOX,0);
    }
    pItem->itemCB(pItem,GUI_PRIVATE_MSG_LISTBOX,0);
  }
  
  
  if(message == GUI_MSG_LISTBOX_SEL_CHANGED)
  {
    char_t *pSelSSID = gWifiListSSIDInstance.plistItem[gWifiListSSIDInstance.iCurSel];
    char_t *pEdit = tSSIDEditWifi;
    uint32_t index = 0;
    while((*pSelSSID!= 0)  &&  (*pSelSSID != ':')  && (index < MAX_SSID_NAME_STRING))
    {
      *pEdit++ = *pSelSSID++;
      index++;
    }
    *pEdit = 0;
  }
  if(message ==GUI_PRIVATE_MSG_WIFI_CONNECTED)
  {
    if(lparam == FALSE)
    {
      pWifiState->pText =  "Disconnected";
    }
    else
    {
      pWifiState->pText =  "Connected";
    }
    
  }
  if(message == GUI_MSG_KEYBOARD_DONE)
  {
    selPage =PAGE_WIFI;
  }
  
  if(message == GUI_PRIVATE_MSG_LISTBOX)
  {
    /* we got the station , we should find this station in the list */
    uint32_t index=0;
    uint8_t bFound = FALSE;
    while(index < gWifiListSSIDInstance.iListNb)
    {
      if(strncmp(gWifiListSSIDInstance.plistItem[index],tSSIDEditWifi,strlen(tSSIDEditWifi)) ==0)
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
      pListSSID->itemCB(pListSSID,GUI_MSG_LISTBOX_SET_CUR_SEL,index);
    }
  }
  
  if(message == GUI_PRIVATE_MSG_WIFI_SCAN_RESULT)
  {
    /* free the previous scan */
    if(gScanResult != 0)
    {
      vPortFree(gScanResult);
    }
    gWifiListSSIDInstance.iListNb = 0;
    
    if(lparam)
    {
      char_t *pScanResult = (char_t *) lparam;
      gSzScanResult = strlen(pScanResult)+1;
      /* Alloc the Scan container */
      gScanResult = pvPortMalloc(gSzScanResult);
      if(gScanResult)
      {
        strcpy(gScanResult,pScanResult);
        /* No error, build the listbox */
        /* each lines with /r  becomes a string terminated by /0 and fill the list */
        char_t *pString =  gScanResult;
        while(*pString)
        {
          if(gWifiListSSIDInstance.iListNb >= MAX_WIFI_SSID) 
          {
            break;
          }
          gWifiListSSIDInstance.plistItem[gWifiListSSIDInstance.iListNb] =pString;
          while((*pString != 0)  && (*pString != '\r')) 
          {
            pString ++;
          }
          if(*pString) 
          {
            *pString++ = 0;
          }
          gWifiListSSIDInstance.iListNb++;
        }
        /* this list is updated by we need to sync the current ssid that macth the list box index */
        pItem->itemCB(pItem,GUI_PRIVATE_MSG_LISTBOX,0);
        
      }
    }
  }  
  
  
  if(message == GUI_PRIVATE_MSG_LISTBOX)
  {
    /* we got the station , we should find this station in the list */
    uint32_t index=0;
    uint8_t bFound = FALSE;
    while(index < gWifiListSSIDInstance.iListNb)
    {
      if(strncmp(gWifiListSSIDInstance.plistItem[index],tSSIDEditWifi,strlen(tSSIDEditWifi)) ==0)
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
      pListSSID->itemCB(pListSSID,GUI_MSG_LISTBOX_SET_CUR_SEL,index);
    }
  }  
  if(message == GUI_MSG_BUTTON_CLICK_NOTIFY)
  {
    if((gFlashPage!= 0)  && ((gui_item_t*)lparam == pFlash))
    {
      selPage =PAGE_WIFI_FLASH_ESP;
      if(gFlashPage)       
      {
        gFlashPage->itemCB(gFlashPage,GUI_MSG_PAGE_ACTIVATE,0);
      }
    }
    
    /* Click on scan */
    if((gui_item_t*)lparam == pScan)
    {
      AVS_Ioctl(hInstance,AVS_IOCTL_WIFI_POST_SCAN,0,0);
    }
    
    /* Click on Pass to open the keyboard */
    if((gui_item_t*)lparam == pButtonPass)
    {
      /* just show an hidden page */
      selPage =PAGE_KEYBOARD;
    }
    /* click on connect wifi*/
    if((gui_item_t*)lparam == pConnect)
    {
      /* Save it in the persitant */
      char_t  tPersist[100];
      snprintf(tPersist,sizeof(tPersist),"ssid:'%s' pass:'%s'", tSSIDEditWifi,tPassEditWifi);
      if(sInstanceFactory.persistentCB(AVS_PERSIST_SET,AVS_PERSIST_ITEM_WIFI,tPersist,strlen(tPersist)+1) == AVS_OK)
      {
        /* now reconnect the station */
        if(AVS_Ioctl(hInstance,AVS_IOCTL_WIFI_CONNECT,TRUE,0) != AVS_OK)
        {
          AVS_TRACE_ERROR("Can't reconnect the wifi using %s:%s",tSSIDEditWifi,tPassEditWifi);
        }
      }
    }
    
  }
  
  return 0;
}

/*
check the station connection state 
*/
AVS_Result service_gui_event_page_wifi_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
AVS_Result service_gui_event_page_wifi_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  switch(evt)
  {
  case EVT_WIFI_CONNECTED:
    gMainPage->itemCB(gMainPage,GUI_PRIVATE_MSG_WIFI_CONNECTED,pparam);
    break;
    
    
  case EVT_WIFI_SCAN_RESULT:
    gMainPage->itemCB(gMainPage,GUI_PRIVATE_MSG_WIFI_SCAN_RESULT,pparam);
    break;
    
  default:
    break;
  }
  return AVS_OK;
}




/*

Wifi pages initialization ( weak function )

*/
void gui_add_page_wifi(void);
void gui_add_page_wifi(void)
{
  
  gFlashPage = gui_add_page_wifi_flash_esp();
  gMainPage = gui_add_item(0, 0, 1, 1, gui_proc_wifi_main,NULL);
  
  
}
#endif
