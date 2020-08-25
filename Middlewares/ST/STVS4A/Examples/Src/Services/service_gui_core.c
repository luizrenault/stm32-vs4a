/**
******************************************************************************
* @file    service_gui_core.c
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



static uint32_t   tPageNext[] = {
  PAGE_MAIN,
  PAGE_TEST,
  PAGE_LANGUAGE,
#ifdef AVS_USE_NETWORK_WIFI
  PAGE_WIFI,
#endif
};

static uint32_t             FirstPage = PAGE_TEST;
uint32_t                    selPage = 0;
static TS_StateTypeDef      gts_State;
static uint32_t             gNumPage = 0;
static gui_item_t           tListItem[50];             /* Position entry */
static uint32_t             gItemCount = 0;            /* Position entry count */
static uint32_t             gLastTsDelay = 0;          /* Last touch screen delay */
static task_t *             hTask;
uint32_t                    gAnimationCounter;
uint32_t                    bGuiInitialized;
gui_rect_t                  gScreenClip;               /* Clipping position */


static void      gui_activate_page(uint32_t sel);
static uint32_t  gui_proc_next_page(gui_item_t *pItem, uint32_t message, uint32_t lparam);
static uint32_t  gui_proc_time(gui_item_t *pItem, uint32_t message, uint32_t lparam);
static uint32_t  gui_proc_state(gui_item_t *pItem, uint32_t message, uint32_t lparam);
static uint32_t  gui_proc_ip(gui_item_t *pItem, uint32_t message, uint32_t lparam);
static uint32_t  gui_proc_heart(gui_item_t *pItem, uint32_t message, uint32_t lparam);
static uint32_t  gui_get_text_info(char_t *pText, uint32_t *pInfo, uint32_t *max);
static void      gui_update(void);
uint32_t         gui_check_ts_calibration(void);
uint8_t          gui_get_ts_state(uint32_t *x1, uint32_t *y1);
static int32_t   gui_get_ts_entry_index(gui_rect_t *pRect, uint32_t type);
uint32_t         gui_get_ts_delay(void);
static void      gui_init(void);
void             gui_clear_items(void);
void             gui_render_ts_entry(void);
void             gui_render_pos_entry(void);
void             gui_get_ts_key(void);

static void      service_gui_task(const void *pCookie);
AVS_Result       service_gui_delete(void);


static const uint8_t webdings_Bitmaps[] =
{
	/* @0 '3' (24 pixels wide)*/
	0x00, 0x01, 0x00,   
	0x00, 0x03, 0x00,   
	0x00, 0x07, 0x00,   
	0x00, 0x0F, 0x00,   
	0x00, 0x1F, 0x00,   
	0x00, 0x3F, 0x00,   
	0x00, 0x7F, 0x00,   
	0x00, 0xFF, 0x00,   
	0x01, 0xFF, 0x00,   
	0x03, 0xFF, 0x00,   
	0x07, 0xFF, 0x00,   
	0x0F, 0xFF, 0x00,   
	0x0F, 0xFF, 0x00,   
	0x07, 0xFF, 0x00,   
	0x03, 0xFF, 0x00,   
	0x01, 0xFF, 0x00,   
	0x00, 0xFF, 0x00,   
	0x00, 0x7F, 0x00,   
	0x00, 0x3F, 0x00,   
	0x00, 0x1F, 0x00,   
	0x00, 0x0F, 0x00,   
	0x00, 0x07, 0x00,   
	0x00, 0x03, 0x00,   
	0x00, 0x01, 0x00,   

	/* @72 '4' (24 pixels wide)*/
	0x00, 0x40, 0x00, 
	0x00, 0x60, 0x00, 
	0x00, 0x70, 0x00, 
	0x00, 0x78, 0x00, 
	0x00, 0x7C, 0x00, 
	0x00, 0x7E, 0x00, 
	0x00, 0x7F, 0x00, 
	0x00, 0x7F, 0x80, 
	0x00, 0x7F, 0xC0, 
	0x00, 0x7F, 0xE0, 
	0x00, 0x7F, 0xF0, 
	0x00, 0x7F, 0xF8, 
	0x00, 0x7F, 0xF8, 
	0x00, 0x7F, 0xF0, 
	0x00, 0x7F, 0xE0, 
	0x00, 0x7F, 0xC0, 
	0x00, 0x7F, 0x80, 
	0x00, 0x7F, 0x00, 
	0x00, 0x7E, 0x00, 
	0x00, 0x7C, 0x00, 
	0x00, 0x78, 0x00, 
	0x00, 0x70, 0x00, 
	0x00, 0x60, 0x00, 
	0x00, 0x40, 0x00, 

	/* @144 '5' (24 pixels wide)*/
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x18, 0x00, 
	0x00, 0x3C, 0x00, 
	0x00, 0x7E, 0x00, 
	0x00, 0xFF, 0x00, 
	0x01, 0xFF, 0x80, 
	0x03, 0xFF, 0xC0, 
	0x07, 0xFF, 0xE0, 
	0x0F, 0xFF, 0xF0, 
	0x1F, 0xFF, 0xF8, 
	0x3F, 0xFF, 0xFC, 
	0x7F, 0xFF, 0xFE, 
	0xFF, 0xFF, 0xFF, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 

	/* @216 '6' (24 pixels wide)*/
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0xFF, 0xFF, 0xFF, 
	0x7F, 0xFF, 0xFE, 
	0x3F, 0xFF, 0xFC, 
	0x1F, 0xFF, 0xF8, 
	0x0F, 0xFF, 0xF0, 
	0x07, 0xFF, 0xE0, 
	0x03, 0xFF, 0xC0, 
	0x01, 0xFF, 0x80, 
	0x00, 0xFF, 0x00, 
	0x00, 0x7E, 0x00, 
	0x00, 0x3C, 0x00, 
	0x00, 0x18, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 
};


sFONT   FontWebDings={webdings_Bitmaps,24,24};

/* char set definition */

static char_t* tCharSet[]=
{
  "qwertyuiopasdfghjklzxcvbnm",
  "QWERTYUIOPASDFGHJKLZXCVBNM",
  "1234567890@$&_():;<>!#=/+?.,-'"
};




/* Weak section for plugged UI */
void gui_add_page_wifi(void);
__weak void gui_add_page_wifi(void)
{
}
__weak AVS_Result service_gui_event_page_wifi_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
__weak AVS_Result service_gui_event_page_wifi_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak void gui_add_page_test(void);
__weak void gui_add_page_test(void)
{
}
__weak AVS_Result service_gui_event_page_test_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
__weak AVS_Result service_gui_event_page_test_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
__weak void gui_add_page_info(void);
__weak void gui_add_page_info(void)
{
}
__weak AVS_Result service_gui_event_page_info_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
__weak AVS_Result service_gui_event_page_info_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}

__weak gui_item_t*  gui_add_page_language(void);
__weak gui_item_t*  gui_add_page_language(void)
{
  return 0;
}

__weak AVS_Result service_gui_event_page_language_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
__weak AVS_Result service_gui_event_page_language_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}


/*
Check the limite of  pixel position 
*/
uint32_t gui_check_limit(int32_t base, int32_t base_ref, int32_t dx_ref)
{
  if (base < base_ref)
  {
    return 0;
  }
  if (base > base_ref + dx_ref) 
  {
    return 0;
  }
  return 1;
}

/*
Check  if a rect hits a rect 
*/
uint8_t  gui_hit_test(gui_rect_t *pRef, gui_rect_t *pRect)
{
  int32_t l1 = gui_check_limit(pRef->x, pRect->x, pRect->dx);
  int32_t l2 = gui_check_limit(pRef->x + pRef->dx, pRect->x, pRect->dx) ;
  
  if( ( l1 != 0U) || (l2 != 0U))
  {
    l1 = gui_check_limit(pRef->y, pRect->y, pRect->dy) ;
    l2 = gui_check_limit(pRef->y + pRef->dy, pRect->y, pRect->dy) ;
    if ((l1!= 0) || (l2 != 0))
    {
      return 1;
    }
  }
  return 0;
}



static int32_t findTextEntry(GUI_TEXT_CODE *pList, uint32_t code);
static int32_t findTextEntry(GUI_TEXT_CODE *pList, uint32_t code)
{
  
  for (int32_t a = 0; pList[a].pText; a++)
  {
    if (pList[a].evt_code == code)
    {
      return a;
    }
  }
  return -1;
}


/*
Update the State ctrl according to stavs state
*/
static void gui_evt_proc_change_state(GUI_TEXT_CODE *pItem, uint32_t param);
static void gui_evt_proc_change_state(GUI_TEXT_CODE *pItem, uint32_t param)
{
  static GUI_TEXT_CODE tState[] =
  {
    {"Restart ...", AVS_STATE_RESTART, 0},
    {"Idle ...", AVS_STATE_IDLE, 0},
    {"Busy ...", AVS_STATE_BUSY, 0},
    {"Start Capture", AVS_STATE_START_CAPTURE, 0},
    {"Stop  Capture", AVS_STATE_STOP_CAPTURE, 0},
    {0, 0, 0}
  };
  int32_t index = findTextEntry(tState, param);
  if (index == -1)
  {
    pItem->pText = "Evt unknown";
  }
  else
  {
    pItem->pText = tState[index].pText;
  }
}



/*
List of string events 
*/

GUI_TEXT_CODE   tStateTextList[25] =
{
  {"Booting", EVT_RESTART, 0},
  {"Awaiting an IP.", EVT_WAIT_NETWORK_IP, 0},
  {"Received an IP", EVT_NETWORK_IP_OK, 0},
  {"Connecting...", EVT_CONNECTION_TASK_START, 0},
  {"Awaiting token ....", EVT_WAIT_TOKEN, 0},
  {"Token Valid", EVT_VALID_TOKEN, 0},
  {"Token Renew", EVT_RENEW_ACCESS_TOKEN, 0},
  {"Start connection", EVT_RESET_HTTP, 0},
  {"Resolving Amazon... ", EVT_HOSTNAME_RESOLVED, 0},
  {"Start Downstream", EVT_DOWNSTREAM_TASK_START, 0},
  {"Connected", EVT_HTTP2_CONNECTED, 0},
  {"Dying", EVT_HTTP2_CONNECTED, 0},
  {"Start capture", EVT_START_REC, 0},
  {"Stop capture", EVT_STOP_REC, 0},
  {"Start Speaking", EVT_START_SPEAK, 0},
  {"Stop Speaking", EVT_STOP_SPEAK, 0},
  { "Player starts", EVT_PLAYER_START,0},
  { "Player buffering", EVT_PLAYER_BUFFERING,0},
  { "Player paused", EVT_PLAYER_PAUSED,0},
  { "Player resumed", EVT_PLAYER_RESUMED,0},
  { "Open stream", EVT_PLAYER_OPEN_STREAM,0},
  { "Close stream", EVT_PLAYER_CLOSE_STREAM,0},
  {"Wakeup", EVT_WAKEUP, 0},
  {"", EVT_CHANGE_STATE, gui_evt_proc_change_state},
  {0, 0,0},
};



/*
Check the BMP presents before to draw it in order to prevent crashes when the flash is not flashed ( using stlink-utility)

*/

uint8_t gui_is_bmp_present(uint8_t *pBmp)
{
  if (pBmp[0] != 'B')
  {
    return 0;
  }
  if (pBmp[1] != 'M')
  {
    return 0;
  }
  return 1;
}






/*--------------------------------------------------------------------

Support generic Bnt Map control


---------------------------------------------------------------------*/

/*
Manage button  map click

*/

static int32_t gui_proc_bnt_map_click(gui_item_t *pItem,keyboardBox_t *pMap,uint32_t x,uint32_t y,int32_t offx,int32_t offy, bntBoxItem_t * pBntMap, uint32_t nbBnt);
static int32_t gui_proc_bnt_map_click(gui_item_t *pItem,keyboardBox_t *pMap,uint32_t x,uint32_t y,int32_t offx,int32_t offy, bntBoxItem_t * pBntMap, uint32_t nbBnt)
{
  gui_rect_t r;
  r.x = x;
  r.y= y;
  r.dx = 1;
  r.dy = 1;
  gui_inflat_rect(SIZE_TOUCH_HIT, SIZE_TOUCH_HIT,&r);
  for(uint32_t a = 0; a < nbBnt ; a++)
  {
    struct t_bntBoxItem    *pBnt=&pBntMap[a];
    /* get the rect to hit */
    gui_rect_t tr = pBnt->rect;
    /* update scroll pos */
    tr.x += offx;
    tr.y += offy;
    /* clip in case of hide zones */
    if(gui_clip_rect(&pItem->rect,&tr))
    {
      /* test */
      if(gui_hit_test(&r,&tr ))
      {
        return a;
      }
    }
    
  }
  return -1;
}

/*
Render the edit box with cursor blicking

*/
static uint32_t gui_bnt_edit_box_render(gui_item_t *pItem,keyboardBox_t *pBntMap );
static uint32_t gui_bnt_edit_box_render(gui_item_t *pItem,keyboardBox_t *pBntMap )
{
  if(((gAnimationCounter % 4) == 0))
  {
    uint32_t len =  strlen(pBntMap->tEditBox[1].pString);
    gui_rect_t rect = pBntMap->tEditBox[1].rect;
    rect.x +=  pItem->rect.x;
    rect.y +=  pItem->rect.y;
    
    gui_inflat_rect(INFLAT_BNT_RECT,INFLAT_BNT_RECT,&rect);
    gui_inflat_rect(INFLAT_BNT_TXT_X,INFLAT_BNT_TXT_Y,&rect);
    /* compute the X placement */
    rect.x +=  len * pItem->pFont->Width;
    rect.dx = 15;
    gui_inflat_rect(0,-15,&rect);
    
    
    
    
    BSP_LCD_SetTextColor(pItem->txtColor);
    BSP_LCD_FillRect(rect.x , rect.y,rect.dx, rect.dy);
  }
  
  return 0;
}

/*
Render the list of buttons

*/
static uint32_t gui_bnt_map_render(gui_item_t *pItem,keyboardBox_t *pBntMap,uint32_t offx,uint32_t offy,struct  t_bntBoxItem *pBntDef,uint32_t nbBnt  );
static uint32_t gui_bnt_map_render(gui_item_t *pItem,keyboardBox_t *pBntMap,uint32_t offx,uint32_t offy,struct  t_bntBoxItem *pBntDef,uint32_t nbBnt  )
{
  for(uint32_t a=0; a < nbBnt ; a++)
  {
    struct  t_bntBoxItem *pBnt = &pBntDef[a];
    gui_rect_t rectZone = pBnt->rect;
    rectZone.x += offx+pItem->rect.x;
    rectZone.y += offy+pItem->rect.y;
    gui_rect_t rectBnt =  rectZone;
    gui_inflat_rect(INFLAT_BNT_RECT,INFLAT_BNT_RECT,&rectBnt);
    if(gui_clip_rect(&gScreenClip,&rectBnt) ==0) 
    {
      continue;
    }
    
    
    if((pBnt->flag & BNT_FLAG_SELECTED)==0)
    {
      pItem->txtColor = GUI_COLOR_BLACK;
      pItem->bkColor = GUI_COLOR_WHITE;
      BSP_LCD_SetTextColor(pItem->txtColor);
      BSP_LCD_SetBackColor(pItem->bkColor);
      if((pBnt->flag & BNT_FLAG_NO_FRAME)==0)
      {
        BSP_LCD_DrawRect(rectBnt.x , rectBnt.y,rectBnt.dx, rectBnt.dy);
      }
    }
    else
    {
      rectBnt =  rectZone;
      gui_inflat_rect(-2,-2,&rectBnt);
      if(!gui_clip_rect(&gScreenClip,&rectBnt)) 
      {
        continue;
      }
      
      pItem->txtColor = GUI_COLOR_WHITE;
      pItem->bkColor = GUI_COLOR_BLACK;
      
      BSP_LCD_SetTextColor(pItem->bkColor);
      BSP_LCD_FillRect(rectBnt.x , rectBnt.y,rectBnt.dx, rectBnt.dy);
      BSP_LCD_SetTextColor(pItem->txtColor);
      BSP_LCD_SetBackColor(pItem->bkColor);
    }
    
    gui_rect_t rectText = rectZone;
    gui_inflat_rect(INFLAT_BNT_TXT_X,INFLAT_BNT_TXT_Y,&rectText);
    int32_t s1 = strcmp(pBnt->pString," ");
    int32_t s2 = strcmp(pBnt->pString,"!");
    if((s1==0) || (s2==0))
    {
      BSP_LCD_SetFont(GUI_FONT_WEBDINGS);
    }
    else
    {
      BSP_LCD_SetFont(pItem->pFont);
    }
    
    gui_render_rect_text_entry(pBnt->pString, rectText.x , rectText.y,rectText.dx, rectText.dy, pBnt->flag & 0x3F);
  }
  return 0;
}



/*
Compute the sting buttons according to a charset

*/

void gui_bnt_map_charSet(keyboardBox_t *pBntMap,int32_t charSet )
{
  pBntMap->charSet =  charSet;
  for(uint32_t a = 0 ; a < pBntMap->nbBnt ; a++)
  {
    pBntMap->tMap[a].tKey[0] = tCharSet[pBntMap->charSet][a];
    pBntMap->tMap[a].tKey[1] = 0;
    pBntMap->tMap[a].pString =pBntMap->tMap[a].tKey;
    
    
  }
  
}

/*--------------------------------------------------------------------

Support generic Keyboard control


---------------------------------------------------------------------*/



/*
Compute buttons keyboard placement
*/
static void gui_bnt_map_compute(gui_item_t *pItem,keyboard_instance_t *pInstance);
static void gui_bnt_map_compute(gui_item_t *pItem,keyboard_instance_t *pInstance)
{
  keyboardBox_t *pBntMap   = &pInstance->keyCtrl;
  pBntMap->nbBnt = 0;
  uint32_t szX=KEY_DX;
  uint32_t szY=KEY_DY;
  pBntMap->szDx = KEY_MAP_DX*szX;
  pBntMap->szDy = KEY_MAP_DY*szY;
  
  
  
  for(uint32_t x=0; x < 10 ; x++)
  {
    struct  t_bntBoxItem *pBnt = &pBntMap->tMap[pBntMap->nbBnt];
    pBnt->rect.x =x*szX;
    pBnt->rect.y =0;
    pBnt->rect.dx =szX;
    pBnt->rect.dy =szY;
    pBnt->flag  = DRT_HCENTER | DRT_VCENTER;
    pBntMap->nbBnt++;
  }
  
  for(uint32_t x=0; x < 9 ; x++)
  {
    struct  t_bntBoxItem *pBnt = &pBntMap->tMap[pBntMap->nbBnt];
    pBnt->rect.x =x*szX + szX/2;
    pBnt->rect.y =1*szY;
    pBnt->rect.dx =szX;
    pBnt->rect.dy =szY;
    pBnt->flag  = DRT_HCENTER | DRT_VCENTER;
    pBntMap->nbBnt++;
  }
  
  for(uint32_t x=0; x < 7 ; x++)
  {
    struct  t_bntBoxItem *pBnt = &pBntMap->tMap[pBntMap->nbBnt];
    pBnt->rect.x =x*szX + szX/2 + szX;
    pBnt->rect.y =2*szY;
    pBnt->rect.dx =szX;
    pBnt->rect.dy =szY;
    pBnt->flag  = DRT_HCENTER | DRT_VCENTER;
    pBntMap->nbBnt++;
  }
  
  uint32_t posEscapeY =    3*szY+10;
  
  pBntMap->tEscape[ICHARSET].rect.x =0 ;
  pBntMap->tEscape[ICHARSET].rect.y = posEscapeY;
  pBntMap->tEscape[ICHARSET].rect.dx =150;
  pBntMap->tEscape[ICHARSET].rect.dy =szY;
  pBntMap->tEscape[ICHARSET].flag  = DRT_HCENTER | DRT_VCENTER;
  pBntMap->tEscape[ICHARSET].pString = "charSet";
  
  pBntMap->tEscape[ISPACE].rect.x =150 ;
  pBntMap->tEscape[ISPACE].rect.y =posEscapeY;
  pBntMap->tEscape[ISPACE].rect.dx =pItem->rect.dx-2*150 ;
  pBntMap->tEscape[ISPACE].rect.dy =szY;
  pBntMap->tEscape[ISPACE].flag  = DRT_HCENTER | DRT_VCENTER;
  pBntMap->tEscape[ISPACE].pString = "space";
  
  
  pBntMap->tEscape[IBACK].rect.x =pItem->rect.dx -150 ;
  pBntMap->tEscape[IBACK].rect.y =posEscapeY;
  pBntMap->tEscape[IBACK].rect.dx =150;
  pBntMap->tEscape[IBACK].rect.dy =szY;
  pBntMap->tEscape[IBACK].flag  = DRT_HCENTER | DRT_VCENTER;
  pBntMap->tEscape[IBACK].pString = "Back";
  
  posEscapeY =    pItem->rect.dy-(2*70+10);
  
  pBntMap->tEscape[IDONE].rect.x =pItem->rect.dx/2 -(150/2) ;
  pBntMap->tEscape[IDONE].rect.y =posEscapeY;
  pBntMap->tEscape[IDONE].rect.dx =150;
  pBntMap->tEscape[IDONE].rect.dy =szY;
  pBntMap->tEscape[IDONE].flag  = DRT_HCENTER | DRT_VCENTER;
  pBntMap->tEscape[IDONE].pString = "Done";
  
  
  pBntMap->tEscape[ILEFT].rect.x =0 ;
  pBntMap->tEscape[ILEFT].rect.y =posEscapeY;
  pBntMap->tEscape[ILEFT].rect.dx =(pItem->rect.dx/2) - (150/2) - 5;
  pBntMap->tEscape[ILEFT].rect.dy =szY;
  pBntMap->tEscape[ILEFT].flag  = DRT_HCENTER | DRT_VCENTER;
  pBntMap->tEscape[ILEFT].pString = " ";
  
  
  pBntMap->tEscape[IRIGHT].rect = pBntMap->tEscape[ILEFT].rect;
  pBntMap->tEscape[IRIGHT].rect.x  =  pBntMap->tEscape[IDONE].rect.x +  pBntMap->tEscape[IDONE].rect.dx + 5;
  pBntMap->tEscape[IRIGHT].flag  = DRT_HCENTER | DRT_VCENTER;
  pBntMap->tEscape[IRIGHT].pString = "!";
  
  
  
  
  pBntMap->tEditBox[IEDITLABEL].rect.x =5;
  pBntMap->tEditBox[IEDITLABEL].rect.y =pItem->rect.dy-70;
  pBntMap->tEditBox[IEDITLABEL].rect.dx =150;
  pBntMap->tEditBox[IEDITLABEL].rect.dy =59;
  pBntMap->tEditBox[IEDITLABEL].flag=DRT_HCENTER | BNT_FLAG_NO_FRAME;
  pBntMap->tEditBox[IEDITLABEL].pString =pInstance->pEditLabel;
  
  
  pBntMap->tEditBox[IEDITBOX].rect.x =5+150+10;
  pBntMap->tEditBox[IEDITBOX].rect.y =pItem->rect.dy-70;
  pBntMap->tEditBox[IEDITBOX].rect.dx =pItem->rect.dx-(5+150+10);
  pBntMap->tEditBox[IEDITBOX].rect.dy =59;
  pBntMap->tEditBox[IEDITBOX].flag=DRT_HCENTER;
  pBntMap->tEditBox[IEDITBOX].pString = pInstance->pEditBuffer;
}



/*

Manage a keyboard instance ( ie DialogBox )

*/


uint32_t gui_proc_keyboard(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  static uint32_t iTimeStartMove;
  static uint32_t bNoClick;
  static int32_t baseX;
  static int32_t baseY;
  static int32_t  baseOffx;
  static int32_t  baseOffy;
  keyboard_instance_t  *pInstance = (keyboard_instance_t  *)pItem->pInstance;
  keyboardBox_t *pBntMap   = &pInstance->keyCtrl;
  
  if(message == GUI_MSG_INIT  )
  {
    pItem->page   = PAGE_KEYBOARD;
    pItem->format |= DRT_FRAME;
    pItem->format |= DRT_TS_ITEM;
    pItem->pFont = GUI_FONT_BIG;
    pItem->txtColor = GUI_COLOR_BLACK;
    pItem->bkColor = GUI_COLOR_WHITE;
    gui_bnt_map_compute(pItem,pInstance);
    gui_bnt_map_charSet(&pInstance->keyCtrl,0);
  }
  
  
  if(message == GUI_MSG_KEYBOARD_LF)
  {
    pBntMap->offx = 0;
    return 0;
  }
  if(message == GUI_MSG_KEYBOARD_RG)
  {
    int32_t offx =  10;
    /* check the limit of the scroll x */
    if(pBntMap->szDx  > pItem->rect.dx)
    {
      if((offx != 0)  && (offx > -((int32_t)pBntMap->szDx - (int32_t)pItem->rect.dx)))
      {
        offx   =  -((int32_t)pBntMap->szDx - (int32_t)pItem->rect.dx) ;
      }
    }
    else
    {
      offx = 0;
      
    }
    pBntMap->offx = offx;
    return 0;
  }
  
  
  if(message == GUI_MSG_KEYBOARD_CLICK_EXTENDED)
  {
    if(bNoClick == 0)
    {
      
      if(((pBntMap->lastSel & FLG_KEY_ESCAPE)!= 0)  && ((pBntMap->lastSel & ~FLG_KEY_ESCAPE)==IRIGHT))
      {
        pItem->itemCB(pItem,GUI_MSG_KEYBOARD_RG,0);
        return 0;
      }
      
      if(((pBntMap->lastSel & FLG_KEY_ESCAPE)!=0)  && ((pBntMap->lastSel & ~FLG_KEY_ESCAPE)==ILEFT))
      {
        pItem->itemCB(pItem,GUI_MSG_KEYBOARD_LF,0);
        return 0;
        
      }
      
      
      
      if(((pBntMap->lastSel & FLG_KEY_ESCAPE) != 0) && ((pBntMap->lastSel & ~FLG_KEY_ESCAPE)==ICHARSET))
      {
        int32_t cs = pBntMap->charSet;
        cs++;
        if(cs >= 3) 
        {
          cs = 0;
        }
        gui_bnt_map_charSet(pBntMap,cs);
        return 0;
      }
      
      if(((pBntMap->lastSel & FLG_KEY_ESCAPE)!= 0) && ((pBntMap->lastSel & ~FLG_KEY_ESCAPE)==ISPACE))
      {
        char_t *pString = pInstance->pEditBuffer;
        if(strlen(pString) < pInstance->szEditBuffer)
        {
          strcat(pString," ");
        }
        return 0;
        
      }
      if(((pBntMap->lastSel & FLG_KEY_ESCAPE)!= 0)  && ((pBntMap->lastSel & ~FLG_KEY_ESCAPE)==IBACK))
      {
        char_t *pString = pInstance->pEditBuffer;
        if(strlen(pString) >0)
        {
          pString[strlen(pString)-1]=0;
        }
        return 0;
      }
      
      if(((pBntMap->lastSel & FLG_KEY_ESCAPE)!= 0)  && ((pBntMap->lastSel & ~FLG_KEY_ESCAPE)==IDONE))
      {
        if(pItem->parent) 
        {
          pItem->parent->itemCB(pItem->parent,GUI_MSG_KEYBOARD_DONE,(uint32_t)pItem);
        }
        return 0;
      }
      
      
      if((pBntMap->lastSel & FLG_KEY_ESCAPE)==0)
      {
        char_t *pString = pInstance->pEditBuffer;
        if(strlen(pString) < pInstance->szEditBuffer)
        {
          strcat(pString,pBntMap->tMap[pBntMap->lastSel].pString);
          
        }
        return 0;
      }
      
    }
  }
  
  
  if(message == GUI_MSG_BUTTON_DN)
  {
    /* Save relative info */
    iTimeStartMove = osKernelSysTick();
    bNoClick  = 0;
    baseX = GUI_HIWORD(lparam);
    baseY = GUI_LOWORD(lparam);
    baseOffx = pBntMap->offx;
    baseOffy = pBntMap->offy;
    uint32_t  bKeyDone = FALSE;
    
    uint32_t  key = gui_proc_bnt_map_click(pItem,pBntMap,GUI_HIWORD(lparam),GUI_LOWORD(lparam),pBntMap->offx,pBntMap->offy,pBntMap->tMap,pBntMap->nbBnt );
    if(key != (uint32_t)-1)
    {
      pBntMap->tMap[key].flag |= BNT_FLAG_SELECTED;
      pBntMap->lastSel = key;
      bKeyDone  = TRUE;
    }
    
    key = gui_proc_bnt_map_click(pItem,pBntMap,GUI_HIWORD(lparam),GUI_LOWORD(lparam),0,0,pBntMap->tEscape,MAX_BNT_ESC);
    if((bKeyDone==FALSE) && (key != (uint32_t)-1))
    {
      pBntMap->tEscape[key].flag |= BNT_FLAG_SELECTED;
      pBntMap->lastSel = key | FLG_KEY_ESCAPE;
    }
  }
  
  
  if(message == GUI_MSG_BUTTON_UP)
  {
    /* To allow scrolling , we check the hit when the button is up */
    pItem->itemCB(pItem,GUI_MSG_KEYBOARD_CLICK_EXTENDED,lparam);
    for(uint32_t a = 0 ; a < pBntMap->nbBnt ; a++)
    {
      pBntMap->tMap[a].flag &= ~BNT_FLAG_SELECTED;
    }
    for(uint32_t  a = 0 ; a <  MAX_BNT_ESC; a++)
    {
      pBntMap->tEscape[a].flag &=~BNT_FLAG_SELECTED;
    }
   return 0; 
  }
  if(message == GUI_MSG_BUTTON_MOVE)
  {
    uint32_t f = osKernelSysTick() -  iTimeStartMove ;
    
    if(osKernelSysTick() -  iTimeStartMove > LIMIT_TIME_START_SCROLL)
    {
      bNoClick = 1;
      /*  Get X and Y */
      int32_t x  =  GUI_HIWORD(lparam);
      int32_t y  =  GUI_LOWORD(lparam);
      
      /* compute the relatif offset from the hit pos  */
      int32_t offx   = baseOffx +   ( x-baseX);
      int32_t offy   = baseOffy +    (y-baseY);
      
      /* scroll is always negative */
      if(offy > 0)
      {
        offy = 0;
      }
      if(offx > 0)
      {
        offx = 0;
      }
      
      /* check the limit of the scroll x */
      if(pBntMap->szDx  > pItem->rect.dx)
      {
        if((offx != 0)  && (offx < -((int32_t)pBntMap->szDx - (int32_t)pItem->rect.dx)) )
        {
          offx   =  -((int32_t)pBntMap->szDx - (int32_t)pItem->rect.dx) ;
        }
      }
      else
      {
        offx = 0;
        
      }
      /* check the limit of the scroll y */
      
      if(pBntMap->szDy  > pItem->rect.dy)
      {
        if((offy != 0) && (offy < -((int32_t)pBntMap->szDy - (int32_t)pItem->rect.dy)) )
        {
          offy   =  -((int32_t)pBntMap->szDy - (int32_t)pItem->rect.dy) ;
        }
      }
      else
      {
        offy = 0;
        
      }
      /* Apply and render */
      pBntMap->offx =   offx;
      pBntMap->offy =   offy;
      gui_render();
      return  0;
    }
  }
  if(message == GUI_MSG_RENDER)
  {
    pItem->pFont = GUI_FONT_BIG;
    gui_set_clip(&pItem->rect);
    BSP_LCD_SetFont(pItem->pFont);
    gui_bnt_map_render(pItem,pBntMap,pBntMap->offx,pBntMap->offy,pBntMap->tMap,pBntMap->nbBnt);
    gui_bnt_map_render(pItem,pBntMap,0,0,pBntMap->tEscape,MAX_BNT_ESC);
    gui_bnt_map_render(pItem,pBntMap,0,0,pBntMap->tEditBox,MAX_BNT_EDIT);
    gui_bnt_edit_box_render(pItem,pBntMap);
    return  0;

    
  }
  return gui_proc_default(pItem,message,lparam);
}



/*--------------------------------------------------------------------

Support generic list box control


---------------------------------------------------------------------*/


/*
Ensure the current selection is visible
*/
void gui_listbox_ensure_visible(gui_item_t *pItem,listBox_instance_t *pList)
{
  if((pList->iCurSel >=pList->iListOffset)  && (pList->iCurSel < pList->iListOffset +  LIST_BOX_NBITEM))
  {
    return;
  }
  
  pList->iListOffset = pList->iCurSel;
  if(pList->iListOffset + LIST_BOX_NBITEM > pList->iListNb)
  {
    pList->iListOffset = pList->iListNb -  LIST_BOX_NBITEM;
  }
  if(pList->iListOffset < 0)
  {
    pList->iListOffset = 0;
  }
}
static void gui_listBox_render_bnt(gui_item_t *pItem,listBox_bnt_t  *pbnt);
static void gui_listBox_render_bnt(gui_item_t *pItem,listBox_bnt_t  *pbnt)
{
  if((pbnt->flg & 1U)==0)
  {
    pItem->txtColor = GUI_COLOR_BLACK;
    pItem->bkColor = GUI_COLOR_WHITE;
    BSP_LCD_SetTextColor(pItem->txtColor);
    BSP_LCD_DrawRect(pbnt->pos.x ,pbnt->pos.y, pbnt->pos.dx, pbnt->pos.dy);
  }
  else
  {
    pItem->txtColor = GUI_COLOR_WHITE;
    pItem->bkColor = GUI_COLOR_BLACK;
    BSP_LCD_SetTextColor(pItem->bkColor);
    BSP_LCD_FillRect(pbnt->pos.x ,pbnt->pos.y, pbnt->pos.dx, pbnt->pos.dy);
  }
  BSP_LCD_SetTextColor(pItem->txtColor);
  BSP_LCD_SetBackColor(pItem->bkColor );

  BSP_LCD_SetFont(GUI_FONT_WEBDINGS);

  gui_render_rect_text_entry(pbnt->pText, pbnt->pos.x , pbnt->pos.y, pbnt->pos.dx, pbnt->pos.dy, DRT_HCENTER | DRT_VCENTER);

}


uint32_t gui_proc_listBox(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  static uint32_t iTimeStartMove;
  static uint32_t bNoClick;
  static int32_t baseY;
  static int32_t  baseOffset;
  static int32_t  bNoMove =0;

  
  listBox_instance_t *pInstance = (listBox_instance_t *)pItem->pInstance;
  
  
  if(message == GUI_MSG_INIT)
  {
    pItem->format |= DRT_FRAME;
    pItem->format |= DRT_TS_ITEM;
    pItem->pFont = GUI_FONT_BIG;
    pItem->txtColor = GUI_COLOR_BLACK;
    pItem->bkColor = GUI_COLOR_WHITE;
    if(pInstance->sizeUpDn ==0 ) 
    {
      pInstance->sizeUpDn = 90;
    }

    gui_listbox_ensure_visible(pItem,pInstance);
    strncpy(pInstance->tCurSel,pInstance->plistItem[pInstance->iCurSel],sizeof(pInstance->tCurSel));
    pInstance->rClient =pItem->rect;
    pInstance->rClient .dx -= pInstance->sizeUpDn ;
    pInstance->rBnt[LISTBOX_BNT_UP].pos =pItem->rect;
    pInstance->rBnt[LISTBOX_BNT_UP].pos.x += pItem->rect.dx - pInstance->sizeUpDn ;
    pInstance->rBnt[LISTBOX_BNT_UP].pos.dy /= 2;
    pInstance->rBnt[LISTBOX_BNT_UP].pos.dx = pInstance->sizeUpDn ;
    pInstance->rBnt[LISTBOX_BNT_UP].pText="\"";

    pInstance->rBnt[LISTBOX_BNT_DN].pos =pItem->rect;
    pInstance->rBnt[LISTBOX_BNT_DN].pos.x += pItem->rect.dx - pInstance->sizeUpDn ;
    pInstance->rBnt[LISTBOX_BNT_DN].pos.dy /= 2;
    pInstance->rBnt[LISTBOX_BNT_DN].pos.y += pInstance->rBnt[LISTBOX_BNT_UP].pos.dy;
    pInstance->rBnt[LISTBOX_BNT_DN].pos.dx = pInstance->sizeUpDn ;
    pInstance->rBnt[LISTBOX_BNT_DN].pText="#";

  }
  
  if(message == GUI_MSG_LISTBOX_SET_CUR_SEL)
  {
    pInstance->iCurSel = lparam;
    gui_listbox_ensure_visible(pItem,pInstance);
    if(pItem->parent) 
    {
      pItem->parent->itemCB(pItem,GUI_MSG_LISTBOX_SEL_CHANGED,(uint32_t)pItem);
    }
  }
  
  if(message == GUI_MSG_BUTTON_CLICK)
  {
    if(bNoClick==0 )
    {
      int32_t x = GUI_HIWORD(lparam);
      int32_t y = GUI_LOWORD(lparam);
      uint32_t posY       =  pInstance->rClient.y;
      uint32_t posDy      =     pInstance->rClient.dy / LIST_BOX_NBITEM;
      gui_rect_t r;
      r.x = x;
      r.y= y;
      r.dx = 1;
      r.dy = 1;
      int32_t ix = pInstance->rClient.x;
      uint32_t idx = pInstance->rClient.dx;
      
      gui_inflat_rect(SIZE_TOUCH_HIT, SIZE_TOUCH_HIT,&r);
      uint32_t bExit = 1;
      for(uint32_t a = pInstance->iListOffset;bExit; a++)
      {
        if( posY+posDy >  pInstance->rClient.y+ pInstance->rClient.dy )
        {
          bExit = 0;
          continue;
        }
        gui_rect_t rRef;
        rRef.x = ix;
        rRef.y = posY;
        rRef.dx = idx;
        rRef.dy = posDy;
        
        
        if(gui_hit_test(&r,&rRef))
        {
          pInstance->iCurSel =a;
          strncpy(pInstance->tCurSel,pInstance->plistItem[pInstance->iCurSel],sizeof(pInstance->tCurSel));
          /* reflect the state on the UI */
          gui_render();
          if(pItem->parent) 
          {
            pItem->parent->itemCB(pItem->parent,GUI_MSG_LISTBOX_SEL_CHANGED,(uint32_t)pItem);
          }
          break;
        }
        posY +=    posDy;
      }
    }
  }
  if(message == GUI_MSG_BUTTON_DN)
  {
    iTimeStartMove = osKernelSysTick();
    bNoClick  = 0;
    baseOffset = pInstance->iListOffset;
    baseY = GUI_LOWORD(lparam);
    gui_rect_t r;
    int32_t x = GUI_HIWORD(lparam);
    int32_t y = GUI_LOWORD(lparam);

    r.x = x;
    r.y= y;
    r.dx = 1;
    r.dy = 1;
    gui_inflat_rect(SIZE_TOUCH_HIT, SIZE_TOUCH_HIT,&r);
    pInstance->rBnt[LISTBOX_BNT_UP].flg = 0;
    pInstance->rBnt[LISTBOX_BNT_DN].flg = 0;

    if(gui_hit_test(&r,&pInstance->rBnt[LISTBOX_BNT_UP].pos))
    {
      pInstance->rBnt[LISTBOX_BNT_UP].flg = 1;
      bNoMove = TRUE;
      gui_render();
    }
    if(gui_hit_test(&r,&pInstance->rBnt[LISTBOX_BNT_DN].pos))
    {
      pInstance->rBnt[LISTBOX_BNT_DN].flg = 1;
      bNoMove = TRUE;
      gui_render();
    }

    return 0;

  }
  if(message == GUI_MSG_BUTTON_MOVE)
  {
    uint32_t f = osKernelSysTick() -  iTimeStartMove ;
    int32_t l1 = (osKernelSysTick() -  iTimeStartMove);
    if((bNoMove == FALSE) && (l1 > LIMIT_TIME_START_SCROLL))
    {
      bNoClick = 1;
      int32_t y = GUI_LOWORD(lparam);
      int32_t iDy      =     pInstance->rClient.dy / LIST_BOX_NBITEM/4;
      int32_t offset   =    ((baseY-y)/2)/iDy;
      int32_t baseOff = baseOffset + offset;
      if(baseOff  < 0)
      {
        baseOff = 0;
      }
      if(baseOff+LIST_BOX_NBITEM > pInstance->iListNb)
      {
        baseOff = pInstance->iListNb -   LIST_BOX_NBITEM;
        if(baseOff < 0)
        {
          baseOff  = 0;
        }
      }
      pInstance->iListOffset = baseOff;
      gui_render();
      return 0;

    }
  }
  if(message == GUI_MSG_BUTTON_UP)
  {
    if((pInstance->rBnt[LISTBOX_BNT_DN].flg  & 1) != 0)
    {
      pItem->itemCB(pItem,GUI_MSG_LISTBOX_MOVEDOWN,0);
    }
    if((pInstance->rBnt[LISTBOX_BNT_UP].flg  & 1) != 0)
    {
      pItem->itemCB(pItem,GUI_MSG_LISTBOX_MOVEUP,0);
    }

    bNoMove = FALSE;
    pInstance->rBnt[LISTBOX_BNT_DN].flg = 0;
    pInstance->rBnt[LISTBOX_BNT_UP].flg = 0;
    gui_render();
    return 0;


  }
  if(message == GUI_MSG_LISTBOX_MOVEUP)
  {
    if(pInstance->iCurSel)
    {
      pInstance->iCurSel--;
      gui_listbox_ensure_visible(pItem,pInstance);
    gui_render();
  }
  }
  if(message == GUI_MSG_LISTBOX_MOVEDOWN)
  {
    if(pInstance->iCurSel+1 < pInstance->iListNb )
    {
      pInstance->iCurSel++;
      gui_listbox_ensure_visible(pItem,pInstance);
    gui_render();
  }
  }
  if(message == GUI_MSG_RENDER)
  {
    uint32_t  curIndex = pInstance->iListOffset;
    uint32_t posY   =  pInstance->rClient.y;
    BSP_LCD_SetFont(pItem->pFont);
    uint32_t  posDy      =     pInstance->rClient.dy / LIST_BOX_NBITEM;
    for(uint32_t a = pInstance->iListOffset; curIndex  < pInstance->iListNb  ; a++)
    {
      if(posY+posDy >  pInstance->rClient.y+ pInstance->rClient.dy) 
      {
        break;
      }
      
      if(a != pInstance->iCurSel)
      {
        pItem->txtColor = GUI_COLOR_BLACK;
        pItem->bkColor = GUI_COLOR_WHITE;
        BSP_LCD_SetTextColor(pItem->txtColor);
        BSP_LCD_DrawRect(pInstance->rClient.x + pItem->offX, posY, pInstance->rClient.dx, posDy);
      }
      else
      {
        pItem->txtColor = GUI_COLOR_WHITE;
        pItem->bkColor = GUI_COLOR_BLACK;
        BSP_LCD_SetTextColor(pItem->bkColor);
        BSP_LCD_FillRect(pInstance->rClient.x + pItem->offX, posY, pInstance->rClient.dx, posDy);
      }
      BSP_LCD_SetTextColor(pItem->txtColor);
      BSP_LCD_SetBackColor(pItem->bkColor );
      gui_render_rect_text_entry((char_t *)pInstance->plistItem[a], pInstance->rClient.x + pItem->offX, posY, pInstance->rClient.dx, posDy, DRT_HCENTER | DRT_VCENTER);
      posY +=    posDy;
      curIndex++;
    }
    BSP_LCD_SetTextColor(GUI_COLOR_BLACK);
    BSP_LCD_DrawRect(pInstance->rClient.x , pInstance->rClient.y, pInstance->rClient.dx, pInstance->rClient.dy);
    if((pInstance->sEmptyMsg != 0) && (pInstance->iListNb==0) )
    {
      gui_rect_t rect = pInstance->rClient;
      gui_inflat_rect(-2, -2,&rect );
      gui_render_rect_text_entry((char_t *)pInstance->sEmptyMsg, rect.x ,rect.y, rect.dx, rect.dy, DRT_HCENTER | DRT_VCENTER);
    }

    gui_listBox_render_bnt(pItem,&pInstance->rBnt[LISTBOX_BNT_DN]);
    gui_listBox_render_bnt(pItem,&pInstance->rBnt[LISTBOX_BNT_UP]);
  }
  return 0;
}




/*

Default item proc. manage inherited behaviour

*/

uint32_t gui_proc_default(gui_item_t *pItem, uint32_t message, uint32_t lparam)
{
  if (message == GUI_MSG_RENDER)
  {
    /* render an item */
    gui_draw_item(pItem, pItem->txtColor, pItem->bkColor);
  }
  if (message == GUI_MSG_BUTTON_DN)
  {
    /* manage button down*/
    gui_render_feedback(pItem, 1);
  }
  if (message == GUI_MSG_BUTTON_UP)
  {
    /* manage button up*/
    gui_render_feedback(pItem, 0);
  }
  if (message == GUI_MSG_BUTTON_CLICK)
  {
    /* manage button click*/
    if (pItem->parent) 
    {
      pItem->parent->itemCB(pItem->parent, GUI_MSG_BUTTON_CLICK_NOTIFY, (uint32_t)pItem);
    }
  }
  return 0;
}


/*

This function manages a group of common items only and has no rendering


*/

static uint32_t  gui_proc_group_common(gui_item_t *pItem, uint32_t message, uint32_t lparam);
static uint32_t  gui_proc_group_common(gui_item_t *pItem, uint32_t message, uint32_t lparam)
{
  static gui_item_t* pLogoST;
  if (message == GUI_MSG_INIT)
  {
    if ((sizeof(tPageNext) / sizeof(uint32_t)) > 1)
    {
      gui_add_item_res(GUI_ITEM_NEXT_PAGE_ST, gui_proc_next_page, 0);
    }
    /* Define logos & heart */
    pLogoST = gui_add_item_res(GUI_ITEM_LOGO_ST, gui_proc_default, "logoST_bmp");
    pLogoST->format |= DRT_TS_ITEM | DRT_IMAGE; /* Secret key */
    pLogoST->page = PAGE_MAIN | PAGE_WIFI | PAGE_TEST;
    pLogoST->parent = pItem;
    
    /* define heart */
    gui_add_item_res(GUI_ITEM_HEART, gui_proc_heart, 0);
    
    gui_item_t* pLogoSensory = gui_add_item_res(GUI_ITEM_LOGO_SENSORY, gui_proc_default, "logoSensory_bmp");
    pLogoSensory->format |= DRT_TS_ITEM | DRT_IMAGE; /* Secret key */
    pLogoSensory->page = 0; /* disabled */
    
    /* multi page  state ctrl*/
    gui_add_item_res(GUI_ITEM_IP, gui_proc_ip, 0);
    gui_add_item_res(GUI_ITEM_STATE, gui_proc_state, 0);
    gui_add_item_res(GUI_ITEM_TIME, gui_proc_time, 0);
    
  }
  /* Mange the secret key and call a Debug or test tool at the drv level*/
  if (message == GUI_MSG_BUTTON_CLICK_NOTIFY)
  {
    if (pLogoST == (gui_item_t*)lparam)
    {
      AVS_Ioctl(hInstance, AVS_TEST_IOCTL, 0, 0);
    }
  }
  return     gui_proc_default(pItem, message, lparam);
}


/*
Activate items according to the selection flag
*/
static void gui_activate_page(uint32_t sel)
{
  for (uint32_t a = 0; a < gItemCount; a++)
  {
    if ((tListItem[a].page &  selPage) != 0)
    {
      tListItem[a].itemCB(&tListItem[a], GUI_MSG_PAGE_ACTIVATE, 0);
      
    }
  }
  
}

/*

proc  next page


*/
static uint32_t gui_proc_next_page(gui_item_t *pItem, uint32_t message, uint32_t lparam)
{
  static char_t txtPages[10];
  
  if (message == GUI_MSG_BUTTON_CLICK)
  {
    /* change the selection page */
    gNumPage++;
    if (gNumPage >= sizeof(tPageNext) / sizeof(tPageNext[0]))
    {
      gNumPage = 0;
    }
    selPage = tPageNext[gNumPage];
    gui_activate_page(selPage);
  }
  
  if (message == GUI_MSG_INIT)
  {
    pItem->page = (uint32_t)PAGE_ALL; /* All pages */
    pItem->format |= DRT_TS_ITEM;
    pItem->format |= DRT_FRAME;
  }
  if (message == GUI_MSG_RENDER)
  {
    snprintf(txtPages, sizeof(txtPages), "%lu/%lu", gNumPage + 1, (long unsigned int)(sizeof(tPageNext) / sizeof(uint32_t)));
    pItem->pText = txtPages;
  }
  return gui_proc_default(pItem, message, lparam);
  
}

/*

proc state , render a string according to the global state


*/
static uint32_t  gui_proc_state(gui_item_t *pItem, uint32_t message, uint32_t lparam)
{
  uint32_t index = 0;
  if (message == GUI_MSG_INIT)
  {
    pItem->format |= DRT_FRAME;
    pItem->pText = "No State";
    pItem->page = PAGE_MAIN | PAGE_TEST;
  }
  if (message == GUI_MSG_RENDER)
  {
    while (tStateTextList[index].pText)
    {
      if (tStateTextList[index].evt_code == gAppState.evt)
      {
        if (tStateTextList[index].itemCB)
        {
          tStateTextList[index].itemCB(&tStateTextList[index], gAppState.evt_param);
        }
        pItem->pText = tStateTextList[index].pText;
        break;
      }
      index++;
    }
    
    pItem->txtColor = GUI_COLOR_BLACK;
    pItem->bkColor = GUI_COLOR_WHITE;
  }
  return gui_proc_default(pItem, message, lparam);
}
/*

proc time waits for a sync time and render it


*/
static uint32_t   gui_proc_time(gui_item_t *pItem, uint32_t message, uint32_t lparam)
{
  if (message == GUI_MSG_INIT)
  {
    pItem->format |= DRT_FRAME;
    pItem->page = PAGE_MAIN | PAGE_TEST;
  }
  if (message == GUI_MSG_RENDER)
  {
    if (gAppState.curTime)
    {
      AVS_TIME itime = gAppState.curTime;
      itime += service_alarm_get_time_zone() * 60 * 60;
      struct tm *ptm = localtime((const time_t *)(uint32_t)&itime);
      if (ptm)
      {
        strftime(gAppState.tTimeString, sizeof(gAppState.tTimeString), "%D %T", ptm);
        pItem->pText = gAppState.tTimeString;
      }
    }
    else
    {
      pItem->pText = "No Sync Time";
    }
  }
  return gui_proc_default(pItem, message, lparam);
  
}


/*

Render IP when resolved
*/

static uint32_t gui_proc_ip(gui_item_t *pItem, uint32_t message, uint32_t lparam)
{
  if (message == GUI_MSG_INIT)
  {
    pItem->format |= DRT_FRAME;
    pItem->page = PAGE_MAIN | PAGE_TEST | PAGE_WIFI;
  }
  if (message == GUI_MSG_RENDER)
  {
    if (strncmp(sInstanceFactory.ipV4_host_addr, "xxx", 3) != 0)
    {
      pItem->pText = sInstanceFactory.ipV4_host_addr;
    }
    else
    {
      pItem->pText = "No IP";
    }
  }
  return gui_proc_default(pItem, message, lparam);
}





/* 
Render the heart beating 
*/

static uint32_t gui_proc_heart(gui_item_t *pItem, uint32_t message, uint32_t lparam)
{
  static const void *pImage = 0;
  if (message == GUI_MSG_INIT)
  {
    pItem->page = PAGE_MAIN | PAGE_TEST | PAGE_WIFI | PAGE_LANGUAGE;
    ;  /* PAGE_MAIN | PAGE_DEBUG; all pages */
  }
  if (message == GUI_MSG_RENDER)
  {
    if (pImage == 0)
    {
      pImage = service_assets_load("Heart_bmp", 0, 0);
    }
    int32_t bFound = gui_is_bmp_present((uint8_t *)(uint32_t)pImage);
    
    if (((gAnimationCounter % 3) == 0) && (bFound != 0))
    {
      /*  every  4 update */
      BSP_LCD_DrawBitmap(pItem->rect.x, pItem->rect.y, (uint8_t*)(uint32_t)pImage);
    }
    return 0;
  }
  return gui_proc_default(pItem, message, lparam);
}





/*

Set a screen clipping for draw primitives


*/


void gui_set_clip(gui_rect_t *pRect)
{
  gScreenClip.x = pRect->x;
  gScreenClip.y = pRect->y;
  gScreenClip.dx = pRect->dx;
  gScreenClip.dy = pRect->dy;
}

/*

Restore the full screen clipping


*/


void gui_set_full_clip(void)
{
  gScreenClip.x = 0;
  gScreenClip.y = 0;
  gScreenClip.dx = BSP_LCD_GetXSize();
  gScreenClip.dy = BSP_LCD_GetYSize();
}




/*


Rect helper , inflat or deflat a rect according to its offsets



*/

void gui_inflat_rect(int32_t offx, int32_t offy, gui_rect_t *pRect)
{
  pRect->x -= offx;
  pRect->y -= offy;
  pRect->dx += offx * 2;
  pRect->dy += offy * 2;
  
}


/*


Perform a  rect clipping , update Posx and destination rect according to the global clipping



*/

uint32_t  gui_clip_rect(gui_rect_t *pClip, gui_rect_t *pRect)
{
  int32_t x1 = pRect->x;
  int32_t x2 = pRect->x + pRect->dx;
  int32_t y1 = pRect->y;
  int32_t y2 = pRect->y + pRect->dy;
  
  
  if (pClip->x >= x1)
  {
    x1 = pClip->x;
    
  }
  if (x2 > pClip->x + ((int32_t)pClip->dx))
  {
    x2 = pClip->x + pClip->dx;
    
  }
  
  if (pClip->y >= y1)
  {
    y1 = pClip->y;
    
  }
  if (y2 > pClip->y + ((int32_t)pClip->dy))
  {
    y2 = pClip->y + pClip->dy;
    
  }
  pRect->x = x1;
  pRect->y = y1;
  pRect->dx = x2 - x1;
  pRect->dy = y2 - y1;
  if ((((int32_t)pRect->dx) <= 0) || (((int32_t)pRect->dy) <= 0)) 
  {
    return 0;
  }
  
  
  return 1;
}


/*

Helper compute nbline and nb char by line from a text



*/
static uint32_t gui_get_text_info(char_t *pText, uint32_t *pInfo, uint32_t *max)
{
  int32_t countLine = 0;
  int32_t countChar = 0;
  *max = 0;
  while ((*pText) && (countLine < 10))
  {
    if (*pText == '\r')
    {
      *pInfo = countChar;
      pInfo++;
      countLine++;
      if (*max < countChar)
      {
        *max = countChar;
      }
      countChar = 0;
      
      pText++;
      continue;
    }
    countChar++;
    pText++;
  }
  if ((countChar) && (countLine < 10))
  {
    *pInfo = countChar;
    if (*max < countChar)
    {
      *max = countChar;
    }
    countLine++;
  }
  
  return   countLine;
}

/*

draw a wired rect


*/
void gui_render_rect(gui_rect_t *pRect, uint32_t color)
{
  gui_rect_t rclipped = *pRect;
  BSP_LCD_SetTextColor(color);
  gui_clip_rect(&gScreenClip, &rclipped);
  BSP_LCD_DrawRect(rclipped.x, rclipped.y, rclipped.dx, rclipped.dy);
}

/*


This function allows to render a string in a box according its flags



*/
void     gui_render_rect_text_entry(char_t *pText, uint32_t x, uint32_t y, int32_t  dx, int32_t  dy, uint32_t flag)
{
  uint32_t tInfo[10];
  uint32_t count;
  sFONT  *pFont = BSP_LCD_GetFont();
  uint32_t szX = 0;
  if ((count = gui_get_text_info(pText, tInfo, &szX)) == 0)
  {
    return;
  }
  int32_t szY = count *   pFont->Height;
  int32_t  baseY = 0;
  
  
  
  
  switch (flag & 0x0F)
  {
  case DRT_HUP:
    {
      baseY = y;
      break;
    }
  case DRT_HDN:
    {
      baseY = y + (dy - szY - 1);
      break;
    }
  case DRT_HCENTER:
    {
      baseY = y + (dy - szY) / 2;
      break;
    }
  default:
    break;
    
    
  }
  
  for (int32_t a = 0; a < count; a++)
  {
    int32_t baseX = 0;
    
    switch (flag & 0xF0)
    {
    case DRT_VLF:
      {
        baseX = x;
        break;
      }
    case DRT_VRG:
      {
        int32_t off = dx - (tInfo[a] * pFont->Width);
        baseX = x + (off - 1);
        break;
      }
    case DRT_VCENTER:
      {
        int32_t off = dx - (tInfo[a] * pFont->Width);
        baseX = x + (off / 2);
        break;
      }
    default:
      break;
      
      
    }
    int32_t i = 0;
    
    while ((*pText) && (*pText != '\r'))
    {
      int32_t  px = baseX + i * pFont->Width;
      int32_t  py = baseY;
      if ((px >= gScreenClip.x) && ((px + pFont->Width) <= gScreenClip.x + gScreenClip.dx))
      {
        if ((py >= gScreenClip.y) && ((py + pFont->Height) <= gScreenClip.y + gScreenClip.dy))
        {
          BSP_LCD_DisplayChar(px, py, *pText);
        }
      }
      pText++;
      i++;
    }
    if (*pText == '\r')
    {
      baseY += pFont->Height;
      pText++;
    }
    
  }
  
  gui_set_full_clip();
}

/*
Draw an item 
*/
void gui_draw_item(gui_item_t *pItem, uint32_t txtColor, uint32_t bkColor)
{
  if ((pItem->format & DRT_FRAME) != 0)
  {
    gui_rect_t r = pItem->rect;
    gui_set_full_clip();
    gui_inflat_rect(2, 2, &r);
    gui_render_rect(&r, GUI_COLOR_FRAME);
  }
  
  if (pItem->pText)
  {
    if ((pItem->format & DRT_IMAGE) == 0)
    {
      gui_render_text(pItem, pItem->pText, txtColor, bkColor);
    }
    else
    {
      const void *pImage = service_assets_load(pItem->pText, 0, 0);
      int32_t bFound = gui_is_bmp_present((uint8_t *)(uint32_t)pImage);
      if (bFound != 0)
      {
        BSP_LCD_DrawBitmap(pItem->rect.x, pItem->rect.y, (uint8_t*)(uint32_t)pImage);
      }
    }
  }
}



/*
Render all items

*/
static void gui_update(void)
{
  if (hInstance == 0)
  {
    return;
  }
  
  AVS_Get_Sync_Time(hInstance, &gAppState.curTime);
  if (gAppState.curTime - gAppState.lastEvtTime > MAX_EVT_PRESENTATION_TIME)
  {
    gAppState.lastEvtTime = gAppState.curTime;
    gAppState.evt = EVT_CHANGE_STATE;
    gAppState.evt_param = AVS_Get_State(hInstance);
    
  }
  
  for (int32_t a = 0; a < gItemCount; a++)
  {
    gui_item_t *pItem = &tListItem[a];
    if (((tListItem[a].page & (selPage)) != 0) && ((tListItem[a].format & DRT_RENDER_ITEM) != 0))
    {
      gui_set_clip(&pItem->rect);
      pItem->itemCB(pItem, GUI_MSG_RENDER, 0);
    }
  }
}



/*


full render



*/

void gui_render(void)
{
  BSP_LCD_Clear(GUI_BACKROUND_COLOR);
  gui_update();
  gui_swap_buffer();
  gAnimationCounter++;
}




/*


Check the Touch screen calibration  and run a calibration tool if it is mandatory
for  F7  we just initialize the TS , no calibration mandatory


*/
uint32_t  gui_check_ts_calibration(void)
{
  uint32_t ts_status = TS_OK;
  ts_status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  if (ts_status != TS_OK)
  {
    return 0;
  }
  
  
  return 1;
}
/*
this function must be overloaded in platform_adap.c to get a TS support

*/
__weak void TS_Get_State(TS_StateTypeDef      *pTsState, uint32_t *touchDetected, uint32_t *touchX, uint32_t *touchY);
__weak void TS_Get_State(TS_StateTypeDef      *pTsState, uint32_t *touchDetected, uint32_t *touchX, uint32_t *touchY)
{
  
  AVS_TRACE_ERROR(" The touch screen  HAL adaptation is not overloaded");
}


/*


Check the Touch screen state and position



*/
uint8_t gui_get_ts_state(uint32_t *x1, uint32_t *y1)
{
  
  uint32_t touchDetected, touchX, touchY;
  TS_Get_State(&gts_State, &touchDetected, &touchX, &touchY);
  if (touchDetected)
  {
    /* One or dual touch have been detected          */
    /* Only take into account the first touch so far */
    
    /* Get X and Y position of the first touch post calibrated */
    if (x1)
    {
      *x1 = touchX;
    }
    if (y1)
    {
      *y1 = touchY;
    }
    return 1;
  }
  return 0;
}


/*


Find an entry in the TS list and returns its index



*/


static int32_t gui_get_ts_entry_index(gui_rect_t *pRect, uint32_t type)
{
  for (int32_t a = 0; a < gItemCount; a++)
  {
    if (((tListItem[a].format & type) != 0) && ((tListItem[a].page & (selPage)) != 0))
    {
      gui_item_t *pItem = &tListItem[a];
      if (gui_hit_test(pRect, &pItem->rect))
      {
        return a;
      }
    }
  }
  
  return -1;
  
}



/*


Add an entry in the item list
the idea is to named boxes ( ie POS_VERSION) and attach a callback
the callback will be called before each render
the callback is in charge to modify the text or the style according to its info

*/

gui_item_t * gui_add_item(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, itemProc procCB, char_t *pText)
{
  
  if (gItemCount >= sizeof(tListItem) / sizeof(tListItem[0]))
  {
    return NULL;
  }
  gui_item_t *pItem = &tListItem[gItemCount];
  memset(pItem, 0, sizeof(*pItem));
  
  pItem->format = DRT_HCENTER | DRT_VCENTER | DRT_RENDER_ITEM;
  pItem->pFont = GUI_FONT_BIG;
  pItem->txtColor = GUI_COLOR_BLACK;
  pItem->bkColor = GUI_COLOR_WHITE;
  
  pItem->offX = 0;
  pItem->rect.x = x;
  pItem->rect.y = y;
  pItem->rect.dx = dx;
  pItem->rect.dy = dy;
  pItem->itemCB = procCB;
  pItem->pText = pText;
  pItem->page = PAGE_MAIN;
  gItemCount++;
  return pItem;
}

gui_item_t * gui_add_item_res(gui_item_id_t id, itemProc procCB, char_t *pText)
{
  gui_resource_t *pRes = gui_get_res(id);
  AVS_ASSERT(pRes);
  gui_item_t *pItem = gui_add_item(pRes->rect.x, pRes->rect.y, pRes->rect.dx, pRes->rect.dy, procCB, pText);
  if (pRes->pFont)
  {
    pItem->pFont = pRes->pFont;
  }
  
  return pItem;
}


/*


Render the feedback when the TS is pressed



*/
void   gui_render_feedback(gui_item_t *pItem, uint32_t draw)
{
  if (draw)
  {
    uint32_t bkc = pItem->bkColor;
    uint32_t txc = pItem->txtColor;
    pItem->bkColor = GUI_COLOR_BLACK;
    pItem->txtColor = GUI_COLOR_WHITE;
    gui_render();
    pItem->bkColor = bkc;
    pItem->txtColor = txc;
  }
  else
  {
    gui_render();
  }
}



/*


returns the last delay of the TS



*/
uint32_t gui_get_ts_delay(void)
{
  return gLastTsDelay;
}




/*


check and manage the touch screen , return index



*/
void gui_get_ts_key(void);
void gui_get_ts_key(void)
{
  uint32_t x, y;
  
  
  if (!gui_get_ts_state(&x, &y))
  {
    return;
  }
  gui_rect_t r;
  r.x = x;
  r.y = y;
  r.dx = 1;
  r.dy = 1;
  gui_inflat_rect(SIZE_TOUCH_HIT, SIZE_TOUCH_HIT, &r);
  int32_t index = gui_get_ts_entry_index(&r, DRT_TS_ITEM);
  if (index == -1)
  {
    return;
  }
  gui_item_t *pItem = &tListItem[index];
  pItem->itemCB(pItem, GUI_MSG_BUTTON_DN, MAKE_LPARAM(x, y));
  pItem->itemCB(pItem, GUI_MSG_BUTTON_CLICK, MAKE_LPARAM(x, y));
  uint32_t tickstart = osKernelSysTick();
  while (gui_get_ts_state(&x, &y))
  {
#ifdef SHOW_TOUCH
    r.x = x;
    r.y = y;
    r.dx = 1;
    r.dy = 1;
    gui_inflat_rect(SIZE_SHOW_TOUCH_HIT, SIZE_SHOW_TOUCH_HIT, &r);
    BSP_LCD_SetTextColor(GUI_COLOR_BLACK);
    BSP_LCD_DrawRect(r.x, r.y, r.dx, r.dy);
    gui_swap_buffer();
#endif
    pItem->itemCB(pItem, GUI_MSG_BUTTON_MOVE, MAKE_LPARAM(x, y));
    osDelay(10);
  }
  gLastTsDelay = osKernelSysTick() - tickstart;
  pItem->itemCB(pItem, GUI_MSG_BUTTON_UP, MAKE_LPARAM(x, y));
}




/*


render text position only ( for debug )



*/
void gui_render_pos_entry(void)
{
  for (int32_t a = 0; a < gItemCount; a++)
  {
    if ((tListItem[a].format & DRT_RENDER_ITEM) != 0)
    {
      BSP_LCD_SetTextColor(GUI_COLOR_TS);
      BSP_LCD_DrawRect(tListItem[a].rect.x, tListItem[a].rect.y, tListItem[a].rect.dx, tListItem[a].rect.dy);
    }
  }
}


/*


render touch screen   position only  ( for debug)



*/
void gui_render_ts_entry(void)
{
  for (int32_t a = 0; a < gItemCount; a++)
  {
    if ((tListItem[a].format & DRT_TS_ITEM) != 0)
    {
      BSP_LCD_SetTextColor(GUI_COLOR_TS);
      BSP_LCD_DrawRect(tListItem[a].rect.x, tListItem[a].rect.y, tListItem[a].rect.dx, tListItem[a].rect.dy);
    }
  }
}


/*


RAZ Position TS



*/
void gui_clear_items(void)
{
  /* Initialize default values */
  memset(tListItem, 0, sizeof(tListItem));
  for (int32_t a = 0; a < sizeof(tListItem) / sizeof(tListItem[0]); a++)
  {
    tListItem[a].format = DRT_HCENTER | DRT_VCENTER | DRT_RENDER_ITEM;
    tListItem[a].pFont = GUI_FONT_BIG;
    tListItem[a].txtColor = GUI_COLOR_BLACK;
    tListItem[a].bkColor = GUI_COLOR_WHITE;
  }
  gItemCount = 0;
}


void gui_render_text(gui_item_t *pItem, char_t *pText, uint32_t txtColor, uint32_t bkColor)
{
  if (pItem)
  {
    BSP_LCD_SetFont(pItem->pFont);
    BSP_LCD_SetTextColor(pItem->bkColor);
    BSP_LCD_FillRect(pItem->rect.x, pItem->rect.y, pItem->rect.dx, pItem->rect.dy);
    BSP_LCD_SetTextColor(pItem->txtColor);
    BSP_LCD_SetBackColor(pItem->bkColor);
    gui_render_rect_text_entry((char_t *)pText, pItem->rect.x + pItem->offX, pItem->rect.y, pItem->rect.dx, pItem->rect.dy, pItem->format);
  }
}


void gui_swap_buffer(void)
{
  uint32_t writeBuff = LCD_SwapBuffer();
  LCD_WaitVSync();
  LCD_SetFBStarAdress(0, writeBuff);
  
}



/*


Initialize the GUI LCD etc...
We assumes the Initialize is done in the platform Initialize


*/
static void gui_init(void)
{
  /* Initialize 1 layer */
  BSP_LCD_Clear(GUI_BACKROUND_COLOR);
  LCD_SetFBStarAdress(0U,LCD_FB_START_ADDRESS);
  BSP_LCD_Clear(GUI_COLOR_WHITE);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_SetTextColor(GUI_COLOR_BLACK);
  BSP_LCD_SetBackColor(GUI_COLOR_WHITE);
  BSP_LCD_DisplayStringAt(0, (uint16_t)BSP_LCD_GetYSize() / 2, (uint8_t *)(uint32_t) "STVS4A Booting...", CENTER_MODE);
  gui_swap_buffer();
  gui_check_ts_calibration();
  gui_set_full_clip();
  
  
  /* Init the item control positions */
  gui_clear_items();
  gNumPage = 1;
  
  
  gui_add_item(0, 0, 1, 1, gui_proc_group_common, NULL);
  
  gui_add_page_wifi();
  gui_add_page_test();
  gui_add_page_info();
  gui_add_page_language();
  
  /* Init controls */
  for (uint32_t a = 0; a < gItemCount; a++)
  {
    tListItem[a].itemCB(&tListItem[a], GUI_MSG_INIT, 0);
  }
  
  /* Select first page */
  uint32_t iPage;
  for (iPage = 0; iPage < sizeof(tPageNext) / sizeof(tPageNext[0]); iPage++)
  {
    if (tPageNext[iPage] == FirstPage)
    {
      gNumPage = iPage;
      break;
    }
  }
  
  selPage = tPageNext[iPage];
  gui_activate_page(selPage);
  
  
  
  
  bGuiInitialized = 1;
}

/* Idle task that render periodically the UI */
static void service_gui_task(const void *pCookie)
{
  char_t tMessage[100];
  
  while (1)
  {
    if (hInstance != 0)
    {
      break;
    }
    osDelay(500);
  }
  
  AVS_Sys_info sysinfo;
  AVS_Get_Sys_Info(hInstance, &sysinfo);
  
  snprintf(tMessage, sizeof(tMessage), "Mem  DTCM    %03lu KB -> %-3.1f%%", sysinfo.memDtcmFreeSpace / 1024, 100.0F * sysinfo.memDtcmFreeSpace / sysinfo.memDtcmTotalSpace);
  AVS_Send_Evt(hInstance, EVT_ENDURANCE_MSG, (uint32_t)tMessage);
  snprintf(tMessage, sizeof(tMessage), "Mem  PRAM    %03lu KB -> %-3.1f%%", sysinfo.memPRamFreeSpace / 1024, 100.0F * sysinfo.memPRamFreeSpace / sysinfo.memPRamTotalSpace);
  AVS_Send_Evt(hInstance, EVT_ENDURANCE_MSG, (uint32_t)tMessage);
  snprintf(tMessage, sizeof(tMessage), "Mem  HEAP    %03lu KB -> %-3.1f%%", sysinfo.memFreeSpace / 1024, 100.0F * sysinfo.memFreeSpace / sysinfo.memTotalSpace);
  AVS_Send_Evt(hInstance, EVT_ENDURANCE_MSG, (uint32_t)tMessage);
  
  
  
  if (strstr(sInstanceFactory.portingAudioName, "afe") != 0)
  {
    gNumPage = 1;
  }
  
  while (1)
  {
    gui_render();
    gui_get_ts_key();
    osDelay(GUI_REFRESH_IDLE_PERIOD);
  }
}




/*

create a GUI service.. Init the LCD using the BSP , create a thread with an idle priority in order to make the GUI less intrusif



*/

AVS_Result service_gui_create(void)
{
  
  gui_init();
  
  hTask = task_Create(TASK_NAME_GUI, service_gui_task, NULL, TASK_STACK_GUI, TASK_NAME_PRIORITY_GUI);
  if (hTask == 0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_GUI);
    return AVS_ERROR;
  }
  return AVS_OK;
}



AVS_Result service_gui_delete(void)
{
  
  task_Delete(hTask);
  return AVS_OK;
}




/*

catch all events coming from AVS, and config the GUI idle to render appropriate info



*/

AVS_Result service_gui_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  service_gui_event_page_wifi_cb(handle, pCookie, evt, pparam);
  service_gui_event_page_test_cb(handle, pCookie, evt, pparam);
  service_gui_event_page_info_cb(handle, pCookie, evt, pparam);
  service_gui_event_page_language_cb(handle, pCookie, evt, pparam);
  
  int32_t index = findTextEntry(tStateTextList, evt);
  if (index != -1)
  {
    
    /* PLAYER_PERIOD_STOP is sent too often */
    if (evt != ((AVS_Event)EVT_PLAYER_STOPPED))
    {
      gAppState.evt = evt;
      gAppState.evt_param = pparam;
      AVS_Get_Sync_Time(handle, &gAppState.lastEvtTime);
    }
  }
  return AVS_OK;
}


