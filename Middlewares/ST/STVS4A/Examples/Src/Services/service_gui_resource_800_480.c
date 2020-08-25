/**
******************************************************************************
* @file    service_gui_resource_800_480.c
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   graphic  user interface item placement
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


static gui_resource_t tRessource[]=
{
  {GUI_ITEM_NEXT_PAGE_ST,{610, 225-10 + 115 + 100 - 60+50, 180, 50},GUI_FONT_MEDIUM},
  {GUI_ITEM_LOGO_ST,{5, 5, 198, 96},0},
  {GUI_ITEM_LOGO_SENSORY,{800 - 128 - 5, 5, 198, 96},0},
  {GUI_ITEM_HEART,{5 + 160, 5 + 10, 198, 96},0},
  {GUI_ITEM_IP,{239, 85 - 70, 350, 50},0},
  {GUI_ITEM_STATE,{239, 85, 350, 50},0},
  {GUI_ITEM_TIME,{238, 85 + 70, 350, 50},0},
  
  /* Info */
  
  {GUI_ITEM_PLAYER,{238, 225 + 115, 350, 100},GUI_FONT_MEDIUM},
  {GUI_ITEM_ALARM,{238, 225, 350, 100},GUI_FONT_MEDIUM},
  {GUI_ITEM_INITIATOR,{20, 260, 200, 60},GUI_FONT_MEDIUM},
  
  /* test */
  
  {GUI_ITEM_VERSION,{20, 150, 200, 100},0},
  {GUI_ITEM_SYS_INFO,{20, 250, 320, 80},0},
  {GUI_ITEM_BUFF_INFO,{20, 250+80, 320, 110},0},
  
  {GUI_ITEM_INFO_TEST,{238, 225, 350, 215},GUI_FONT_MEDIUM},
  {GUI_ITEM_BNT_TEST,{610, 120, 180, 50},GUI_FONT_MEDIUM},
  
  {GUI_ITEM_NETWORK_BAD_SIM,{610, 120 + 2 * 70, 180, 50},GUI_FONT_MEDIUM},
  {GUI_ITEM_NETWORK_LINK_SIM,{610, 120 + 1 * 70, 180, 50},GUI_FONT_MEDIUM},
  
  
  /* wifi */
  
  
  
  {GUI_ITEM_WIFI_LIST_SSID,{20, 100, 500, 250},0},
  {GUI_ITEM_WIFI_SSID_NAME_STATIC,{20, 350+10, 100, 50},0},
  {GUI_ITEM_WIFI_SSID_PASS_STATIC,{20, 350+60, 100, 50},0},
  
  {GUI_ITEM_WIFI_SSID_NAME,{20+100, 350+10, 400, 50},0},
  {GUI_ITEM_WIFI_SSID_PASS,{20+100, 350+60, 400, 50},0},
  
  {GUI_ITEM_WIFI_RECONNECT,{550, 100, 230, 80},0},
  {GUI_ITEM_WIFI_SCAN,{550, 100+1*90, 230, 80},0},
  {GUI_ITEM_WIFI_FLASH_ESP,{550, 100+2*90, 230, 80},0},
  {GUI_ITEM_WIFI_CONNECT_STATUS,{530, 100+3*90, 260, 30},0},
  
  
  /* Flash esp */
 
  {GUI_ITEM_WIFI_FLASH_ESP_IMAGE,{5, 240, 256, 219},0},
  {GUI_ITEM_WIFI_FLASH_ESP_REBOOT,{300, 240, 230, 80},0},
  {GUI_ITEM_WIFI_FLASH_ESP_STAT_STATIC,{300, 480-60, 800-330, 50},0},
  {GUI_ITEM_WIFI_FLASH_ESP_HELP_STATIC,{5, 5, 800-10, 210},0},
  
  /* Language selection */
  {GUI_ITEM_LANGUAGE_TITLE,{240, 15, 320, 30},0},
  {GUI_ITEM_APPLY_LANG,{610, 420-30-80, 180, 80},0},
  {GUI_ITEM_LIST_LANG,{20, 60, 500, 480-2*50},0},

  {0}
};

__weak gui_resource_t *gui_get_res(gui_item_id_t id)
{
  for(uint32_t a = 0; tRessource[a].id ; a++)
  {
    if(tRessource[a].id ==  id) 
    {
      return &tRessource[a];
    }
  }
  return 0;
}



