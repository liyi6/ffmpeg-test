#include <stdio.h>
#include "SDL.h"

//Video Refresh Event
#define REFRESH_EVENT       (SDL_USEREVENT + 1)   // 告知SDL消息循环开始渲染新的一帧视频
#define EXIT_REFRESH_EVENT  (SDL_USEREVENT + 2)   // 告知SDL消息循环刷新线程已经停止，可以优雅的退出消息循环

struct VideoStatus{
	bool video_thread_exit{false};    // 刷新线程退出标志位
	bool exit{ false };               // SDL消息循环退出标志位
	int screen_width{ 352 };          // 视频窗体的宽度
	int screen_height{ 288 };         // 视频窗体的宽度
	int pixel_w{ 352 };               // 视频的宽度(像素个数)
	int pixel_h{ 288 };               // 视频的高度
	int bits_per_pixel{ 12 };         // 视频单个像素所占bits，与像素格式有关，本例采用的都是yuv420p的图像，因此单个像素12bits
	int loop{ 3 };                    // 播放视频循环次数
	int delay{ 40 };                  // 每隔delay毫秒数播放一帧视频，默认40ms也即1s播放25帧
	Uint32 pix_format{ SDL_PIXELFORMAT_IYUV };  // 像素格式，yuv420p的像素格式为SDL_PIXELFORMAT_IYUV  /**< Planar mode: Y + U + V  (3 planes) */
};

VideoStatus is;

int videoRefreshThread(void *data) {
	while (!is.video_thread_exit) {    // 每隔delay毫秒发送一次REFRESH_EVENT的自定义SDL消息
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(is.delay);
	}

	SDL_Event event;                  // 线程退出前，发送自定义的SDL消息EXIT_REFRESH_EVENT
	event.type = EXIT_REFRESH_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

int main(int argc, char* agrv[]) {

	/**
	 * 初始化SDL
	 * 由于只是渲染视频，因此只用初始化Video子系统，
	 * 当然初始化Video子系统，默认也会初始化Event子系统
	 */
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL init error: %s", SDL_GetError());
		return -1;
	}

	/**
	 * 创建SDL窗体，类比画布
	 * 最后一个参数表示使用OpenGL渲染，其他选项可以参见SDL_CreateWindow函数说明
 	 */
	SDL_Window* window = SDL_CreateWindow("This is my SDL video renderer", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, is.screen_width, is.screen_height, SDL_WINDOW_OPENGL);
	if (!window) {
		printf("SDL create window error: %s", SDL_GetError());
		return -2;
	}

	/**
	 * 创建SDL渲染器，类比画笔
	 * 最后一个参数表示使用了软件渲染(占CPU)，还可以选择硬件渲染(使用GPU)，
	 * 其他选项见SDL_CreateRenderer说明
	 */
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
		printf("SDL create renderer error: %s", SDL_GetError());
		return -3;
	}

	/**
	 * 创建SDL纹理，类比需要画的内容
	 * 第二个参数是像素格式 IYUV （由于示例视频使用的是yuv420p格式的，因此决定了像素格式为IYUV）
	 * 第三个参数是Texture的Access模式，SDL_TEXTUREACCESS_STREAMING表示访问Texture需要加锁，因为是播放视频，需要频繁更新Texture
	 * 第四 五个参数是文理的宽高（设置为示例视频的宽高）
	 */
	SDL_Texture* texture = SDL_CreateTexture(renderer, is.pix_format, SDL_TEXTUREACCESS_STREAMING, is.pixel_w, is.pixel_h);
	if (!texture) {
		printf("SDL create texture error: %s", SDL_GetError());
		return -4;
	}

	/** 打开示例文件 **/
	FILE* fp = fopen("BUS_352x288_30_orig_01.yuv", "rb+");
	if (!fp){
		printf("Cannot open file BUS_352x288_30_orig_01.yuv\n");
		return -5;
	}

	/** 
	 * 计算一帧图像的大小  pixel_w * pixel_h * bits_per_pixel/8
	 * 图像的（宽*高*单个像素所占比特数/8），由于是yuv420p的图像，所以单个像素所占比特为12(Y为8bits，uv各为2bits)
	 */
	int frameSize = is.pixel_w * is.pixel_h * is.bits_per_pixel / 8;
	unsigned char* frame_buffer = new unsigned char[frameSize]{ 0 };
	
	/**
	 * 开启刷新线程，控制每隔多长时间（使用SDL_Delay(msec)）发送一个刷新的SDL_Event到SDL消息循环，
	 * 从而控制每隔多长时间去读取并渲染一帧视频。
	 */
	SDL_CreateThread(videoRefreshThread, "refresh thread", NULL);

	SDL_Rect renderArea;
	SDL_Event event;
	while (!is.exit) {
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case REFRESH_EVENT: {    // 刷新event，从文件读取一帧视频进行渲染
			if ((frameSize) != fread(frame_buffer, 1,  frameSize,  fp)) {
				if (is.loop--) {
					fseek(fp, 0, SEEK_SET);
					fread(frame_buffer, 1, frameSize, fp);
				}
				else {
					is.video_thread_exit = true;   // 循环播放完毕，使刷新线程退出标志置位
				}
			}

			if (!is.video_thread_exit) {
				/**
                 * 更新文理，将需要显示的视频帧数据
				 * 最后一个参数是原始一行数据占bytes数，当然是一行像素个数*单个像素所占bits/8
				 */
				SDL_UpdateTexture(texture, NULL, frame_buffer, is.pixel_w*is.bits_per_pixel/8);

				/**
				 * 当窗体大小变化时，更新记录窗体大小
				 * 控制渲染区域占据整个窗体
				 */
				renderArea.x = 0;
				renderArea.y = 0;
				renderArea.w = is.screen_width;
				renderArea.h = is.screen_height;

				/**
				 * 清空renderer，这步不做也没问题，SDL_RenderCopy将执行这个操作
				 */
				SDL_RenderClear(renderer);
				
				/**
				 * 将准备好的纹理传递交给渲染器，并告知渲染在窗体的哪个区域
				 */
				SDL_RenderCopy(renderer, texture, NULL, &renderArea);

				/**
				 * 渲染
				 */
				SDL_RenderPresent(renderer);
			}
			break;
		}
		case SDL_QUIT: {                   // 响应SDL_QUIT消息，当窗口主动关闭时，不直接退出消息循环，
			is.video_thread_exit = true;   // 而是给线程退出标志置位，让刷新线程优雅结束。刷新线程结束后，
			break;                         // 推送EXIT_REFRESH_EVENT，在响应此消息时给消息循环退出标志置位
		}
		case EXIT_REFRESH_EVENT: {         // 刷新线程退出后会发送此消息到消息循环，给消息循环退出标志位置位
			is.exit = true;
			break;
		}
		case SDL_WINDOWEVENT: {            // 响应窗口变化，更新记录窗体大小
			SDL_GetWindowSize(window, &is.screen_width, &is.screen_height);
			break;
		}
		default:
			break;
		}
		
	}

	/* 按照与创建相反的顺序销毁对象 */
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}