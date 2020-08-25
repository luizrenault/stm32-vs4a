/**
******************************************************************************
* @file    service_gui_page_test.c
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


static uint32_t gui_proc_buff_info(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t gui_proc_buff_info(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  if(message == GUI_MSG_INIT)
  {
    pItem->page   = PAGE_TEST;
    pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
    pItem->pFont  = GUI_FONT_SMALL;
  }

  if(message == GUI_MSG_RENDER)
  {
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
               "Net   RX   : %05.1f Kb/s\r"
               "Net   TX   : %05.1f Kb/s\r"
               "Reco  Buf  : %03lu%%:%03lu%%\r"
               "Synth Buf  : %03lu%%:%03lu%%\r"
               "Aux   Buf  : %03lu%%:%03lu%%\r"
               "MP3   Buf  : %03lu%%:%03lu%%\r"
               "Play  Buf  : %03lu%%\r"
                 ,
               refRxBw,
               refTxBw,
               sysinfo.recoBufferPercent,sysinfo.recoPeakPercent,
               sysinfo.synthBufferPercent,sysinfo.synthPeakPercent,
               sysinfo.auxBufferPercent,sysinfo.auxPeakPercent,
               sysinfo.mp3BufferPercent,sysinfo.mp3PeakPercent,
               service_player_get_buffer_percent(hInstance)
                 );

      pItem->pText = gAppState.tBuffInfo;
    }
  }
 return gui_proc_default(pItem,message,lparam);
}




static uint32_t gui_proc_sys_info(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t  gui_proc_sys_info(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  if(message == GUI_MSG_INIT)
  {
    pItem->page   = PAGE_TEST;
    pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
    pItem->pFont  = GUI_FONT_SMALL;
  }
  if(message == GUI_MSG_RENDER)
  {
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
      if((sysinfo.memTotalSpace ==0) && (sysinfo.memFreeSpace ==0))
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
  return gui_proc_default(pItem,message,lparam);
}



/*

 render chip version SDK version and App version


*/
static uint32_t gui_proc_version(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t gui_proc_version(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  if(message == GUI_MSG_INIT)
  {
    pItem->page =  PAGE_TEST;
  }
  if(message == GUI_MSG_RENDER)
  {
    if(pItem->pText == 0)
    {
      pItem->format = DRT_HUP | DRT_VLF | DRT_RENDER_ITEM;
      pItem->pFont   = GUI_FONT_SMALL;
      snprintf(gAppState.tVersionString, sizeof(gAppState.tVersionString), "Chip : %s\rSdk  : %s\rApp  : %s\rAudio: %s\rNet  : %s\rBuild: %s\rToolC: %s", sInstanceFactory.cpuID, AVS_VERSION, APP_VERSION, sInstanceFactory.portingAudioName, sInstanceFactory.netSupportName, __DATE__, sInstanceFactory.toolChainID);
      pItem->pText = gAppState.tVersionString;
    }
  }
 return gui_proc_default(pItem,message,lparam);
}



/*

 format the box Test info


*/

static uint32_t  gui_proc_btn_test(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t   gui_proc_btn_test(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{

  if(message== GUI_MSG_BUTTON_CLICK)
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



  if(message == GUI_MSG_INIT)
  {
    pItem->page = PAGE_TEST;
    pItem->format |= DRT_TS_ITEM | DRT_FRAME;
  }
  if(message == GUI_MSG_INIT)
  {
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
 return gui_proc_default(pItem,message,lparam);
}

static uint32_t  gui_proc_info_test(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t  gui_proc_info_test(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  static char_t sTime[20] = "none";
  static char_t sTestName[20] = "none";
  if(message == GUI_MSG_INIT)
  {
    pItem->page = PAGE_TEST;

  }

  if(message == GUI_MSG_RENDER)
  {

    service_endurance_get_current_test_name(sTestName, sizeof(sTestName));
    pItem->offX     = 20;
    pItem->format = DRT_HCENTER | DRT_VLF | DRT_RENDER_ITEM | DRT_FRAME;
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
  }

return gui_proc_default(pItem,message,lparam);

}

static uint32_t gui_proc_bad_network_sim(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t gui_proc_bad_network_sim(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  
  
  if(message== GUI_MSG_BUTTON_CLICK)
  {
    uint32_t state = service_endurance_enable_network_sim(3);
    if(state )
    {
      service_endurance_enable_network_sim(0);
    }
    else
    {
      service_endurance_enable_network_sim(1);
    }
    osDelay(500);
  }

    


  if(message == GUI_MSG_INIT)
  {
    pItem->page   = PAGE_TEST;
    pItem->format |= DRT_FRAME;
    pItem->format |= DRT_TS_ITEM;
    pItem->txtColor = GUI_COLOR_BLACK;
  }
  if(message == GUI_MSG_RENDER)
  {
    if(service_endurance_enable_network_sim(3))
    {
      pItem->pText  = "Network Sim On";
      pItem->txtColor  = GUI_COLOR_RED;
    }
    else
    {
      pItem->txtColor  = GUI_COLOR_BLACK;
      pItem->pText  = "Network Sim Off";
    }
  }
 return gui_proc_default(pItem,message,lparam);
}

/*

 proc Network simulation state


*/
static uint32_t gui_proc_network_link_sim(gui_item_t *pItem,uint32_t message,uint32_t lparam);
static uint32_t gui_proc_network_link_sim(gui_item_t *pItem,uint32_t message,uint32_t lparam)
{
  if(message == GUI_MSG_INIT)
  {
    pItem->page   = PAGE_TEST;
    pItem->format |= DRT_FRAME;
    pItem->format |= DRT_TS_ITEM;
    pItem->txtColor  = GUI_COLOR_BLACK;
  }
  
  if(message== GUI_MSG_BUTTON_CLICK)
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
  
  
  if(message == GUI_MSG_RENDER)
  {
    if(AVS_Ioctl(hInstance, AVS_NETWORK_SIMULATE_DISCONNECT, 2, 0) == TRUE)
    {
      pItem->txtColor  = GUI_COLOR_RED;
      pItem->pText = "Set Link down";
    }
    else
    {
      pItem->txtColor  = GUI_COLOR_BLACK;
      pItem->pText = "Set Link up";
    }
  }
 return gui_proc_default(pItem,message,lparam);
}


AVS_Result service_gui_event_page_test_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);
AVS_Result service_gui_event_page_test_cb(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam)
{
  return AVS_OK;
}
void gui_add_page_test(void);
void gui_add_page_test(void)
{
  gui_add_item_res(GUI_ITEM_VERSION, gui_proc_version, 0);
  gui_add_item_res(GUI_ITEM_SYS_INFO, gui_proc_sys_info, 0);
  gui_add_item_res(GUI_ITEM_BUFF_INFO, gui_proc_buff_info, 0);
  
  
  
  gui_add_item_res(GUI_ITEM_INFO_TEST, gui_proc_info_test, 0);
  gui_add_item_res(GUI_ITEM_BNT_TEST, gui_proc_btn_test, 0);
  
#ifdef AVS_USE_DEBUG
  gui_add_item_res(GUI_ITEM_NETWORK_LINK_SIM, gui_proc_network_link_sim, 0);
  gui_add_item_res(GUI_ITEM_NETWORK_BAD_SIM, gui_proc_bad_network_sim, 0);
#endif
  
}
