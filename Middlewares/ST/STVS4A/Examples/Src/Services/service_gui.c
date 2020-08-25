/**
******************************************************************************
* @file    service_gui.c
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

/******************************************************************************
 *
 * this code is provided as an example , It is not a production code
 *
 ******************************************************************************/


#include "service.h"

void      LCD_Swap_buffer_Init(void);
uint32_t  LCD_SwapBuffer(void);
void      LCD_WaitVSync(void);
void      LCD_Vsync_Init(void);
extern    LTDC_HandleTypeDef hltdc_discovery;
extern    DSI_HandleTypeDef  hdsi_discovery;


/* LCD frame buffers */
#define MAX_EVT_PRESENTATION_TIME       3


#define PAGE_MAIN          1U               /* Main page */
#define PAGE_DEBUG         2U               /* Debug page */
#define PAGE_AFE           4U               /* Front end option */



#define TASK_NAME_GUI               "AVS:Task GUI"
#define TASK_STACK_GUI               500
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
#define GUI_COLOR_FRAME          GUI_COLOR_BLACK
#define GUI_BACKROUND_COLOR     GUI_COLOR_WHITE


#define DRT_HUP                  0x00U
#define DRT_HDN                  0x01U
#define DRT_HCENTER              0x02U
#define DRT_VLF                  0x00U
#define DRT_VRG                  0x10U
#define DRT_VCENTER              0x20U
#define DRT_FRAME                0x2000U
#define DRT_TS_ITEM              0x4000U            /* Entry is a position for touch screen */
#define DRT_RENDER_ITEM          0x8000U           /* Entry is a position for render txt */




/* Position ID */
typedef enum
{

  POS_IP = 0x1,
  POS_NEXT_PAGE,
  POS_STATE,
  POS_TIME,
  POS_ALARM,
  POS_PLAYER,
  POS_VERSION,
  POS_LOGO_ST,
  POS_HEART,
  POS_LOGO_SENSORY,
  POS_SYS_INFO,
  POS_MEMLOAD,
  POS_INITIATOR,
  POS_INFO_TEST,
  POS_START_STOP_TEST,
  POS_CONNECT,
  POS_BUFFER_INFO,
  POS_NETWORK_SIM,
  POS_AFE_HEADER,
  POS_BF_LOC,
  POS_AFE_INFO,

} GUI_POS;



/* Item render info */
typedef struct gui_item
{

  uint32_t offX;   /* Translate position */
  uint32_t x;   /* Pos x */
  uint32_t y;   /* Pos y */
  uint32_t dx;  /* Pos dx */
  uint32_t dy;  /* Pos dy */
  uint32_t code; /* Pos id */
  uint32_t txtColor;    /* Background colour */
  uint32_t bkColor;     /* Text colour */
  uint32_t format;      /* Item format */
  sFONT   *pFont ;      /* Text font */
  char_t  *pText;       /* Text to render */
  uint32_t  page;
  void    (*formatCB)(struct gui_item *pItem);  /* Callback formatter */
} GUI_ITEM;


/* Clipping */
typedef struct _tagClip
{
  uint32_t iClipX1;
  uint32_t iClipY1;
  uint32_t iClipX2;
  uint32_t iClipY2;
} GUI_CLIP;

/* Text translation for avs state */
typedef struct gui_text_Code
{
  char_t          *pText;
  uint32_t      evt_code;
  void          (*formatCB)(struct gui_text_Code *pItem, uint32_t param); /* Callback formatter */
} GUI_TEXT_CODE;

typedef struct t_avs_point
{
  int32_t   type;
  float_t x, y;
} avs_point;

static float_t     tSinus[360];
static float_t     tCosinus[360];
static uint32_t  gLastBfDir;
/* Vector shape for the Bean forming */
avs_point tListPointCone[] =
{
  {1, 0.3354, -0.4409},
  {0, 0.3352, -0.4228},
  {0, 0.3344, -0.4049},
  {0, 0.3331, -0.3872},
  {0, 0.3313, -0.3697},
  {0, 0.3290, -0.3524},
  {0, 0.3262, -0.3354},
  {0, 0.3230, -0.3186},
  {0, 0.3197, -0.3040},
  {0, 0.3161, -0.2896},
  {0, 0.3121, -0.2754},
  {0, 0.3078, -0.2615},
  {0, 0.3032, -0.2478},
  {0, 0.2983, -0.2343},
  {0, 0.2930, -0.2212},
  {0, 0.2859, -0.2050},
  {0, 0.2783, -0.1893},
  {0, 0.2703, -0.1740},
  {0, 0.2618, -0.1593},
  {0, 0.2529, -0.1451},
  {0, 0.2435, -0.1314},
  {0, 0.2338, -0.1183},
  {0, 0.2173, -0.0984},
  {0, 0.1999, -0.0802},
  {0, 0.1816, -0.0636},
  {0, 0.1624, -0.0487},
  {0, 0.1425, -0.0356},
  {0, 0.1218, -0.0244},
  {0, 0.1005, -0.0151},
  {0, 0.0888, -0.0110},
  {0, 0.0769, -0.0075},
  {0, 0.0649, -0.0046},
  {0, 0.0527, -0.0023},
  {0, 0.0404, -0.0006},
  {0, 0.0279, 0.0004},
  {0, 0.0154, 0.0007},
  {0, -0.0081, -0.0005},
  {0, -0.0310, -0.0039},
  {0, -0.0535, -0.0096},
  {0, -0.0753, -0.0173},
  {0, -0.0966, -0.0271},
  {0, -0.1171, -0.0389},
  {0, -0.1369, -0.0525},
  {0, -0.1496, -0.0625},
  {0, -0.1619, -0.0733},
  {0, -0.1738, -0.0847},
  {0, -0.1853, -0.0970},
  {0, -0.1964, -0.1098},
  {0, -0.2070, -0.1234},
  {0, -0.2172, -0.1376},
  {0, -0.2268, -0.1523},
  {0, -0.2360, -0.1677},
  {0, -0.2447, -0.1836},
  {0, -0.2528, -0.2000},
  {0, -0.2605, -0.2169},
  {0, -0.2676, -0.2344},
  {0, -0.2741, -0.2523},
  {0, -0.2783, -0.2650},
  {0, -0.2821, -0.2778},
  {0, -0.2857, -0.2909},
  {0, -0.2891, -0.3042},
  {0, -0.2921, -0.3176},
  {0, -0.2948, -0.3313},
  {0, -0.2971, -0.3451},
  {0, -0.2991, -0.3584},
  {0, -0.3008, -0.3718},
  {0, -0.3022, -0.3854},
  {0, -0.3033, -0.3991},
  {0, -0.3041, -0.4129},
  {0, -0.3046, -0.4268},
  {0, -0.3047, -0.4409},
  {0, -0.3044, -0.4588},
  {0, -0.3033, -0.4757},
  {0, -0.3016, -0.4915},
  {0, -0.2993, -0.5063},
  {0, -0.2963, -0.5201},
  {0, -0.2928, -0.5329},
  {0, -0.2886, -0.5448},
  {0, -0.2830, -0.5576},
  {0, -0.2766, -0.5691},
  {0, -0.2696, -0.5795},
  {0, -0.2618, -0.5886},
  {0, -0.2534, -0.5967},
  {0, -0.2444, -0.6037},
  {0, -0.2348, -0.6096},
  {0, -0.2234, -0.6151},
  {0, -0.2112, -0.6194},
  {0, -0.1985, -0.6225},
  {0, -0.1852, -0.6246},
  {0, -0.1714, -0.6256},
  {0, -0.1572, -0.6256},
  {0, -0.1425, -0.6247},
  {0, -0.1274, -0.6229},
  {0, -0.1120, -0.6203},
  {0, -0.0962, -0.6170},
  {0, -0.0803, -0.6129},
  {0, -0.0642, -0.6082},
  {0, -0.0479, -0.6030},
  {0, -0.0315, -0.5972},
  {0, 0.0390, -0.5675},
  {0, 0.0496, -0.5730},
  {0, 0.0602, -0.5783},
  {0, 0.0707, -0.5834},
  {0, 0.0812, -0.5884},
  {0, 0.0916, -0.5931},
  {0, 0.1020, -0.5977},
  {0, 0.1123, -0.6019},
  {0, 0.1255, -0.6071},
  {0, 0.1386, -0.6118},
  {0, 0.1515, -0.6160},
  {0, 0.1641, -0.6195},
  {0, 0.1765, -0.6225},
  {0, 0.1886, -0.6248},
  {0, 0.2004, -0.6264},
  {0, 0.2139, -0.6274},
  {0, 0.2268, -0.6272},
  {0, 0.2393, -0.6259},
  {0, 0.2512, -0.6234},
  {0, 0.2624, -0.6195},
  {0, 0.2730, -0.6143},
  {0, 0.2830, -0.6077},
  {0, 0.2915, -0.6002},
  {0, 0.2994, -0.5913},
  {0, 0.3066, -0.5811},
  {0, 0.3130, -0.5694},
  {0, 0.3187, -0.5562},
  {0, 0.3237, -0.5414},
  {0, 0.3278, -0.5250},
  {0, 0.3298, -0.5148},
  {0, 0.3315, -0.5039},
  {0, 0.3329, -0.4925},
  {0, 0.3340, -0.4805},
  {0, 0.3348, -0.4679},
  {0, 0.3353, -0.4547},
  {0, 0.3354, -0.4409}
};


uint16_t       gts_status;                /* Local */
static TS_StateTypeDef      gts_State;
static uint32_t             gCurPage = 0;
static GUI_ITEM             tListItem[40];             /* Position entry */
static uint32_t         gItemCount = 0;        /* Position entry count */
static uint32_t          gLastTsDelay = 0;          /* Last touch screen delay */
static task_t *       hTask;
static GUI_CLIP         gScreenClip ;               /* Clipping position */
static uint32_t          gAnimationCounter;
static uint32_t         gNbPages = 2;

void            gui_pos_clear_Entry(void);
void             gui_add_pos_entry(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, uint32_t code, void (*formatterCB)(struct gui_item *pItem), char_t *pText);
uint32_t         gui_get_pos_entry(uint32_t code, uint32_t *x, uint32_t *y, uint32_t *dx, uint32_t *dy);
static int32_t         gui_get_pos_entry_index(uint32_t code);
static void                 gui_render_text_pos(uint32_t pos, char_t *pText, uint32_t txtColor, uint32_t bkColor);
static void             gui_process_key(uint32_t index);

       uint32_t             bGuiInitialized;

void     gui_swap_buffer(void);
void BSP_LCD_SetFBStarAdress(uint32_t layer, uint32_t offset);
void BSP_LCD_SetFBStarAdress(uint32_t layer, uint32_t offset)
{
  hltdc_discovery.LayerCfg[layer].FBStartAdress = offset;
}

static int32_t findTextEntry(GUI_TEXT_CODE *pList, uint32_t code);
static int32_t findTextEntry(GUI_TEXT_CODE *pList, uint32_t code)
{

  for(int32_t a = 0; pList[a].pText ; a++)
  {
    if(pList[a].evt_code == code)
    {
      return a;
    }
  }
  return -1;
}




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


static void gui_evt_formatter_change_state(GUI_TEXT_CODE *pItem, uint32_t param);
static void gui_evt_formatter_change_state(GUI_TEXT_CODE *pItem, uint32_t param)
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
  if(index == -1)
  {
    pItem->pText = "Evt unknown";
  }
  else
  {
    pItem->pText = tState[index].pText;
  }
}





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
  {"", EVT_CHANGE_STATE, gui_evt_formatter_change_state},
  {0, 0,0},
};






/*


 Check the BMP presents before to draw it in order to prevent crashes when the flash is not flashed ( using stlink-utility)



*/
static uint8_t gui_is_bmp_present(uint8_t *pBmp);
static uint8_t gui_is_bmp_present(uint8_t *pBmp)
{
  if(pBmp[0] != 'B')
  {
    return 0;
  }
  if(pBmp[1] != 'M')
  {
    return 0;
  }
  return 1;
}


static void gui_formatter_network_sim(GUI_ITEM *pItem);
static void gui_formatter_network_sim(GUI_ITEM *pItem)
{
  pItem->page   = PAGE_DEBUG;
  pItem->format |= DRT_FRAME;
  pItem->format |= DRT_TS_ITEM;
  pItem->pFont = GUI_FONT_MEDIUM;
  pItem->txtColor = GUI_COLOR_BLACK;


  if(service_endurance_enable_network_sim(3))
  {
    pItem->pText  = "Network Sim On";
    pItem->txtColor  = GUI_COLOR_RED;
  }
  else
  {
    pItem->pText  = "Network Sim Off";
  }
}



/*

 render info from afe


*/
static void gui_afe_info_formatter(GUI_ITEM *pItem);
static void gui_afe_info_formatter(GUI_ITEM *pItem)
{
  pItem->page = PAGE_AFE;

  if(pItem->pText == 0)
  {
    pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
    pItem->pFont   = GUI_FONT_SMALL;

    if(AVS_Ioctl(hInstance, AVS_IOCTL_AUDIO_GET_INFO, sizeof(gAppState.tFrontEndInfo), (uint32_t)gAppState.tFrontEndInfo) == AVS_NOT_IMPL)
    {
      strcpy(gAppState.tFrontEndInfo, "no info");
    }

    pItem->pText = gAppState.tFrontEndInfo;
  }
}

static void gui_render_vector(uint32_t  centerx, uint32_t centery, int32_t deg, float_t scale, uint32_t color, avs_point *pVectors, uint32_t nbVector);
static void gui_render_vector(uint32_t  centerx, uint32_t centery, int32_t deg, float_t scale, uint32_t color, avs_point *pVectors, uint32_t nbVector)
{

  /* Deg += 180; // add offset model */
  uint32_t alpha = deg % 360;
  int32_t oldx = 0;
  int32_t oldy = 0;
  BSP_LCD_SetTextColor(color);
  for (int32_t a = 0; a < nbVector; a++)
  {
    float_t px = pVectors[a].x * tCosinus[alpha]  - pVectors[a].y * tSinus[alpha];
    float_t py = pVectors[a].y * tCosinus[alpha]  + pVectors[a].x * tSinus[alpha];

    int32_t x = (int32_t)(float_t)((px * scale) + centerx);
    int32_t y = (int32_t)(float_t)((py * scale) + centery);

    if (tListPointCone[a].type == 1)
    {
      oldx = x;
      oldy = y;
    }
    if (tListPointCone[a].type == 0)
    {
      BSP_LCD_DrawLine(oldx, oldy, x, y);
      oldx = x;
      oldy = y;
    }
  }
}



/*

 Formatter BF draw the Bean forming


*/
static void gui_bf_formatter(GUI_ITEM *pItem);
static void gui_bf_formatter(GUI_ITEM *pItem)
{
  AVS_Ioctl(hInstance, AVS_IOCTL_AUDIO_ENABLE_BEANFORMING, 3, 0);

  if((PAGE_AFE & (1U << gCurPage)) != 0)
  {
    pItem->format |= DRT_FRAME ;
    pItem->pFont   = GUI_FONT_MEDIUM;
    pItem->page    = PAGE_AFE;
    static const void *pBfImage = 0;
    uint32_t coneCol = GUI_RGB(255U, 0U, 0U);
    uint32_t drawCone = FALSE;

    if(strstr(gAppState.tFrontEndInfo, "AFE BF 4M 360"))
    {
      pBfImage = service_assets_load("mb1388A_bmp", 0, 0);
      coneCol = GUI_RGB(255U, 255U, 255U);
      drawCone  = 1;

    }
    if(strstr(gAppState.tFrontEndInfo, "AFE BF 4M 180"))
    {
      pBfImage = service_assets_load("mb1337A_bmp", 0, 0);
      coneCol = GUI_RGB(255U, 255U, 255U);
      drawCone  = 1;

    }
    if(strstr(gAppState.tFrontEndInfo, "AFE BF 2M 360"))
    {
      pBfImage = service_assets_load("mb1337A_bmp", 0, 0);
      coneCol = GUI_RGB(255U, 255U, 255U);
      drawCone  = 1;

    }
    int32_t bFound = (uint32_t)strstr(gAppState.tFrontEndInfo, "AFE Omni 1M AEC");
    if(((uint32_t)strstr(gAppState.tFrontEndInfo, "AFE 1M Omni") != 0)  || (bFound!= 0) )
    {
      pBfImage = service_assets_load("omni_mic_bmp", 0, 0);
      coneCol = GUI_RGB(255U, 255U, 255U);
    }
    else
    {
      pBfImage = service_assets_load("beanforming_bmp", 0, 0);
    }
    if(gui_is_bmp_present((uint8_t *)(uint32_t)pBfImage ))
    {
      BSP_LCD_DrawBitmap(pItem->x, pItem->y, (uint8_t*)(uint32_t)pBfImage );
    }
    if(drawCone )
    {
      gui_render_vector(pItem->x + pItem->dx / 2, pItem->y + pItem->dy / 2, gLastBfDir, 190.0F, coneCol, tListPointCone, sizeof(tListPointCone) / sizeof(tListPointCone[0]));
    }
  }

}

/*

 Formatter AFE options


*/
static void gui_text_afe_option(GUI_ITEM *pItem);
static void gui_text_afe_option(GUI_ITEM *pItem)
{
  pItem->format |= DRT_FRAME ;
  pItem->pFont   = GUI_FONT_MEDIUM;
  pItem->page    = PAGE_AFE;
}


/*

 Formatter next page


*/
static void gui_formatter_next_page(GUI_ITEM *pItem);
static void gui_formatter_next_page(GUI_ITEM *pItem)
{
  static char_t txtPages[10];
  pItem->page    = (uint32_t)-1; /* All pages */
  pItem->format |= DRT_TS_ITEM;
  pItem->format |= DRT_FRAME ;
  pItem->pFont   = GUI_FONT_MEDIUM;
  snprintf(txtPages, sizeof(txtPages), "%lu/%lu", gCurPage + 1, gNbPages);
  pItem->pText = txtPages;
}

/*

 Formatter state , render a string according to the global state


*/
static void gui_formatter_state(GUI_ITEM *pItem);
static void gui_formatter_state(GUI_ITEM *pItem)
{
  uint32_t index = 0;
  pItem->format |= DRT_FRAME ;
  pItem->pText = "No State";
  pItem->page = PAGE_MAIN | PAGE_DEBUG;

  while(tStateTextList[index].pText)
  {
    if(tStateTextList[index].evt_code == gAppState.evt)
    {
      if(tStateTextList[index].formatCB)
      {
        tStateTextList[index].formatCB(&tStateTextList[index], gAppState.evt_param);
      }
      pItem->pText = tStateTextList[index].pText;
      break;
    }
    index++;
  }

  pItem->txtColor = GUI_COLOR_BLACK;
  pItem->bkColor  = GUI_COLOR_WHITE;
}
/*

 Formatter time waits for a sync time and render it


*/
static void gui_formatter_time(GUI_ITEM *pItem);
static void gui_formatter_time(GUI_ITEM *pItem)
{
  pItem->format |= DRT_FRAME ;
  pItem->page = PAGE_MAIN | PAGE_DEBUG;

  if(gAppState.curTime)
  {
    AVS_TIME itime = gAppState.curTime;
    itime += service_alarm_get_time_zone() * 60 * 60;
    struct tm *ptm = localtime((const time_t *)(uint32_t)&itime);
    if(ptm)
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


/*

 render IP when resolved


*/
static void gui_formatter_ip(GUI_ITEM *pItem);
static void gui_formatter_ip(GUI_ITEM *pItem)
{
  pItem->format |= DRT_FRAME ;
  pItem->page = PAGE_MAIN | PAGE_DEBUG;
  if(strncmp(sInstanceFactory.ipV4_host_addr, "xxx", 3) != 0)
  {
    pItem->pText = sInstanceFactory.ipV4_host_addr;
  }
  else
  {
    pItem->pText = "No IP";
  }
}


/*

 render all alarms


*/
static void gui_formatter_alarm(GUI_ITEM *pItem);
static void gui_formatter_alarm(GUI_ITEM *pItem)
{
  pItem->offX     = 20;
  pItem->format  &= ~(uint32_t)0xFF;
  pItem->format  |= DRT_HCENTER | DRT_VLF ;
  pItem->format  |= DRT_FRAME ;
  pItem->pText = service_alarm_get_string();
  if(pItem->pText == 0)
  {
    pItem->pText = "";
  }
  pItem->pFont   = GUI_FONT_MEDIUM;

}

/*

 Formatter player state


*/
static void gui_formatter_player(GUI_ITEM *pItem);
static void gui_formatter_player(GUI_ITEM *pItem)
{
  pItem->offX     = 20;
  pItem->format  &= ~(uint32_t)0xFF;
  pItem->format  |= DRT_HCENTER | DRT_VLF ;
  pItem->format  |= DRT_FRAME ;
  pItem->pText = service_player_get_string();
  if(pItem->pText == 0)
  {
    pItem->pText = "";
  }
  pItem->pFont   = GUI_FONT_MEDIUM;

}


/*

 render chip version SDK version and App version


*/
static void gui_formatter_version(GUI_ITEM *pItem);
static void gui_formatter_version(GUI_ITEM *pItem)
{


  pItem->page = PAGE_MAIN | PAGE_DEBUG;

  if(pItem->pText == 0)
  {
    pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
    pItem->pFont   = GUI_FONT_SMALL;
    snprintf(gAppState.tVersionString, sizeof(gAppState.tVersionString), "Chip : %s\rSdk  : %s\rApp  : %s\rAudio: %s\rNet  : %s\rBuild: %s\rToolC: %s", sInstanceFactory.cpuID, AVS_VERSION, APP_VERSION, sInstanceFactory.portingAudioName, sInstanceFactory.netSupportName, __DATE__, sInstanceFactory.toolChainID);
    pItem->pText = gAppState.tVersionString;
  }
}


/* Render the heart beating */

static void gui_formatter_heart(GUI_ITEM *pItem);
static void gui_formatter_heart(GUI_ITEM *pItem)
{
  static const void *pImage = 0;
  pItem->page = (uint32_t)-1 ;  /* PAGE_MAIN | PAGE_DEBUG; all pages */
  pItem->format |= DRT_TS_ITEM; /* Secret key */

  if(pImage == 0)
  {
    pImage = service_assets_load("Heart_bmp", 0, 0);
  }
  int32_t bFound = gui_is_bmp_present((uint8_t *)(uint32_t)pImage);

  if(((gAnimationCounter % 3) == 0) && (bFound != 0))
  {
    BSP_LCD_DrawBitmap(pItem->x, pItem->y, (uint8_t*)(uint32_t)pImage);
  }

}



/* Render ST logo */
static void gui_formatter_logoST(GUI_ITEM *pItem);
static void gui_formatter_logoST(GUI_ITEM *pItem)
{
  static const void *pLogo = 0;
  pItem->page = (uint32_t)-1 ;  /* PAGE_MAIN | PAGE_DEBUG; all pages */
  pItem->format |= DRT_TS_ITEM; /* Secret key */

  if(pLogo == 0)
  {
    pLogo = service_assets_load("logoST_bmp", 0, 0);
  }
  if(gui_is_bmp_present((uint8_t *)(uint32_t)pLogo))
  {
    BSP_LCD_DrawBitmap(pItem->x, pItem->y, (uint8_t*)(uint32_t)pLogo);
  }

}

/* Render Sensory logo */

void gui_formatter_sensoryST(GUI_ITEM *pItem);
void gui_formatter_sensoryST(GUI_ITEM *pItem)
{
  static const void *pLogo = 0;
  pItem->page = (uint32_t)-1 ;  /* PAGE_MAIN | PAGE_DEBUG; all pages */

  if(pLogo == 0)
  {
    pLogo = service_assets_load("logoSensory_bmp", 0, 0);
  }


  if(gui_is_bmp_present((uint8_t *)(uint32_t)pLogo))
  {
    BSP_LCD_DrawBitmap(pItem->x, pItem->y, (uint8_t*)(uint32_t)pLogo);
  }

}



/*

 formatter Network simulation state


*/
void gui_formatter_connect_test(GUI_ITEM *pItem);
void gui_formatter_connect_test(GUI_ITEM *pItem)
{
  pItem->page   = PAGE_DEBUG;
  pItem->format |= DRT_FRAME;
  pItem->format |= DRT_TS_ITEM;
  pItem->pFont = GUI_FONT_MEDIUM;
  pItem->txtColor  = GUI_COLOR_BLACK;
  if(AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, 2, 0) == TRUE)
  {
    pItem->txtColor  = GUI_COLOR_RED;
    pItem->pText = "Set Link down";
  }
  else
  {
    pItem->pText = "Set Link up";
  }
}


/*

 generic formatter for a KEY


*/


void gui_formatter_ts_test(GUI_ITEM *pItem);
void gui_formatter_ts_test(GUI_ITEM *pItem)
{
  pItem->page   = PAGE_DEBUG;
  pItem->format |= DRT_FRAME;
  pItem->format |= DRT_TS_ITEM;
  pItem->pFont = GUI_FONT_SMALL;

}



/*

 format the box Test info


*/
static void gui_formatter_info_test(GUI_ITEM *pItem);
static void gui_formatter_info_test(GUI_ITEM *pItem)
{
  static char_t sTime[20] = "none";
  static char_t sTestName[20] = "none";
  pItem->page = PAGE_DEBUG;


  service_endurance_get_current_test_name(sTestName, sizeof(sTestName));
  pItem->offX     = 20;
  pItem->format = DRT_HCENTER | DRT_VLF | DRT_RENDER_ITEM | DRT_FRAME;
  pItem->pFont   = GUI_FONT_MEDIUM;
  uint32_t tstState = service_endurance_get_state();
  AVS_TIME itime = gAppState.curTime - gAppState.testTimeStart;
  if(!tstState)
  {
    itime = 0;
  }
  struct tm *ptm = localtime((const time_t *)(uint32_t)&itime);
  if(ptm)
  {
    strftime(sTime, sizeof(sTime), "%T", ptm);
  }
  uint32_t days = itime / (24 * 60 * 60);
  uint32_t percentSuccess = 0;
  uint32_t total = gAppState.testPassedCount + gAppState.testErrorCount + gAppState.testTimeoutCount;
  if(total)
  {
    percentSuccess  = (gAppState.testPassedCount  *  100) / total;
  }
  snprintf(gAppState.tInfoTest, sizeof(gAppState.tInfoTest), "Test success : %03lu%% \r"
           "Current : %s\r"
           "Loops   : %lu\r"
           "Passed  : %lu\r"
           "Error   : %lu\r"
           "Timeout : %lu\r"
           "Retry   : %lu\r"
           "%lu days %s", percentSuccess, sTestName, gAppState.testLoops, gAppState.testPassedCount, gAppState.testErrorCount, gAppState.testTimeoutCount, gAppState.testRetryCount, days, sTime);
  pItem->pText = gAppState.tInfoTest;
  pItem->page = PAGE_DEBUG;


}
static void gui_formatter_btn_test(GUI_ITEM *pItem);
static void gui_formatter_btn_test(GUI_ITEM *pItem)
{
  pItem->page = PAGE_DEBUG;
  pItem->format |= DRT_TS_ITEM | DRT_FRAME;
  pItem->pFont   = GUI_FONT_MEDIUM;
  if (service_assets_check_integrity() != AVS_OK)
  {
     pItem->pText = "Invalid Assets" ;
  }
  else if(!service_endurance_get_state())
  {
    pItem->pText = "Test start" ;
  }
  else
  {
    pItem->pText = "Test stop" ;
  }

}

static void gui_formatter_buff_info(GUI_ITEM *pItem);
static void gui_formatter_buff_info(GUI_ITEM *pItem)
{
  pItem->page   = PAGE_DEBUG;
  pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
  pItem->pFont  = GUI_FONT_SMALL;

  if(((gAnimationCounter % 2) == 0) || (pItem->pText == 0))
  {
    static float_t refTxBw;
    static float_t refRxBw;
    static uint32_t refTime;
    static uint32_t prevTxCount;
    static uint32_t prevRxCount;
    uint32_t curTxCount = 0;
    uint32_t curRxCount = 0;
    uint32_t curTime = osKernelSysTick() ;
    AVS_Ioctl(hInstance, AVS_NETWORK_COUNTS, (uint32_t)&curRxCount, (uint32_t)&curTxCount );

    /* Compte the num of byte by sec */
    refTxBw  =   (curTxCount -  prevTxCount) / ((curTime - refTime) / 1000.0F);
    refTxBw  *= 8; /* Convert in bits */
    refTxBw  /= 1000; /* Presentation in Kb/s */

    refRxBw  =   (curRxCount -  prevRxCount) / ((curTime - refTime) / 1000.0F);
    refRxBw  *= 8;
    refRxBw  /= 1000;

    prevTxCount = curTxCount;
    prevRxCount = curRxCount;
    refTime = curTime;

    AVS_Sys_info sysinfo;
    AVS_Get_Sys_Info(hInstance, &sysinfo);
    /* This function could be heavy , let's call it some time only */
    pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
    pItem->pFont   = GUI_FONT_SMALL;

    snprintf(gAppState.tBuffInfo, sizeof(gAppState.tBuffInfo),
             "Net   RX   : %05.1f Kbp/s\r"
             "Net   TX   : %05.1f Kbp/s\r"
             "Reco  Buf  : %03lu%%\r"
             "Synth Buf  : %03lu%%\r"
             "Aux   Buf  : %03lu%%\r"
             "Play  Buf  : %03lu%%\r"
               ,
             refRxBw,
             refTxBw,
             sysinfo.recoBufferPercent,
             sysinfo.synthBufferPercent,
             sysinfo.auxBufferPercent,
             service_player_get_buffer_percent(hInstance)
               );

    pItem->pText = gAppState.tBuffInfo;
  }

}



static void gui_formatter_sys_info(GUI_ITEM *pItem);
static void gui_formatter_sys_info(GUI_ITEM *pItem)
{
  pItem->page   = PAGE_DEBUG;
  pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
  pItem->pFont  = GUI_FONT_SMALL;

  if(((gAnimationCounter % 10) == 0) || (pItem->pText == 0))
  {
    AVS_Sys_info sysinfo;
    AVS_Get_Sys_Info(hInstance, &sysinfo);
    /* This function could be heavy , let's call it some time only */
    pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
    pItem->pFont   = GUI_FONT_SMALL;
    uint32_t  totalSpace = sysinfo.memDtcmTotalSpace + sysinfo.memPRamTotalSpace + sysinfo.memTotalSpace;
    uint32_t  totalFree  = sysinfo.memDtcmFreeSpace  + sysinfo.memPRamFreeSpace  + sysinfo.memFreeSpace;
    
    static  char_t tmpString[30];
    if(sysinfo.memTotalSpace ==0 && sysinfo.memFreeSpace ==0)
    {
      snprintf(tmpString, sizeof(tmpString),"HEAP   : Not available");
    }
    else
    {
    snprintf(tmpString, sizeof(tmpString),"HEAP   : %03luU+%04luF. Free %d%%",
             (sysinfo.memTotalSpace-sysinfo.memFreeSpace)/1024,
             sysinfo.memFreeSpace / 1024, (int)(float)(100.0F * sysinfo.memFreeSpace / sysinfo.memTotalSpace));
    }
    
    


    snprintf(gAppState.tSysInfo, sizeof(gAppState.tSysInfo),
             "CPU    : %d%%\r"
             "DTCM   : %03luU+%04luF. Free %d%%\r"
             "PRAM   : %03luU+%04luF. Free %d%%\r"
             "%s\r"
             "TOTAL  : %03luU+%04luF. Free %d%%\r",
             service_GetCPUUsage(),
             (sysinfo.memDtcmTotalSpace-sysinfo.memDtcmFreeSpace)/1024,
             sysinfo.memDtcmFreeSpace / 1024, (int)(float)((100.0 * sysinfo.memDtcmFreeSpace) / sysinfo.memDtcmTotalSpace),
             
             (sysinfo.memPRamTotalSpace-sysinfo.memPRamFreeSpace)/1024,
             sysinfo.memPRamFreeSpace / 1024, (int)(float)(100.0F * sysinfo.memPRamFreeSpace / sysinfo.memPRamTotalSpace),
             
             tmpString,
             
             (totalSpace-totalFree)/1024,
             totalFree / 1024, (int)(float)(100.0F * totalFree / totalSpace));
    
    pItem->pText = gAppState.tSysInfo;
  }


}

static void gui_formatter_initiator(GUI_ITEM *pItem);
static void gui_formatter_initiator(GUI_ITEM *pItem)
{
  static char_t *tInitiatorStr[] = {"DEFAULT", "Push To Talk", "Tap To Talk", "Voice Initiated\rTap To Talk"};

  pItem->format |= DRT_FRAME;
  pItem->format |= DRT_TS_ITEM;
  pItem->pFont   = GUI_FONT_MEDIUM;
  int32_t index = sInstanceFactory.initiator - AVS_INITIATOR_DEFAULT;
  if(index > AVS_INITIATOR_VOICE_INITIATED)
  {
    index = 0;
  }
  pItem->pText = tInitiatorStr[index];

}






/*

 Set a screen clipping for draw primitives


*/

static void gui_set_clip(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t  y2);
static void gui_set_clip(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t  y2)
{
  gScreenClip.iClipX1 = x1;
  gScreenClip.iClipY1 = y1;
  gScreenClip.iClipX2 = x2;
  gScreenClip.iClipY2 = y2;
}

/*

 Restore the full screen clipping


*/


static void gui_set_full_clip(void);
static void gui_set_full_clip(void)
{
  gScreenClip.iClipX1 = 0;
  gScreenClip.iClipY1 = 0;
  gScreenClip.iClipX2 = BSP_LCD_GetXSize();
  gScreenClip.iClipY2 = BSP_LCD_GetYSize();
}




/*


 Rect helper , inflat or deflat a rect according to its offsets



*/
static void gui_inflat_rect(int32_t offx, int32_t offy, int32_t *x, int32_t *y, uint32_t *dx, uint32_t *dy);
static void gui_inflat_rect(int32_t offx, int32_t offy, int32_t *x, int32_t *y, uint32_t *dx, uint32_t *dy)
{
  *x -= offx;
  *y -= offy;
  *dx += offx * 2;
  *dy += offy * 2;

}

/*


 Perform a  rect clipping , update Posx and destination rect according to the global clipping



*/
static uint32_t  gui_clip_rect(GUI_CLIP *pClip, int32_t *posX, int32_t *posY, int32_t *r2x, int32_t *r2y, uint32_t *r2dx, uint32_t *r2dy);
static uint32_t  gui_clip_rect(GUI_CLIP *pClip, int32_t *posX, int32_t *posY, int32_t *r2x, int32_t *r2y, uint32_t *r2dx, uint32_t *r2dy)
{
  int32_t tmp;
  if( (tmp  = (*posX - pClip->iClipX1)) < 0 )
  {
    *r2x  -= tmp;
    *r2dx += tmp;
    *posX -= tmp;
  }
  if( (tmp  = (*posY - pClip->iClipY1)) < 0 )
  {
    *r2y   -= tmp;
    *r2dy  += tmp;
    *posY  -= tmp;
  }
  if( (tmp = (pClip->iClipX2) - (*posX + *r2dx)) < 0 )
  {
    *r2dx += tmp;
  }
  if( (tmp = (pClip->iClipY2) - (*posY + *r2dy)) < 0 )
  {
    *r2dy += tmp;
  }
  if((*r2dy == 0) || (*r2dx == 0) )
  {
    return 0;
  }
  return 1;
}


/*

 Helper compute nbline a,d nb char by line from a text



*/
static uint32_t gui_get_text_info(char_t *pText, uint32_t *pInfo, uint32_t *max);
static uint32_t gui_get_text_info(char_t *pText, uint32_t *pInfo, uint32_t *max)
{
  int32_t countLine = 0;
  int32_t countChar = 0;
  *max = 0;
  while((*pText) && (countLine  < 10))
  {
    if(*pText == '\r')
    {
      *pInfo = countChar;
      pInfo ++;
      countLine++;
      if(*max < countChar)
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
  if((countChar) && (countLine  < 10))
  {
    *pInfo = countChar;
    if(*max < countChar)
    {
      *max = countChar;
    }
    countLine++;
  }

  return   countLine;
}

/*

 draw a wire rect


*/
static void gui_render_rect(int32_t x, int32_t y, uint32_t  dx, uint32_t  dy, uint32_t color);
static void gui_render_rect(int32_t x, int32_t y, uint32_t  dx, uint32_t  dy, uint32_t color)
{
  BSP_LCD_SetTextColor(color);
  int32_t posx = x;
  int32_t posy = y;
  x = 0;
  y = 0;
  gui_clip_rect(&gScreenClip, &posx, &posy, &x, &y, &dx, &dy);
  BSP_LCD_DrawRect(posx, posy, dx, dy);
}



/*


  This function allows to render a string in a box according its flags



*/
static void     gui_render_rect_text_entry(char_t *pText, uint32_t x, uint32_t y, int32_t  dx, int32_t  dy, uint32_t flag);
static void     gui_render_rect_text_entry(char_t *pText, uint32_t x, uint32_t y, int32_t  dx, int32_t  dy, uint32_t flag)
{
  uint32_t tInfo[10];
  uint32_t count;
  sFONT  *pFont   = BSP_LCD_GetFont();
  uint32_t szX = 0;
  if((count = gui_get_text_info(pText, tInfo, &szX)) == 0)
  {
    return;
  }
  int32_t szY = count *   pFont->Height;
  int32_t  baseY = 0;
  gui_set_clip(x, y, x + dx, y + dy);



  switch(flag & 0x0F)
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

  for(int32_t a = 0; a < count  ; a ++)
  {
    int32_t baseX = 0;

    switch(flag & 0xF0)
    {
      case DRT_VLF:
      {
        baseX = x;
        break;
      }
      case DRT_VRG:
      {
        int32_t off = dx  - (tInfo[a] * pFont->Width);
        baseX = x + (off - 1);
        break;
      }
      case DRT_VCENTER:
      {
        int32_t off = dx  - (tInfo[a] * pFont->Width);
        baseX = x + (off / 2);
        break;
      }
  default:
    break;


    }
    int32_t i = 0;

    while((*pText) && (*pText != '\r'))
    {
      int32_t  px = baseX + i * pFont->Width;
      int32_t  py = baseY;
      if((px >= gScreenClip.iClipX1) && ((px + pFont->Width) <= gScreenClip.iClipX2))
      {
        if((py >= gScreenClip.iClipY1) && ((py + pFont->Height) <= gScreenClip.iClipY2))
        {
          BSP_LCD_DisplayChar(px, py, *pText);
        }
    }
      pText++;
      i++;
    }
    if(*pText == '\r')
    {
      baseY += pFont->Height;
      pText++;
    }

  }

  gui_set_full_clip();
}
static void DrawItem(GUI_ITEM *pItem, uint32_t txtColor, uint32_t bkColor);
static void DrawItem(GUI_ITEM *pItem, uint32_t txtColor, uint32_t bkColor)
{
  if((pItem->format & DRT_FRAME) != 0)
  {
    int32_t x, y;
    uint32_t dx, dy;
    x = pItem->x;
    y = pItem->y;
    dx = pItem->dx;
    dy = pItem->dy;
    gui_inflat_rect(2, 2, &x, &y, &dx, &dy);
    gui_render_rect(x, y, dx, dy, GUI_COLOR_FRAME);
  }
  if(pItem->pText)
  {
    gui_render_text_pos(pItem->code, pItem->pText, txtColor, bkColor);
  }
}



/*


 render all items



*/
static void gui_update(void);
static void gui_update(void)
{
  if(hInstance == 0)
  {
    return;
  }

  AVS_Get_Sync_Time(hInstance, &gAppState.curTime);
  if(gAppState.curTime - gAppState.lastEvtTime > MAX_EVT_PRESENTATION_TIME)
  {
    gAppState.lastEvtTime  = gAppState.curTime;
    gAppState.evt = EVT_CHANGE_STATE;
    gAppState.evt_param = AVS_Get_State(hInstance);

  }

  for(int32_t a = 0; a < gItemCount ; a++)
  {
    GUI_ITEM *pItem = &tListItem[a];
    if(pItem->formatCB)
    {
      pItem->formatCB(pItem);
    }
    if(((tListItem[a ].page & (1U << gCurPage)) != 0) &&  ((tListItem[a ].format & DRT_RENDER_ITEM) !=0 ) )
    {
      DrawItem(&tListItem[a], tListItem[a].txtColor, tListItem[a].bkColor);
    }
  }
}



/*


 full render



*/
void gui_render(void);
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
uint32_t  gui_check_ts_calibration(void);
uint32_t  gui_check_ts_calibration(void)
{
  uint32_t ts_status = TS_OK;
  ts_status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  if(ts_status != TS_OK)
  {
    return 0;
  }


  return 1;
}

/*


  Check the Touch screen state and position



*/
uint8_t gui_get_ts_state(uint32_t *x1, uint32_t *y1);
uint8_t gui_get_ts_state(uint32_t *x1, uint32_t *y1)
{

  gts_status = BSP_TS_GetState(&gts_State);
  if(gts_State.touchDetected)
  {
    /* One or dual touch have been detected          */
    /* Only take into account the first touch so far */

    /* Get X and Y position of the first touch post calibrated */
    if(x1)
    {
      *x1 = gts_State.touchX[0];
    }
    if(y1)
    {
      *y1 = gts_State.touchY[0];
    }
    return 1;
  }
  return 0;
}


/*


  Find an entry in the TS list and returns its index



*/

__STATIC_INLINE uint8_t gui_check_limit(int32_t base, int32_t base_ref, int32_t dx_ref)
{
  if(base < base_ref) return 0;
  if(base >base_ref+dx_ref) return 0;
  return 1;
}

static int32_t gui_get_ts_entry_index(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, uint32_t type);
static int32_t gui_get_ts_entry_index(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, uint32_t type)
{
  for(int32_t a = 0; a < gItemCount ; a++)
  {
    if(((tListItem[a ].format & type)!= 0)  && ((tListItem[a ].page & (1U << gCurPage)) != 0))
    {
      GUI_ITEM *pItem = &tListItem[a];
      if(gui_check_limit(x,pItem ->x,pItem ->dx) !=0 || gui_check_limit(x+dx,pItem ->x,pItem ->dx) != 0 )
      {
          if(gui_check_limit(y,pItem ->y,pItem ->dy) !=0 || gui_check_limit(y+dy,pItem ->y,pItem ->dy) != 0 )
          {
            return a;
          }
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

void gui_add_pos_entry(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, uint32_t code, void (*formatterCB)(struct gui_item *pItem), char_t *pText)
{
  if(gItemCount  >= sizeof(tListItem) / sizeof(tListItem[0]))
  {
    return ;
  }
  tListItem[gItemCount ].offX = 0;
  tListItem[gItemCount ].x = x;
  tListItem[gItemCount ].y = y;
  tListItem[gItemCount ].dx = dx;
  tListItem[gItemCount ].dy = dy;
  tListItem[gItemCount ].code = code;
  tListItem[gItemCount ].formatCB = formatterCB;
  tListItem[gItemCount ].pText = pText;
  tListItem[gItemCount ].page = PAGE_MAIN;
  gItemCount ++;
}




/*


  Render the feedback when the TS is pressed



*/
static void   gui_render_feedback(GUI_ITEM *pRect, uint32_t draw);
static void   gui_render_feedback(GUI_ITEM *pRect, uint32_t draw)
{
  if(draw)
  {
    uint32_t bkc = pRect->bkColor ;
    uint32_t txc = pRect->txtColor ;
    pRect->bkColor  = GUI_COLOR_BLACK;
    pRect->txtColor = GUI_COLOR_WHITE;
    gui_render();
    pRect->bkColor  = bkc;
    pRect->txtColor = txc;
  }
  else
  {
    gui_render();
  }
}



/*


  returns the last delay of the TS



*/
uint32_t gui_get_ts_delay(void);
uint32_t gui_get_ts_delay(void)
{
  return gLastTsDelay;
}




/*


  check and manage the touch screen , return index



*/
int32_t gui_get_ts_key(uint32_t feedback);
int32_t gui_get_ts_key(uint32_t feedback)
{
  uint32_t x1, y1;
  uint32_t dx, dy;

  dx = 1;
  dy = 1;

  if(!gui_get_ts_state(&x1, &y1))
  {
    return -1;
  }
  gui_inflat_rect(15, 15, (int32_t *)(uint32_t)&x1, (int32_t *)(uint32_t)&y1, &dx, &dy);
  int32_t index =  gui_get_ts_entry_index(x1, y1, dx, dy, DRT_TS_ITEM);
  if(index == -1)
  {
    return -1;
  }
  uint32_t tickstart = osKernelSysTick();
  if(feedback)
  {
    gui_render_feedback(&tListItem[index], 1);
  }
  while(gui_get_ts_state(&x1, &y1))
  {
    osDelay(10);
  }
  gLastTsDelay = osKernelSysTick() - tickstart;
  if(feedback)
  {
    gui_render_feedback(&tListItem[index], 0);
  }
  return index;
}




/*


  render text position only ( for debug )



*/
void gui_render_pos_entry(void);
void gui_render_pos_entry(void)
{
  for(int32_t a = 0; a < gItemCount ; a++)
  {
    if((tListItem[a ].format & DRT_RENDER_ITEM) != 0)
    {
      BSP_LCD_SetTextColor(GUI_COLOR_TS);
      BSP_LCD_DrawRect(tListItem[a ].x, tListItem[a ].y, tListItem[a].dx, tListItem[a].dy);
    }
  }
}


/*


  render touch screen   position only  ( for debug)



*/

void gui_render_ts_entry(void);
void gui_render_ts_entry(void)
{
  for(int32_t a = 0; a < gItemCount ; a++)
  {
    if((tListItem[a ].format & DRT_TS_ITEM)!= 0)
    {
      BSP_LCD_SetTextColor(GUI_COLOR_TS);
      BSP_LCD_DrawRect(tListItem[a ].x, tListItem[a ].y, tListItem[a].dx, tListItem[a].dy);
    }
  }
}


/* Return the pos id index in the list */

static int32_t  gui_get_pos_entry_index(uint32_t code)
{
  for(int32_t a = 0; a < gItemCount ; a++)
  {
    if(tListItem[a].code == code)
    {
      return a;
    }
  }
  return -1;
}

/* Return the post id rect */


uint32_t  gui_get_pos_entry(uint32_t code, uint32_t *x, uint32_t *y, uint32_t *dx, uint32_t *dy)
{
  for(int32_t a = 0; a < gItemCount ; a++)
  {
    if(tListItem[a].code == code)
    {
      if(x)
      {
        *x = tListItem[a].x;
      }
      if(y)
      {
        *y = tListItem[a].y;
      }
      if(dx)
      {
        *dx = tListItem[a].dx;
      }
      if(dy)
      {
        *dy = tListItem[a].dy;
      }
      return 1;

    }
  }
  return 0;
}

/*


  RAZ Position TS



*/
void gui_pos_clear_entry(void);
void gui_pos_clear_entry(void)
{
  /* Initialize default values */
  memset(tListItem, 0, sizeof(tListItem));
  for(int32_t a = 0; a < sizeof(tListItem) / sizeof(tListItem[0]) ; a++)
  {
    tListItem[a].format   = DRT_HCENTER | DRT_VCENTER  | DRT_RENDER_ITEM;
    tListItem[a].pFont    = GUI_FONT_BIG;
    tListItem[a].txtColor = GUI_COLOR_BLACK;
    tListItem[a].bkColor  = GUI_COLOR_WHITE;
  }
  gItemCount = 0;
}


static void gui_render_text_pos(uint32_t pos, char_t *pText, uint32_t txtColor, uint32_t bkColor)
{
  int32_t index = gui_get_pos_entry_index(pos);
  if(index  != -1)
  {
    GUI_ITEM *pPos = &tListItem[index];
    BSP_LCD_SetFont(pPos->pFont);
    BSP_LCD_SetTextColor(pPos->bkColor);
    BSP_LCD_FillRect(pPos->x, pPos->y, pPos->dx, pPos->dy);
    BSP_LCD_SetTextColor(pPos->txtColor);
    BSP_LCD_SetBackColor(pPos->bkColor);
    gui_render_rect_text_entry((char_t *)pText, pPos->x + pPos->offX, pPos->y, pPos->dx, pPos->dy, pPos->format);
  }
}


void gui_swap_buffer(void)
{
  uint32_t writeBuff = LCD_SwapBuffer();
  LCD_WaitVSync();
  BSP_LCD_SetFBStarAdress(0, writeBuff );

}



/*


  Initialize the GUI LCD etc...
  We assumes the Initialize is done in the platform Initialize


*/
static void gui_init(void);
static void gui_init(void)
{
  /* Initialize 1 layer */
  BSP_LCD_Clear(GUI_BACKROUND_COLOR);
  BSP_LCD_SetFBStarAdress(0, LCD_FB_START_ADDRESS );
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAt(0, (uint16_t)BSP_LCD_GetYSize() / 2,(uint8_t *)(uint32_t) "STVS4A Booting...", CENTER_MODE);
  gui_swap_buffer();
  gui_check_ts_calibration();
  gui_set_full_clip();

  /* Because cos() and sin() are not HW , it is done by SW */
  /* So to optimize the bean forming rendering we pre compute cos and sin in an array */
  for(int32_t a = 0; a < 360 ; a++)
  {
    tSinus[a]   = (float_t)(double_t)sin(a * (3.14 / 180.0));
    tCosinus[a] = (float_t)(double_t)cos(a * (3.14 / 180.0));
  }

  /* Init the item control positions */
  gui_pos_clear_entry();
  gCurPage = 1;
  /* Cpuload */
  gui_add_pos_entry(20, 150, 200, 100, POS_VERSION, gui_formatter_version, 0);
  gui_add_pos_entry(20, 250, 320, 80, POS_SYS_INFO, gui_formatter_sys_info, 0);
  gui_add_pos_entry(20, 250+80, 320, 110, POS_BUFFER_INFO, gui_formatter_buff_info, 0);

  /* Product initiator type */
  gui_add_pos_entry(20, 260, 200, 60, POS_INITIATOR, gui_formatter_initiator, 0);

  if(gNbPages > 1)
  {
    gui_add_pos_entry(610, 225 + 115 + 100 - 60, 180, 50, POS_NEXT_PAGE, gui_formatter_next_page, 0);
  }
  /* Define logos & heart */
  gui_add_pos_entry(5, 5, 198, 96, POS_LOGO_ST, gui_formatter_logoST, 0);
  gui_add_pos_entry(5 + 160, 5 + 10, 198, 96, POS_HEART, gui_formatter_heart, 0);

#ifdef PORTING_LAYER_AFE  
  gui_add_pos_entry(800 - 128 - 5, 5, 198, 96, POS_LOGO_SENSORY, gui_formatter_sensoryST, 0);
  gCurPage++;
#endif  

  /* Text zone */
  gui_add_pos_entry(239, 85 - 70, 350, 50, POS_IP, gui_formatter_ip, 0);
  gui_add_pos_entry(239, 85, 350, 50, POS_STATE, gui_formatter_state, 0);
  gui_add_pos_entry(238, 85 + 70, 350, 50, POS_TIME, gui_formatter_time, 0);
  gui_add_pos_entry(238, 225, 350, 100, POS_ALARM, gui_formatter_alarm, 0);
  gui_add_pos_entry(238, 225 + 115, 350, 100, POS_PLAYER, gui_formatter_player, 0);

  /* Define test zone */
  gui_add_pos_entry(238, 225, 350, 215, POS_INFO_TEST, gui_formatter_info_test, 0);
  gui_add_pos_entry(610, 120, 180, 50, POS_START_STOP_TEST, gui_formatter_btn_test, 0);
#ifdef AVS_USE_DEBUG  
  gui_add_pos_entry(610, 120 + 2 * 70, 180, 50, POS_NETWORK_SIM, gui_formatter_network_sim, 0);
  gui_add_pos_entry(610, 120 + 1 * 70, 180, 50, POS_CONNECT, gui_formatter_connect_test, 0);
#endif  


  /* Front end option */
  gui_add_pos_entry(239, 85 - 70, 350, 50, POS_AFE_HEADER, gui_text_afe_option, "Audio Front End");
  gui_add_pos_entry(238 + 20, 120, 280, 256, POS_BF_LOC, gui_bf_formatter, 0);
  gui_add_pos_entry(20, 150, 200, 100, POS_AFE_INFO, gui_afe_info_formatter, 0);
  bGuiInitialized = 1;
}

/* Idle task that render periodically the UI */
static void service_gui_task(const void *pCookie);
static void service_gui_task(const void *pCookie)
{
  char_t tMessage[100];

  while(1)
  {
    if(hInstance != 0)
    {
      if(AVS_Get_State(hInstance) == AVS_STATE_WAIT_START)
      {
        break;
      }
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



  if(strstr(sInstanceFactory.portingAudioName, "afe") != 0)
  {
    gNbPages = 2;
    gCurPage = 1;
  }

  if(strstr(sInstanceFactory.portingAudioName, "afe") != 0)
  {
    gNbPages = 3;
    gCurPage = 1;
  }



  while(1)
  {
    gui_render();
    int32_t nitem;
    if((nitem = gui_get_ts_key(1)) != -1)
    {
      gui_process_key(nitem);
    }

    osDelay(GUI_REFRESH_IDLE_PERIOD);
  }
}



/*

    Process touch screen keys

*/


static void gui_process_key(uint32_t index)
{

  AVS_ASSERT(index < gItemCount);

  if(tListItem[index].code == POS_INITIATOR)
  {
    gui_change_avs_initiator_type();
  }

  if(tListItem[index].code  == POS_CONNECT)
  {
    uint32_t state = AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, 2, 0);
    if(state )
    {
      AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, FALSE, 0);
    }
    else
    {
      AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, TRUE, 0);
    }
    osDelay(500);

  }

  if(tListItem[index].code  == POS_NETWORK_SIM)
  {
    if(service_endurance_enable_network_sim(3))
    {
      service_endurance_enable_network_sim(0);
    }
    else
    {
      service_endurance_enable_network_sim(1);
    }
  }

  if(tListItem[index].code  == POS_START_STOP_TEST)
  {
    if(service_endurance_get_state())
    {
      service_endurance_stop();
    }
    else
    {
      service_endurance_start();
    }
    osDelay(1000);
  }


  if(tListItem[index].code  == POS_LOGO_ST)
  {
#ifdef AVS_USE_LEAK_DETECTOR
      CheckLeaks();
#endif
    osDelay(1000);
  }


  if(tListItem[index].code  == POS_NEXT_PAGE)
  {
    gCurPage++;
    if(gNbPages)
    {
      gCurPage = gCurPage % gNbPages;
    }
    else
    {
      gCurPage=0;
    }
  }




}


/*

 create a GUI service.. Init the LCD using the BSP , create a thread with an idle priority in order to make the GUI less intrusif



*/

AVS_Result service_gui_create(void)
{

  gui_init();

  hTask = task_Create(TASK_NAME_GUI,service_gui_task,NULL,TASK_STACK_GUI,TASK_NAME_PRIORITY_GUI);
  if(hTask  == 0)
  {
    AVS_TRACE_ERROR("Create task %s", TASK_NAME_GUI);
    return AVS_ERROR;
  }
  return AVS_OK;
}


AVS_Result service_gui_delete(void);
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
  if(evt == EVT_BEANFORMING_DIR)
  {
    gLastBfDir = pparam;
  }

  int32_t index = findTextEntry(tStateTextList, evt);
  if(index  != -1)
  {

    /* PLAYER_PERIOD_STOP is sent too often */
    if(evt != ((AVS_Event)EVT_PLAYER_STOPPED))
    {
      gAppState.evt = evt;
      gAppState.evt_param = pparam;
      AVS_Get_Sync_Time(handle, &gAppState.lastEvtTime);
    }
  }
  return AVS_OK;
}


