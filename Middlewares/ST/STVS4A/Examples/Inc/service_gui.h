/**
******************************************************************************
* @file    service_gui.h
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
* This software component is licensed by ST under Ultimate Liberty license
* SLA0044, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0044
*
******************************************************************************
*/
#ifndef _service_gui_
#define _service_gui_
void      LCD_Swap_buffer_Init(void);
uint32_t  LCD_SwapBuffer(void);
void      LCD_WaitVSync(void);
void      LCD_Vsync_Init(void);

#define NO_SHOW_TOUCH 

/* LCD frame buffers */
#define MAX_EVT_PRESENTATION_TIME       3

#define PAGE_ALL           (PAGE_MAIN |PAGE_TEST |  PAGE_WIFI | PAGE_LANGUAGE)
#define PAGE_MAIN           1U               /* Main page */
#define PAGE_TEST           2U               /* test  page */
#define PAGE_WIFI           8U
#define PAGE_KEYBOARD       16U
#define PAGE_WIFI_FLASH_ESP 32U
#define PAGE_LANGUAGE       64U


#define TASK_NAME_GUI               "AVS:Task GUI"
#define TASK_STACK_GUI               650
#define TASK_NAME_PRIORITY_GUI      (osPriorityIdle)
#define GUI_REFRESH_IDLE_PERIOD     (300)             /* An update speed */

#ifdef  GUI_16BITS

#define GUI_RGB(r,g,b) (uint16_t) ( ((((r>>3U) << (5U+6U))& 0xF800U)) | ((((g>>2U) << (5U))& 0x07E0U)) |  (((b>>3U)& 0x001FU)))
#define GUI_GetRValue(color) (uint8_t) ((((color & 0xF800U) >> (5U+6U)) *255U)/32U)
#define GUI_GetGValue(color) (uint8_t) ((((color & 0x07E0U) >> (5U)) *255U)/64U)
#define GUI_GetBValue(color) (uint8_t) ((((color & 0x001FU) >> (0U)) *255U)/32U)

#else

#define GUI_RGB(r,g,b) (uint32_t) (0xFF000000U |  ((r<<(2*8U)) & (uint32_t)0xFF0000U)  |  ((g<<(1*8U)) & (uint32_t)0xFF00U)  |  ((b<<(0*8U)) & (uint32_t)0xFFU))
#define GUI_GetRValue(color) (uint8_t)  ((color >> (2*8U)) & 0xFFU )
#define GUI_GetGValue(color) (uint8_t)  ((color >> (1*8U)) & 0xFFU )
#define GUI_GetBValue(color) (uint8_t)  ((color >> (0*8U)) & 0xFFU )

#endif

#define GUI_COLOR_WHITE         GUI_RGB(255,255,255)
#define GUI_COLOR_BLACK         GUI_RGB(0U,0U,0U)
#define GUI_COLOR_RED           GUI_RGB(255U,0U,0U)

#define GUI_COLOR_TS            GUI_RGB(0,0,0)

#define GUI_FONT_BIG             (&Font24)
#define GUI_FONT_MEDIUM          (&Font16)
#define GUI_FONT_SMALL           (&Font12)
#define GUI_FONT_VERY_SMALL      (&Font8)
#define GUI_FONT_WEBDINGS        (&FontWebDings)
extern  sFONT                    FontWebDings;

#define GUI_COLOR_FRAME          GUI_COLOR_BLACK
#define GUI_BACKROUND_COLOR     GUI_COLOR_WHITE


#define DRT_HUP                  0x00U
#define DRT_HDN                  0x01U
#define DRT_HCENTER              0x02U
#define DRT_VLF                  0x00U
#define DRT_VRG                  0x10U
#define DRT_VCENTER              0x20U
#define DRT_IMAGE                0x1000U
#define DRT_FRAME                0x2000U
#define DRT_TS_ITEM              0x4000U            /* Entry is a position for touch screen */
#define DRT_RENDER_ITEM          0x8000U           /* Entry is a position for render txt */

#define MAKE_LPARAM(h,l)        ((((uint32_t)h) << 16) | (uint32_t)l)
#define GUI_HIWORD(l)           ((((uint32_t)l) >> 16) & 0xFFFF)
#define GUI_LOWORD(l)           ((((uint32_t)l) ) & 0xFFFF)
#define SIZE_TOUCH_HIT          15
#define SIZE_SHOW_TOUCH_HIT     40
#define LIST_BOX_NBITEM         4
#define LIMIT_TIME_START_SCROLL 1000


#define MAX_TEXT_EDIT_BOX       32              /* Max char in the edit string */
#define MAX_BNT_MAP             30              /* Max Key in the bnt ctrl */
#define MAX_BNT_ESC             6               /* Max Esc Key in the keyboard */
#define MAX_BNT_EDIT            2               /* Max boxes in the edit string box */
#define BNT_FLAG_SELECTED       0x1000U          /* flag bnt selected key ( black) */
#define BNT_FLAG_NO_FRAME       0x2000U          /* flag disable framing */

#define KEY_DX                  120             /* Keyboard Key DX size in pixel  */
#define KEY_DY                  79              /* Keyboard Key DY size in pixel  */
#define KEY_MAP_DX              10              /* Keyboard nb key by line */
#define KEY_MAP_DY              3

#define ICHARSET                0U              /* Key index */
#define ISPACE                  1U              /* Key index */
#define IBACK                   2U              /* Key index */
#define IDONE                   3U              /* Key index */
#define ILEFT                   4U              /* Key index */
#define IRIGHT                  5U              /* Key index */

#define IEDITLABEL              0U               /* Key index */
#define IEDITBOX                1U               /* Key index */

#define FLG_KEY_ESCAPE          0x00000080U     /* flag for not ascii key */


#define INFLAT_BNT_RECT         -2              /* Rect retractation for each keyboard key */
#define INFLAT_BNT_TXT_X        -8              /* Rect retractation for text in keyboard key */
#define INFLAT_BNT_TXT_Y        -2              /* Rect retractation for text in keyboard key */


#define MAX_WIFI_SSID           60U
#define MAX_SSID_NAME_STRING    30U
#define MAX_PASS_NAME_STRING    30U




typedef enum
{
  GUI_MSG_INIT,
  GUI_MSG_RENDER,
  GUI_MSG_BUTTON_DN,
  GUI_MSG_BUTTON_MOVE,
  GUI_MSG_BUTTON_UP,
  GUI_MSG_BUTTON_CLICK,
  GUI_MSG_KEYBOARD_DONE,
  GUI_MSG_LISTBOX_SEL_CHANGED,
  GUI_MSG_LISTBOX_SET_CUR_SEL,
  GUI_MSG_LISTBOX_MOVEUP,
  GUI_MSG_LISTBOX_MOVEDOWN,
  GUI_MSG_BUTTON_CLICK_NOTIFY,
  GUI_MSG_PAGE_ACTIVATE,
  GUI_MSG_KEYBOARD_RG,
  GUI_MSG_KEYBOARD_LF,
  GUI_MSG_KEYBOARD_CLICK_EXTENDED,
  GUI_MSG_USER
}GUI_PROC_MSG;

typedef struct t_gui_rect
{
  int32_t     x;
  int32_t     y;
  uint32_t     dx;
  uint32_t     dy;
}gui_rect_t;

typedef struct t_gui_item gui_item_t;
typedef uint32_t      (*itemProc)(gui_item_t *pItem,uint32_t message, uint32_t lparam);


/* Item render info */
struct t_gui_item
{

  uint32_t offX;   /* Translate position */
  gui_rect_t rect;
  uint32_t code1; /* Pos id */
  uint32_t txtColor;    /* Background colour */
  uint32_t bkColor;     /* Text colour */
  uint32_t format;      /* Item format */
  sFONT   *pFont ;      /* Text font */
  char_t  *pText;       /* Text to render */
  uint32_t  page;
  itemProc itemCB;
  void     *pInstance;
  gui_item_t *parent;
};



/* Text translation for avs state */
typedef struct gui_text_Code
{
  char_t          *pText;
  uint32_t      evt_code;
  void          (*itemCB)(struct gui_text_Code *pItem, uint32_t param); /* Callback item */
} GUI_TEXT_CODE;

typedef struct t_avs_point
{
  int32_t   type;
  float_t x, y;
} avs_point;




/*
Button box control item handle
*/
typedef struct t_bntBoxItem
{
  char_t      tKey[2];        /* mini string  buffer */
  gui_rect_t  rect;           /*  button position */
  char_t      *pString;       /* button string */
  uint32_t    flag;           /* button propertiy */
}bntBoxItem_t;

/*

Keyboard control handle

*/

typedef struct t_keyboardBox
{
  uint32_t lastSel;              /* last button bnt selection */
  int32_t offx,offy;            /* offset x and y for the scrolling */
  int32_t szDy;                 /* control size dx */
  int32_t szDx;                 /* control size fy */
  int32_t charSet;              /* char set minus capital etc... */
  int32_t nbBnt;                /* nb keys in the map  */
  bntBoxItem_t tMap[MAX_BNT_MAP];       /*  key map (scrolling) */
  bntBoxItem_t tEscape[MAX_BNT_ESC];    /*  key escape (no scrolling) */
  bntBoxItem_t tEditBox[MAX_BNT_EDIT];  /*  edit box (no scrolling) */
}keyboardBox_t;


typedef struct t_keyboard_instance
{
  keyboardBox_t       keyCtrl;                /* keyboard control */
  char_t*             pEditBuffer;    /* string to edit */
  uint32_t            szEditBuffer;   /* Max string buffer */
  char_t*             pEditLabel;     /* edit zone label */
  
}keyboard_instance_t;




#define LISTBOX_BNT_UP  0
#define LISTBOX_BNT_DN  1
#define LISTBOX_BNT_MAX 2

typedef struct t_listBox_bnt
{
  uint32_t    flg;
  char_t      *pText;
  gui_rect_t  pos;
}listBox_bnt_t;

/*
List box handle
*/
typedef struct t_listBox_instance
{
  int32_t    iListNb;           /* Nb Entry in the list */
  int32_t    iListOffset;       /* scrolling pos */
  int32_t    iCurSel;           /* current Selection */
  char_t      **plistItem;      /* List box entries */
  char_t     *sEmptyMsg;
  uint32_t    sizeUpDn;
  char_t     tCurSel[100];
  gui_rect_t  rClient;
  listBox_bnt_t  rBnt[LISTBOX_BNT_MAX];

} listBox_instance_t;



typedef enum 
{
	GUI_ITEM_NEXT_PAGE_ST=1,
	GUI_ITEM_LOGO_ST,
	GUI_ITEM_LOGO_SENSORY,
	GUI_ITEM_HEART,
	GUI_ITEM_IP,
	GUI_ITEM_STATE,
	GUI_ITEM_TIME,
        /* Info */
        GUI_ITEM_PLAYER,
        GUI_ITEM_ALARM,
        GUI_ITEM_INITIATOR,
        /* Test */
        GUI_ITEM_VERSION,
        GUI_ITEM_SYS_INFO,
        GUI_ITEM_BUFF_INFO,
        GUI_ITEM_INFO_TEST,
        GUI_ITEM_BNT_TEST,
        GUI_ITEM_NETWORK_LINK_SIM,
        GUI_ITEM_NETWORK_BAD_SIM,
        /* Wifi */
        GUI_ITEM_WIFI_LIST_SSID,
        GUI_ITEM_WIFI_SSID_NAME_STATIC,
        GUI_ITEM_WIFI_SSID_PASS_STATIC,
        GUI_ITEM_WIFI_SSID_NAME,
        GUI_ITEM_WIFI_SSID_PASS,
        GUI_ITEM_WIFI_RECONNECT,
        GUI_ITEM_WIFI_SCAN,
        GUI_ITEM_WIFI_FLASH_ESP,
        GUI_ITEM_WIFI_CONNECT_STATUS,
        
        /* Flash esp */
        GUI_ITEM_WIFI_FLASH_ESP_IMAGE,
        GUI_ITEM_WIFI_FLASH_ESP_REBOOT,
        GUI_ITEM_WIFI_FLASH_ESP_STAT_STATIC,
        GUI_ITEM_WIFI_FLASH_ESP_HELP_STATIC,
        
        /* Language */
        GUI_ITEM_LANGUAGE_TITLE,
        GUI_ITEM_LIST_LANG,
        GUI_ITEM_APPLY_LANG        
          
        
	
}gui_item_id_t;

typedef struct t_gui_resource
{
	uint32_t  	id;
	gui_rect_t      rect;
	sFONT           *pFont;
}gui_resource_t;





/* Wifi */
extern gui_rect_t       gScreenClip ;               /* main Clipping */
extern uint32_t         selPage;
extern uint32_t         gAnimationCounter;

gui_item_t*             gui_add_item(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, itemProc procCB , char_t *pText);
gui_item_t*             gui_add_item_res(gui_item_id_t id,  itemProc procCB , char_t *pText);
gui_resource_t*         gui_get_res(gui_item_id_t id);
void                    gui_render_text(gui_item_t *pItem, char_t *pText, uint32_t txtColor, uint32_t bkColor);
void                    gui_swap_buffer(void);
void                    gui_render_rect_text_entry(char_t *pText, uint32_t x, uint32_t y, int32_t  dx, int32_t  dy, uint32_t flag);
void                    gui_set_clip(gui_rect_t *pRect);
uint32_t                gui_clip_rect(gui_rect_t *pClip, gui_rect_t *pRect);
void                    gui_draw_item(gui_item_t *pItem, uint32_t txtColor, uint32_t bkColor);
void                    gui_render_feedback(gui_item_t *pItem, uint32_t draw);
void                    gui_inflat_rect(int32_t offx,int32_t offy, gui_rect_t *pRect);
void                    gui_render(void);
uint32_t                gui_proc_default(gui_item_t *pItem,uint32_t message,uint32_t lparam);
uint32_t                gui_check_limit(int32_t base, int32_t base_ref, int32_t dx_ref);
uint8_t                 gui_hit_test(gui_rect_t *pRef,gui_rect_t *pRect);
uint8_t                 gui_is_bmp_present(uint8_t *pBmp);
void                    gui_render_rect(gui_rect_t *pRect, uint32_t color);
void                    gui_set_full_clip(void);

void                    gui_listbox_ensure_visible(gui_item_t *pItem,listBox_instance_t *pList);
uint32_t                gui_proc_listBox(gui_item_t *pItem,uint32_t message,uint32_t lparam);
void                    gui_bnt_map_charSet(keyboardBox_t *pBntMap,int32_t charSet );
uint32_t                gui_proc_keyboard(gui_item_t *pItem,uint32_t message,uint32_t lparam);

AVS_Result service_gui_create(void);
AVS_Result service_gui_event_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
void                    LCD_SetFBStarAdress(uint32_t layer, uint32_t offset);

#endif /* _service_gui_*/


