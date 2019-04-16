#include "SDL.h"
#include "stdio.h"

Uint32  audio_data_len = 0;
Uint8  *audio_data_pos = NULL;

static void av_read_audio(void *userdata, Uint8 * stream, int len) {
	/**
	* len值会等于Audio Buffer的大小4096
	*/
	SDL_memset(stream, 0, len);
	if (audio_data_len == 0) {
		return;
	}
	len = (len > audio_data_len ? audio_data_len : len);	

	/**
	* 从缓存buffer中拷贝数据到Audio Buffer
	*/
	SDL_MixAudio(stream, audio_data_pos, len, SDL_MIX_MAXVOLUME);
	
	/*修改缓存的指针*/
	audio_data_pos += len;
	audio_data_len -= len;
}

int main(int argc, char* argv[]) {

	/**
	* 初始化audio子系统和timer子系统，
	* timer子系统中含有SDL_Delay函数，能让线程让渡一定ms数(即暂停运行一定ms)
	*/
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("SDL Init audio and timer system error: %s", SDL_GetError());
		return -1;
	}

	/**
	*  注意文件打开方式rb，以只读二进制的形式打开，
	*  千万记得b，否则按文本方式打开会无法播放
	*/
	FILE* file = fopen("pcm_2_s16le_44100_34s.pcm", "rb"); 
	if (!file) {
		printf("Open audio file pcm_2_s16L2_44100.pcm error");
		return -2;
	}

	SDL_AudioSpec audioSpec;               
	audioSpec.freq = 44100;              /**< DSP frequency -- samples per second */
	audioSpec.format = AUDIO_S16LSB;     /**< Audio data format */ /*16位有符号整数，小端格式s16le*/
	audioSpec.channels = 2;              /**< Number of channels: 1 mono, 2 stereo */
	audioSpec.silence =  0;              /**< Audio buffer silence value (calculated) */
	audioSpec.samples = 1024;            /**< Audio buffer size in sample FRAMES (total samples divided by channel count) 
										    按照当前的采样格式，那么缓冲区大小为1024*2*2=4096，采样点数*单个采样点所占bytes*通道数
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
	* 打印Audio buffer的大小，此处应该是4096
	*/
	printf("Audio buffer size : %d ", audioSpec2.size);

	/**
	* 开启音频播放
	*/
	SDL_PauseAudio(0);

	/** 
	*  读取文件的缓存大小大于等于Audio buffer，
	*  否则播放不正常有噪音等
	*/
	int buffer_size = 8192;  
	char* buffer = (char*) malloc(buffer_size);

	/**
	* 播放的基本思路是从pcm文件中读取buffer_size大小的数据到buffer中
	* 主线程进入wait状态
	* 等待buffer中的数据被复制到Audio Buffer并播放完后，再次读取数据
	* 当读到的数据不足buffer_size大小的时候，设置标志位，表示是最后一次读取数据
	* 等待最后一次读取的数据被复制到Audio Buffer并被播放后，退出主线程循环
	*/
	bool exit = false;
	while (!exit) {
		int size = fread(buffer, 1, buffer_size, file);
		if (size != buffer_size) {        // 处理剩余音频数据  
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