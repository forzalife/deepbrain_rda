#include "YTManage.h"
//#include "duer_app.h"
#include "events.h"
#include "baidu_media_manager.h"
#include "baidu_media_play.h"
#include "heap_monitor.h"
#include "gpadckey.h"
#include "YTDebug.h"
#include "duer_recorder.h"
#include "lightduer_log.h"
#include "lightduer_queue_cache.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "YTLight.h"
#include "PwmOut.h"

#include "audio.h"
#include <limits.h>

#include "app_framework.h"

#define ADD_BY_LIJUN_1 1

#if ADD_BY_LIJUN_1
extern void vbat_sleep();
#endif

namespace deepbrain {
extern bool is_magic_voice_mode();
}
namespace duer {
//#define LOG(_fmt, ...)      printf("[DEVC] ==> %s(%d): "_fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG(...) DUER_LOGI(__VA_ARGS__)

/*
 * use this class to play main url after played front tone url
 */
static duer_qcache_handler s_music_queue;
static duer_mutex_t s_music_queue_lock;

static duer_qcache_handler s_wchat_queue;
static duer_mutex_t s_wchat_queue_lock;

static mbed::PwmOut s_motor(GPIO_PIN22);
//static mbed::PwmOut *s_motor = NULL;
//static mbed::DigitalOut GPIO_MOTOR(GPIO_PIN22,1);

class YTMDMPlayerListener : public MediaPlayerListener {
public:
    virtual int on_start(int flags);

    virtual int on_stop(int flags);

    virtual int on_finish(int flags);
};

static YTMDMPlayerListener s_mdm_media_listener;



#if ADD_BY_LIJUN_1
#define AIRKISS_TIMEOUT_SLEEP 1000*60*10

void AirkissTimeOutSleep(void const *argument)
{
	DUER_LOGI( "AirkissTimeOutSleep");	
	// add  aoto_sleep code here	
	duer::YTMediaManager::instance().play_data(YT_SLEEP,sizeof(YT_SLEEP), MEDIA_FLAG_SPEECH|!MEDIA_FLAG_SAVE_PREVIOUS);
#if 0
	while(duer::YTMediaManager::instance().get_media_status() != MEDIA_PLAYER_IDLE) 
	{
		   Thread::wait(50);
	}
#endif


	app_send_message(APP_MAIN_NAME, APP_MSG_TO_ALL, APP_EVENT_DEFAULT_EXIT, NULL, 0);
	vbat_sleep();
	duer::event_trigger(duer::EVT_STOP);
	
}

rtos::RtosTimer rtAirkissSleep(AirkissTimeOutSleep,osTimerOnce,NULL);


#define PWM_TIME_PERIODIC 200
int nTime = 0;
const int nRunTime = (PWM_TIME_PERIODIC * 5 *3);
const int nSleepTime = (PWM_TIME_PERIODIC * 5 *6);
bool bRun = false;
bool bRun1 = true;
int gflags = 0;

float gPwm = 1.0;
int nStep = 0;
int nStartStep = 3;
int nStopStep = 5;
float nAddVal = -0.1;
float gPrePwm = 1.0;
void PwmTime(void const *argument)
{
	nStep ++;
	if(nStep > nStartStep && nStep < nStopStep)
	{
		gPwm += nAddVal;
		if(gPwm <= 0.0)gPwm = 0.0;
		if(gPwm >= 1.0)gPwm = 1.0;
	}
	else if(nStep <= nStartStep)
	{
		gPrePwm = gPwm;
	}
	else if(nStep >= nStopStep)
	{
		gPwm = gPrePwm;
	}
	
#if 1	
	s_motor.clock_set(1, 4);
	s_motor.period_ms(1);
#if 1
	if(gflags & MEDIA_FLAG_DCS_URL)
	{
		if(bRun)/// play
		{
			
			if(bRun1)
			{
				if(nTime >= nRunTime)
				{
					nTime = 0;
					bRun1 = false;		
				}
			}
			else 
			{
				gPwm = 1.0;
				if(nTime >= nSleepTime)
				{
					nTime = 0;
					bRun1 = true;
				}			
			}
			nTime += PWM_TIME_PERIODIC;	
		}
		else/// stop  / finish
		{
			nTime = 0;
			gPwm = 1.0;
		}
	}
#endif	
	s_motor.write(gPwm); 
#endif

}
rtos::RtosTimer rtPwm(PwmTime,osTimerPeriodic,NULL);

#endif


int YTMDMPlayerListener::on_start(int flags)
{
#if 1

	DUER_LOGI("YTMDMPlayerListener : on_start");

	DUER_LOGI("%d",flags & MEDIA_FLAG_DCS_URL);

	gflags = flags;
	
	if (flags & MEDIA_FLAG_DCS_URL || flags & MEDIA_FLAG_MAGIC_VOICE) 
	{	

#if 1	
		if(flags & MEDIA_FLAG_DCS_URL)
		  	gPwm = 0.00f;//0.3	  
		else if(flags & MEDIA_FLAG_MAGIC_VOICE)
			gPwm = 0.0f;//0.1
		nTime = 0;
		nStep = 0;
		bRun = true;
		bRun1 = true;
		rtPwm.start(PWM_TIME_PERIODIC);
#endif
		
#if 1//def YTMOTOR
		//mbed::PwmOut s_motor(GPIO_PIN22);
		
#if 0
	  gpio_t gpio;
	  gpio_init(&gpio,GPIO_PIN22);
	  gpio_dir(&gpio, PIN_OUTPUT);
	  if(flags & MEDIA_FLAG_DCS_URL)
	  	gpio_write(&gpio,0);
	  else if(flags & MEDIA_FLAG_MAGIC_VOICE)
	  	gpio_write(&gpio,0);
#endif	  

#if 0
		GPIO_MOTOR=0;
#endif

	  
#if 0
	  s_motor.clock_set(1, 4);
	  s_motor.period_ms(1);	  
	  if(flags & MEDIA_FLAG_DCS_URL)
	  	s_motor.write(0.10f);//0.3	  
	  else if(flags & MEDIA_FLAG_MAGIC_VOICE)
	  	s_motor.write(0);//0.1		 
#endif

#endif
	}
#endif


#if ADD_BY_LIJUN_1
	rtAirkissSleep.stop();
#endif

    return 0;
}

int YTMDMPlayerListener::on_stop(int flags)
{		

	DUER_LOGI("YTMDMPlayerListener : on_stop");
	
#if 1//def YTLED
	YTLED::instance().disp_led(LED_IDLE);
#endif	

#if 1
	#if 1
	s_motor = 1;
	float val = s_motor.read();
	DUER_LOGI("%f",val);
	#endif
	#if 0
	  gpio_t gpio;
	  gpio_init(&gpio,GPIO_PIN22);
	  gpio_dir(&gpio, PIN_OUTPUT);
	  gpio_write(&gpio,1);	  
	#endif
    #if 0
	  GPIO_MOTOR=1;
	#endif

#endif

#if 1
	bRun = false;
	bRun1 = false;

	nStep = 0;
	rtPwm.stop();
#endif


	if(flags & duer::MEDIA_FLAG_NO_POWER_TONE){
		YTLED::instance().disp_led(LED_SHUTDOWN);
		//event_trigger(EVT_SHUTDOWN);
	}

#if ADD_BY_LIJUN_1
	rtAirkissSleep.start(AIRKISS_TIMEOUT_SLEEP);
#endif

    return 0;
}

int YTMDMPlayerListener::on_finish(int flags)
{
	DUER_LOGI("YTMDMPlayerListener : on_finish");
#if 1//def YTLED
	YTLED::instance().disp_led(LED_IDLE);
#endif

#if 1//def YTMOTOR
	#if 1
	s_motor = 1;
	float val = s_motor.read();
	DUER_LOGI("%f",val);
	#else
	  gpio_t gpio;
	  gpio_init(&gpio,GPIO_PIN22);
	  gpio_dir(&gpio, PIN_OUTPUT);
	  gpio_write(&gpio,1);
	 #endif
  
#endif


#if 1
	nStep = 0;
	bRun = false;
	bRun1 = false;
	rtPwm.stop();
#endif



	if(deepbrain::is_magic_voice_mode())
	{
		event_trigger(EVT_KEY_START_RECORD);
	}
	else if (flags & MEDIA_FLAG_SPEECH || flags & MEDIA_FLAG_DCS_URL) {		
		YTMediaManager::instance().play_queue();
	}
	else if(flags & MEDIA_FLAG_WCHAT) {
		YTMediaManager::instance().play_wchat_queue();
	}
	else if(flags & duer::MEDIA_FLAG_URL_CHAT){
		event_trigger(EVT_KEY_REC_PRESS);
	}
	else if(flags & duer::MEDIA_FLAG_RECORD_TONE){
		event_trigger(EVT_KEY_START_RECORD);
	}
	else if(flags & duer::MEDIA_FLAG_BT_TONE){
		event_trigger(EVT_KEY_ENTER_BT);
	}
	else if(flags & duer::MEDIA_FLAG_NO_POWER_TONE){
		YTLED::instance().disp_led(LED_SHUTDOWN);
		event_trigger(EVT_SHUTDOWN);
	}
	else if(flags & duer::MEDIA_FLAG_PROMPT_TONE){
		if(flags & duer::MEDIA_FLAG_SAVE_PREVIOUS)
		{
			duer::YTMediaManager::instance().play_previous_media_continue();
		}
	}

#if ADD_BY_LIJUN_1
	rtAirkissSleep.start(AIRKISS_TIMEOUT_SLEEP);
#endif	

    return 0;
}

static void yt_media_play_failed_cb(int flags)
{
    if (flags & MEDIA_FLAG_SPEECH || flags & MEDIA_FLAG_DCS_URL) {
		YTMediaManager::instance().play_queue();
	}
}

YTMediaManager YTMediaManager::s_instance;

YTMediaManager::YTMediaManager()
{
}

YTMediaManager::~YTMediaManager() 
{
}

YTMediaManager& YTMediaManager::instance()
{

    return s_instance;
}

void YTMediaManager::init()
{
    if (!s_music_queue_lock) {
        s_music_queue_lock = duer_mutex_create();
        if (!s_music_queue_lock) {
            DUER_LOGE("Failed to create wechat queue lock");
        }
    }

    duer_mutex_lock(s_music_queue_lock);
    if (!s_music_queue) {
        s_music_queue = duer_qcache_create();
        if (!s_music_queue) {
            DUER_LOGE("Failed to create s_music_queue");
        }
    }
    duer_mutex_unlock(s_music_queue_lock);

    if (!s_wchat_queue_lock) {
        s_wchat_queue_lock = duer_mutex_create();
        if (!s_wchat_queue_lock) {
            DUER_LOGE("Failed to create wechat queue lock");
        }
    }

    duer_mutex_lock(s_wchat_queue_lock);
    if (!s_wchat_queue) {
        s_wchat_queue = duer_qcache_create();
        if (!s_wchat_queue) {
            DUER_LOGE("Failed to create s_music_queue");
        }
    }
    duer_mutex_unlock(s_wchat_queue_lock);

	YTMediaManager::instance().set_volume(get_volume());
	MediaManager::instance().register_listener(&s_mdm_media_listener);
    MediaManager::instance().register_play_failed_cb(yt_media_play_failed_cb);

	event_set_handler(EVT_KEY_PAUSE, this, &YTMediaManager::pause_or_resume);
	
	event_set_handler(EVT_KEY_VOICE_UP, this, &YTMediaManager::volume_up);
	event_set_handler(EVT_KEY_VOICE_DOWN, this, &YTMediaManager::volume_down);

	event_set_handler(EVT_KEY_PLAY_PREV,  this, &YTMediaManager::play_prev);
	event_set_handler(EVT_KEY_PLAY_NEXT, this, &YTMediaManager::play_next);

	event_set_handler(EVT_KEY_ENTER_BT, this, &YTMediaManager::start_bt);		
	event_set_handler(EVT_KEY_BT_SWITCH, this, &YTMediaManager::switch_bt);	


	//s_bt_button.fall(this, &YTMediaManager::bt_button_fall_handle);
}

void YTMediaManager::deinit()
{
}

int YTMediaManager::get_media_status()
{
	MediaPlayerStatus status = MediaManager::instance().get_media_player_status();
	return status;	
}
bool YTMediaManager::is_playing()
{
	MediaPlayerStatus status = MediaManager::instance().get_media_player_status();
	//printf("status:%d\r\n",status);
	return (status == MEDIA_PLAYER_PLAYING)?true:false;
}

bool YTMediaManager::is_bt()
{	
	MediaPlayerStatus status = MediaManager::instance().get_media_player_status();
	return (status == MEDIA_PLAYER_BT)?true:false;
}

void YTMediaManager::stop()
{	
	MediaManager::instance().stop();	
}

void YTMediaManager::stop_completely()
{	
	clear_queue();
	MediaManager::instance().stop_completely();	
}

int YTMediaManager::get_play_progress() 
{
    return MediaManager::instance().get_play_progress();
}

unsigned char YTMediaManager::get_volume()
{
	return MediaManager::instance().get_volume();
}

void YTMediaManager::set_volume(unsigned char vol)
{
	MediaManager::instance().set_volume(vol);
}

void YTMediaManager::volume_up_repeat()
{
	if(is_bt())
		media_bt_volup();
	else
		MediaManager::instance().voice_up_repeat();
}

void YTMediaManager::volume_up()
{
	if(is_bt())
		media_bt_volup();
	else
		MediaManager::instance().voice_up();
}

void YTMediaManager::volume_down()
{	
	if(is_bt())
		media_bt_volup();
	else
		MediaManager::instance().voice_down();
}

void YTMediaManager::pause_or_resume()
{		
	if(is_bt())
		media_bt_play();
	else
		MediaManager::instance().pause_or_resume();
}

void YTMediaManager::rec_start()
{	
	MediaManager::instance().rec_start();
}

void YTMediaManager::rec_stop()
{
	MediaManager::instance().rec_stop();
}

int YTMediaManager::rec_read_data(char *read_ptr, int size)
{
	return MediaManager::instance().read_data(read_ptr, size);
}

int YTMediaManager::set_wchat_queue(const char *url)
{
	if(duer_qcache_length(s_wchat_queue) > 10){
		DUER_LOGE("url cache > 10");
		return -1;
	}

	int len = strlen(url) + 1; 
	char *url_dup = (char *)DUER_MALLOC(len);
	if (!url_dup) {
		DUER_LOGE("no memory!!");
		return -1;
	}
	snprintf(url_dup, len, "%s", url);
	
	duer_mutex_lock(s_wchat_queue_lock);
	duer_qcache_push(s_wchat_queue, url_dup);
	duer_mutex_unlock(s_wchat_queue_lock);
	
	DUER_LOGI("wchat_url[%d]:%s",duer_qcache_length(s_wchat_queue),url_dup);

	return 0;
}

int YTMediaManager::play_wchat_queue()
{
    char *url = NULL;

	duer_mutex_lock(s_wchat_queue_lock);
	int qcache_len = duer_qcache_length(s_wchat_queue);
	if (qcache_len > 0) {
		if ((url = (char*)duer_qcache_pop(s_wchat_queue)) != NULL) {		
			DUER_LOGI("play_queue[%d]",duer_qcache_length(s_wchat_queue));
			duer::YTMediaManager::instance().play_url(url, MEDIA_FLAG_WCHAT);
			DUER_FREE(url);
		}
	}
	duer_mutex_unlock(s_wchat_queue_lock);


#if 0	
	if(qcache_len == 0)
		play_queue();
#endif
}

int YTMediaManager::set_music_queue(const char *url)
{
	if(duer_qcache_length(s_music_queue_lock) > 5){
		DUER_LOGE("url cache > 5");
        return -1;
    }

	int len = strlen(url) + 1; 
    char *url_dup = (char *)DUER_MALLOC(len);
    if (!url_dup) {
		DUER_LOGE("no memory!!");
        return -1;
    }
    snprintf(url_dup, len, "%s", url);
	
    duer_mutex_lock(s_music_queue_lock);
    duer_qcache_push(s_music_queue, url_dup);
    duer_mutex_unlock(s_music_queue_lock);
	
	DUER_LOGI("set_url_to_media_queue[%d]:%s",duer_qcache_length(s_music_queue),url_dup);

	return 0;
}

int YTMediaManager::clear_queue()
{
    char *url = NULL;

    duer_mutex_lock(s_music_queue_lock);
    while ((url = (char*)duer_qcache_pop(s_music_queue)) != NULL) {
		DUER_FREE(url);
    }
    duer_mutex_unlock(s_music_queue_lock);
}

int YTMediaManager::play_queue()
{
    char *url = NULL;
	//DUER_LOGI("play_queue");

	duer_mutex_lock(s_music_queue_lock);
	if (duer_qcache_length(s_music_queue) > 0) {
		if ((url = (char*)duer_qcache_pop(s_music_queue)) != NULL) {		
			DUER_LOGI("play_queue[%d]",duer_qcache_length(s_music_queue));
			duer::YTMediaManager::instance().play_url(url, MEDIA_FLAG_DCS_URL);
			DUER_FREE(url);
		}
	}
	duer_mutex_unlock(s_music_queue_lock);
}

int YTMediaManager::set_previous_media(const char *url, int flags)
{	
	return MediaManager::instance().set_previous_media(url, flags);
}

int YTMediaManager::play_previous_media_continue()
{	
	return MediaManager::instance().play_previous_media_continue();
}

void YTMediaManager::play_url(const char* url, int flags)
{	
	LOG("play_url url:%s",url);
	MediaManager::instance().play_url(url, flags);
}

void YTMediaManager::play_local(const char* path, int flags) 
{	
 	MediaManager::instance().play_local(path,flags);	
}


void YTMediaManager::play_data(const char* data, size_t size, int flags) {	
 	MediaManager::instance().play_data(data,size,flags);
}

void YTMediaManager::play_magic_voice(char* data, size_t size, int flags)	
{
	MediaManager::instance().play_magic_voice(data,size,flags);
}

void YTMediaManager::play_prev()
{
	if(is_bt())
		media_bt_backward();
	//else
		//dcs_play_prev();
}

void YTMediaManager::play_next()
{
	if(is_bt())
		media_bt_forward();
	//else
		//dcs_play_prev();
}

void YTMediaManager::start_bt()
{		
    if (get_media_status() != MEDIA_PLAYER_IDLE) {
        Thread::wait(50);
        if (get_media_status() != MEDIA_PLAYER_IDLE) {
            DUER_LOGE("start_bt fail");
            return;
        }
    }

	MediaManager::instance().bt_mode();
}


void YTMediaManager::switch_bt(void) 
{
	if(!is_bt())
	{	
		play_data(YT_OPEN_BT,sizeof(YT_OPEN_BT), MEDIA_FLAG_BT_TONE);
	}else{
		MediaManager::instance().uart_mode();
		play_data(YT_CLOSE_BT,sizeof(YT_CLOSE_BT), MEDIA_FLAG_PROMPT_TONE);
	}
}
}
