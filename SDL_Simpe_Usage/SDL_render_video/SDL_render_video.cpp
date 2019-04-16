#include <stdio.h>
#include "SDL.h"

//Video Refresh Event
#define REFRESH_EVENT       (SDL_USEREVENT + 1)   // ��֪SDL��Ϣѭ����ʼ��Ⱦ�µ�һ֡��Ƶ
#define EXIT_REFRESH_EVENT  (SDL_USEREVENT + 2)   // ��֪SDL��Ϣѭ��ˢ���߳��Ѿ�ֹͣ���������ŵ��˳���Ϣѭ��

struct VideoStatus{
	bool video_thread_exit{false};    // ˢ���߳��˳���־λ
	bool exit{ false };               // SDL��Ϣѭ���˳���־λ
	int screen_width{ 352 };          // ��Ƶ����Ŀ��
	int screen_height{ 288 };         // ��Ƶ����Ŀ��
	int pixel_w{ 352 };               // ��Ƶ�Ŀ��(���ظ���)
	int pixel_h{ 288 };               // ��Ƶ�ĸ߶�
	int bits_per_pixel{ 12 };         // ��Ƶ����������ռbits�������ظ�ʽ�йأ��������õĶ���yuv420p��ͼ����˵�������12bits
	int loop{ 3 };                    // ������Ƶѭ������
	int delay{ 40 };                  // ÿ��delay����������һ֡��Ƶ��Ĭ��40msҲ��1s����25֡
	Uint32 pix_format{ SDL_PIXELFORMAT_IYUV };  // ���ظ�ʽ��yuv420p�����ظ�ʽΪSDL_PIXELFORMAT_IYUV  /**< Planar mode: Y + U + V  (3 planes) */
};

VideoStatus is;

int videoRefreshThread(void *data) {
	while (!is.video_thread_exit) {    // ÿ��delay���뷢��һ��REFRESH_EVENT���Զ���SDL��Ϣ
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(is.delay);
	}

	SDL_Event event;                  // �߳��˳�ǰ�������Զ����SDL��ϢEXIT_REFRESH_EVENT
	event.type = EXIT_REFRESH_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

int main(int argc, char* agrv[]) {

	/**
	 * ��ʼ��SDL
	 * ����ֻ����Ⱦ��Ƶ�����ֻ�ó�ʼ��Video��ϵͳ��
	 * ��Ȼ��ʼ��Video��ϵͳ��Ĭ��Ҳ���ʼ��Event��ϵͳ
	 */
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL init error: %s", SDL_GetError());
		return -1;
	}

	/**
	 * ����SDL���壬��Ȼ���
	 * ���һ��������ʾʹ��OpenGL��Ⱦ������ѡ����Բμ�SDL_CreateWindow����˵��
 	 */
	SDL_Window* window = SDL_CreateWindow("This is my SDL video renderer", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, is.screen_width, is.screen_height, SDL_WINDOW_OPENGL);
	if (!window) {
		printf("SDL create window error: %s", SDL_GetError());
		return -2;
	}

	/**
	 * ����SDL��Ⱦ������Ȼ���
	 * ���һ��������ʾʹ���������Ⱦ(ռCPU)��������ѡ��Ӳ����Ⱦ(ʹ��GPU)��
	 * ����ѡ���SDL_CreateRenderer˵��
	 */
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
		printf("SDL create renderer error: %s", SDL_GetError());
		return -3;
	}

	/**
	 * ����SDL���������Ҫ��������
	 * �ڶ������������ظ�ʽ IYUV ������ʾ����Ƶʹ�õ���yuv420p��ʽ�ģ���˾��������ظ�ʽΪIYUV��
	 * ������������Texture��Accessģʽ��SDL_TEXTUREACCESS_STREAMING��ʾ����Texture��Ҫ��������Ϊ�ǲ�����Ƶ����ҪƵ������Texture
	 * ���� �������������Ŀ�ߣ�����Ϊʾ����Ƶ�Ŀ�ߣ�
	 */
	SDL_Texture* texture = SDL_CreateTexture(renderer, is.pix_format, SDL_TEXTUREACCESS_STREAMING, is.pixel_w, is.pixel_h);
	if (!texture) {
		printf("SDL create texture error: %s", SDL_GetError());
		return -4;
	}

	/** ��ʾ���ļ� **/
	FILE* fp = fopen("BUS_352x288_30_orig_01.yuv", "rb+");
	if (!fp){
		printf("Cannot open file BUS_352x288_30_orig_01.yuv\n");
		return -5;
	}

	/** 
	 * ����һ֡ͼ��Ĵ�С  pixel_w * pixel_h * bits_per_pixel/8
	 * ͼ��ģ���*��*����������ռ������/8����������yuv420p��ͼ�����Ե���������ռ����Ϊ12(YΪ8bits��uv��Ϊ2bits)
	 */
	int frameSize = is.pixel_w * is.pixel_h * is.bits_per_pixel / 8;
	unsigned char* frame_buffer = new unsigned char[frameSize]{ 0 };
	
	/**
	 * ����ˢ���̣߳�����ÿ���೤ʱ�䣨ʹ��SDL_Delay(msec)������һ��ˢ�µ�SDL_Event��SDL��Ϣѭ����
	 * �Ӷ�����ÿ���೤ʱ��ȥ��ȡ����Ⱦһ֡��Ƶ��
	 */
	SDL_CreateThread(videoRefreshThread, "refresh thread", NULL);

	SDL_Rect renderArea;
	SDL_Event event;
	while (!is.exit) {
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case REFRESH_EVENT: {    // ˢ��event�����ļ���ȡһ֡��Ƶ������Ⱦ
			if ((frameSize) != fread(frame_buffer, 1,  frameSize,  fp)) {
				if (is.loop--) {
					fseek(fp, 0, SEEK_SET);
					fread(frame_buffer, 1, frameSize, fp);
				}
				else {
					is.video_thread_exit = true;   // ѭ��������ϣ�ʹˢ���߳��˳���־��λ
				}
			}

			if (!is.video_thread_exit) {
				/**
                 * ������������Ҫ��ʾ����Ƶ֡����
				 * ���һ��������ԭʼһ������ռbytes������Ȼ��һ�����ظ���*����������ռbits/8
				 */
				SDL_UpdateTexture(texture, NULL, frame_buffer, is.pixel_w*is.bits_per_pixel/8);

				/**
				 * �������С�仯ʱ�����¼�¼�����С
				 * ������Ⱦ����ռ����������
				 */
				renderArea.x = 0;
				renderArea.y = 0;
				renderArea.w = is.screen_width;
				renderArea.h = is.screen_height;

				/**
				 * ���renderer���ⲽ����Ҳû���⣬SDL_RenderCopy��ִ���������
				 */
				SDL_RenderClear(renderer);
				
				/**
				 * ��׼���õ������ݽ�����Ⱦ��������֪��Ⱦ�ڴ�����ĸ�����
				 */
				SDL_RenderCopy(renderer, texture, NULL, &renderArea);

				/**
				 * ��Ⱦ
				 */
				SDL_RenderPresent(renderer);
			}
			break;
		}
		case SDL_QUIT: {                   // ��ӦSDL_QUIT��Ϣ�������������ر�ʱ����ֱ���˳���Ϣѭ����
			is.video_thread_exit = true;   // ���Ǹ��߳��˳���־��λ����ˢ���߳����Ž�����ˢ���߳̽�����
			break;                         // ����EXIT_REFRESH_EVENT������Ӧ����Ϣʱ����Ϣѭ���˳���־��λ
		}
		case EXIT_REFRESH_EVENT: {         // ˢ���߳��˳���ᷢ�ʹ���Ϣ����Ϣѭ��������Ϣѭ���˳���־λ��λ
			is.exit = true;
			break;
		}
		case SDL_WINDOWEVENT: {            // ��Ӧ���ڱ仯�����¼�¼�����С
			SDL_GetWindowSize(window, &is.screen_width, &is.screen_height);
			break;
		}
		default:
			break;
		}
		
	}

	/* �����봴���෴��˳�����ٶ��� */
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}