#pragma once

#include <stdint.h>
#include <string>
#include <fstream>
#include <vector>
#include "util.h"

#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32

class Chip8
{
private:
	uint8_t chip8_fontset[80] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, //0
		0x20, 0x60, 0x20, 0x20, 0x70, //1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
		0x90, 0x90, 0xF0, 0x10, 0x10, //4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
		0xF0, 0x10, 0x20, 0x40, 0x40, //7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
		0xF0, 0x90, 0xF0, 0x90, 0x90, //A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
		0xF0, 0x80, 0x80, 0x80, 0xF0, //C
		0xE0, 0x90, 0x90, 0x90, 0xE0, //D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
		0xF0, 0x80, 0xF0, 0x80, 0x80  //F
	};

	void init();
	void clear_display();
	void set_pixel(int x, int y, uint8_t value);
	void fetch_opcode();
	bool loadFile(std::string path);
	
	void draw_sprite();

	/* stores the current opcode */
	uint16_t opcode;
	
	/* the chip 8's memory */
	uint8_t memory[4096];

	/* the chip8's registers */
	uint8_t V[16];

	/* sound and delay timers */
	uint8_t delay_timer;
	uint8_t sound_timer;

	/* the index register */
	uint16_t index_register;
	uint16_t program_counter;

	uint16_t stack[16];
	uint16_t stack_pointer;
public:
	Chip8(std::string filepath);
	~Chip8();

	
	void emulate_cycle();

	uint8_t graphics_buffer[2048];
	uint8_t key[16];
	bool draw_flag;
};

