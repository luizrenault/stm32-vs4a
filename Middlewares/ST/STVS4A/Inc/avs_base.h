/**
******************************************************************************
* @file    avs_base.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   AVS SDK type definition
*
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


#ifndef _avs_base_
#define _avs_base_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>
/*!

Defines some groups for the auto-documentation Doxygen

@defgroup api stvs4a api 
@name api Describes STVS4A API functions
@defgroup type stvs4a types
@name type Describes STVS4A types
@defgroup enum stvs4a enums
@name enum Describes STVS4A enumerations
@defgroup macro stvs4a macros
@name macro Describes STVS4A macros
@defgroup struct stvs4a struct
@name macro Describes STVS4A strutcs 
*/  
  
  

typedef  void *   AVS_Handle;   /*!< generic instance handle @ingroup type*/
typedef  void *   AVS_HStream;  /*!< stream instance handle @ingroup type*/
typedef  void *   AVS_HTls;     /*!< TLS stream instance handle @ingroup type*/
typedef  time_t   AVS_TIME;     /*!< Avs time value equivalent to epoch time or time_t @ingroup type*/

#ifndef HAVE_AVS_FLOAT_T
typedef  float    avs_float_t; /*!< Float overload @ingroup type*/
#define HAVE_AVS_FLOAT_T
#endif

#ifndef HAVE_AVS_DOUBLE_T
typedef  double    avs_double_t; /*!< Double overload @ingroup type*/
#define HAVE_AVS_DOUBLE_T
#endif


#ifndef HAVE_CHAR_T
typedef   char char_t; /*!< Char overload @ingroup type*/
#define HAVE_CHAR_T
#endif


#ifndef HAVE_LONG_T
typedef long long_t; /*!< Long overload @ingroup type*/
#define HAVE_LONG_T
#endif


#ifndef HAVE_ULONG_T
typedef unsigned long ulong_t; /*!< ULong overload @ingroup type*/
#define HAVE_ULONG_T
#endif

#ifndef HAVE_TRUE
#define TRUE    (1) /*!< True overload @ingroup type*/
#define HAVE_TRUE
#endif

#ifndef HAVE_FALSE
#define FALSE   (0) /*!< False overload @ingroup type*/
#define HAVE_FALSE
#endif



/* ! SDK Version number */
#define AVS_VERSION         "v1.1.2"            /*!< SDK version number @ingroup macro*/
/*!< NULL */                                            
#define AVS_NULL  0                             /*!< null overload @ingroup macro*/
#define AVS_UNUSED(x) ((void)(x))               /*!< prevent unused warning @ingroup macro*/        

/*!

@brief  AVS errors
@ingroup enum
*/
typedef enum avs_result
{
  /* Generic errors */
    AVS_ERROR                 = 0   /*!< Error */
  , AVS_OK                    = 1   /*!< Success */
  , AVS_NOT_IMPL              = 2   /*!< The function or a function used by the caller is not implemented */
  , AVS_PARAM_INCORECT        = 3   /*!< Parameter is incorrect */
  , AVS_BUSY                  = 4   /*!< Device Busy */
  , AVS_NOT_SYNC              = 5   /*!< Time not sync with NTP server */
  , AVS_EOF                   = 6   /*!< End of file/stream/etc... */
  , AVS_EVT_HANDLED           = 7   /*!< The event is handled returning, AVS_EVT_HANDLED means that the engine will stop the event processing and assume that the user caught and has processed the data */
  , AVS_NOT_AVAILABLE         = 8   /*!< Resource not available */
  , AVS_TIMEOUT               = 9   /*!< The Operation has produced a time-out */
  , AVS_RETRY                 = 10  /*!< The Operation has produced a retry */
  , AVS_OVERFLOW              = 11  /*!< The Operation has produced an overflow or the operation is not completed */

  /* Errors when calling a specific library */
  , AVS_ERROR_RTOS            = 20  /*!< Error when calling an RTOS service */
  , AVS_ERROR_TLS_LIB         = 21  /*!< Error in the TLS software stack */
  , AVS_ERROR_TCP             = 22  /*!< Error when calling a TCP/IP service */
  , AVS_END_RESULT            = 23  /*!< End of Result code*/
} AVS_Result;

/*!

@brief Debug message level
@ingroup enum

*/
typedef enum avs_trace_lvl
{
    AVS_TRACE_LVL_ALL =           (0xFFFF)   /*!< All traces messages */
  , AVS_TRACE_LVL_MUTE =          (0)        /*!< No Trace messages */
  , AVS_TRACE_LVL_ERROR =         (1 << 0)   /*!< Trace error   messages */
  , AVS_TRACE_LVL_WARNING =       (1 << 1)   /*!< Trace warning messages */
  , AVS_TRACE_LVL_NETWORK =       (1 << 2)   /*!< Trace network messages */
  , AVS_TRACE_LVL_INFO =          (1 << 3)   /*!< Trace info    messages */
  , AVS_TRACE_LVL_TEST =          (1 << 4)   /*!< Trace test    messages */
  , AVS_TRACE_LVL_DEBUG =         (1 << 5)   /*!< Trace debug    messages */
  , AVS_TRACE_LVL_JSON_FORMATED = (1 << 6)   /*!< If AVS_TRACE_LVL_JSON is selected the JSON string will be re-formatted to be human readable */
  , AVS_TRACE_LVL_JSON =          (1 << 7)   /*!< Trace JSON   messages */
  , AVS_TRACE_LVL_DIRECTIVE =     (1 << 8)   /*!< Trace directive   messages */
  , AVS_TRACE_LVL_USER_1 =        (1 << 9)   /*!< Trace user 1 messages */
  , AVS_TRACE_LVL_USER_2 =        (1 << 10)  /*!< Trace user 2 messages */
  , AVS_TRACE_LVL_USER_3 =        (1 << 11)  /*!< Trace user 3 messages */
  , AVS_TRACE_LVL_DEFAULT = ( AVS_TRACE_LVL_DEBUG | AVS_TRACE_LVL_NETWORK | AVS_TRACE_LVL_ERROR | AVS_TRACE_LVL_WARNING | AVS_TRACE_LVL_INFO) /*!< Default traces */
} AVS_Trace_Lvl;


#if defined(AVS_USE_DEBUG)
#if !defined(AVS_NO_ASSERT)
#define AVS_ASSERT(a)     AVS_Assert(((int32_t)(a)!= 0) ,#a,__LINE__,__FILE__) /*!< Raise an Assert if the condition is false, Remove the code in release mode @ingroup macro*/
#define AVS_VERIFY(a)     AVS_Assert(((int32_t)(a)!= 0),#a,__LINE__,__FILE__) /*!< Raise an Assert if the condition is false, Keep  the code in release mode @ingroup macro*/
#else
#define AVS_ASSERT(a)           
#define AVS_VERIFY(a)     a
#endif
#define AVS_TRACE_INFO(...)             AVS_Trace((uint32_t)(AVS_TRACE_LVL_INFO),(NULL),0,__VA_ARGS__)                    /*!< Print an info message @ingroup macro*/
#define AVS_TRACE_ERROR(...)            AVS_Trace((uint32_t)AVS_TRACE_LVL_ERROR,(__FILE__),(__LINE__),__VA_ARGS__)        /*!< Print a error message @ingroup macro*/
#define AVS_TRACE_WARNING(...)          AVS_Trace((uint32_t)AVS_TRACE_LVL_WARNING,(__FILE__),(__LINE__),__VA_ARGS__)      /*!< Print a warning  message @ingroup macro*/
#define AVS_TRACE_DEBUG(...)            AVS_Trace((uint32_t)AVS_TRACE_LVL_DEBUG,(__FILE__),(__LINE__),__VA_ARGS__)        /*!< Print a debug message, the trace is removed in release mode @ingroup macro*/
#define AVS_TRACE_NETWORK(...)          AVS_Trace((uint32_t)AVS_TRACE_LVL_NETWORK,(__FILE__),(__LINE__),__VA_ARGS__)         /*!< Print a test message, the trace is removed in release mode  @ingroup macro*/
#define AVS_TRACE_USER(lvl,...)         AVS_Trace((uint32_t)lvl,NULL,0,__VA_ARGS__)                                       /*!< Print a user  message, the trace is removed in release mode  @ingroup macro*/
#define AVS_PRINTF(lvl,...)             AVS_Printf((uint32_t)lvl,__VA_ARGS__)                                             /*!< Just a printf ( without CR) at the end , the trace is removed in release mode @ingroup macro*/
#define AVS_PRINT_STRING(lvl,string)    AVS_Puts((uint32_t)(lvl),(string))                                                /*!< Just a print string ( without string size limitation), the trace is removed in release mode  @ingroup macro*/
#define AVS_TRACE_DUMP(lvl,title,ptr,size)    AVS_Dump((uint32_t)(lvl),(title),(ptr),(size))                              /*!< Dump a buffer on the console, the trace is removed in release mode  @ingroup macro*/
#else
#define AVS_ASSERT(...)                 ((void)0)  
#define AVS_VERIFY(a)                   a
#define AVS_TRACE_INFO(...)             AVS_Trace((uint32_t)(AVS_TRACE_LVL_INFO),(NULL),0,__VA_ARGS__)
#define AVS_TRACE_ERROR(...)            AVS_Trace((uint32_t)AVS_TRACE_LVL_ERROR,(__FILE__),(__LINE__),__VA_ARGS__)
#define AVS_TRACE_WARNING(...)          ((void)0)
#define AVS_TRACE_DEBUG(...)            ((void)0)
#define AVS_TRACE_NETWORK(...)          ((void)0)
#define AVS_TRACE_USER(lvl,...)         ((void)0)
#define AVS_PRINTF(lvl,...)             ((void)0)
#define AVS_PRINT_STRING(lvl,string)    ((void)0)
#define AVS_TRACE_DUMP(lvl,ptr,size)    ((void)0)

#endif


/*!

@brief Generic exception notification
2 bits LSB = leds 
bit 0  : Read
bit 1  : green
value >>2 = lighting delay 
@ingroup enum
*/
typedef enum avs_signal_exception
{
  AVS_SIGNAL_EXCEPTION_HARD_FAULT = (0 << 2) + 3,
  AVS_SIGNAL_EXCEPTION_ASSERT = (3 << 2) + 1,
  AVS_SIGNAL_EXCEPTION_MEM_CURRUPTION = (3 << 2) + 2,
  AVS_SIGNAL_EXCEPTION_GENERAL_ERROR = (5 << 2) + 1,
} AVS_Signal_Exception;


AVS_Result AVS_Init(void);                                                                               /*!< @ingroup api */
AVS_Result AVS_Term(void);                                                                               /*!< @ingroup api */
void       AVS_Trace(uint32_t level, const char_t *pFile, uint32_t  line, ...);                          /*!< @ingroup api */
void       AVS_Dump(uint32_t level,const char_t *pTitle,void *pData,uint32_t size);                      /*!< @ingroup api */
void       AVS_Printf(uint32_t level, const char_t *pFormat, ...);                                       /*!< @ingroup api */
void       AVS_Puts(uint32_t level, const char_t *pString);                                              /*!< @ingroup api */
void       AVS_Assert(int32_t  eval, const char_t *peval, uint32_t line, const char_t*file);            /*!< @ingroup api */
uint32_t   AVS_Set_Debug_Level(uint32_t level);                                                          /*!< @ingroup api */
void       AVS_Signal_Exeception( AVS_Handle hInstance, AVS_Signal_Exception level);                     /*!< @ingroup api */
#ifdef __cplusplus
};
#endif

#endif /* _avs_base_ */
