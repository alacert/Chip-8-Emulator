#include "Chip8.h"

#include <bitset>
#include <sstream>
#include <iomanip>
#include <random>

void Chip8::init()
{
	/* Sets program program counter to 0x200 and resets the opcode, index register and stack pointer */
	this->program_counter = 0x200;
	this->opcode = 0;
	this->index_register = 0;
	this->stack_pointer = 0;

	this->clear_display();

	/* clear stack, registers and keypad */
	for (int i = 0; i < 16; i++) {
		this->stack[i] = 0;
		this->V[i] = 0;
		this->key[i] = 0;
	}

	/* clear memory */
	for (int i = 0; i < 4096; i++) {
		this->memory[i] = 0;
	}

	/* load font set into memory */
	for (int i = 0; i < 80; i++) {
		this->memory[i] = this->chip8_fontset[i];
	}

	/* reset timers */
	this->delay_timer = 0;
	this->sound_timer = 0;

	/* seed random number generation */
	srand(time(0));
}

void Chip8::clear_display()
{
	for (int i = 0; i < 2048; i++) {
		this->graphics_buffer[i] = 0;
	}
}

void Chip8::set_pixel(int x, int y, uint8_t value)
{
	this->graphics_buffer[x + CHIP8_WIDTH * y] = value;
}

void Chip8::fetch_opcode()
{
	this->opcode = this->memory[this->program_counter] << 8 | this->memory[this->program_counter + 1];
	//std::cout << std::bitset<16>(this->memory[this->program_counter] << 8 | this->memory[this->program_counter + 1]) << std::endl;
	//std::cout << std::hex << this->opcode << std::endl;
}

bool Chip8::loadFile(std::string path)
{
	util::log("Loading ROM: " + path);

	/* Load the ROM file in binary mode */
	std::ifstream rom(path, std::ios::binary);
	if (rom.fail()) {
		util::log("Failed to load ROM");
		return false;
	}

	/* Copy ROM to buffer */
	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(rom), {});
	
	if ((4096 - 512) < buffer.size()) {
		util::log("ROM too large to fit in memory");
		return false;
	}

	for (int i = 0; i < buffer.size(); i++) {
		memory[i + 512] = buffer.at(i);
	}

	return true;
}

void Chip8::draw_sprite()
{
	/*
		Draws a sprite at coordinate (X, Y) that has a width of 8 pixels and a height of N+1 pixels. 
		Each row of 8 pixels is read as bit-coded starting from memory location I; 
		I value doesn’t change after the execution of this instruction. 
		As described above, V[F] is set to 1 if any screen pixels are flipped from set to unset when 
		the sprite is drawn, and to 0 if that doesn’t happen
	*/

	unsigned short x = V[(opcode & 0x0F00) >> 8];
	unsigned short y = V[(opcode & 0x00F0) >> 4];
	unsigned short height = opcode & 0x000F;
	unsigned short pixel;

	V[0xF] = 0;
	for (int yline = 0; yline < height; yline++)
	{
		pixel = memory[index_register + yline];
		for (int xline = 0; xline < 8; xline++) {
			if ((pixel & (0x80 >> xline)) != 0) {
				if (this->graphics_buffer[(x + xline + ((y + yline) * 64))] == 1) {
					V[0xF] = 1;
				}
				this->graphics_buffer[x + xline + ((y + yline) * 64)] ^= 1;
			}
		}
	}
}

Chip8::Chip8(std::string filepath)
{
	this->init();
	this->loadFile(filepath);
}

Chip8::~Chip8()
{
}

void Chip8::emulate_cycle()
{
	this->fetch_opcode();

	//system("pause");

	/* decode opcode */
	switch (opcode & 0xF000) {
	case 0x0000:
		switch (opcode & 0x000F) {
		case 0x0000: // 0x00E0: Clears the screen
			this->clear_display();
			this->draw_flag = true;
			this->program_counter += 2;
			break;

		case 0x000E: // 0x00EE: Returns from Subroutine
			this->stack_pointer--;
			this->program_counter = stack[this->stack_pointer];
			this->program_counter += 2;
			break;

		default:
			util::log("unknown opcode! [0x0000]");
			system("pause");
			break;
		}
		break;
	case 0x1000:  // 0x1NNN: Jumps to address NNN
		this->program_counter = opcode & 0x0FFF;
		break;

	case 0x2000: // 0x2NNN: Calls subroutine at 2NNN
		this->stack[this->stack_pointer] = this->program_counter;
		this->stack_pointer++;
		this->program_counter = opcode & 0x0FFF;
		break;

	case 0x3000: // 0x3XNN: Skips the next instruction if V[X] equals NN
		if (this->V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
			this->program_counter += 4;
		}
		else {
			this->program_counter += 2;
		}
		break;
	
	case 0x4000: // 0x4XNN: Skips the next instruction if V[x] doesn't equal NN
		if(this->V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
			this->program_counter += 4;
		}
		else {
		this->program_counter += 2;
		}
		break;

	case 0x5000: // 0x4XY0: Skips the next instruction if V[x] equals V[y]
		if (this->V[(opcode & 0x0F00) >> 8] == this->V[(opcode & 0x00F0) >> 4]) {
			this->program_counter += 4;
		}
		else {
			this->program_counter += 2;
		}
		break;

	case 0x6000: // 0x6XNN: Sets V[X] to NN
		this->V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		this->program_counter += 2;
		break;

	case 0x7000: // 0x7XNN: Adds NN to V[x]
		this->V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
		this->program_counter += 2;
		break;

	case 0x8000:
		switch (opcode & 0x000F) {
		case 0x0000: // 0x8XY0: Sets V[x] to V[y]
			this->V[(opcode & 0x0F00) >> 8] = this->V[(opcode & 0x00F0) >> 4];
			program_counter += 2;
			break;

		case 0x0001: // 0x8XY1: Sets V[x] to V[x] | V[y]
			this->V[(opcode & 0x0F00) >> 8] = this->V[(opcode & 0x0F00) >> 8] | this->V[(opcode & 0x00F0) >> 4];
			program_counter += 2;
			break;

		case 0x0002: // 0x8XY2: Sets V[x] to V[x] & V[y]
			this->V[(opcode & 0x0F00) >> 8] = this->V[(opcode & 0x0F00) >> 8] & this->V[(opcode & 0x00F0) >> 4];
			program_counter += 2;
			break;

		case 0x0003: // 0x8XY2: Sets V[x] to V[x] ^ V[y]
			this->V[(opcode & 0x0F00) >> 8] = this->V[(opcode & 0x0F00) >> 8] ^ this->V[(opcode & 0x00F0) >> 4];
			program_counter += 2;
			break;

		case 0x0004: // 0x8XY4: Sets V[x] to V[x] + V[y]. VF is set to 1 when there's a carry, and 0 when there isn't
			this->V[(opcode & 0x0F00) >> 8] += this->V[(opcode & 0x00F0) >> 4];
			if (this->V[(opcode & 0x00F0) >> 4] > (0xFF - this->V[(opcode & 0x0F00) >> 8])) {
				this->V[0xF] = 1; //carry
			}
			else {
				this->V[0xF] = 0;
			}
			this->program_counter += 2;
			break;

		case 0x0005: // 8XY5 - VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
			if (this->V[(opcode & 0x00F0) >> 4] > this->V[(opcode & 0x0F00) >> 8]) {
				this->V[0xF] = 0; // there is a borrow
			}
			else {
				this->V[0xF] = 1;
			}
			this->V[(opcode & 0x0F00) >> 8] -= this->V[(opcode & 0x00F0) >> 4];
			this->program_counter += 2;
			break;

		case 0x0006: // 0x8XY6 - Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.
			this->V[0xF] = this->V[(opcode & 0x0F00) >> 8] & 0x1;
			this->V[(opcode & 0x0F00) >> 8] >>= 1;

			this->program_counter += 2;
			break;

		case 0x0007: // 0x8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
			if (this->V[(opcode & 0x0F00) >> 8] > this->V[(opcode & 0x00F0) >> 4]) {	// VY-VX
				this->V[0xF] = 0; // there is a borrow
			}
			else {
				V[0xF] = 1;
			}
			this->V[(opcode & 0x0F00) >> 8] = this->V[(opcode & 0x00F0) >> 4] - this->V[(opcode & 0x0F00) >> 8];
			this->program_counter += 2;
			break;

		case 0x000E: // 0x8XYE: Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.
			this->V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
			this->V[(opcode & 0x0F00) >> 8] <<= 1;
			this->program_counter += 2;
			break;

		default:
			util::log("unknown opcode [0x8000]");
			system("pause");
			break;
		}
		break;

	case 0x9000: // 0x9XY0: Skips the next instruction if V[x] doesn't equal V[y]
		if (this->V[(opcode & 0x0F00) >> 8] != this->V[(opcode & 0x00F0) >> 4]) {
			this->program_counter += 4;
		}
		else {
			this->program_counter += 2;
		}
		
		break;

	case 0xA000: // 0xANNN: Sets the index register to NNN
		this->index_register = opcode & 0x0FFF;
		this->program_counter += 2;
		break;

	case 0xB000: // 0xBNNN: Jumps to the address NNN + V[0]
		this->program_counter = (opcode & 0x0FFF) + this->V[0];
		break;

	case 0xC000: // 0xCXNN: Sets VX to the result of a & operation on a random number (between 0-255) and NN
		this->V[(opcode & 0x0F00) >> 8] = (rand() % (0xFF + 1)) & (opcode & 0x00FF);
		program_counter += 2;
		break;

	case 0xD000: // 0xDXYN: Draws a sprite at the coordinate (V[X], V[Y]), see function definition for more information
		this->draw_sprite();

		this->draw_flag = true;
		this->program_counter += 2;
		break;

	case 0xE000:
		switch (opcode & 0x00FF) {
		case 0x009E: // 0xEX9E: Skips the next instruction if the key stored in VX is pressed
			if (key[V[(opcode & 0x0F00) >> 8]] != 0) {
				this->program_counter += 4;
			}
			else {
				this->program_counter += 2;
			}
			break;

		case 0x00A1: // 0xEXA1: Skips the next instruction if the key stored in VX isn't pressed
			if (key[V[(opcode & 0x0F00) >> 8]] == 0) {
				this->program_counter += 4;
			}
			else {
				this->program_counter += 2;
			}
			break;

		default:
			util::log("unknown opcode!");
			system("pause");
			break;
		}
		break;

	case 0xF000: // starting 0xF_
		switch (opcode & 0x00FF) {
		case 0x0007: // 0xFX07: Sets VX to the value of the delay timer
			this->V[(opcode & 0x0F00) >> 8] = this->delay_timer;
			program_counter += 2;
			break;

		case 0x000A:  // 0xFX0A: A key press is awaited then is stored in VX. (Blocking Operation. All instruction halted until next key event)
		{
			bool key_pressed = false;

			for (int i = 0; i < 16; i++) {
				if (this->key[i] != 0) {
					this->V[(opcode & 0x0F00) >> 8] = i;
					key_pressed = true;
				}
			}

			if (!key_pressed) return;

			this->program_counter += 2;
			break;
		}

		case 0x0015: // 0xFX15: Sets the delay timer to V[X]
			this->delay_timer = this->V[(opcode & 0x0F00) >> 8];
			program_counter += 2;
			break;

		case 0x0018: // 0xFX18: Sets the sound timer to V[X]
			this->sound_timer = this->V[(opcode & 0x0F00) >> 8];
			program_counter += 2;
			break;

		case 0x001E: // 0xFX1E: Adds V[X] to index_register
			this->index_register += this->V[(opcode & 0x0F) >> 8];
			program_counter += 2;
			break;

		case 0x0029: // 0xFX29: Sets I to the location of the sprite for the character in VX. Characters 0-F (in hex) are represented by a 4x5 font
			this->index_register = this->V[(opcode & 0x0F00) >> 8] * 0x5; /* 0x5 because characters space in memory is 5 in dimension*/
			this->program_counter += 2;
			break;

		case 0x0033: // 0xFX33: stores the binary coded decimal representation of VX in the memory at index_register
			this->memory[this->index_register] = this->V[(opcode & 0x0F00) >> 8] / 100;
			this->memory[this->index_register + 1] = (this->V[(opcode & 0x0F00) >> 8] / 10) % 10;
			this->memory[this->index_register + 2] = V[(opcode & 0x0F00) >> 8] % 10;

			this->program_counter += 2;
			break;

		case 0x0055: // 0xF55: Stores V[0] to V[X] in memory starting at index_register address. The offset from the index_register is increased by 1 for each value written
			for (int i = 0; i < ((opcode & 0x0F00) >> 8); i++) {
				memory[this->index_register + i] = this->V[i];
			}

			this->index_register += ((opcode & 0x0F00) >> 8) + 1;

			this->program_counter += 2;
			break;

		case 0x0065: // 0xFX65 Fills V[0] to V[X] including V[x] in memory starting at index_register. 
					 //The offset from index_register is incrased by 1 for each value, index register stays the same
			for (int i = 0; i < ((opcode & 0x0F00) >> 8); i++) {
				this->V[i] = memory[this->index_register + i];
			}

			/* In the original CHIP-8 implementation, and also in CHIP-48, I is left incremented after this instruction had been executed. */
			this->index_register += ((opcode & 0x0F00) >> 8) + 1;

			this->program_counter += 2;
			break;

		default:
			util::log("unkown opcode! [0xF000]");
			system("pause");
			break;
		}
		break;

	default:
		util::log("unkown opcode! ");
		system("pause");
		break;
	}

	if (delay_timer > 0) {
		delay_timer--;
	}

	if (sound_timer > 0) {
		if (sound_timer == 1) {
			std::cout << '\a' << std::flush;
			util::log("beep");
			sound_timer--;
		}
	}
}
