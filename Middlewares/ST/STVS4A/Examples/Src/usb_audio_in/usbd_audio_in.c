#include "avs_misra.h"
MISRAC_DISABLE
#include "usbd_devices.h"
#include "usbd_audio_ex.h"
#include "usbd_audio_in.h"
#include "cmsis_os.h"
MISRAC_ENABLE

#include "avs.h"

#define NB_BUFFER_SAMPLE        2000
#define DEFAULT_VOLUME_DB_256   0
#define AUDIO_IN_EP             0x81U
#define CONTROLE_UNIT_ID       0x15U

typedef enum
{
  SESSION_OFF,
  SESSION_INITIALIZED,
  SESSION_STARTED,
  SESSION_STOPPED,
  SESSION_ERROR
} instance_state_t;



static int8_t  AUDIO_USB_Init(USBD_AUDIO_FunctionDescriptionfTypeDef* audio_function, uint32_t private_data);
static int8_t  AUDIO_USB_DeInit(USBD_AUDIO_FunctionDescriptionfTypeDef* audio_function, uint32_t private_data);
static int8_t  AUDIO_USB_GetState(uint32_t private_data);
static int8_t  AUDIO_USB_GetConfigDesc (uint8_t ** pdata, uint16_t * psize, uint32_t private_data);
__ALIGN_BEGIN static uint8_t USBD_AUDIO_ConfigDescriptor[200] __ALIGN_END;
static uint32_t  USBD_AUDIO_ConfigDescriptorCount;
static record_instance_t  gRecordInstance;




/* exported  variable ---------------------------------------------------------*/

USBD_AUDIO_InterfaceCallbacksfTypeDef audio_class_interface =
{
  .Init = AUDIO_USB_Init,
  .DeInit = AUDIO_USB_DeInit,
  .GetConfigDesc = AUDIO_USB_GetConfigDesc,
  .GetState = AUDIO_USB_GetState,
  .private_data = 0
};






static uint16_t  USB_AUDIO_Streaming_IO_GetMaxPacketLength(uint32_t node_handle);
static uint16_t  USB_AUDIO_Streaming_IO_GetMaxPacketLength(uint32_t node_handle)
{
  
  record_instance_t *pInstance = (record_instance_t*)node_handle;
  
  return pInstance ->max_packet_length;
}
static int8_t delegation_GetMute(uint16_t channel, uint8_t* mute, uint32_t node_handle);
static int8_t delegation_GetMute(uint16_t channel, uint8_t* mute, uint32_t node_handle)
{
  record_instance_t *pInstance = (record_instance_t*)node_handle;
  if((pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_GET_MUTE, (uint32_t)mute, 0);
  }
  return 0;
}
static int8_t delegation_SetMute(uint16_t channel, uint8_t mute, uint32_t node_handle);
static int8_t delegation_SetMute(uint16_t channel, uint8_t mute, uint32_t node_handle)
{
  record_instance_t *pInstance = (record_instance_t*)node_handle;
  if((pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_SET_MUTE, (uint32_t)mute, 0);
  }
  
  return 0;
}
static int8_t delegation_GetCurVolume(uint16_t channel, uint16_t* volume, uint32_t node_handle);
static int8_t delegation_GetCurVolume(uint16_t channel, uint16_t* volume, uint32_t node_handle)
{
  record_instance_t *pInstance = (record_instance_t*)node_handle;
  if((pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_GET_VOLUME, (uint32_t)volume, 0);
  }
  return 0;
}
static int8_t delegation_SetCurVolume(uint16_t channel, uint16_t volume, uint32_t node_handle);
static int8_t delegation_SetCurVolume(uint16_t channel, uint16_t volume, uint32_t node_handle)
{
  record_instance_t *pInstance = (record_instance_t*)node_handle;
  if((pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_SET_VOLUME, volume, 0);
  }
  
  
  return 0;
}
#ifdef USBD_SUPPORT_AUDIO_MULTI_FREQUENCES

static int8_t  delegation_GetCurFrequence(uint32_t* freq, uint32_t node_handle);
static int8_t  delegation_GetCurFrequence(uint32_t* freq, uint32_t node_handle)
{
  record_instance_t *pInstance = (record_instance_t*)node_handle;
  *freq = pInstance->iFreq;
  if((pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_GET_FREQ, (uint32_t)freq, 0);
  }
  return 0;
}

static uint32_t  findFreq(record_instance_t *pInstance ,uint32_t freq);
static uint32_t  findFreq(record_instance_t *pInstance ,uint32_t freq)
{
  
  if(freq <= pInstance->tFreq[0])
 {
   return pInstance->tFreq[0];
 }
 else
 {
    if(freq <= pInstance->tFreq[pInstance->nbFreq-1])
   {
     return pInstance->tFreq[pInstance->nbFreq-1];
   }
   else
   {
     for(int32_t i = 1; i<pInstance->nbFreq; i++)
     {
       if(freq >= pInstance->tFreq[i])
       {
         return ((pInstance->tFreq[i-1] - freq )<= ( freq - pInstance->tFreq[i]))?
           pInstance->tFreq[i-1] : pInstance->tFreq[i];
       }
     }
   }
 }
 
return 0; 
}

static int8_t  delegation_SetCurFrequence(uint32_t freq,uint8_t*  restart_req , uint32_t node_handle);
static int8_t  delegation_SetCurFrequence(uint32_t freq,uint8_t*  restart_req , uint32_t node_handle)
{
  record_instance_t *pInstance = (record_instance_t*)node_handle;
  
  if((pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_SET_FREQ, (uint32_t)freq, (uint32_t)pInstance->iFreq );
  }
  uint32_t best_freq = findFreq(pInstance,freq);
  if(pInstance->iFreq  == best_freq)
  {
    pInstance->iFreq = best_freq;
    *restart_req = 0;
    return 0;
  }
  else
  {
    *restart_req = 1;
    pInstance->iFreq = freq;
  }
  pInstance->packet_length     = AUDIO_MS_PACKET_SIZE(pInstance->iFreq, pInstance->iChan, 2);
  pInstance->max_packet_length = AUDIO_MS_MAX_PACKET_SIZE(pInstance->iFreq, pInstance->iChan, 2);
  
  return 0;
}
#endif /*USBD_SUPPORT_AUDIO_MULTI_FREQUENCES*/


/**
* @brief  AUDIO_Recording_GetState
*         recording SA interface status
* @param  session_handle: session
* @retval 0 if no error
*/
static int8_t  AUDIO_Recording_GetState(uint32_t session_handle);
static int8_t  AUDIO_Recording_GetState(uint32_t session_handle)
{
  return 0;
}
static int8_t  USB_AUDIO_Streaming_IO_GetState(uint32_t node_handle);
static int8_t  USB_AUDIO_Streaming_IO_GetState(uint32_t node_handle)
{
  return 0;
}


/**
* @brief  AUDIO_Recording_SetAS_Alternate
*        SA interface set alternate callback
* @param  alternate:
* @param  session_handle: session
* @retval  : 0 if no error
*/
static int8_t  AUDIO_Recording_SetAS_Alternate( uint8_t alternate, uint32_t session_handle );
static int8_t  AUDIO_Recording_SetAS_Alternate( uint8_t alternate, uint32_t session_handle )
{
  record_instance_t *pInstance = (record_instance_t*)session_handle;
  
  if(alternate  ==  0)
  {
    if(pInstance->alternate != 0)
    {
      pInstance->recState = 0;
      pInstance->alternate = 0;
    }
  }
  else
  {
    if(pInstance->alternate  ==  0)
    {
      /* @ADD how to define thershold */
      
      pInstance->recState = 1;
      pInstance->alternate = alternate;
    }
  }
  return 0;
}

static uint8_t *getBufferDelegation(uint32_t privatedata, uint16_t* packet_length);
static uint8_t *getBufferDelegation(uint32_t privatedata, uint16_t* packet_length)
{
  record_instance_t *pInstance = (record_instance_t *)privatedata;
  if((pInstance->pInterface) && (pInstance->pInterface->getBuffer))
  {
    return pInstance->pInterface->getBuffer(pInstance, packet_length);
  }
  *packet_length  = 0;
  return 0;
}



static int8_t  recordInit(USBD_AUDIO_AS_InterfaceTypeDef* as_desc,  USBD_AUDIO_ControlTypeDef* controls_desc, uint8_t* control_count, uint32_t session_handle);
static int8_t  recordInit(USBD_AUDIO_AS_InterfaceTypeDef* as_desc,  USBD_AUDIO_ControlTypeDef* controls_desc, uint8_t* control_count, uint32_t session_handle)
{
  record_instance_t *pInstance = (record_instance_t *)session_handle;
  
  
  
  if((pInstance->pInterface) && (pInstance->pInterface->notifyEvt))
  {
    pInstance->pInterface->notifyEvt(pInstance, AUDIO_INIT, 0, 0);
  }
  
  *control_count = 1;
  
  
  /* create record output set data end point callbacks */
  
  as_desc->data_ep.ep_num = AUDIO_IN_EP;
  as_desc->data_ep.control_name_map = 0;
  as_desc->data_ep.control_selector_map = 0;
  as_desc->data_ep.private_data = (uint32_t)pInstance;
  as_desc->data_ep.DataReceived = 0;
  as_desc->data_ep.GetBuffer          = getBufferDelegation;
  as_desc->data_ep.GetMaxPacketLength = USB_AUDIO_Streaming_IO_GetMaxPacketLength;
  as_desc->data_ep.GetState           = USB_AUDIO_Streaming_IO_GetState;
#ifdef USBD_SUPPORT_AUDIO_MULTI_FREQUENCES  
  as_desc->data_ep.control_selector_map = 1; // multi frequ */
  as_desc->data_ep.control_cbk.GetCurFrequence = delegation_GetCurFrequence;
  as_desc->data_ep.control_cbk.SetCurFrequence = delegation_SetCurFrequence;
  as_desc->data_ep.control_cbk.MinFrequence = pInstance->tFreq[0];
  as_desc->data_ep.control_cbk.MaxFrequence = pInstance->tFreq[pInstance->nbFreq-1];
  as_desc->data_ep.control_cbk.ResFrequence = 1; 
#endif
  
  
  /* Create a minimum controle callback */
  
  pInstance->control.GetMute = delegation_GetMute;
  pInstance->control.SetMute = delegation_SetMute;
  pInstance->control.GetCurVolume = delegation_GetCurVolume;
  pInstance->control.SetCurVolume = delegation_SetCurVolume;
  VOLUME_DB_256_TO_USB(pInstance->control.MaxVolume, 8192);
  VOLUME_DB_256_TO_USB(pInstance->control.MinVolume, -8192);
  pInstance->control.ResVolume = 255;
  
  
  
  
  /* Create the control description and map it */
  
  controls_desc->id = CONTROLE_UNIT_ID;
  controls_desc->control_req_map = 0;
  controls_desc->control_selector_map = 3 ;
  controls_desc->type = USBD_AUDIO_CS_AC_SUBTYPE_FEATURE_UNIT;
  controls_desc->Callbacks.feature_control = &pInstance->control;
  controls_desc->private_data = (uint32_t)pInstance;
  
  /* set USB AUDIO class callbacks */
  
  as_desc->interface_num = USBD_AUDIO_CONFIG_RECORD_SA_INTERFACE;
  as_desc->alternate = 0;
  as_desc->max_alternate = 0x1;
  as_desc->private_data = session_handle;
  as_desc->SetAS_Alternate = AUDIO_Recording_SetAS_Alternate;
  as_desc->GetState = AUDIO_Recording_GetState;
  as_desc->SofReceived =  0;
  
  pInstance->state = SESSION_INITIALIZED;
  return 0;
}





/**
* @brief  AUDIO_USB_Init
*         Initializes the interface
* @param  audio_function: The audio function description
* @param  private_data:  for future usage
* @retval status
*/
static int8_t  AUDIO_USB_Init(USBD_AUDIO_FunctionDescriptionfTypeDef* audio_function, uint32_t private_data)
{
  int32_t i = 0, j = 0;
  uint8_t control_count = 0;
  if(gRecordInstance.hValid)
  {
    /* Initializes the USB record session */
    recordInit(&audio_function->as_interfaces[i], &(audio_function->controls[j]), &control_count, (uint32_t)&gRecordInstance);
    i++;
    j += control_count;
  }
  audio_function->as_interfaces_count = i;
  audio_function->control_count = j;
  return 0;
}

/**
* @brief  AUDIO_USB_DeInit
*         De-Initializes the interface
* @param  audio_function: The audio function description
* @param  private_data:  for future usage
* @retval status 0 if no error
*/
static int8_t  AUDIO_USB_DeInit(USBD_AUDIO_FunctionDescriptionfTypeDef* audio_function, uint32_t private_data)
{
  int32_t i = 0;
  if((gRecordInstance.pInterface) &&  (gRecordInstance.pInterface->notifyEvt))
  {
    gRecordInstance.pInterface->notifyEvt(&gRecordInstance, AUDIO_TERM, 0, 0);
  }
  
  audio_function->as_interfaces[i].alternate = 0;
  return 0;
}

/**
* @brief  AUDIO_USB_GetState
*         This function returns the USB Audio state
* @param  private_data:  for future usage
* @retval status
*/
static int8_t  AUDIO_USB_GetState(uint32_t private_data)
{
  return 0;
}

/**
* @brief  USB_AUDIO_GetConfigDescriptor
*         return configuration descriptor
* @param  desc
* @retval the configuration descriptor size
*/
uint16_t USB_AUDIO_GetConfigDescriptor(uint8_t **desc)
{
  if(desc)
  {
    *desc = USBD_AUDIO_ConfigDescriptor;
  }
  return USBD_AUDIO_ConfigDescriptorCount;
}
/**
* @brief  AUDIO_USB_GetConfigDesc
*         Initializes the interface
* @param  pdata: the returned configuration descriptor
* @param  psize:  configuration descriptor length
* @param  private_data:  for future usage
* @retval status
*/
static int8_t  AUDIO_USB_GetConfigDesc (uint8_t ** pdata, uint16_t * psize, uint32_t private_data)
{
  *psize =  USB_AUDIO_GetConfigDescriptor(pdata);
  return 0;
}

uint16_t USB_AUDIO_GetMaxPacketLen(void)
{
  return gRecordInstance.max_packet_length;
}



__weak void USBD_error_handler(void)
{
  while(1)
  {
  }
}

#define DESC_COPY_16(ptr,val)                       ((uint8_t *)(uint32_t)(ptr))[0]=(val)&0xFF;((uint8_t *)(uint32_t)(ptr))[(1)]=((val)>>8);ptr+=2
#define DESC_START_SECTION(handle,ptr)                (handle)=(uint32_t)(uint8_t *)(ptr)
#define DESC_SIZE_SECTION(handle,ptr)                 ((uint32_t)(uint8_t *)(ptr) - (handle))
#define DESC_PATCH_SECTION(ptr,val,off)               ((uint8_t *)(uint32_t)(ptr))[(off)]=(val)&0xFF;((uint8_t *)(uint32_t)(ptr))[(off+1)]=((val)>>8)
#define DESC_PATCH_SECTION_8(ptr,val,off)             ((uint8_t *)(uint32_t)(ptr))[(off)]=(val)&0xFF
#define DESC_END_SECTION(handle,ptr,off)              DESC_PATCH_SECTION((handle),DESC_SIZE_SECTION((handle),(ptr)),(off))
#define DESC_END_SECTION_8(handle,ptr,off)            DESC_PATCH_SECTION_8((handle),DESC_SIZE_SECTION((handle),(ptr)),(off))

void USBD_AUDIO_Init_Microphone_Descriptor(record_instance_t *pInstance,uint32_t *pFreqs,int32_t nbFreq);
void USBD_AUDIO_Init_Microphone_Descriptor(record_instance_t *pInstance,uint32_t *pFreqs,int32_t nbFreq)
{
  uint8_t *pDesc = USBD_AUDIO_ConfigDescriptor;
  uint32_t configDescInterfaceCount = 1;
  uint8_t Channels = pInstance->iChan;
  uint32_t  totalSize;
  uint32_t  totalClassAcSize;
  uint32_t  totalClassAc;
  uint32_t  totalFeatureUnit;
  uint32_t  totalFormatType;
  uint32_t  bDelay=1;
  
  uint8_t bTerminalID=0x11;
  uint8_t bSourceID=0x011;
  uint8_t bSourceID2=0x15;
  
  DESC_START_SECTION(totalSize,pDesc);
  /* Configuration 1  C.3.2 */
  *pDesc++ = 0x09;                                       /* bLength */
  *pDesc++ = 0x02;                                       /* bDescriptorType */
  *pDesc++ = 0 & 0xFF;                                  /* wTotalLength  writen later */
  *pDesc++ = 0 >> 8;
  *pDesc++ = 0x01 + configDescInterfaceCount;            /* bNumInterfaces */
  *pDesc++ = 0x01;                                       /* bConfigurationValue */
  *pDesc++ = 0x00;                                       /* iConfiguration */
  *pDesc++ = 0xC0;                                       /* bmAttributes  BUS Powred*/
  *pDesc++ = 0x32;                                       /* bMaxPower = 100 mA*/
  
  /* Standard AC Interface Descriptor: Audio control interface C.3.3.1*/
  
  *pDesc++ = 0x09;                                       /* USBD_AUDIO_STANDARD_INTERFACE_DESC_SIZEbLength */
  *pDesc++ = 0x04;                                       /* USB_DESC_TYPE_INTERFACE bDescriptorType */
  *pDesc++ = 0x00;                                       /* bInterfaceNumber */
  *pDesc++ = 0x00;                                       /* bAlternateSetting */
  *pDesc++ = 0x00;                                       /* bNumEndpoints */
  *pDesc++ = 0x01;                                       /* USBD_AUDIO_CLASS_CODE bInterfaceClass */
  *pDesc++ = 0x01;                                       /* USBD_AUDIO_INTERFACE_SUBCLASS_AUDIOCONTROL bInterfaceSubClass */
  *pDesc++ = 0x00;                                       /* USBD_AUDIO_INTERFACE_PROTOCOL_UNDEFINED bInterfaceProtocol */
  *pDesc++ = 0x00;                                       /* iInterface */
  /* 09 byte*/
  
  /* Class-Specific AC Interface Header Descriptor  4.3.2*/
  
  DESC_START_SECTION(totalClassAcSize,pDesc);
  
  DESC_START_SECTION(totalClassAc,pDesc);
  *pDesc++ = 0x08 + (configDescInterfaceCount); /*  USBD_AUDIO_AC_CS_INTERFACE_DESC_SIZE(CONFIG_DESCRIPTOR_AS_INTERFACES_COUNT);  bLength */
  *pDesc++ = 0x24;                                       /* USBD_AUDIO_DESC_TYPE_CS_INTERFACE bDescriptorType */
  *pDesc++ = 0x01;                                       /* USBD_AUDIO_CS_AC_SUBTYPE_HEADER; bDescriptorSubtype */
  DESC_COPY_16(pDesc,USBD_AUDIO_ADC_BCD);
  *pDesc++ = 0 & 0xFF;                                  /* wTotalLength*/
  *pDesc++ = 0>> 8;
  *pDesc++ = configDescInterfaceCount;                  /*  streaming interface count */
  *pDesc++ = 0x01;                                      /* USBD_AUDIO_CONFIG_RECORD_SA_INTERFACE Audio streaming interface for  record  */
  DESC_END_SECTION_8(totalClassAc,pDesc,0);
  /* 10 byte*/
  
  
  /* USB record input : MIC */
  /* Input Terminal Descriptor 4.3.2.1*/
  *pDesc++ = 0x0C;                                         /* Size of this descriptor, in bytes: 12 */
  *pDesc++ = 0x24;                                         /* CS_INTERFACE descriptor type. */
  *pDesc++ = 0x02;                                         /* INPUT_TERMINAL descriptor subtype. */
  *pDesc++ = bTerminalID;                                 /* USB_AUDIO_CONFIG_RECORD_TERMINAL_INPUT_ID bTerminalID */
  DESC_COPY_16(pDesc,USBD_AUDIO_TERMINAL_I_MICROPHONE);   /* wTerminalType MICROPHONE  0x201*/
  *pDesc++ = 0x00;                                       /* bAssocTerminal */
  *pDesc++ = Channels;                                   /* Number of logical output channels in the Terminal's output audio channel cluster. */
  if(Channels == 1)
  {
    DESC_COPY_16(pDesc,1);                               /* Describes the spatial location of the logical channels*/
  }
  else
  {
    DESC_COPY_16(pDesc,3);                               /* Describes the spatial location of the logical channels*/
  }
  *pDesc++ = 0x00;                                       /* Index of a string descriptor, describing the name of the first logical channel. */
  *pDesc++ = 0x00;                                       /* Index of a string descriptor, describing the Input Terminal. */
  /* 12 byte*/
  
  /* USB Record control feature */
  /* Feature Unit Descriptor 4.3.2.5*/
  DESC_START_SECTION(totalFeatureUnit,pDesc);
  *pDesc++ = (0x07 + (((2) + 1) * (1)));                  /* USBD_AUDIO_FEATURE_UNIT_DESC_SIZE(2,1);         bLength */
  *pDesc++ = 0x24;                                         /* CS_INTERFACE descriptor type */
  *pDesc++ = 0x06;                                         /* SELECTOR_UNIT descriptor subtype*/
  *pDesc++ = CONTROLE_UNIT_ID;                                      /*Constant uniquely identifying the Unit within the audio function. This value is used in all requests to address this Unit.*/
  *pDesc++ = bSourceID ;                                  /* USB_AUDIO_CONFIG_RECORD_TERMINAL_INPUT_ID;    bSourceID */
  *pDesc++ = 0x01;                                         /* bControlSize */
  *pDesc++ = 0x03;                                         /* MUTE=1|VOLUME=2;bmaControls(0) */
  for(int32_t a = 0; a < Channels  ;a++)
  {
    *pDesc++ = 0x00;                                         /* bmaControls(1) */
  }
  
  /*  *pDesc++ = 0x00; */                                  /* bmaControls(2) */
  *pDesc++ = 0x00;                                         /* iTerminal */
  DESC_END_SECTION_8(totalFeatureUnit,pDesc,0);
  /* 10 byte*/
  
  /*USB IN: Record output*/
  /* Output Terminal Descriptor  4.3.2.2 */
  *pDesc++ = 0x09;                                        /* USBD_AUDIO_OUTPUT_TERMINAL_DESC_SIZE; bLength */
  *pDesc++ = 0x24;                                        /* USBD_AUDIO_DESC_TYPE_CS_INTERFACE;bDescriptorType */
  *pDesc++ = 0x03;                                        /* USBD_AUDIO_CS_AC_SUBTYPE_OUTPUT_TERMINAL;bDescriptorSubtype */
  *pDesc++ = 0x013;                                       /* USB_AUDIO_CONFIG_RECORD_TERMINAL_OUTPUT_ID;bTerminalID */
  DESC_COPY_16(pDesc,USBD_AUDIO_TERMINAL_IO_USB_STREAMING); /* wTerminalType USBD_AUDIO_TERMINAL_IO_USB_STREAMING   *pDesc++=0x0101 */
  *pDesc++ = 0x00;                                       /* bAssocTerminal */
  *pDesc++ = bSourceID2;                                 /* USB_AUDIO_CONFIG_RECORD_UNIT_FEATURE_ID;bSourceID */
  *pDesc++ = 0x00;                                       /* iTerminal */
  DESC_END_SECTION(totalClassAcSize,pDesc,5);
  
  /* 09 byte*/
  /* USB record Standard AS Interface Descriptor - Audio Streaming Zero Bandwith */
  /* Standard AS Interface Descriptor 4.5.1*/
  *pDesc++ = 0x09;                                       /*USBD_AUDIO_STANDARD_INTERFACE_DESC_SIZE;bLength */
  *pDesc++ = 0x04;                                        /*INTERFACE descriptor type*/
  *pDesc++ = 0x01;                                       /* Number of interface. A zero-based value identifying the index in the array of concurrent interfaces supported by this configuration. */
  *pDesc++ = 0x00;                                       /* bAlternateSetting */
  *pDesc++ = 0x00;                                       /* bNumEndpoints */
  *pDesc++ = 0x01;                                       /* USBD_AUDIO_CLASS_CODE;bInterfaceClass */
  *pDesc++ = 0x02;                                       /* USBD_AUDIO_INTERFACE_SUBCLASS_AUDIOSTREAMING;bInterfaceSubClass */
  *pDesc++ = 0x00;                                       /* USBD_AUDIO_INTERFACE_PROTOCOL_UNDEFINED;bInterfaceProtocol */
  *pDesc++ = 0x00;                                       /* iInterface */
  /* 09 byte*/
  
  /* USB record Standard AS Interface Descriptors - Audio streaming operational */
  /* Standard AS Interface Descriptor */
  *pDesc++ = 0x09;                                       /* USBD_AUDIO_STANDARD_INTERFACE_DESC_SIZE; bLength */
  *pDesc++ = 0x04;                                       /* INTERFACE descriptor type */
  *pDesc++ = 0x01;                                       /* Number of interface. A zero-based value identifying the index in the array of concurrent interfaces supported by this configuration.*/
  *pDesc++ = 0x01;                                       /* bAlternateSetting */
  *pDesc++ = 0x01;                                       /* bNumEndpoints */
  *pDesc++ = 0x01;                                       /* USBD_AUDIO_CLASS_CODE;bInterfaceClass */
  *pDesc++ = 0x02;                                       /* USBD_AUDIO_INTERFACE_SUBCLASS_AUDIOSTREAMING;bInterfaceSubClass */
  *pDesc++ = 0x00;                                       /* USBD_AUDIO_INTERFACE_PROTOCOL_UNDEFINED;bInterfaceProtocol */
  *pDesc++ = 0x00;                                       /* iInterface */
  /* 09 byte*/
  /* offset 0x48 */
  
  /*Class-Specific AS Interface Descriptor 4.5.2 */
  *pDesc++ = 0x07;                                       /* USBD_AUDIO_AS_CS_INTERFACE_DESC_SIZE;bLength */
  *pDesc++ = 0x24;                                       /* CS_INTERFACE descriptor type. */
  *pDesc++ = 0x01;                                       /* AS_GENERAL descriptor subtype. */
  *pDesc++ = 0x013;                                      /* The Terminal ID of the Terminal to which the endpoint of this interface is connected.*/
  *pDesc++ = bDelay;                                       /* Delay (d) introduced by the data path (see Section 3.4, "Inter Channel Synchronization"). Expressed in number of frames.*/
  DESC_COPY_16(pDesc,1);                                 /* (PCM) The Audio Data Format that has to be used to communicate with this interface.*/
  
  /* 07 byte*/
  /* USB Audio Type I Format descriptor B.3.4.2.1.3 */
  DESC_START_SECTION(totalFormatType,pDesc);
  *pDesc++ = 0x0B;                                       /*Size of this descriptor, in bytes.*/
  *pDesc++ = 0x24;                                       /* CS_INTERFACE descriptor*/
  *pDesc++ = 0x02;                                       /* FORMAT_TYPE subtype. */
  *pDesc++ = 0x01;                                       /* FORMAT_TYPE_I */
  *pDesc++ = Channels;                                   /* bNrChannels */
  *pDesc++ = 0x02;                                       /* Two bytes per audio subframe*/
  *pDesc++ = 0x10;                                       /* 16 bits per sample. */
  *pDesc++ = nbFreq;                                       /* n frequency supported*/
  for(uint32_t a = 0; a < nbFreq ; a++)
  {
    uint32_t freq = pFreqs[a];
    /* Audio sampling frequency coded on 3 bytes */
    *pDesc++ = (freq & 0xFF);
    *pDesc++ = ((freq>>8) & 0xFF);
    *pDesc++ = ((freq>>16) & 0xFF);
  }
  DESC_END_SECTION_8(totalFormatType,pDesc,0);
  
  /* USB record data ep  */
  /* Standard AS Isochronous Audio Data Endpoint Descriptor 4.6.1.1*/
  *pDesc++ = 0x09;                                       /* USBD_AUDIO_STANDARD_ENDPOINT_DESC_SIZE;bLength */
  *pDesc++ = 0x05;                                       /* ENDPOINT descriptor type */
  *pDesc++ = AUDIO_IN_EP;                                       /* The address of the endpoint on IN and EP 1 */
  *pDesc++ = 0x01;                                       /* Asynchronous */
  DESC_COPY_16(pDesc,pInstance->max_packet_length);      /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  *pDesc++ = 0x01;                                       /* Interval for polling endpoint for data transfers expressed in milliseconds. Must be set to 1.*/
  *pDesc++ = 0x00;                                       /* Reset to 0.*/
  *pDesc++ = 0x00;                                       /* bSynchAddress */
  /* 09 byte*/
  
  /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor 4.6.1.2*/
  
  *pDesc++ = 0x07;                                       /*USBD_AUDIO_SPECIFIC_DATA_ENDPOINT_DESC_SIZE;   bLength */
  *pDesc++ = 0x25;                                       /* CS_ENDPOINT descriptor type*/
  *pDesc++ = 0x01;                                       /* EP_GENERAL descriptor subtype*/
  *pDesc++ = 0x00;                                       /* A bit in the range D6..0 set to 1 indicates that the mentioned Control is supported by this endpoint. */
  *pDesc++ = 0x00;                                       /* Indicates the units used for the wLockDelay field: */
  *pDesc++ = 0x00;                                       /* Indicates the time it takes this endpoint to reliably lock its internal clock recovery circuitry. Units used depend on the valueof the bLockDelayUnits field.*/
  *pDesc++ = 0x00;
  DESC_END_SECTION(totalSize,pDesc,2 );
  USBD_AUDIO_ConfigDescriptorCount = DESC_SIZE_SECTION(totalSize,pDesc);
}


record_instance_t*  USBD_Audio_Input_Create(uint32_t *pFreq,uint32_t nbFreq,uint32_t ch, usbd_audio_in_if_t *pInterface)
{
  record_instance_t *pInstance = &gRecordInstance;
  memset(pInstance, 0, sizeof(*pInstance));
  if(nbFreq >= (sizeof(pInstance->tFreq)/sizeof(pInstance->tFreq[0])))  
  {
    return NULL;
  }
    
  for(uint32_t a = 0; a < nbFreq ; a++)
  {
    pInstance->tFreq[a] = pFreq[a];
  }
  pInstance->nbFreq = nbFreq;
  pInstance->iFreq = pInstance->tFreq[0];
  pInstance->iChan = ch;
  pInstance->hValid = 1;
  pInstance->pInterface = pInterface;
  pInstance->packet_length     = AUDIO_MS_PACKET_SIZE(pInstance->iFreq, pInstance->iChan, 2);
  pInstance->max_packet_length = AUDIO_MS_MAX_PACKET_SIZE(pInstance->tFreq[pInstance->nbFreq-1], pInstance->iChan, 2);
  USBD_AUDIO_Init_Microphone_Descriptor(pInstance,pInstance->tFreq,pInstance->nbFreq);
  return pInstance;
  
}
void usbd_assert(uint32_t cond,const char_t *pFile,uint32_t line);
void usbd_assert(uint32_t cond,const char_t *pFile,uint32_t line)
{
  if(cond ==0)
  {
    printf("USB Assert %s:%d \r\n",__FILE__,__LINE__);
    while(1)
    {}
  }
}


