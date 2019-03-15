#include "change_voice.h"
#include "st_mv_inf.h"
#include "amr_codec_cns.h"
#include "duer_recorder.h"
#include "baidu_media_play.h"
#include "memory_interface.h"
#include "lightduer_log.h"


#include "yt_mv_222_interface.h"




namespace duer{

#define YT_MV_FRAME_SHIFT 160

static short pOutFrame[160];
static short pOutFrame_Magic[YT_MV_FRAME_SHIFT];
static unsigned int nOffset = 0;


//for VoiceChanger
static void * pVoiceChanger = NULL;
static char *GLOBAL_pBufferForVoiceChanger = NULL;//[YT_MV_222_CHANGER_SIZE_IN_BYTE];


static void *pDecoderState = NULL;

static char *armnb_data = NULL;
static int armnb_data_len = 0;


void yt_voice_set_wave_head(char *wavHeader)
{
	const char channels = 1;
	const int totalDataLen = 0xffffffff;
	const int sampleRate = 8000;
	const char bitsPerSample = 16;
	const int bytePerSecond = sampleRate * (bitsPerSample / 8) * channels;  
	const int audioDataLen = 0xffffffff;

	//ckid��4�ֽ� RIFF ��־����д  
	wavHeader[0]  = 'R';  
	wavHeader[1]  = 'I';  
	wavHeader[2]  = 'F';  
	wavHeader[3]  = 'F';  

	//cksize��4�ֽ��ļ����ȣ�������Ȳ�����"RIFF"��־(4�ֽ�)���ļ����ȱ�����ռ�ֽ�(4�ֽ�),���ó��ȵ��������ļ����� - 8	
	wavHeader[4]  = (totalDataLen & 0xff);  
	wavHeader[5]  = ((totalDataLen >> 8) & 0xff);  
	wavHeader[6]  = ((totalDataLen >> 16) & 0xff);  
	wavHeader[7]  = ((totalDataLen >> 24) & 0xff);  

	//fcc type��4�ֽ� "WAVE" ���Ϳ��ʶ, ��д  
	wavHeader[8]  = 'W';  
	wavHeader[9]  = 'A';  
	wavHeader[10] = 'V';  
	wavHeader[11] = 'E';  

	//ckid��4�ֽ� ��ʾ"fmt" chunk�Ŀ�ʼ,�˿��а����ļ��ڲ���ʽ��Ϣ��Сд, ���һ���ַ��ǿո�  
	wavHeader[12] = 'f';  
	wavHeader[13] = 'm';  
	wavHeader[14] = 't';  
	wavHeader[15] = ' ';  

	//cksize��4�ֽڣ��ļ��ڲ���ʽ��Ϣ���ݵĴ�С�������ֽڣ�һ��Ϊ00000010H��	
	wavHeader[16] = 0x10;  
	wavHeader[17] = 0;	
	wavHeader[18] = 0;	
	wavHeader[19] = 0;	

	//FormatTag��2�ֽڣ���Ƶ���ݵı��뷽ʽ��1����ʾ��PCM ����  
	wavHeader[20] = 1;	
	wavHeader[21] = 0;	

	//Channels��2�ֽڣ���������������Ϊ1��˫����Ϊ2  
	wavHeader[22] = channels;  
	wavHeader[23] = 0;	

	//SamplesPerSec��4�ֽڣ������ʣ���44100  
	wavHeader[24] = (sampleRate & 0xff);	
	wavHeader[25] = ((sampleRate >> 8) & 0xff);  
	wavHeader[26] = ((sampleRate >> 16) & 0xff);	
	wavHeader[27] = ((sampleRate >> 24) & 0xff);	

	//BytesPerSec��4�ֽڣ���Ƶ���ݴ�������, ��λ���ֽڡ���ֵΪ�����ʡ�ÿ�β�����С������������ô�ֵ���Թ��ƻ������Ĵ�С��	
	//bytePerSecond = sampleRate * (bitsPerSample / 8) * channels  
	wavHeader[28] = (bytePerSecond & 0xff);  
	wavHeader[29] = ((bytePerSecond >> 8) & 0xff);  
	wavHeader[30] = ((bytePerSecond >> 16) & 0xff);  
	wavHeader[31] = ((bytePerSecond >> 24) & 0xff);  

	//BlockAlign��2�ֽڣ�ÿ�β����Ĵ�С = ��������*������/8(��λ���ֽ�); ��Ҳ���ֽڶ������С��λ, Ʃ�� 16bit �������������ֵ�� 4 �ֽڡ�  
	//���������Ҫһ�δ�������ֵ��С���ֽ����ݣ��Ա㽫��ֵ���ڻ������ĵ���  
	wavHeader[32] = (bitsPerSample * channels / 8);  
	wavHeader[33] = 0;	

	//BitsPerSample��2�ֽڣ�ÿ�������Ĳ�������; Ʃ�� 16bit �������ֵ����16������ж����������ÿ�������Ĳ������ȴ�С��һ���ģ�  
	wavHeader[34] = bitsPerSample;  
	wavHeader[35] = 0;	

	//ckid��4�ֽڣ����ݱ�־����data������ʾ "data" chunk�Ŀ�ʼ���˿��а�����Ƶ���ݣ�Сд��	
	wavHeader[36] = 'd';  
	wavHeader[37] = 'a';  
	wavHeader[38] = 't';  
	wavHeader[39] = 'a';  

	//cksize����Ƶ���ݵĳ��ȣ�4�ֽڣ�audioDataLen = totalDataLen - 36 = fileLenIncludeHeader - 44  
	wavHeader[40] = (audioDataLen & 0xff);  
	wavHeader[41] = ((audioDataLen >> 8) & 0xff);  
	wavHeader[42] = ((audioDataLen >> 16) & 0xff);  
	wavHeader[43] = ((audioDataLen >> 24) & 0xff);  
}



int yt_voice_change_init(char *data, int size)
{
	duer_recorder_amr_init_decoder(&pDecoderState);
	
	if(!pDecoderState)
	{
		DUER_LOGE("pDecoderState NULL");
		return -1;
	}



#if 0 //TERENCE---2019/03/05
	if(!pVoiceChanger)pVoiceChanger = YT_MV_InitChanger();
		
	if(!pVoiceChanger)
	{		
		if(pDecoderState)
			YT_NB_AMR_FreeDecoder(&pDecoderState);
		
		DUER_LOGE("pVoiceChanger NULL");
		return -1;
	}

	YT_MV_SetConfig(pVoiceChanger, 1.0, 1.0, 8);
#else

if(NULL == pVoiceChanger)
{
	if(NULL == GLOBAL_pBufferForVoiceChanger)
		{
			GLOBAL_pBufferForVoiceChanger = (char*)memory_malloc(YT_MV_222_CHANGER_SIZE_IN_BYTE);
		}

	

	if(NULL != GLOBAL_pBufferForVoiceChanger)
		{

			pVoiceChanger = YT_MV_222_InitChanger(GLOBAL_pBufferForVoiceChanger, YT_MV_222_CHANGER_SIZE_IN_BYTE);
			if(!pVoiceChanger)
			{		
				if(pDecoderState)
					YT_NB_AMR_FreeDecoder(&pDecoderState);
				
				DUER_LOGE("pVoiceChanger NULL");
				return -1;
			}	
		}

	
}


#endif

	
	nOffset = 0;
	int nReturn = YT_NB_AMR_GetDataInfo(data, size, &nOffset);

	if(nReturn != 0)nOffset = 0;
		

	armnb_data = data;
	armnb_data_len = size;
	
	return 0;
}

int yt_voice_change_get_data(char **data, int *size)
{
	int nReturn = 0;
	unsigned int nSampleNumber = 0;
	int nSampleNumber_Magic = 0;
	int i = 0, nOutSampleNumber, nTemp;

	nReturn = YT_NB_AMR_DecodeFrame(pDecoderState,armnb_data,armnb_data_len,&nOffset,						   
									pOutFrame,&nSampleNumber);
	
	if(nSampleNumber > 0) 
	{ 

	    /*
		for(i = 0; i < nSampleNumber; i++)
			pOutFrame[i] /= 2;
		*/


#if 0		
		//*size = YT_MV_ChangeFrame(pVoiceChanger,pOutFrame,YT_MV_FRAME_SHIFT,pOutFrame_Magic,YT_MV_FRAME_SHIFT) * sizeof(short);
		//*data = (char*)&pOutFrame_Magic[0];
		//*size = nOutSampleNumber*2;

		*data = (char*)&pOutFrame[0];
		*size = nSampleNumber*2;	
#else
		//TERENCE---2019/03/05
		YT_MV_222_ChangeFrame(pVoiceChanger, pOutFrame, YT_MV_222_FRAME_SHIFT, 0, 0, pOutFrame_Magic, &nOutSampleNumber);
        ///*
		for(i = 0; i < nOutSampleNumber; i++)
		{
			nTemp = pOutFrame_Magic[i] ;
			nTemp *= 2;
			
			if(nTemp > 32767) nTemp = 32767;
			else if(nTemp < -32768) nTemp = -32768;

			pOutFrame_Magic[i] = (short) nTemp;
		}
		//*/

		*data = (char*)&pOutFrame_Magic[0];
		*size = nOutSampleNumber*2;	
#endif

	}
	
	if(0 == nReturn) 
		return 0;
	else
		return 1;
}

void yt_voice_change_free()
{
#if 0
	YT_MV_FreeChanger(pVoiceChanger);
#else

	if(NULL != GLOBAL_pBufferForVoiceChanger)
	{
		memory_free(GLOBAL_pBufferForVoiceChanger);
		GLOBAL_pBufferForVoiceChanger = NULL;		
	}
	
		
	 YT_MV_222_FreeChanger(pVoiceChanger);
	
	pVoiceChanger = NULL;
	
	if(pDecoderState)
		YT_NB_AMR_FreeDecoder(&pDecoderState);
}
#endif
}
