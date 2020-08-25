#ifndef __avs_board_f7__
#define __avs_board_f7__
#include "avs_misra.h"

#if defined(USE_STM32F769I_DISCO) ||  defined(USE_STM32769I_DISCOVERY)
#include "stm32f7xx.h"
#include "stm32f769xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f769i_discovery.h"
#include "stm32f769i_discovery_audio.h"
#include "stm32f769i_discovery_lcd.h"
#include "stm32f769i_discovery_ts.h"

#define   BOARD_DTCM_SIZE       128     /* DTCM size in KB */
#define   BOARD_PRAM_SIZE       147     /* POOL RAM  size in KB */
#define   BOARD_NCACHED_SIZE    0       /* UNCACHED RAM  size in KB */
#define   BOARD_RAMDTCM_BASE    RAMDTCM_BASE

#elif defined(USE_STM32F769I_EVAL)

#include "stm32f7xx.h"
#include "stm32f769xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f769i_eval.h"
#include "stm32f769i_eval_audio.h"
#include "stm32f769i_eval_lcd.h"
#include "stm32f769i_eval_ts.h"

#define   BOARD_DTCM_SIZE       128     /* DTCM size in KB */
#define   BOARD_PRAM_SIZE       147     /* POOL RAM  size in KB */
#define   BOARD_NCACHED_SIZE    0       /* UNCACHED RAM  size in KB */
#define   BOARD_RAMDTCM_BASE    RAMDTCM_BASE

#endif

#include "stm32f7xx.h"
#include "stm32f746xx.h"
#include "stm32f7xx_hal.h"
//#include "stm32f769i_discovery.h"
//#include "stm32f769i_discovery_audio.h"
//#include "stm32f769i_discovery_lcd.h"
//#include "stm32f769i_discovery_ts.h"

#define   BOARD_DTCM_SIZE       1     /* DTCM size in KB */
#define   BOARD_PRAM_SIZE       1     /* POOL RAM  size in KB */
#define   BOARD_NCACHED_SIZE    0       /* UNCACHED RAM  size in KB */
#define   BOARD_RAMDTCM_BASE    RAMDTCM_BASE



#endif

