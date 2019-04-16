#include "SDL.h"
#include "stdio.h"
#include "SDL_image.h"

int main(int argc, char* argv[]) {

	// ��ʼ��SDL������ֻ����Ⱦһ��ͼƬ�����ֻ��Ҫ��ʼ��Video����
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("Init SDL error: %s", SDL_GetError());
		return -1;
	}

	//Ϊ����ʾJPGͼƬ������ʹ����ͼƬ�⣬����Ҫ������ʼ��
	IMG_Init(IMG_INIT_JPG);

	// ����SDL_Window���壬�������Ļ���
	SDL_Window* window = SDL_CreateWindow("This is a SDL window to reder",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Create SDL window error: %s", SDL_GetError());
		return -2;
	}

	// ����SDL_Renderer��Ⱦ�����������Ļ���
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
		printf("Create SDL renderer error: %s", SDL_GetError());
		return -3;
	}

	// ������Ҫ��ʾ��ͼƬ���洢��surface
	SDL_Surface* surface1 = SDL_LoadBMP("hello.bmp");
	if (!surface1) {
		printf("Load hello.bmp error: %s", SDL_GetError());
		return -4;
	}

	SDL_Surface* surface2 = IMG_Load("sdl_imag.jpg");
	if (!surface2) {
		printf("SDL_Image sdl_imag.jpg error: %s", IMG_GetError());
		return -5;
	}

	// ��surface�п���ͼƬ�γ�����, ��Ҫ��ʾ������
	SDL_Texture* texture1 = SDL_CreateTextureFromSurface(renderer, surface1);
	if (!texture1) {
		printf("Create texture from surface1 error: %s", SDL_GetError());
		return -6;
	}

	SDL_Texture* texture2 = SDL_CreateTextureFromSurface(renderer, surface2);
	if (!texture2) {
		printf("Create texture from surface2 error: %s", SDL_GetError());
		return -7;
	}

	// ������Ⱦ����
	SDL_Rect rect1{ 0, 0, 320, 240 };
	SDL_Rect rect2{ 320, 240, 320, 240 };

	// ��֪��Ⱦ������ڴ�����ĸ�λ����Ⱦ����
	SDL_RenderCopy(renderer, texture1, NULL, &rect1);
	SDL_RenderCopy(renderer, texture2, NULL, &rect2);

	// ��Ⱦ
	SDL_RenderPresent(renderer);

	// �ӳ�10s������������
	SDL_Delay(15000);

	// ���ٴ�����������㣬��Ⱦ�������壬ע������˳���봴��˳���෴��
	SDL_DestroyTexture(texture1);
	SDL_DestroyTexture(texture2);
	SDL_FreeSurface(surface1);
	SDL_FreeSurface(surface2);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	IMG_Quit();
	SDL_Quit();

	return 0;
}
