/**
  ******************************************************************************
  * @file    assetConfig.h
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    26-January-20167
  * @brief   asset configuration
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
* This software component is licensed by ST under Ultimate Liberty license
* SLA0044, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0044
*
  ******************************************************************************
  */
#ifndef __ASSERTCONFIG_H__
#define __ASSERTCONFIG_H__

/* Define to prevent recursive inclusion -------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This File is intended to be included keil .sct file
 * The purposse is to avoid definition of empty sections when assets are already  flashed to QSPI (AVS_USE_QSPI_FLASHED  defined)
 * In this case (empty section) the Keil linker would complain
 */

/* Flag AVS_USE_QSPI_FLASHED shall be defined at projetc levels*/
/*  defined   :  Assets are already flashed in qspi ( save time to build and  flash the product). */
/*  undefined :  Generate exe with asset in qspi sections. */

/* Flag AVS_CREATE_QSPI_SECTION is used for Keil .sct file only */
/*  defined   :  only if AVS_USE_QSPI_FLASHED is undefined  */
/*  undefined :  only if AVS_USE_QSPI_FLASHED is defined . */
#if defined ( __CC_ARM   ) 
/*#define AVS_CREATE_QSPI_SECTION */
#endif

#define AVS_USE_QSPI             /* set the flag if the assets must be placed in qspi ( product with full assets) if unset the assets will go in flash ( hex must be flash once witgh STutility)*/

#ifdef __cplusplus
}
#endif
#endif /* __ASSERTCONFIG_H__ */

