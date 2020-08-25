/**
******************************************************************************
* @file    avs.h
* @author  MCD Application Team
* @version V1.0.0
* @date    20-02-2018
* @brief   AVS SDK definition
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



#ifndef _avs_
#define _avs_

#ifdef __cplusplus
extern "C" {
#endif

#include "avs_base.h"
#include "avs_porting.h"

/*!
@brief PARAM values accepted by the IOctl AVS_IOCTL_CAPTURE_FEED
@ingroup enum
  
*/
typedef enum t_avs_capture_option
{
  AVS_CAPTURE_WAKEUP = 0     /*!< Samples coming from the microphone goes to the ASR wake up recognition */
  , AVS_CAPTURE_AVS = 1      /*!< Samples coming from the microphone goes to the Alexa server */
} AVS_Capture_Option;

#define  AVS_FEED_INPUT_CALLBACK   0    /*!< Hook the Feed input @ingroup macro  */
#define  AVS_FEED_OUTPUT_CALLBACK  1    /*!< Hook the Feed output @ingroup macro  */
#define  AVS_FEED_AEC_CALLBACK     2    /*!< Hook the Feed AEC @ingroup macro  */

#define AVS_DEFAULT_STATE    ((AVS_Instance_State)AVS_STATE_IDLE)

/*!
@brief audio driver callback 
@ingroup type
This callback is used in combination with IOCTL  AVS_IOCTL_AUDIO_FEED_CALLBACK
@param pIdStream Pointer on stream ID (microphones)
@param pData    Pointer on sample (one sample per channel)
@param NbOutChannels Channel number in the stream
@param NbOutPcmSamples Total PCM sample (i.e. Sample * Channel)
*/

typedef void (*AVS_Audio_Feed_CB)(uint32_t *pIdStream, void *pData, uint16_t *NbOutChannels, uint16_t *NbOutPcmSamples);



/*!

@brief Standard ioctl available for all platforms. Some extra ioctl may be added in the avs_porting.h header.
@ingroup enum

*/

typedef enum t_avs_codectl
{
  AVS_INSTANCE_IOCTL_START                                    /*!< IOCTL for instance */
  , AVS_TEST_IOCTL                                            /*!< User test */  
  , AVS_AUDIO_IOCTL_START                                     /*!< IOCTL for audio */
  , AVS_IOCTL_CAPTURE_FEED                                    /*!< Select the microphone sample feed, use the optional parameter as AVS_capture_option_t. returns AVS_OK or AVS_ERROR */
  , AVS_IOCTL_INJECT_MICROPHONE                               /*!< For tests or debug purpose, inject a wave file that overload samples coming from the physical microphone , the parameter LPARAM is a pointer on a .wav file, the parameter WPARAM is TRUE to wait until the wave is played */
  , AVS_IOCTL_MUTE_MICROPHONE                                 /*!< Mute completely the microphone (used in combination with AVS_IOCTL_INJECT_MICROPHONE for test) WPARAM TRUE or FALSE */
  , AVS_IOCTL_AUDIO_FEED_CALLBACK                             /*!< Allow to set an Audio callback to catch a feed, the callback depends on the audio porting layer. WPARAM is the callback id and LPARAM is callback or NULL (returns AVS_OK or AVS_NOT_IMPL) */
  , AVS_IOCTL_AUDIO_VOLUME                                    /*!< Set the audio volume WPARAM = 0 if absolute and WPARAM = 1, if relative WPARAM = 3. Returns the current value LPARAM the volume 0 to 100 or -100 to 100 */
  , AVS_IOCTL_AUDIO_ENABLE_ECHO_CANCELLATION                  /*!< Enable/Disable echo cancellation processing WPARAM is TRUE or FALSE or 3 (returns the state or AVS_NOT_IMPL) */
  , AVS_IOCTL_AUDIO_ENABLE_BEANFORMING                        /*!< Enable/Disable bean forming processing WPARAM is TRUE or FALSE or 3 (returns the state or AVS_NOT_IMPL) */
  , AVS_IOCTL_AUDIO_ENABLE_WORD_SPOTTING_WAKEUP               /*!< Enable/Disable Word spotting wakeup     WPARAM is TRUE or FALSE or the value 3 (returns the state or AVS_OK or AVS_NOT_IMPL) */
  , AVS_IOCTL_AUDIO_GET_INFO                                  /*!< return a string with some detailed info WPARAM, buffer size, LPARAM: pointer on a buffer (returns AVS_OK or AVS_NOT_IMPL) */
  , AVS_AUDIO_IOCTL_END                                       /*!< End of IOCTL audio */

  , AVS_NETWORK_IOCTL_START                                   /*!< IOCTL for network */
  , AVS_IOCTL_WIFI_POST_SCAN                                   /*!< returns the list of wifi spot in a string, wparam = max size string ,lparam pointer on the string */
  , AVS_IOCTL_WIFI_CONNECTED_SPOT                             /*!< returns the name of wifi spot in a string, wparam = max size string ,lparam pointer on the string */
  , AVS_IOCTL_WIFI_CONNECT                                     /*!< Connect/disconnect the Spot using persistant parameters WPARAM = TRUE or FALSE */
  , AVS_NETWORK_SIMULATE_DISCONNECT                           /*!< For debug purpose simulate a wire or WIFI disconnection */
  , AVS_NETWORK_RENEW_TOKEN                                   /*!< For debug purpose, for to renew the http2 token. WPARAM = 0: returns the delay before to renew, WPARAM = 1: Forces and waits the token renew, returns success: TRUE, Error: FALSE */
  , AVS_NETWORK_COUNTS                                        /*!< Returns the number of bytes RX and TX by the network */
  , AVS_AUDIO_NETWORK_IOCTL_END                               /*!< End of IOCTL Network */

  , AVS_SYS_IOCTL_START                                        /*!< IOCTL for system */
  , AVS_SYS_SET_LED                                           /*!< for debug purpose set a led state, WPARAM led number from 0 to N, LAPARAM = 0=OFF or 1=ON */
  , AVS_SYS_IOCTL_END                                         /*!< End of IOCTL sys */
} AVS_CodeCtl;



/*!

@brief Standard events available for all platform. Some extra events may be added by the user application
@ingroup enum
*/

typedef enum t_avs_event
{
    EVT_STATE                         /*!< Notify the state machine PARAM 0 = REC 1=PLAY 2=WAIT WAKEUP */
  , EVT_AUTH                          /*!< Notify the check authentication: PARAM =1: OK PARAM =0: fails */
  , EVT_WAKEUP                        /*!< Notify a wake up word spotting */
  , EVT_REBOOT                        /*!< Notify the system will  reboot*/
  , EVT_RESTART                       /*!< Notify the system is restarting */
  , EVT_WAIT_NETWORK_IP               /*!< Notify the DHCP is waiting for an IP */
  , EVT_NETWORK_IP_OK                 /*!< Notify the system has an IP */
  , EVT_CONNECTION_TASK_START         /*!< Notify connection task start */
  , EVT_CONNECTION_TASK_DYING         /*!< Notify connection task dies */
  , EVT_DOWNSTREAM_TASK_START         /*!< Notify downstream task start */
  , EVT_DOWNSTREAM_TASK_DYING         /*!< Notify downstream task dies */
  , EVT_STATE_TASK_START              /*!< Notify state task start */
  , EVT_STATE_TASK_DYING              /*!< Notify state task dies */
  , EVT_MP3_TASK_START                /*!< Notify mp3 task start */
  , EVT_MP3_TASK_DYING                /*!< Notify mp3 task dies */
  , EVT_AUDIO_SYNTHESIZER_TASK_START  /*!< Notify synthesizer player starting */
  , EVT_AUDIO_SYNTHESIZER_TASK_DYING  /*!< Notify synthesizer player starting */
  , EVT_AUDIO_RECOGNIZER_TASK_START   /*!< Notify recognizer starting */
  , EVT_AUDIO_RECOGNIZER_TASK_DYING   /*!< Notify recognizer starting */
  , EVT_REFRESH_TOKEN_TASK_START      /*!< Notify Refresh Token task */
  , EVT_REFRESH_TOKEN_TASK_DYING      /*!< Notify Refresh Token task */
  , EVT_SYNC_TIME                     /*!< Notify get sync time success */
  , EVT_WAIT_TOKEN                    /*!< Notify awaiting token */
  , EVT_VALID_TOKEN                   /*!< Notify got a token valid */
  , EVT_RENEW_ACCESS_TOKEN            /*!< Notify the token is renewed via “grant_me”*/
  , EVT_CHANGE_STATE                  /*!< Notify change state */
  , EVT_START_TLS                     /*!< Notify start TLS */
  , EVT_READ_TOKEN                    /*!< Notify read token from the CB PARAM = error */
  , EVT_WRITE_TOKEN                   /*!< Notify read token from the CB PARAM = error */
  , EVT_SEND_SYNCHRO_STATE            /*!< Notify send a Sync-State PARAM = error */
  , EVT_SEND_PING                     /*!< Notify send a Sync-State PARAM = error */
  , EVT_RESET_HTTP                    /*!< Notify the start of the http2 connection */
  , EVT_HOSTNAME_RESOLVED             /*!< Notify http2 resolve amazon */
  , EVT_HTTP2_CONNECTING              /*!< Notify http2 waits connection */
  , EVT_HTTP2_CONNECTED               /*!< Notify http2 connected */
  , EVT_OPEN_TLS                      /*!< Notify a connection TLS PARAM = AVS_OK if success or AVS_ERROR */
  , EVT_WIFI_CONNECTED                /*!< Notify a connection wifi with the station PARAM = AVS_OK if success or AVS_ERROR */
  , EVT_WIFI_SCAN_RESULT              /*!< Notify a wifi scan result PARAM = a string or 0*/
  , EVT_TLS_CERT_VERIFY_FAILED        /*!< Notify a connection TLS error due to a certification issue */
  , EVT_START_REC                     /*!< Notify the start the AVS listening */
  , EVT_STOP_REC                      /*!< Notify the stop of the AVS listening */
  , EVT_START_SPEAK                   /*!< Notify starts AVS speaking */
  , EVT_STOP_SPEAK                    /*!< Notify starts AVS speaking */
  , EVT_STOP_CAPTURE                  /*!< Notify a directive stop capture */
  , EVT_CHANGE_VOLUME                 /*!< Notify the change of the global volume PARAM = volume from 0 to 100 */
  , EVT_CHANGE_MUTE                   /*!< Notify the change of the mute state PARAM = TRUE (muted) or FALSE */
  , EVT_CHANGE_MP3_FREQUENCY          /*!< Notify the MP3 frequency is changed */
  , EVT_BEANFORMING_DIR               /*!< Notify a Word spotting detection with a specific direction of the audio front end support this feature */
  , EVT_UPDATE_SYNCHRO                /*!< Notify a sync message is about to be send the user can update the JSON message according to its status PARAM is the json_t * structure */
  , EVT_REENTRANT_END
  , EVT_NO_REENTRANT_START
  , EVT_DIRECTIVE                     /*!< Notify a general directive PARAM is the json_t *structure */
  , EVT_DIRECTIVE_SYNTHESIZER         /*!< Notify a synthesizer directive PARAM is the json_t * structure */
  , EVT_DIRECTIVE_NOTIFICATION        /*!< Notify a notification directive PARAM is the json_t * structure */
  , EVT_DIRECTIVE_SPEECH_RECOGNIZER   /*!< Notify a SpeechRecognizer directive PARAM is the json_t * structure */
  , EVT_DIRECTIVE_AUDIO_PLAYER        /*!< Notify a AudioPlayer directive PARAM is the json_t * structure */
  , EVT_DIRECTIVE_SPEAKER             /*!< Notify a Speaker directive PARAM is the json_t * structure */
  , EVT_DIRECTIVE_SYSTEM              /*!< Notify a System directive PARAM is the json_t * structure */
  , EVT_DIRECTIVE_ALERT               /*!< Notify a Alert directive PARAM is the json_t * structure */
  , EVT_NO_REENTRANT_END              /*!< End of the Re-entrant events */
  , EVT_USER                          /*!< Start of the user events */
} AVS_Event;

/*!

@brief SDK engine states
@ingroup enum

STVS4A can change its state according the directive sent by the server or the application itself.
The main state is IDLE, when the engine waits for an event but the state could change for capture or speak. Each state change is notified by an event EVT_CHANGE_STATE

*/

typedef enum t_avs_instance_state
{
   AVS_STATE_WAIT_START                 /*!< AVS is not started yet */
  , AVS_STATE_RESTART                   /*!< AVS is restarting */
  , AVS_STATE_IDLE                      /*!< AVS is IDLE and is waiting for a StartCapture */
  , AVS_STATE_START_CAPTURE             /*!< AVS starts AVS listening */
  , AVS_STATE_STOP_CAPTURE              /*!< AVS stops  the AVS listening state*/
  , AVS_STATE_BUSY                      /*!< AVS playing or other task */
} AVS_Instance_State;

/*!

@brief  Defines how an interaction with AVS was initiated.
@ingroup enum

The imitator must be set, according to the interaction model but also with the profile AVS_Profile_t

*/

typedef enum t_avs_initiator
{
  AVS_INITIATOR_DEFAULT                   /*!< 0 the SDK will set the default value */
  , AVS_INITIATOR_PUSH_TO_TALK            /*!< The user should push the button until the end of the record, the stop must be manual by change the state */
  , AVS_INITIATOR_TAP_TO_TALK             /*!< The user should Tape the button to start the record, the StopCapture will come for Alexa */
  , AVS_INITIATOR_VOICE_INITIATED         /*!< Initiated by the use of a wake word and terminated when a StopCapture directive is received */
} AVS_Initiator;


/*!
@brief Defines a factors acoustic, environments and use cases.
@ingroup enum

The profile must be set according to the interaction model but also with the profile AVS_Profile_t

*/
typedef enum t_avs_profile
{
  AVS_PROFILE_DEFAULT        /*!< 0 the SDK will set the default value */
  , AVS_PROFILE_CLOSE_TALK    /*!< Speaker from 0 to 2.5 ft */
  , AVS_PROFILE_NEAR_FIELD    /*!< Speaker from 0 to 5 ft */
  , AVS_PROFILE_FAR_FIELD     /*!< Speaker from 0 to 20+ ft */
} AVS_Profile;




/*!
@brief Actions for Persistent items 
@ingroup enum
Actions for Persistent  see also  AVS_Persist_CB
*/
typedef enum t_avs_persist_action
{
  
    AVS_PERSIST_GET,             /*!< Get the item  */
    AVS_PERSIST_GET_POINTER      /*!< return a  direct pointer on the item */
  , AVS_PERSIST_SET              /*!< Set the Item */
} AVS_Persist_Action;


/*!
@brief Persistent items 
@ingroup enum
Persistent items see also  AVS_Persist_CB
*/
typedef enum t_avs_persist_item
{
  
  AVS_PERSIST_ITEM_TOKEN,           /*!< Persistent  Token item */
  AVS_PERSIST_ITEM_CA_ROOT,         /*!< Persistent  Root certificat  */
  AVS_PERSIST_ITEM_WIFI,            /*!< Persistent  SIID and PASS*/
  AVS_PERSIST_ITEM_END
} Avs_Persist_Item;





/*!
@brief User event callback
@ingroup type
@param  handle   Instance handle
@param  pCookie  User cookie you can set in the factory
@param  evt      Event fired
@param  pparam   Event parameter
@return Error code
*/

typedef AVS_Result(*AVS_Event_CB)(AVS_Handle handle, uint32_t pCookie, AVS_Event evt, uint32_t  pparam);

/*!
@brief Peristant storage READ/WRITE callback
@ingroup type
@param  action   Persistent  action
@param  item     Persistent   item
@param  pItem    Item pointer 
@param  itemSize Item size
Item itemSize 
@return Error code or a pointer 
*/

typedef uint32_t (*AVS_Persist_CB)(AVS_Persist_Action action,Avs_Persist_Item item ,char_t *pItem,uint32_t itemSize);

/*!
@brief write token callback
@ingroup type
@param  pToken   Token string buffer to read
@param  maxSize  Token buffer  size
*/

typedef AVS_Result(*AVS_Token_Read_CB)(char_t *pToken, uint32_t maxSize);


/*!
@brief Instance Configuration
@ingroup struct

AVS_Instance_Factory allows to configure the AVS instance using several parameters.
A zero in a field, means that the engine will use a default parameter.
After the call to AVS_CreateInstance , all fields with 0 will be updated with a default value used by the engine.
Notice, some fields are mandatory to connect the server (client Id, client Secret, product Id, serial number)

*/
typedef struct t_avs_instance_factory
{
  char_t *      clientId;                        /*!< Alexa client id example: "amzn1.application-oa2-client.5c3418b8c0b4451f9fc64b7ab7116e1f" */
  char_t *      clientSecret;                    /*!< Alexa client secret example: "053250532b68a77b3bd247f2e0a19afb36e1607aa1b5f17e17fc648cb1d3e22d" */
  char_t *      productId;                       /*!< Alexa product id   example: "basic_Avs_demo" */
  char_t *      serialNumber;                    /*!< Product serial number. Must be different from one board to another. Example: "12345874755" */
  uint8_t        macAddr[6];                      /*!< Network mac address or 0 */
  char_t        *hostName;                       /*!< Network host name: example "myDemo" */
  char_t        *redirectUri;                    /*!< Uri redirection requested to Amazon server */
  uint32_t      used_dhcp;                       /*!< Network enable DHCP true or false, if false the PAI will use ip manual setting (ipV4_host_addr) */
  uint32_t      use_mdns_responder;              /*!< Network enable the MDNS responder (bonjour protocol) */
  char_t        *ipV4_host_addr;                 /*!< Network host v4 IP: example "192.168.0.0" */
  char_t        *ipV4_subnet_msk;                /*!< Network sub net mask: example "255.255.255.0" */
  char_t        *ipV4_default_gatway;            /*!< Network default gateway example "192.168.0.254" */
  char_t        *ipV4_primary_dns;               /*!< Network primary DNS example "192.168.0.254" */
  char_t        *ipV4_secondary_dns;             /*!< Network secondary DNS example "192.168.0.254" */
  char_t         *urlNtpServer;                  /*!< Server name for the NTP example "0.fr.pool.ntp.org" */
  char_t         *urlEndPoint;                   /*!< Server end point example: "avs-alexa-na.amazon.com" */
  char_t         *cpuID;                         /*!< Chip and CPU version (string) */
  char_t         *toolChainID;                   /*!< compiler or tool chain String id (string) */

  char_t *      portingAudioName;                /*!< Porting audio name (string) */
  char_t *      netSupportName;                  /*!< Network porting name (LWIP or Cyclone or Other) */
  char_t          *alexaKeyWord;                 /*!< Keyword used by STVS4A and reported in the AVS synchro state, the default is ALEXA */
  uint32_t      synthesizerSampleRate;          /*!< AVS frequency used by Alexa voice speak */
  uint32_t      synthesizerSampleChannels;      /*!< AVS channels used by Alexa voice speak */
  uint32_t      recognizerSampleRate;           /*!< AVS frequency used for the Alexa microphone */
  uint32_t      useAuxAudio;                    /*!< True the SDK initialize an auxiliary audio output channel (Footprint impact) */
  uint32_t      auxAudioSampleRate;             /*!< Default frequency used by the auxiliary audio output */
  uint32_t      auxAudioSampleChannels;         /*!< Default channels used by the auxiliary audio output */
  uint32_t      initiator;                      /*!< How Alexa voice recognition is initiated (push to talk, voice...) @see AVS_Initiator_t */
  uint32_t      profile;                        /*!< Alexa voice recognition profile @see AVS_Profile_t */
  AVS_Event_CB   eventCB;                       /*!< Application notification callback */
  AVS_Persist_CB persistentCB;                  /*!< Overload to store the Persistent item in the persistent storage */
  uint32_t       eventCB_Cookie;                /*!< User cookie you can set and receive during the notification */
  uint32_t       memDTCMSize;                   /*!< Pool Size of the STVS4A DTCM in KB, this value must be accurate with the link script */
  uint32_t       memPRAMSize;                   /*!< Pool Size of the STVS4A Ram   in KB, this value must be accurate with the link script*/
  uint32_t       memNCACHEDSize;                /*!< Pool Size of the STAVS Uncached Ram   in KB, this value must be accurate with the link script*/

  /* Audio Factory */
  uint32_t      audioInLatency;                  /*!< Audio In Buffer size that absorbe the network latency */
  uint32_t      audioOutLatency;                 /*!< Audio Out Buffer size that absorbe the network latency */
  uint32_t      audioMp3Latency;                 /*!< MP3 Buffer size that absorbe the network latency */
    
  uint32_t      initVolume;                     /*!< Audio initial speaker volume */
  uint32_t      freqenceOut;                    /*!< Audio frequency device output in HZ example: 48000 */
  avs_float_t       buffSizeOut;                /*!< Audio output buffer size in MS (read only) */
  uint32_t      freqenceIn;                     /*!< Audio frequency device input in HZ example: 16000 */
  avs_float_t       buffSizeIn;                 /*!< Audio input buffer size in ms (read only) */
  uint32_t      chOut;                          /*!< Audio channel output example: 2 */
  uint32_t      chIn;                           /*!< Audio channel input example: 1 */
  AVS_Audio_Platform  platform;                 /*!< Audio Front End initialization parameters (according to the Use case and Board features, see porting layer) */
} AVS_Instance_Factory;



/*!
@brief AVS_Play_Sound flags
@ingroup enum

Defines a set of flags to get the sound status or to initiate a playback

*/
typedef enum t_avs_playsound_flag
{
  AVS_PLAYSOUND_PLAY = 0x0100                  /*!< Start the play */
  , AVS_PLAYSOUND_PLAY_LOOP = 0x0200           /*!< The play will be looped until an AVS_PLAYSOUND_STOP is set */
  , AVS_PLAYSOUND_STOP = 0x0400                /*!< Stop the playback */
  , AVS_PLAYSOUND_ACTIVE = 0x800               /*!< The playback is active */
  , AVS_PLAYSOUND_WAIT_END = 0x1000            /*!< Used with AVS_PLAYSOUND_PLAY, waits until the end of the sound */
} Avs_PlaySound_Flags;


/*!
@brief AVS_Play_Sound flags
@ingroup enum

Defines a set of flags to get the pipeline status or to imitate a playback
*/

typedef enum t_avs_pipe_flags
{
  AVS_PIPE_RUN = 1                    /*!< The pipe is running */
  , AVS_PIPE_MUTED = 2                /*!< The pipe is muted */
  , AVS_PIPE_EVT_HALF = 4             /*!< The pipe has received a half buffer completion */
  , AVS_PIPE_PROCESS_PART1 = 8        /*!< Internal flag */
  , AVS_PIPE_PROCESS_PART2 = 16       /*!< Internal flag */
  , AVS_PIPE_PURGE = 32               /*!< Don't wait for threshold to consume all sample */
  , AVS_PIPE_BUFFERING = 64           /*!< Do not consume sample for buffering */
} Avs_Pipe_Flags;



/*!
@brief AVS_Aux_Info flags
@ingroup enum

Defines flags to set specifically fields in the AVS_Aux_Info struct
*/

typedef enum t_avs_aux_info_flags
{
  AVS_AUX_INFO_FLAGS = 1              /*!< Affect only iFlags */
  , AVS_AUX_INFO_SZBUFF = 2           /*!< Affect only szProducer & szConsumer */
  , AVS_AUX_INFO_FREQ = 4             /*!< Affect only frequency */
  , AVS_AUX_INFO_CH = 8               /*!< Affect only nbChannel */
  , AVS_AUX_INFO_THRESHOLD = 16       /*!< Affect only threshold */
  , AVS_AUX_INFO_VOLUME = 32          /*!< Affect only volume */
  , AVS_AUX_INFO_NBSAMPLE = 64        /*!< Affect only sample number */
} Avs_Aux_Info_Flags;



/*!
@brief Structure passed to AVS_Audio_Aux_Info_Get and AVS_Audio_Aux_Info_Set
@ingroup struct

Allows to handle the audio auxiliary channel

*/

typedef struct t_avs_aux_info
{
  uint32_t      infoFlags;              /*!< Field Selector when set */
  uint32_t      iFlags;                 /*!< Ring buffer flags @see Avs_pipe_flags */
  uint32_t      szBuffer;               /*!< Ring buffer size in samples */
  uint32_t      nbChannel;              /*!< Ring buffer channel number */
  int32_t       szProducer;             /*!< Ring buffer size consumer */
  int32_t       szConsumer;             /*!< Ring buffer size producer */
  uint32_t      frequency;              /*!< Ring buffer frequency */
  uint32_t      threshold;              /*!< Ring buffer threshold */
  uint32_t      volume;                 /*!< Ring buffer volume */
} AVS_Aux_Info;

/*!
@brief returns some information concering the system
@ingroup struct

Returns some information concerning the system

*/
typedef struct t_avs_sys_info
{
  uint32_t      memFreeSpace;          /*!< Free memory space available */
  uint32_t      memTotalSpace;         /*!< Total memory space available */
  uint32_t      memDtcmTotalSpace;     /*!< Total memory DTCM Pool space available */
  uint32_t      memDtcmFreeSpace;      /*!< Free memory DTCM space available */
  uint32_t      memPRamTotalSpace;     /*!< Total memory RAM Pool space available */
  uint32_t      memPRamFreeSpace;      /*!< Free memory RAM Pool space available */
  uint32_t      memNCachedTotalSpace;  /*!< Total uncached memory RAM Pool space available */
  uint32_t      memNCachedFreeSpace;    /*!< Free uncached memory RAM Pool space available */
  
  uint32_t      synthBufferPercent;    /*!< Synthesizer buffer occupation*/
  uint32_t      synthPeakPercent;      /*!< Synthesizer Peak occupation*/
  uint32_t      recoBufferPercent;     /*!< Recognizer buffer occupation*/
  uint32_t      recoPeakPercent;       /*!< Recognizer Peak occupation*/
  uint32_t      auxBufferPercent;      /*!< Aux buffer occupation*/
  uint32_t      auxPeakPercent;        /*!< Aux peak occupation*/
  uint32_t      mp3BufferPercent;      /*!< Mp3 buffer occupation*/
  uint32_t      mp3PeakPercent;        /*!< Mp3 Peak occupation*/
} AVS_Sys_info;


AVS_Handle          AVS_Create_Instance(AVS_Instance_Factory *pFactory);                               /*!< @ingroup api */
AVS_Result          AVS_Delete_Instance(AVS_Handle hHandle);                                           /*!< @ingroup api */
AVS_Result          AVS_Set_Grant_Code(AVS_Handle hHandle, const char_t *grantCode);                   /*!< @ingroup api */
AVS_Result          AVS_Ioctl(AVS_Handle hInstance, AVS_CodeCtl id, uint32_t wparam, uint32_t lparam); /*!< @ingroup api */
AVS_Result          AVS_Set_State(AVS_Handle hHandle, AVS_Instance_State state);                       /*!< @ingroup api */
AVS_Instance_State  AVS_Get_State(AVS_Handle hHandle);                                                 /*!< @ingroup api */
AVS_Result          AVS_Play_Sound(AVS_Handle hHandle, uint32_t flags, void *pWave, int32_t volumePercent);/*!< @ingroup api */
uint32_t            AVS_Feed_Audio_Aux(AVS_Handle hHandle, int16_t *pSamples, uint32_t nbSample);      /*!< @ingroup api */
AVS_Result          AVS_Get_Audio_Aux_Info(AVS_Handle hHandle, AVS_Aux_Info *pInfo);                   /*!< @ingroup api */
AVS_Result          AVS_Set_Audio_Aux_Info(AVS_Handle hHandle, AVS_Aux_Info *pInfo);                   /*!< @ingroup api */
AVS_Result          AVS_Show_Config(AVS_Handle hInstance);                                             /*!< @ingroup api */
AVS_Result          AVS_Get_Sync_Time(AVS_Handle hInstance, AVS_TIME *pEpoch);                         /*!< @ingroup api */
AVS_Result          AVS_Set_Sync_Time(AVS_Handle hInstance);                                           /*!< @ingroup api */
AVS_Result          AVS_Set_Audio_Mute(AVS_Handle hInstance, uint32_t state);                          /*!< @ingroup api */
AVS_Result          AVS_Send_Evt(AVS_Handle hInstance, AVS_Event evt, uint32_t  pparam);               /*!< @ingroup api */
AVS_Result          AVS_Post_Evt(AVS_Handle hInstance, AVS_Event evt, uint32_t  pparam);               /*!< @ingroup api */
AVS_Result          AVS_Send_JSon(AVS_Handle hInstance, const char_t  *pJson);                         /*!< @ingroup api */
AVS_Result          AVS_Json_Add_Context(AVS_Handle hInstance, void *json_t_root);                     /*!< @ingroup api */

const char_t *      AVS_Get_Token_Access(AVS_Handle hInstance, char_t* bearer, size_t maxSize);        /*!< @ingroup api */
void *              AVS_Get_Http2_Instance(AVS_Handle hInstance);                                      /*!< @ingroup api */
AVS_Result          AVS_Post_Sychro(AVS_Handle hInstance);                                             /*!< @ingroup api */
AVS_Result          AVS_Get_Sys_Info(AVS_Handle hInstance, AVS_Sys_info *pSysInfo);                    /*!< @ingroup api */


AVS_HStream *       AVS_Open_Stream(AVS_Handle hHandle);                                               /*!< @ingroup api */
AVS_Result          AVS_Add_Body_Stream(AVS_Handle hInstance, AVS_HStream hStream, const char *pJson); /*!< @ingroup api */
AVS_Result          AVS_Read_Stream(AVS_Handle hInstance, AVS_HStream hStream, void *pBuffer, uint32_t szInSByte, uint32_t *retSize);/*!< @ingroup api */
AVS_Result          AVS_Write_Stream(AVS_Handle hInstance, AVS_HStream hStream, const void *pBuffer, size_t lengthInBytes);/*!< @ingroup api */
AVS_Result          AVS_Stop_Stream(AVS_Handle hInstance, AVS_HStream hStream);                        /*!< @ingroup api */
AVS_Result          AVS_Close_Stream_(AVS_Handle hInstance, AVS_HStream hStream);                      /*!< @ingroup api */
AVS_Result          AVS_Process_Json_Stream(AVS_Handle hInstance, AVS_HStream hStream);                /*!< @ingroup api */
const char_t *      AVS_Get_Reponse_Type_Stream(AVS_Handle hInstance, AVS_HStream hStream);            /*!< @ingroup api */


AVS_HTls            AVS_Open_Tls(AVS_Handle hHandle, char_t *pHost, uint32_t port); /*!< @ingroup api */
AVS_Result          AVS_Read_Tls(AVS_Handle hInstance, AVS_HTls hStream, void *pBuffer, uint32_t szInSByte, uint32_t *retSize); /*!< @ingroup api */
AVS_Result          AVS_Write_Tls(AVS_Handle hInstance, AVS_HTls hStream, const void *pBuffer, uint32_t lengthInBytes, uint32_t *retSize); /*!< @ingroup api */
AVS_Result          AVS_Close_Tls(AVS_Handle hInstance, AVS_HTls hStream);

#ifdef __cplusplus
};
#endif

#endif /* _avs_ */

