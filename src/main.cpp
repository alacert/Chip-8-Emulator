#include <iostream>
#include <string>
#include <thread>
#include <conio.h>
#include <SDL.h>

#include "util.h"
#include "Chip8.h"

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 512;
const char* WINDOW_TITLE = "Chip-8 Emulator";

uint32_t pixels[2048];

uint8_t keymap[16] = {
	SDLK_x,
	SDLK_1,
	SDLK_2,
	SDLK_3,
	SDLK_q,
	SDLK_w,
	SDLK_e,
	SDLK_a,
	SDLK_s,
	SDLK_d,
	SDLK_z,
	SDLK_c,
	SDLK_4,
	SDLK_r,
	SDLK_f,
	SDLK_v,
};


int main(int argc, char** args)
{
	Chip8 chip8 = Chip8("C:\\Programming\\C++\\Chip8-Emulator\\Debug\\roms\\WIPEOFF");

	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		util::log("error initializing SDL");
		return -1;
	}


	SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

	if (window == NULL) {
		util::log("error creating SDL Window");
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

	/* Create a texture to store the frame buffer */
	SDL_Texture* sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

	bool running = true;
	SDL_Event e;
	while (running) {
		chip8.emulate_cycle();

		while (SDL_PollEvent(&e) > 0)
		{
			if (e.type == SDL_QUIT) {
				running = false;
			}

			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					running = false;
				}

				for (int i = 0; i < 16; i++) {
					if (e.key.keysym.sym == keymap[i]) {
						chip8.key[i] = 1;
					}
				}
			}

			if (e.type == SDL_KEYUP) {
				for (int i = 0; i < 16; i++) {
					if (e.key.keysym.sym == keymap[i]) {
						chip8.key[i] = 0;
					}
				}
			}
		}

		if (chip8.draw_flag) {
			chip8.draw_flag = false;

			for (int i = 0; i < 2048; i++) {
				uint8_t pixel = chip8.graphics_buffer[i];
				pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
			}

			SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(uint32_t));

			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
			SDL_RenderPresent(renderer);
		}

		/* sleep to slow down for emulation speed */
		std::this_thread::sleep_for(std::chrono::microseconds(1200));
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}