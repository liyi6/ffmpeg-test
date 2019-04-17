
#include "SDL2/SDL.h"
#include "stdio.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

const char* file_path = "oceans.mp4";

int main(int argc, char* argv[]) {
    
    printf("--------------------------------ffmpeg-sdl2-video-player start-------------------------\n");
    
    av_register_all();

    AVFormatContext* avformat_context = avformat_alloc_context();
    if (!avformat_context) {
        printf("avformat_alloc_context fail.");
        return -1;
    }

    if (avformat_open_input(&avformat_context, file_path, NULL, NULL)) {
        printf("avformat_open_input fail.");
        return -2;
    }

    if (avformat_find_stream_info(avformat_context, NULL)) {
        printf("avformat_find_stream_info fail.");
        return -3;
    }

    int video_stream_index = -1;
    for (int i = 0; i < avformat_context->nb_streams; i++) {
        if (avformat_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        printf("cant find video stream in file.");
        return -4;
    }

    AVCodec* codec = avcodec_find_decoder(avformat_context->streams[video_stream_index]->codecpar->codec_id);
    if (!codec) {
        printf("cant find codec.");
        return -5;
    }

    AVCodecContext* codec_context = avformat_context->streams[video_stream_index]->codec;
    if (avcodec_open2(codec_context, codec, NULL)) {
        printf("cant open codec");
        return -6;
    }


    AVPacket* packet = av_packet_alloc();       // 读取的未解码包
    AVFrame* avframe = av_frame_alloc();        // 解码后的帧
    AVFrame* yuvFrame = av_frame_alloc();       // 用于SDL显示的帧, 由avframe转化

    /* 计算一帧图片的大小 */
    int frame_buffer_size = av_image_get_buffer_size(codec_context->pix_fmt, codec_context->width, codec_context->height, 1);
    if (frame_buffer_size < 0) {
        printf("Open codec fail.");
        return -7;
    }

    /* 分配一帧大小的buffer */
    unsigned char* frame_buffer = (unsigned char *)av_malloc(frame_buffer_size);

    /* 根据后四个数据填充前两个数据， 具体分析需要看AVFrame结构体的data和linesize是如何表征不同像素格式及其内存布局的 */
    av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, frame_buffer, codec_context->pix_fmt, codec_context->width, codec_context->height, 1);
  
    /* 创建一个SwsContext上下文，可以用于指示sws_scale函数将解码后的数据，转换成想要的格式 */
    SwsContext* img_convert_ctx = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt, codec_context->width, codec_context->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if (!img_convert_ctx) {
        printf("sws_getContext  fail.");
        return -8;
    }
    
    /* 初始化SDL库 */
    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -8;
    }

    /* 创建SDL窗体 */
    SDL_Window* window = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, codec_context->width, codec_context->height, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -9;
    }

    /* 创建渲染器 */
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    /* 创建纹理 */
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, codec_context->width, codec_context->height);

    /* 创建需要在窗体上渲染的区域 */
    SDL_Rect rect{ 0, 0, codec_context->width, codec_context->height };

    /* 开始读取帧，并解码 */
    while (av_read_frame(avformat_context, packet) >= 0){  // 从文件中读取一帧数据放入AVPacket中
        if (packet->stream_index == video_stream_index){   // 如果此帧数据是第一个视频流的数据，则开始解码
            /* 解码packet，存入frame中，got_picture非0表示有可解码的数据，返回值小于0表示解码出错*/
            int got_picture = 0;
            int ret = avcodec_decode_video2(codec_context, avframe, &got_picture, packet);
            if (ret < 0){
                printf("Decode Error.\n");
                return -10;
            }

            if (got_picture){
                /* 将解码后的图像转换成YUV420P planar图像 */
                sws_scale(img_convert_ctx, (const unsigned char* const*)avframe->data, avframe->linesize, 0, codec_context->height, yuvFrame->data, yuvFrame->linesize);

                /* 渲染该图像 */
                SDL_UpdateYUVTexture(texture, &rect, yuvFrame->data[0], yuvFrame->linesize[0], yuvFrame->data[1], yuvFrame->linesize[1], yuvFrame->data[2], yuvFrame->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                SDL_RenderPresent(renderer);

                /* 延迟40s后读取并解码下一帧， 即帧率约25*/
                SDL_Delay(40);
            }
        }
        av_free_packet(packet);
    }

    sws_freeContext(img_convert_ctx);

    printf("--------------------------------ffmpeg-sdl2-video-player end-------------------------\n");
    
    getchar();

    return 0;
}