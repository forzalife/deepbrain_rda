/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef APP_EVENT_H
#define APP_EVENT_H

//音乐播放事件
typedef enum APP_EVENT_AUDIO_PLAYER_t
{
	APP_EVENT_AUDIO_PLAYER_BASE = APP_EVENT_BASE_ADDR,
	APP_EVENT_AUDIO_PLAYER_PLAY,
	APP_EVENT_AUDIO_PLAYER_PAUSE,
	APP_EVENT_AUDIO_PLAYER_RESUME,
	APP_EVENT_AUDIO_PLAYER_STOP,
}APP_EVENT_AUDIO_PLAYER_t;

//WIFI事件
typedef enum APP_EVENT_WIFI_t
{
	APP_EVENT_WIFI_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_AUDIO_PLAYER_BASE),
	APP_EVENT_WIFI_DEVICE_INIT_DONE,//wifi初始化完成
	APP_EVENT_WIFI_CONNECTING,		//连接中 
	APP_EVENT_WIFI_CONNECTED,		//已连接
	APP_EVENT_WIFI_DISCONNECTED, 	//断开连接
	APP_EVENT_WIFI_CONFIG,			//WIFI-AP配网
	APP_EVENT_WIFI_AIRKISS_CONFIG,	//WIFI-AIRKISS配网
	APP_EVENT_WIFI_AIRKISS_RECV_OK, //WIFI-AIRKISS接收信息成功
	APP_EVENT_WIFI_CONNECT_FAIL,	//连接失败
}APP_EVENT_WIFI_t;

//SDCARD事件
typedef enum APP_EVENT_SDCARD_t
{
	APP_EVENT_SDCARD_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_WIFI_BASE),
	
}APP_EVENT_SDCARD_t;

//POWER MANAGE事件
typedef enum APP_EVENT_POWER_t
{
	APP_EVENT_POWER_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_SDCARD_BASE),
	APP_EVENT_POWER_CHARGING,
	APP_EVENT_POWER_CHARGING_STOP,
	APP_EVENT_POWER_LOW_POWER,
	APP_EVENT_POWER_NO_OPERATION,
}APP_EVENT_POWER_t;

//nlp请求
typedef enum APP_EVENT_NLP_t
{
	APP_EVENT_NLP_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_POWER_BASE),
	APP_EVENT_NLP_REQUEST,
	APP_EVENT_NLP_DECODE_REQUEST,
}APP_EVENT_NLP_t;

//play list
typedef enum APP_EVENT_PLAYLIST_t
{
	APP_EVENT_PLAYLIST_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_NLP_BASE),
	APP_EVENT_PLAYLIST_REQUEST,
	APP_EVENT_PLAYLIST_PREV,
	APP_EVENT_PLAYLIST_NEXT,
	APP_EVENT_PLAYLIST_STOP,
	APP_EVENT_PLAYLIST_CLEAR,
}APP_EVENT_PLAYLIST_t;

//audio input process
typedef enum APP_EVENT_AIP_t
{
	APP_EVENT_AIP_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_PLAYLIST_BASE),
	APP_EVENT_AIP_START_RECORD,
	APP_EVENT_AIP_STOP_RECORD,
}APP_EVENT_AIP_t;

//wechat event
typedef enum APP_EVENT_WECHAT_t
{
	APP_EVENT_WECHAT_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_AIP_BASE),
	APP_EVENT_WECHAT_NEW_MSG,			
	APP_EVENT_WECHAT_NEW_MSG_RESPONSE,	
	APP_EVENT_WECHAT_SEND_MSG_OK,
	APP_EVENT_WECHAT_SEND_MSG_FAIL,
}APP_EVENT_WECHAT_t;

//ota event
typedef enum APP_EVENT_OTA_t
{
	APP_EVENT_OTA_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_WECHAT_BASE),
	APP_EVENT_OTA_START,
	APP_EVENT_OTA_STOP,
}APP_EVENT_OTA_t;

//chat talk event
typedef enum APP_EVENT_CHAT_TALK_t
{
	APP_EVENT_CHAT_TALK_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_OTA_BASE),
	APP_EVENT_CHAT_TALK_NEW_RESULT,
}APP_EVENT_CHAT_TALK_t;

//free talk事件
typedef enum APP_EVENT_FREE_TALK_t
{
	APP_EVENT_FREE_TALK_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_CHAT_TALK_BASE),
	APP_EVENT_FREE_TALK_RESULT,
	APP_EVENT_FREE_TALK_START_SILENT,	//静默启动
	APP_EVENT_FREE_TALK_START_PROMPT,	//带提示启动
}APP_EVENT_FREE_TALK_t;

//asr service event
typedef enum APP_EVENT_ASR_SERVICE_t
{
	APP_EVENT_ASR_SERVICE_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_FREE_TALK_BASE),
	APP_EVENT_ASR_SERVICE_NEW_REQUEST,
}APP_EVENT_ASR_SERVICE_t;

//display event
typedef enum APP_EVENT_DISPLAY_t
{
	APP_EVENT_DISPLAY_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_ASR_SERVICE_BASE),
	APP_EVENT_DISPLAY_IDLE,
	APP_EVENT_DISPLAY_ENABLE,
	APP_EVENT_DISPLAY_DISABLE,
	APP_EVENT_DISPLAY_PAUSE,
	APP_EVENT_DISPLAY_RESUME,
	APP_EVENT_DISPLAY_TTS_START,
	APP_EVENT_DISPLAY_TTS_STOP,
	APP_EVENT_DISPLAY_TONE_START,
	APP_EVENT_DISPLAY_TONE_STOP,
	APP_EVENT_DISPLAY_MUSIC_START,
	APP_EVENT_DISPLAY_MUSIC_STOP,
	APP_EVENT_DISPLAY_PLAYER_STOP,
	APP_EVENT_DISPLAY_RECORD_START,
	APP_EVENT_DISPLAY_RECORD_STOP,
}APP_EVENT_DISPLAY_t;

//keyboard event
typedef enum APP_EVENT_KEYBOARD_EVENT_t
{
	APP_EVENT_KEYBOARD_EVENT_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_DISPLAY_BASE),
	APP_EVENT_KEYBOARD_EVENT_PRESS,
	APP_EVENT_KEYBOARD_EVENT_RELEASE,
}APP_EVENT_KEYBOARD_EVENT_t;

//memo service
typedef enum APP_EVENT_MEMO_t
{
	APP_EVENT_MEMO_BASE = APP_EVENT_BASE_NEXT(APP_EVENT_KEYBOARD_EVENT_BASE),
	APP_EVENT_MEMO_CANCLE,
}APP_EVENT_MEMO_t;


#endif /*APP_EVENT_H*/
