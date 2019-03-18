#include "mbed.h"
#include "rtos.h"
#include "vbat.h"
#include "lightduer_log.h"
#include "YTLight.h"
#include "YTManage.h"
#include "audio.h"
#include "rda_wdt_api.h"
#include "Rda_sleep_api.h"
#include "events.h"
#include "baidu_media_play.h"
//#define LOG(_fmt, ...)      printf("[DEVC] ==> %s(%d): "_fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG(...) DUER_LOGI(__VA_ARGS__)


/// 开发板 用这个可能会造成死机
// other 
//AnalogIn ain(ADC_PIN2);


////kaifaban
AnalogIn ain(ADC_PIN0);

static Thread s_vbat_thread(osPriorityHigh, 1024);

const unsigned int vbat_low_power_alert_times = 5*60*1000; 

static unsigned short get_vbat()
{
	unsigned short _ival[10];
	unsigned short ival_max = 0;	
	unsigned short ival_min = 0;
	unsigned int sum = 0;
	int i = 0;

	_ival[0] = ain.read_u16();
	ival_max = ival_min = _ival[0];
	sum += _ival[0];
	
	for(i = 1; i < 10; i++)
	{		
		Thread::wait(2000);
		_ival[i] = ain.read_u16();
		if(_ival[i] < ival_min)
		{
			ival_min = _ival[i];
		}

		if(_ival[i] > ival_max)
		{
			ival_max = _ival[i];
		}

		sum += _ival[i];
	}

	sum -= ival_max;
	sum -= ival_min;

	return (unsigned short)(sum / 8);
}

bool bVbatRun=true;
/*static */void vbat_sleep()
{
	printf("[vbat]sleep\r\n");

	duer::media_play_codec_off();
	sleep_set_level(SLEEP_POWERDOWN);			
	user_sleep_allow(1);

 #if 1
    bVbatRun=false;
    s_vbat_thread.terminate();
 #endif
 
}

static void vbat_check()
{
	unsigned short _ival;
    unsigned int   _delay = 5*60*1000;
	VBAT_SCENE _vbat_status = VBAT_FULL;
	unsigned int alert_times = vbat_low_power_alert_times;

	while(bVbatRun)
	{
		_ival = get_vbat();
		printf("sys vbat val:%d\r\n",_ival);
		
		if(_ival < 605)
		{
			if(_vbat_status < VBAT_NORMAL)
			{			
				_delay = 60*1000;
				_vbat_status = VBAT_NORMAL;			
			}
		}
		
		if(_ival < 585)//3.55v
		{	
			if(_vbat_status < VBAT_LOW_POWER)
			{			
				_delay = 60*1000;
				_vbat_status = VBAT_LOW_POWER;
			}
		
			if(_ival >= 575)
			{
				if(alert_times > vbat_low_power_alert_times)
				{
					alert_times = 0;
					duer::YTMediaManager::instance().play_data(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);
				}
				else 
				{
					alert_times += _delay;
				}
			}
		}
		
		if(_ival < 575)//3.5v
		{
			if(_vbat_status < VBAT_SHUTDOWN)
			{			
				duer::YTMediaManager::instance().play_data(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT),duer::MEDIA_FLAG_NO_POWER_TONE);				
				_vbat_status = VBAT_SHUTDOWN;
			}
		}
		
		Thread::wait(_delay);
	}
}

void vbat_start()
{	
	duer::event_set_handler(duer::EVT_SHUTDOWN, vbat_sleep);
	s_vbat_thread.start(vbat_check);
}

