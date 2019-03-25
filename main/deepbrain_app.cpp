#include "YTManage.h"
#include "deepbrain_app.h"

#include "Memory_interface.h"
#include "userconfig.h"
#include "dcl_tts_api.h"
#include "dcl_interface.h"
#include "dcl_asr_api.h"
#include "dcl_service.h"
#include "dcl_nlp_decode.h"
#include "dcl_volume_decode.h"
#include "dcl_mpush_push_msg.h"

#include "asr_service.h"
#include "wifi_manage.h"
#include "debug_log_interface.h"
#include "events.h"
#include "duer_recorder.h"
#include "yt_key.h"
#include "audio.h"
#include "change_voice.h"
#include "authorize.h"


#include "events.h"
#include "Factory_test.h"

//#include "YTDebug.h"

//static ASR_RESULT_t asr_result = {0};
//static NLP_RESULT_T nlp_result = {0};
#define LOG_TAG "DCL MAIN" 

//#define AMRNB_ENCODE_FRAME_MS	20
#define AMRNB_ENCODE_IN_BUFF_SIZE 640
//#define AMRNB_ENCODE_OUT_BUFF_SIZE (AMRNB_ENCODE_IN_BUFF_SIZE/10)

//#define WECHAT_MAX_RECORD_MS	(10*1000)
#define AMR_MAX_AUDIO_SIZE  AMRNB_ENCODE_IN_BUFF_SIZE*13 + 9  //AMRNB_ENCODE_IN_BUFF_SIZE*6 + 9//(WECHAT_MAX_RECORD_MS*AMRNB_ENCODE_OUT_BUFF_SIZE/20 + 9)



#define ZXP_PCBA 0
#define HB_PCBA 1

#if ZXP_PCBA
static duer::YTGpadcKey s_talk_button(KEY_B3);
static duer::YTGpadcKey s_wchat_button(KEY_B1);
static duer::YTGpadcKey s_pig_button(KEY_B2);
static duer::YTGpadcKey s_magic_button(KEY_B0);
#elif HB_PCBA
static duer::YTGpadcKey s_talk_button(KEY_B4);
static duer::YTGpadcKey s_volume_down_button(KEY_B2);
static duer::YTGpadcKey s_volume_up_button(KEY_B1);
static duer::YTGpadcKey s_wifi_bt_button(KEY_B3);
static duer::YTGpadcKey s_play_pause_button(KEY_B0);
#endif



static int record_sn = 0;		//录音序列号
static int record_id;		//录音索引
duer_timer_handler dcl_timeout_timer =  NULL;

static char *wechat_amrnb_data = NULL;	//amrnb录音
static int wechat_amrnb_data_len = 0;	//amrnb录音

static char *magic_amrnb_data = NULL;	//amrnb录音
static int magic_amrnb_data_len = 0;	//amrnb录音

static int dcl_mode;
static int rec_mode;
static const char amr_head[] =  "#!AMR\n";

extern bool bIsConnectedOnce;

namespace deepbrain {

void start_recorder();
void stop_recorder();

bool is_magic_voice_mode()
{
	return (dcl_mode == deepbrain::DEEPBRAIN_MODE_MAGIC_VOICE);
}



bool yt_dcl_process_stop_chat(NLP_RESULT_T *nlp_result)
{
	if ((strcmp(nlp_result->input_text, "闭嘴") == 0)
		|| (strcmp(nlp_result->input_text, "再见") == 0)
		|| (strcmp(nlp_result->input_text, "拜拜") == 0)
		|| (strcmp(nlp_result->input_text, "回聊") == 0))
	{
		return true;
	}

	return false;
}

void yt_dcl_process_result(ASR_RESULT_t *asr_result)
{
	int i = 0;
	static char NO_ASR_TIMES = 0;
	
	switch (asr_result->error_code)
	{
		case DCL_ERROR_CODE_OK: 	//成功
		{
			break;
		}
		case DCL_ERROR_CODE_FAIL:	//失败
		case DCL_ERROR_CODE_SYS_INVALID_PARAMS: //无效参数
		case DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM: //内存不足
		{
			//系统错误
			//audio_play_tone_mem(FLASH_MUSIC_XI_TONG_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//network error code
		case DCL_ERROR_CODE_NETWORK_DNS_FAIL:	//网络DNS解析失败
		{
			//audio_play_tone_mem(FLASH_MUSIC_DNS_JIE_XI_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_UNAVAILABLE://网络不可用
		{	
			//audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_KE_YONG, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_POOR:		//网络信号差
		{
			//audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_JIA, AUDIO_TERM_TYPE_NOW);
			return;
		}

		//server error
		case DCL_ERROR_CODE_SERVER_ERROR:		//服务器端错误
		{
			//audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//asr error code
		case DCL_ERROR_CODE_ASR_SHORT_AUDIO:	//语音太短,小于1秒
		{
			//audio_play_tone_mem(FLASH_MUSIC_CHAT_SHORT_AUDIO, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_ASR_ENCODE_FAIL:	//语音识别PCM数据编码失败
		case DCL_ERROR_CODE_ASR_MAKE_PACK_FAIL: //语音识别组包失败	
		{
			//语音识别错误
			//audio_play_tone_mem(FLASH_MUSIC_BIAN_MA_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		default:
			break;
	}

	NLP_RESULT_T *nlp_result = (NLP_RESULT_T*)memory_malloc(sizeof(NLP_RESULT_T));
	if(nlp_result == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "nlp_result malloc fail!!");
		return;
	}

	memset(nlp_result, 0x00, sizeof(NLP_RESULT_T));

	if (dcl_nlp_result_decode(asr_result->str_result, nlp_result) != NLP_DECODE_ERRNO_OK)
	{
		//to do something
		//audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
		DEBUG_LOGE(LOG_TAG, "dcl_nlp_result_decode failed");
		return;
	}
	DEBUG_LOGE(LOG_TAG, "nlp_result type:%d",nlp_result->type);
	if(strlen(nlp_result->input_text) == 0)
	{
		NO_ASR_TIMES++;
	}

	switch (nlp_result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		case NLP_RESULT_TYPE_ERROR:
		case NLP_RESULT_TYPE_NO_ASR:
		case NLP_RESULT_TYPE_CHAT:
		{
			bool stop_flag = yt_dcl_process_stop_chat(nlp_result);
			
			if(NO_ASR_TIMES <= 2) 
			{
				int flags = (stop_flag) ? duer::MEDIA_FLAG_PROMPT_TONE : duer::MEDIA_FLAG_URL_CHAT;
			
				if (strlen(nlp_result->chat_result.link) > 0)
				{				
					duer::YTMediaManager::instance().clear_queue();
					duer::YTMediaManager::instance().play_url(nlp_result->chat_result.link,flags);
				}
				else
				{
					bool ret = get_tts_play_url(nlp_result->chat_result.text, (char*)nlp_result->chat_result.link, sizeof(nlp_result->chat_result.link));
					if (!ret)
					{
						ret = get_tts_play_url(nlp_result->chat_result.text, (char*)nlp_result->chat_result.link, sizeof(nlp_result->chat_result.link));
					}
					
					if (ret)
					{					
						duer::YTMediaManager::instance().clear_queue();
						duer::YTMediaManager::instance().play_url(nlp_result->chat_result.link, flags);
					}
					else if(!stop_flag)
					{
						duer::YTMediaManager::instance().play_queue();
						//audio_play_tone_mem(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT, AUDIO_TERM_TYPE_NOW);
					}
				}
			}
			else {
				NO_ASR_TIMES = 0;
			}

			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			NLP_RESULT_LINKS_T *links = &nlp_result->link_result;
			DEBUG_LOGI(LOG_TAG, "p_links->link_size=[%d]", links->link_size);

			if(links->link_size > 0)
			{				
				duer::YTMediaManager::instance().clear_queue();
				if(strlen(links->link[0].name_tts_url) == 0) {
					bool ret = get_tts_play_url(links->link[0].link_name, links->link[0].name_tts_url, sizeof(links->link[0].name_tts_url));
					if (!ret)
					{
						bool ret = get_tts_play_url(links->link[0].link_name, links->link[0].name_tts_url, sizeof(links->link[0].name_tts_url));
					}
				}
			}	

			
			for (i=0; i<links->link_size; i++)
			{
				duer::YTMediaManager::instance().set_music_queue(links->link[i].link_url);
			}

			duer::YTMediaManager::instance().play_url(links->link[0].name_tts_url, duer::MEDIA_FLAG_SPEECH | duer::MEDIA_FLAG_SAVE_PREVIOUS);
			break;
		}

		case NLP_RESULT_TYPE_VOL_CMD:
		{	
			VOLUME_CONTROL_TYPE_t vol_ret = dcl_nlp_volume_cmd_decode(asr_result->str_result);//音量调节

			switch (vol_ret)
			{
				case VOLUME_CONTROL_INITIAL://error state
				{
					duer::YTMediaManager::instance().play_data(YT_DB_VOL_CTRL_FAIL,sizeof(YT_DB_VOL_CTRL_FAIL), duer::MEDIA_FLAG_URL_CHAT); 
					//audio_play_tone_mem(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_HIGHER:
				{					
					duer::YTMediaManager::instance().play_data(YT_DB_VOL_CTRL_UP,sizeof(YT_DB_VOL_CTRL_UP), duer::MEDIA_FLAG_URL_CHAT); 
					//audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_UP, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_LOWER:
				{
					duer::YTMediaManager::instance().play_data(YT_DB_VOL_CTRL_DOWN,sizeof(YT_DB_VOL_CTRL_DOWN), duer::MEDIA_FLAG_URL_CHAT); 
					//audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_DOWN, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_MODERATE:
				{
					duer::YTMediaManager::instance().play_data(YT_DB_VOL_CTRL_SUCCESS,sizeof(YT_DB_VOL_CTRL_SUCCESS), duer::MEDIA_FLAG_URL_CHAT); 
					//audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_CHANGE_OK, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_MAX:
				{
					duer::YTMediaManager::instance().play_data(YT_DB_VOL_CTRL_HIGH,sizeof(YT_DB_VOL_CTRL_HIGH), duer::MEDIA_FLAG_URL_CHAT); 
					//audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_ALREADY_HIGHEST, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_MIN:
				{
					duer::YTMediaManager::instance().play_data(YT_DB_VOL_CTRL_LOW,sizeof(YT_DB_VOL_CTRL_LOW), duer::MEDIA_FLAG_URL_CHAT); 
					//audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_ALREADY_LOWEST, AUDIO_TERM_TYPE_NOW);
					break;
				}
				default:
					break;
			}
			break;
		}
		
		case NLP_RESULT_TYPE_CMD:
		{
			break;
		}
		default:
			break;
	}

	if(nlp_result)
		memory_free(nlp_result);

	return;
}

void yt_dcl_rec_on_result(ASR_RESULT_t *asr_result)
{
	if(asr_result == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "asr_result NULL!!");
		return;
	}

	if(asr_result->record_sn != record_sn)
	{
		DEBUG_LOGE(LOG_TAG, "ignore record_sn[%d] asr_result!");
		return;
	}

	duer_timer_stop(dcl_timeout_timer);
	yt_dcl_process_result(asr_result);
}

void yt_dcl_rec_on_result_timeout(void *evt)
{
	record_sn = 0;	
	duer::YTMediaManager::instance().play_data(YT_DB_WIFI_LOW_RSSI,sizeof(YT_DB_WIFI_LOW_RSSI),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);
}

static bool bFirstWechatRelease = false;

void yt_dcl_rec_on_data(char *data, int size)
{	
	switch(rec_mode)
	{
		case DEEPBRAIN_MODE_ASR:

			ASR_PCM_OBJECT_t *asr_obj = NULL;
			record_id++;
			//语音识别请求
			if (asr_service_new_asr_object(&asr_obj) == APP_FRAMEWORK_ERRNO_OK)
			{
				asr_obj->asr_call_back = yt_dcl_rec_on_result;
				asr_obj->record_sn = record_sn;
				asr_obj->record_id = record_id;
				asr_obj->record_ms = 0;
				asr_obj->time_stamp = get_time_of_day();
				asr_obj->asr_mode = DCL_ASR_MODE_ASR_NLP;
				asr_obj->asr_lang = DCL_ASR_LANG_CHINESE;
				asr_obj->asr_engine = ASR_ENGINE_TYPE_DP_ENGINE;
				if (size > 0)
				{
					if(record_id == 1) {
						memcpy(asr_obj->record_data, amr_head, strlen(amr_head));
						memcpy(asr_obj->record_data + strlen(amr_head), data, size);
						asr_obj->record_len = size + strlen(amr_head);
					}
					else {
						memcpy(asr_obj->record_data, data, size);
						asr_obj->record_len = size;
					}
				}

				if (asr_service_send_request(asr_obj) != APP_FRAMEWORK_ERRNO_OK)
				{
					asr_service_del_asr_object(asr_obj);
					asr_obj = NULL;
					DEBUG_LOGE(LOG_TAG, "asr_service_send_request failed");
				}
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "asr_service_new_asr_object failed");
			}
			break;
			
		case DEEPBRAIN_MODE_MAGIC_VOICE:	
			if(magic_amrnb_data && magic_amrnb_data_len + size < AMR_MAX_AUDIO_SIZE)
			{
				if (size > 0)
				{					
					memcpy(magic_amrnb_data + magic_amrnb_data_len, data, size);
					magic_amrnb_data_len += size;					
					//DEBUG_LOGI(LOG_TAG, "DEEPBRAIN_MODE_WECHAT data size:%d wechat_data_len:%d",size, amrnb_data_len);
				}
			}
			else
			{	
				//DEBUG_LOGE(LOG_TAG, "DEEPBRAIN_MODE_WECHAT stop wechat_data:%x wechat_data_len:%d",amrnb_data,amrnb_data_len);
				duer::event_trigger(duer::EVT_KEY_STOP_RECORD);
			}
			break;

		case DEEPBRAIN_MODE_WECHAT:
			if(wechat_amrnb_data && wechat_amrnb_data_len + size < AMR_MAX_AUDIO_SIZE)
			{
				if (size > 0)
				{					
					memcpy(wechat_amrnb_data + wechat_amrnb_data_len, data, size);
					wechat_amrnb_data_len += size;					
					//DEBUG_LOGI(LOG_TAG, "DEEPBRAIN_MODE_WECHAT data size:%d wechat_data_len:%d",size, amrnb_data_len);
				}
			}
			else
			{	
				//DEBUG_LOGE(LOG_TAG, "DEEPBRAIN_MODE_WECHAT stop wechat_data:%x wechat_data_len:%d",amrnb_data,amrnb_data_len);
				duer::event_trigger(duer::EVT_KEY_STOP_RECORD);
			}
			break;

		default:			
			DEBUG_LOGE(LOG_TAG, "no such mode!!");
			break;
	}
}

void yt_dcl_rec_on_start()
{	
	if(magic_amrnb_data)
		memory_free(magic_amrnb_data);
	
	magic_amrnb_data = NULL;
	magic_amrnb_data_len = 0;
	
	if(wechat_amrnb_data)
		memory_free(wechat_amrnb_data);

	wechat_amrnb_data = NULL;
	wechat_amrnb_data_len = 0;

	switch(rec_mode)
	{
		case DEEPBRAIN_MODE_ASR:
			record_id = 0;			
			record_sn++;
			break;

		case DEEPBRAIN_MODE_WECHAT:
			wechat_amrnb_data = (char*)memory_malloc(AMR_MAX_AUDIO_SIZE);
			if(!wechat_amrnb_data) 	
			{
				DEBUG_LOGE(LOG_TAG, "rec_malloc wechat_data failed!!");
			}
			memset(wechat_amrnb_data, 0, AMR_MAX_AUDIO_SIZE);
			
			wechat_amrnb_data_len = 0;
			
			memcpy(wechat_amrnb_data, amr_head, strlen(amr_head));
			wechat_amrnb_data_len += strlen(amr_head);
			break;
			
		case DEEPBRAIN_MODE_MAGIC_VOICE:			
			magic_amrnb_data = (char*)memory_malloc(AMR_MAX_AUDIO_SIZE);
			if(!magic_amrnb_data) 	
				DEBUG_LOGE(LOG_TAG, "rec_malloc wechat_data failed!!");
			
			magic_amrnb_data_len = 0;
			
			memcpy(magic_amrnb_data, amr_head, strlen(amr_head));
			magic_amrnb_data_len += strlen(amr_head);
			break;

	}
}

void yt_dcl_rec_on_stop()
{
	switch(rec_mode)
	{
		case DEEPBRAIN_MODE_ASR:
			ASR_PCM_OBJECT_t *asr_obj = NULL;
			record_id++;
			record_id *= -1;
			//语音识别请求
			if (asr_service_new_asr_object(&asr_obj) == APP_FRAMEWORK_ERRNO_OK)
			{
				asr_obj->asr_call_back = yt_dcl_rec_on_result;
				asr_obj->record_sn = record_sn;
				asr_obj->record_id = record_id;
				asr_obj->record_len = 0;
				asr_obj->record_ms = (abs(record_id) > 0) ? 2000 : 0;
				asr_obj->time_stamp = get_time_of_day();
				asr_obj->asr_mode = DCL_ASR_MODE_ASR_NLP;
				asr_obj->asr_lang = DCL_ASR_LANG_CHINESE;
				asr_obj->asr_engine = ASR_ENGINE_TYPE_DP_ENGINE;

				if (asr_service_send_request(asr_obj) != APP_FRAMEWORK_ERRNO_OK)
				{
					asr_service_del_asr_object(asr_obj);
					asr_obj = NULL;
					DEBUG_LOGE(LOG_TAG, "asr_service_send_request failed");					
					duer::YTMediaManager::instance().play_data(YT_DB_WIFI_LOW_RSSI,sizeof(YT_DB_WIFI_LOW_RSSI),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);
				}
				else {
					duer_timer_start(dcl_timeout_timer, 5000);
				}
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "asr_service_new_asr_object failed");
			}

			break;
			
		case DEEPBRAIN_MODE_WECHAT:
			uint64_t encode_time = get_time_of_day();
			DCL_AUTH_PARAMS_t dcl_auth_params = {0};
			get_dcl_auth_params(&dcl_auth_params);

			DEBUG_LOGI(LOG_TAG, "wechat_amrnb_data_len: %d",wechat_amrnb_data_len);			
			DCL_ERROR_CODE_t ret = dcl_mpush_push_msg(&dcl_auth_params, wechat_amrnb_data, wechat_amrnb_data_len, "", "");

			 
			
			if (ret == DCL_ERROR_CODE_OK)
			{
				//audio_play_tone_mem(FLASH_MUSIC_SEND_SUCCESS, TERMINATION_TYPE_NOW);
				DEBUG_LOGI(LOG_TAG, "mpush_service_send_file success"); 
			}
			else
			{
				//audio_play_tone_mem(FLASH_MUSIC_SEND_FAIL, TERMINATION_TYPE_NOW);
				DEBUG_LOGE(LOG_TAG, "mpush_service_send_file failed"); 
			}
			
			if(wechat_amrnb_data)	
				memory_free(wechat_amrnb_data);

			wechat_amrnb_data = NULL;
			wechat_amrnb_data_len = 0;
			
			duer::YTMediaManager::instance().play_data(YT_WECHAT_SEND,sizeof(YT_WECHAT_SEND), duer::MEDIA_FLAG_SPEECH); 	
			break;
			
		case DEEPBRAIN_MODE_MAGIC_VOICE:
			if(dcl_mode == DEEPBRAIN_MODE_MAGIC_VOICE) {
				
				DUER_LOGE("armnb_data[%p],data_size[%d]", magic_amrnb_data, magic_amrnb_data_len);
				duer::YTMediaManager::instance().play_magic_voice(magic_amrnb_data, magic_amrnb_data_len,duer::MEDIA_FLAG_MAGIC_VOICE);
			}
			break;
			
		default:			
			DEBUG_LOGE(LOG_TAG, "no such mode!!");
			break;
	}
}

void start_recorder()
{	
	duer::duer_recorder_start();
}

void stop_recorder()
{
	duer::duer_recorder_stop();
}

void talk_start()
{
	if(!youngtone_is_authorized())
		return;

	if(duer::duer_recorder_is_busy())
	{
		duer::duer_recorder_stop();
		wait_ms(500); 
		if(duer::duer_recorder_is_busy())
			return;
	}
	if(!is_wifi_connected()&& !bIsConnectedOnce)
	{
		DEBUG_LOGI(LOG_TAG, "wifi is not connected");
		// do play something
		duer::YTMediaManager::instance().play_data(YT_CUR_NOTCONNECT_VOICE,sizeof(YT_CUR_NOTCONNECT_VOICE),duer::MEDIA_FLAG_SPEECH);
		return ;
	}	
	else if(!is_wifi_connected() && bIsConnectedOnce)
	{
		// 重新配网
		duer::event_trigger(duer::EVT_RESET_WIFI);
		return ;
	}
	else
	{
	
	}

	
	rec_mode = DEEPBRAIN_MODE_ASR;
	dcl_mode = DEEPBRAIN_MODE_ASR;
	
	duer::duer_recorder_set_vad(true);
	duer::duer_recorder_set_vad_asr(true);

	duer::YTMediaManager::instance().play_data(YT_MCHAT_START,sizeof(YT_MCHAT_START), duer::MEDIA_FLAG_RECORD_TONE);
	
}

void talk_stop()
{
    DEBUG_LOGI(LOG_TAG, "talk_stop");
	duer::duer_recorder_stop();	
}

void mchat_start()
{	
	if(!youngtone_is_authorized())
		return;
		
	if(duer::duer_recorder_is_busy())
	{
		duer::duer_recorder_stop();
		wait_ms(500);
		if(duer::duer_recorder_is_busy())
			return;
	}


	if(!is_wifi_connected())
	{
		DEBUG_LOGI(LOG_TAG, "wifi is not connected");
		// do play something
		duer::YTMediaManager::instance().play_data(YT_CUR_NOTCONNECT_VOICE,sizeof(YT_CUR_NOTCONNECT_VOICE),duer::MEDIA_FLAG_SPEECH);
		return ;
	}	



	rec_mode = DEEPBRAIN_MODE_WECHAT;
	dcl_mode = DEEPBRAIN_MODE_WECHAT;
	
	duer::duer_recorder_set_vad(false);
	duer::YTMediaManager::instance().play_data(YT_MCHAT_START,sizeof(YT_MCHAT_START), duer::MEDIA_FLAG_RECORD_TONE); 
}

void mchat_stop()
{
    DEBUG_LOGI(LOG_TAG, "mchat_stop");
	
	if(duer::duer_recorder_is_busy())
	{
		duer::duer_recorder_stop();	
	} else {
		wait_ms(500);		
		duer::duer_recorder_stop();	
	}
}

void magic_voice_start()
{	
	if(duer::duer_recorder_is_busy())
	{
		duer::duer_recorder_stop();
		wait_ms(500);
		if(duer::duer_recorder_is_busy()) {			
			return;
		}
	}
	dcl_mode = (dcl_mode == DEEPBRAIN_MODE_MAGIC_VOICE)	? DEEPBRAIN_MODE_ASR : DEEPBRAIN_MODE_MAGIC_VOICE;

	if(dcl_mode == DEEPBRAIN_MODE_MAGIC_VOICE) {
		rec_mode = DEEPBRAIN_MODE_MAGIC_VOICE;
		yt_dcl_stop();
		duer::duer_recorder_set_vad(true);
		duer::YTMediaManager::instance().play_data(YT_DB_ENTER_MAGIC_VOICE,sizeof(YT_DB_ENTER_MAGIC_VOICE), duer::MEDIA_FLAG_RECORD_TONE);
	}
	else {
		duer::duer_recorder_set_vad_asr(false);
		yt_dcl_start();
		duer::YTMediaManager::instance().play_data(YT_DB_EXIT_MAGIC_VOICE,sizeof(YT_DB_EXIT_MAGIC_VOICE), duer::MEDIA_FLAG_PROMPT_TONE);
	}
}

void play_pig_voice()
{
	static const char *voice = YT_DEEPBRAIN_PIG_1;
	int voice_len =  (voice == YT_DEEPBRAIN_PIG_1) ? sizeof(YT_DEEPBRAIN_PIG_1) : sizeof(YT_DEEPBRAIN_PIG_2);
	duer::YTMediaManager::instance().play_data(voice, voice_len, duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
	voice = (voice == YT_DEEPBRAIN_PIG_1) ? YT_DEEPBRAIN_PIG_2 : YT_DEEPBRAIN_PIG_1;
}

void change_mode(DEEPBRAIN_MODE_t mode)
{
	dcl_mode = mode;
}

void change_volume()
{
	duer::YTMediaManager::instance().volume_up_repeat();
	
	if(is_wifi_connected() )	
	{
		duer::YTMediaManager::instance().play_data(YT_KEY_TONE,sizeof(YT_KEY_TONE), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS); 	
	}
	else
	{
	
	}
}

void mchat_play()
{
	DEBUG_LOGI(LOG_TAG, "mchat_play");
	duer::YTMediaManager::instance().play_wchat_queue();	
}

void reset_wifi()
{	
	DEBUG_LOGI(LOG_TAG, "reset_wifi");
	wifi_manage_start_airkiss();
}




////// just add for HB_PCBA
void voice_up()
{
	duer::YTMediaManager::instance().volume_up();
	if(is_wifi_connected() )	
	{
		//duer::YTMediaManager::instance().play_data(YT_KEY_TONE,sizeof(YT_KEY_TONE), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS); 	
	}
	else
	{
	
	}	
}

void voice_down()
{
	duer::YTMediaManager::instance().volume_down();
	if(is_wifi_connected() )	
	{
		//duer::YTMediaManager::instance().play_data(YT_KEY_TONE,sizeof(YT_KEY_TONE), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS); 	
	}
	else
	{
	
	}	
}

void wifi_bt()
{
	duer::YTMediaManager::instance().switch_bt();
}

void play_pause()
{
	duer::YTMediaManager::instance().pause_or_resume();
}

#if ZXP_PCBA
void talk_button_fall_handle()
{
    duer::event_trigger(duer::EVT_KEY_REC_PRESS);
}

void talk_button_rise_handle()
{
    duer::event_trigger(duer::EVT_KEY_REC_RELEASE);
}

static bool _mchat_is_start;
void mchat_button_longpress_handle()
{	
	_mchat_is_start = true;
	duer::event_trigger(duer::EVT_KEY_MCHAT_PRESS);
}

void mchat_button_rise_handle()
{	
    if (_mchat_is_start) {
		duer::event_trigger(duer::EVT_KEY_MCHAT_RELEASE);
    }
	else
	{
		duer::event_trigger(duer::EVT_KEY_MCHAT_PLAY);
	}
	
	_mchat_is_start = false;
}

void magic_button_fall_handle()
{
    duer::event_trigger(duer::EVT_KEY_MAGIC_VOICE_PRESS);
}

void volume_button_fall_handle()
{
    duer::event_trigger(duer::EVT_KEY_VOLUME_PRESS);
}

void mode_button_fall_handle()
{
	duer::event_trigger(duer::EVT_KEY_MODE_PRESS);
}

void pig_button_fall_handle()
{
	duer::event_trigger(duer::EVT_KEY_PIG_PRESS);
}

void wifi_button_longpress_handle()
{
	duer::event_trigger(duer::EVT_RESET_WIFI);
}
#elif HB_PCBA

void talk_button_fall_handle()
{
	//bKeyPressed = true;
    duer::event_trigger(duer::EVT_KEY_REC_PRESS);
}

void talk_button_rise_handle()
{
    duer::event_trigger(duer::EVT_KEY_REC_RELEASE);
}


void wifi_button_longpress_handle()
{
	duer::event_trigger(duer::EVT_RESET_WIFI);
}



void play_prev()
{
	duer::YTMediaManager::instance().play_prev();
}


void play_next()
{
	duer::YTMediaManager::instance().play_next();
}

void volume_up_fall_handle()
{
	//duer::event_trigger(duer::EVT_KEY_VOICE_UP);
	printf("volume_up_fall_handle\r\n");

	duer::event_trigger(duer::EVT_KEY_PLAY_PREV);

	
	return;
}

void volume_down_fall_handle()
{
	//duer::event_trigger(duer::EVT_KEY_VOICE_DOWN);

	
	printf("volume_down_fall_handle\r\n");


	duer::event_trigger(duer::EVT_KEY_PLAY_NEXT);

	return;

}	


void volume_up_longpress_handle()
{
	printf("volume_up_longpress_handle\r\n");

	duer::event_trigger(duer::EVT_KEY_VOICE_UP);

}

void volume_down_longpress_handle()
{
	printf("volume_down_longpress_handle\r\n");
	duer::event_trigger(duer::EVT_KEY_VOICE_DOWN);
}

void play_pause_fall_handle()
{
	duer::event_trigger(duer::EVT_KEY_PAUSE);
}

void wifi_bt_fall_handle()
{
	duer::event_trigger(duer::EVT_KEY_SWITCH_MODE);
	
}

#endif



void RegistWifi()
{
	DEBUG_LOGI(LOG_TAG, "RegistWifi");
	duer::event_set_handler(duer::EVT_RESET_WIFI, &reset_wifi);
}

void RegistRec()
{
	DEBUG_LOGI(LOG_TAG, "RegistRec");
	duer::event_set_handler(duer::EVT_KEY_REC_PRESS, &talk_start);
	duer::event_set_handler(duer::EVT_KEY_REC_RELEASE, &talk_stop);
}



void yt_dcl_start()
{
	record_sn = 0;
	
	if(!dcl_timeout_timer)
		dcl_timeout_timer = duer_timer_acquire(yt_dcl_rec_on_result_timeout, NULL, DUER_TIMER_ONCE);	

	if (!dcl_timeout_timer) {
		DEBUG_LOGE(LOG_TAG,  "fail to creat dcl_timeout_timer"); 
		return;
	}

	DEBUG_LOGI(LOG_TAG, "yt_dcl_start");
#if ZXP_PCBA	
	duer::event_set_handler(duer::EVT_KEY_REC_PRESS, &talk_start);
	duer::event_set_handler(duer::EVT_KEY_REC_RELEASE, &talk_stop);
	duer::event_set_handler(duer::EVT_KEY_MCHAT_PRESS, &mchat_start);
	duer::event_set_handler(duer::EVT_KEY_MCHAT_RELEASE, &mchat_stop);
	duer::event_set_handler(duer::EVT_KEY_MCHAT_PLAY, &mchat_play);
	duer::event_set_handler(duer::EVT_KEY_PIG_PRESS, &play_pig_voice);
#elif HB_PCBA
	duer::event_set_handler(duer::EVT_KEY_REC_PRESS, &talk_start);
	duer::event_set_handler(duer::EVT_KEY_REC_RELEASE, &talk_stop);	
	duer::event_set_handler(duer::EVT_KEY_SWITCH_MODE, &wifi_bt);
	duer::event_set_handler(duer::EVT_KEY_PAUSE, &play_pause);
	duer::event_set_handler(duer::EVT_KEY_VOICE_UP, &voice_up);
	duer::event_set_handler(duer::EVT_KEY_VOICE_DOWN, &voice_down);	
#endif	
	duer::event_set_handler(duer::EVT_RESET_WIFI, &reset_wifi);
}

void yt_dcl_stop()
{
	duer_timer_stop(dcl_timeout_timer);
	
	DEBUG_LOGI(LOG_TAG, "yt_dcl_stop");
#if ZXP_PCBA
	duer::event_set_handler(duer::EVT_KEY_REC_PRESS, NULL);
	duer::event_set_handler(duer::EVT_KEY_REC_RELEASE, NULL);
	duer::event_set_handler(duer::EVT_KEY_MCHAT_PRESS, NULL);
	duer::event_set_handler(duer::EVT_KEY_MCHAT_RELEASE, NULL);
	duer::event_set_handler(duer::EVT_KEY_MCHAT_PLAY, NULL);
	duer::event_set_handler(duer::EVT_KEY_PIG_PRESS, NULL);
#elif HB_PCBA
	duer::event_set_handler(duer::EVT_KEY_REC_PRESS, NULL);
	duer::event_set_handler(duer::EVT_KEY_REC_RELEASE, NULL);
	duer::event_set_handler(duer::EVT_KEY_SWITCH_MODE, NULL);
	duer::event_set_handler(duer::EVT_KEY_PAUSE, NULL);
	duer::event_set_handler(duer::EVT_KEY_VOICE_UP, NULL);
	duer::event_set_handler(duer::EVT_KEY_VOICE_DOWN, NULL);
#endif
	duer::event_set_handler(duer::EVT_RESET_WIFI, NULL);
}

void yt_dcl_init()
{
	rec_mode = DEEPBRAIN_MODE_ASR;	
	dcl_mode = DEEPBRAIN_MODE_ASR;	

	
#if ZXP_PCBA
	duer::event_set_handler(duer::EVT_KEY_START_RECORD, &start_recorder);
	duer::event_set_handler(duer::EVT_KEY_STOP_RECORD, &stop_recorder);
	duer::event_set_handler(duer::EVT_KEY_MAGIC_VOICE_PRESS, &magic_voice_start);
    duer::event_set_handler(duer::EVT_RESET_WIFI, &reset_wifi);	
	duer::event_set_handler(duer::EVT_KEY_PIG_PRESS, &play_pig_voice);
	duer::event_set_handler(duer::EVT_KEY_VOLUME_PRESS, &change_volume);
	duer::yt_key_init();
	s_talk_button.fall(&talk_button_fall_handle);
    s_wchat_button.rise(&mchat_button_rise_handle);
    s_wchat_button.longpress(&mchat_button_longpress_handle, 1000, duer::YT_LONG_KEY_WITH_RISE);
	s_magic_button.rise(&volume_button_fall_handle);
    s_magic_button.longpress(&magic_button_fall_handle, 3000, duer::YT_LONG_KEY_ONCE);
	s_pig_button.longpress(&wifi_button_longpress_handle, 5000, duer::YT_LONG_KEY_ONCE);
	s_pig_button.rise(&pig_button_fall_handle);
#elif HB_PCBA
	duer::event_set_handler(duer::EVT_KEY_START_RECORD, &start_recorder);
	duer::event_set_handler(duer::EVT_KEY_STOP_RECORD, &stop_recorder);
	duer::event_set_handler(duer::EVT_KEY_SWITCH_MODE, &wifi_bt);
    duer::event_set_handler(duer::EVT_RESET_WIFI, &reset_wifi);	
	duer::event_set_handler(duer::EVT_KEY_PAUSE, &play_pause);
	duer::event_set_handler(duer::EVT_KEY_VOICE_UP, &voice_up);
	duer::event_set_handler(duer::EVT_KEY_VOICE_DOWN, &voice_down);
	duer::event_set_handler(duer::EVT_KEY_PLAY_PREV, &play_prev);
	duer::event_set_handler(duer::EVT_KEY_PLAY_NEXT, &play_next);
	duer::yt_key_init();
	s_talk_button.fall(&talk_button_fall_handle);

#if 0	
    s_volume_up_button.rise(&volume_up_fall_handle);
    s_volume_up_button.longpress(&volume_up_longpress_handle, 1000, duer::YT_LONG_KEY_PERIODIC);
	//s_volume_up_button.fall(&volume_up_fall_handle);
	//s_volume_down_button.fall(&volume_down_fall_handle);
	s_volume_down_button.rise(&volume_down_fall_handle);
    s_volume_down_button.longpress(&volume_down_longpress_handle, 1000, duer::YT_LONG_KEY_PERIODIC);
#else
    s_volume_up_button.rise(&volume_up_longpress_handle);
    s_volume_up_button.longpress(&volume_up_fall_handle, 1000, duer::YT_LONG_KEY_ONCE);
	s_volume_down_button.rise(&volume_down_longpress_handle);
    s_volume_down_button.longpress(&volume_down_fall_handle, 1000, duer::YT_LONG_KEY_ONCE);	
#endif
	s_wifi_bt_button.rise(&wifi_bt_fall_handle);
	s_wifi_bt_button.longpress(&wifi_button_longpress_handle, 5000, duer::YT_LONG_KEY_ONCE);
	s_play_pause_button.fall(&play_pause_fall_handle);
#endif
}


//add by lijun	20190311
void yt_key_clear()
{	
	int i = 0;
	for(i = 0; i < PAD_KEY_NUM; i++)	
	{		
		duer::YTGpadcKey::_fall[i].attach(NULL);
		duer::YTGpadcKey::_rise[i].attach(NULL);
		duer::YTGpadcKey::_longpress[i].attach(NULL);
		duer::YTGpadcKey::_key_longpress_time_map[i] = 0;
		duer::YTGpadcKey::_key_longpress_type[i] = -1;
	}
}


void auto_test_key(){	duer::event_trigger(duer::EVT_KEY_FACTORY_KEY);}
void auto_test_key1(){duer::event_trigger(duer::EVT_KEY_FACTORY1_KEY);}
void auto_test_key2(){duer::event_trigger(duer::EVT_KEY_FACTORY2_KEY);}
void auto_test_key3(){duer::event_trigger(duer::EVT_KEY_FACTORY3_KEY);}
void auto_test_key4(){duer::event_trigger(duer::EVT_KEY_FACTORY4_KEY);}
void auto_test_key5(){duer::event_trigger(duer::EVT_KEY_FACTORY5_KEY);}

void auto_test_start()
{
	DUER_LOGI("auto_test_start");
	duer::yt_key_init();
	
#if ZXP_PCBA	
	DUER_LOGI("ZXP_PCBA");
	s_magic_button.fall(&auto_test_key);
	s_wchat_button.fall(&auto_test_key1);
	s_pig_button.fall(&auto_test_key2);
	s_talk_button.fall(&auto_test_key3);
#elif HB_PCBA
	s_volume_down_button.fall(&auto_test_key);
	s_volume_up_button.fall(&auto_test_key1);
	s_wifi_bt_button.fall(&auto_test_key2);
	s_talk_button.fall(&auto_test_key3);
	s_play_pause_button.fall(&auto_test_key4);
#endif
	yt_auto_test_start();
}


int auto_test()
{
	int ret = yt_auto_test();
	if(ret)
	{		
		yt_key_clear();
		auto_test_start();
	}
	
	return ret;
}
//add by lijun	20190311 end

}
