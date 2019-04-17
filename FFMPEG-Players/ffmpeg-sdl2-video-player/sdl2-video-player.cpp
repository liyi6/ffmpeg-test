
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


    AVPacket* packet = av_packet_alloc();       // ��ȡ��δ�����
    AVFrame* avframe = av_frame_alloc();        // ������֡
    AVFrame* yuvFrame = av_frame_alloc();       // ����SDL��ʾ��֡, ��avframeת��

    /* ����һ֡ͼƬ�Ĵ�С */
    int frame_buffer_size = av_image_get_buffer_size(codec_context->pix_fmt, codec_context->width, codec_context->height, 1);
    if (frame_buffer_size < 0) {
        printf("Open codec fail.");
        return -7;
    }

    /* ����һ֡��С��buffer */
    unsigned char* frame_buffer = (unsigned char *)av_malloc(frame_buffer_size);

    /* ���ݺ��ĸ��������ǰ�������ݣ� ���������Ҫ��AVFrame�ṹ���data��linesize����α�����ͬ���ظ�ʽ�����ڴ沼�ֵ� */
    av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, frame_buffer, codec_context->pix_fmt, codec_context->width, codec_context->height, 1);
  
    /* ����һ��SwsContext�����ģ���������ָʾsws_scale���������������ݣ�ת������Ҫ�ĸ�ʽ */
    SwsContext* img_convert_ctx = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt, codec_context->width, codec_context->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if (!img_convert_ctx) {
        printf("sws_getContext  fail.");
        return -8;
    }
    
    /* ��ʼ��SDL�� */
    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -8;
    }

    /* ����SDL���� */
    SDL_Window* window = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, codec_context->width, codec_context->height, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -9;
    }

    /* ������Ⱦ�� */
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    /* �������� */
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, codec_context->width, codec_context->height);

    /* ������Ҫ�ڴ�������Ⱦ������ */
    SDL_Rect rect{ 0, 0, codec_context->width, codec_context->height };

    /* ��ʼ��ȡ֡�������� */
    while (av_read_frame(avformat_context, packet) >= 0){  // ���ļ��ж�ȡһ֡���ݷ���AVPacket��
        if (packet->stream_index == video_stream_index){   // �����֡�����ǵ�һ����Ƶ�������ݣ���ʼ����
            /* ����packet������frame�У�got_picture��0��ʾ�пɽ�������ݣ�����ֵС��0��ʾ�������*/
            int got_picture = 0;
            int ret = avcodec_decode_video2(codec_context, avframe, &got_picture, packet);
            if (ret < 0){
                printf("Decode Error.\n");
                return -10;
            }

            if (got_picture){
                /* ��������ͼ��ת����YUV420P planarͼ�� */
                sws_scale(img_convert_ctx, (const unsigned char* const*)avframe->data, avframe->linesize, 0, codec_context->height, yuvFrame->data, yuvFrame->linesize);

                /* ��Ⱦ��ͼ�� */
                SDL_UpdateYUVTexture(texture, &rect, yuvFrame->data[0], yuvFrame->linesize[0], yuvFrame->data[1], yuvFrame->linesize[1], yuvFrame->data[2], yuvFrame->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                SDL_RenderPresent(renderer);

                /* �ӳ�40s���ȡ��������һ֡�� ��֡��Լ25*/
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