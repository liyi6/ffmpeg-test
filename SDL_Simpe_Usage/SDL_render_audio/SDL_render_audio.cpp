#include "SDL.h"
#include "stdio.h"

Uint32  audio_data_len = 0;
Uint8  *audio_data_pos = NULL;

static void av_read_audio(void *userdata, Uint8 * stream, int len) {
	/**
	* lenֵ�����Audio Buffer�Ĵ�С4096
	*/
	SDL_memset(stream, 0, len);
	if (audio_data_len == 0) {
		return;
	}
	len = (len > audio_data_len ? audio_data_len : len);	

	/**
	* �ӻ���buffer�п������ݵ�Audio Buffer
	*/
	SDL_MixAudio(stream, audio_data_pos, len, SDL_MIX_MAXVOLUME);
	
	/*�޸Ļ����ָ��*/
	audio_data_pos += len;
	audio_data_len -= len;
}

int main(int argc, char* argv[]) {

	/**
	* ��ʼ��audio��ϵͳ��timer��ϵͳ��
	* timer��ϵͳ�к���SDL_Delay�����������߳��ö�һ��ms��(����ͣ����һ��ms)
	*/
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("SDL Init audio and timer system error: %s", SDL_GetError());
		return -1;
	}

	/**
	*  ע���ļ��򿪷�ʽrb����ֻ�������Ƶ���ʽ�򿪣�
	*  ǧ��ǵ�b�������ı���ʽ�򿪻��޷�����
	*/
	FILE* file = fopen("pcm_2_s16le_44100_34s.pcm", "rb"); 
	if (!file) {
		printf("Open audio file pcm_2_s16L2_44100.pcm error");
		return -2;
	}

	SDL_AudioSpec audioSpec;               
	audioSpec.freq = 44100;              /**< DSP frequency -- samples per second */
	audioSpec.format = AUDIO_S16LSB;     /**< Audio data format */ /*16λ�з���������С�˸�ʽs16le*/
	audioSpec.channels = 2;              /**< Number of channels: 1 mono, 2 stereo */
	audioSpec.silence =  0;              /**< Audio buffer silence value (calculated) */
	audioSpec.samples = 1024;            /**< Audio buffer size in sample FRAMES (total samples divided by channel count) 
										    ���յ�ǰ�Ĳ�����ʽ����ô��������СΪ1024*2*2=4096����������*������������ռbytes*ͨ����
											*/
	audioSpec.userdata = NULL;           /**< Userdata passed to callback (ignored for NULL callbacks). */
	audioSpec.callback = av_read_audio;

	SDL_AudioSpec audioSpec2;

	/**
	*  This function opens the audio device with the desired parameters, and
    *  returns 0 if successful, placing the actual hardware parameters in the
    *  structure pointed to by \c obtained.  If \c obtained is NULL, the audio
    *  data passed to the callback function will be guaranteed to be in the
    *  requested format, and will be automatically converted to the hardware
    *  audio format if necessary.  This function returns -1 if it failed
    *  to open the audio device, or couldn't set up the audio thread.
	*/
	if (SDL_OpenAudio(&audioSpec, &audioSpec2)) {
		printf("SDL_OpenAudio  error: %s", SDL_GetError());
		return -3;
	}

	/**
	* ��ӡAudio buffer�Ĵ�С���˴�Ӧ����4096
	*/
	printf("Audio buffer size : %d ", audioSpec2.size);

	/**
	* ������Ƶ����
	*/
	SDL_PauseAudio(0);

	/** 
	*  ��ȡ�ļ��Ļ����С���ڵ���Audio buffer��
	*  ���򲥷Ų�������������
	*/
	int buffer_size = 8192;  
	char* buffer = (char*) malloc(buffer_size);

	/**
	* ���ŵĻ���˼·�Ǵ�pcm�ļ��ж�ȡbuffer_size��С�����ݵ�buffer��
	* ���߳̽���wait״̬
	* �ȴ�buffer�е����ݱ����Ƶ�Audio Buffer����������ٴζ�ȡ����
	* �����������ݲ���buffer_size��С��ʱ�����ñ�־λ����ʾ�����һ�ζ�ȡ����
	* �ȴ����һ�ζ�ȡ�����ݱ����Ƶ�Audio Buffer�������ź��˳����߳�ѭ��
	*/
	bool exit = false;
	while (!exit) {
		int size = fread(buffer, 1, buffer_size, file);
		if (size != buffer_size) {        // ����ʣ����Ƶ����  
			exit = true;
		} 
		audio_data_pos = (Uint8 *)buffer;
		audio_data_len = size;
		while (audio_data_len > 0){      
			SDL_Delay(1);
		}
	}

	fclose(file);
	delete buffer;
	SDL_Quit();

	getchar();
	return 0;
}