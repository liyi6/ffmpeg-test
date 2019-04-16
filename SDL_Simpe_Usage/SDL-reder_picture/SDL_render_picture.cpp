#include "SDL.h"
#include "stdio.h"
#include "SDL_image.h"

int main(int argc, char* argv[]) {

	// 初始化SDL，由于只是渲染一张图片，因此只需要初始化Video即可
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("Init SDL error: %s", SDL_GetError());
		return -1;
	}

	//为了显示JPG图片，额外使用了图片库，所以要单独初始化
	IMG_Init(IMG_INIT_JPG);

	// 创建SDL_Window窗体，即作画的画布
	SDL_Window* window = SDL_CreateWindow("This is a SDL window to reder",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Create SDL window error: %s", SDL_GetError());
		return -2;
	}

	// 创建SDL_Renderer渲染器，即作画的画笔
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
		printf("Create SDL renderer error: %s", SDL_GetError());
		return -3;
	}

	// 加载需要显示的图片，存储到surface
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

	// 从surface中拷贝图片形成纹理, 即要显示的内容
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

	// 设置渲染区域
	SDL_Rect rect1{ 0, 0, 320, 240 };
	SDL_Rect rect2{ 320, 240, 320, 240 };

	// 告知渲染器如何在窗体的哪个位置渲染文理
	SDL_RenderCopy(renderer, texture1, NULL, &rect1);
	SDL_RenderCopy(renderer, texture2, NULL, &rect2);

	// 渲染
	SDL_RenderPresent(renderer);

	// 延迟10s后进入清理程序
	SDL_Delay(15000);

	// 销毁创建的纹理，表层，渲染器，窗体，注意销毁顺序与创建顺序相反。
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
