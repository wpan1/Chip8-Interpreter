#include <stdio.h>
#include <stdlib.h>
#include "chip8-CPU.h"

#define FONTSET_SIZE 80
#define MEMORY_SIZE 4096
#define REGISTER_SIZE 16
#define CHAR_SIZE 5
#define STACK_SIZE 16
#define KEYPAD_SIZE 16

#define PIXEL_WIDTH 8

#define MAX_FILE_SIZE 3584

// Current OPCODE
unsigned short curr_opcode;
// Memory storage 4K
unsigned char memory[MEMORY_SIZE];
// 16 V Memory Registers
unsigned char V[REGISTER_SIZE];
// Index register
unsigned short ir;
// Program counter
unsigned short pc;

/*
Chip-8 Memory map
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM
*/

const unsigned char fontset[FONTSET_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,		// 0
	0x20, 0x60, 0x20, 0x20, 0x70,		// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,		// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,		// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,		// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,		// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,		// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,		// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,		// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,		// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,		// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,		// B
	0xF0, 0x80, 0x80, 0x80, 0xF0,		// C
	0xE0, 0x90, 0x90, 0x90, 0xE0,		// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,		// E
	0xF0, 0x80, 0xF0, 0x80, 0x80		// F
};

// Chip-8 has two timer registers
unsigned char delay_timer;
unsigned char sound_timer;

// Use a stack to store program jumps, stack pointer
unsigned short stack[STACK_SIZE];
unsigned short sp;

// HEX based keypad, stores current state of key
unsigned char keypad[KEYPAD_SIZE];

/*
* Initialise the CPU registers and memory
*/
void cpuInit(){
	pc = 0x200;			// Program counter starts at 0x200 as defined
	curr_opcode = 0;	// Reset current operation code
	ir = 0;				// Reset index register
	sp = 0;				// Reset stack pointer

	// Clear display
	for (int i = 0; i < RESOLUTION_H * RESOLUTION_W; i++){
		gfx_out[i] = 0;
	}

	// Clear stack
	for (int i = 0; i < STACK_SIZE; i++){
		stack[i] = 0;
	}

	// Clear registers
	for (int i = 0; i < REGISTER_SIZE; i++){
		V[i] = 0;
	}

	// Clear memory
	for (int i = 0; i < MEMORY_SIZE; i++){
		memory[i] = 0;
	}

	// Clear keys
	for (int i = 0; i < KEYPAD_SIZE; i++){
		keypad[i] = 0;
	}

	// Load fontset
	for (int i = 0; i < FONTSET_SIZE; i++){
		memory[i] = fontset[i];
	}

	// Reset timers
	delay_timer = 0;
	sound_timer = 0;

	// Load in program from file
	printf("Please input filename of game\n");
	char filename[50];
	scanf("%s", filename);
	cpuLoadFile(filename, &memory[FONTSET_SIZE+1]);
}

/*
* Load program file into memory
*/
void cpuLoadFile(char *filename, unsigned char *memory_buffer){
	FILE *ptr_program;
	int file_size;

	// Open file
	ptr_program = fopen(filename, "rb");
	if (ptr_program == NULL){
		fprintf(stderr, "Could not find file %s", filename);
		exit(1);
	}
	// Read file into memory
	fseek(ptr_program, 0, SEEK_END);
	file_size = ftell(ptr_program);
	rewind(ptr_program);

	// Exit if file is too large
	if (file_size > MAX_FILE_SIZE) {
		fprintf(stderr, "File is too large to load.\n"); 
		exit(1);
	}

	// Read program into memory_buffer/memory
	fread(memory_buffer, 1, file_size, ptr_program);

	printf("Load Program %s Success\n", filename);
}

void cpuEmulateLoop(){
	/* 
	 * FETCH OPCODE (16 bits)
	 * Add the two 8 bit memory words together 
	 * This is done by first adding 8 zeros (<<)
	 * Then logical or the last 8 bits with zeros
	*/
	curr_opcode = memory[pc] << 8 | memory[pc+1];
	/*
	 * DECODE OPCODE & EXECUTE OPCODE(16 bits)
	 * Read first four bit nibble to find relating opcode
	 * Execute specific opcode
	*/
	switch (curr_opcode & 0xF000){
		// Opcode 00E0 and 00EE have same first 4 bits
		case 0x0000:
			// We need to look at the last 4 bits
			switch(curr_opcode & 0x000F){
				// Op code 00E0 - Clear screen
				case 0x0000:
					for (int i = 0; i<RESOLUTION_W*RESOLUTION_H; i++){
						gfx_out[i] = 0;
					}
					draw_flag = 1;
					pc += 2;
				break;

				// Op code 00EE - Return from subroutine
				case 0x000E:
					// Decrement stack as pointer is at first free spot
					sp--;
					pc = stack[sp];
					pc += 2;
				break;
			}
		break;

		// Opcode 1NNN - Jump pc to address NNN
		case 0x1000:
			pc = curr_opcode & 0x0FFF;
		break;

		// Opcode 2NNN - Call subroutine at address NNN
		case 0x2000:
			// Store current state in stack
			stack[sp++] = pc;
			// Run address NNN
			pc = curr_opcode & 0x0FFF;
		break;

		// Opcode 3XNN - Skip if V[x] == NN
		case 0x3000:
			// Need to remove ending 8 zero bits
			if (V[(curr_opcode & 0x0F00) >> 8] == (curr_opcode & 0x00FF))
				pc += 4;
			pc += 2;
		break;

		// Opcode 4XNN - Skip if V[x] == NN
		case 0x4000:
			// Need to remove ending 8 zero bits
			if (V[(curr_opcode & 0x0F00) >> 8] != (curr_opcode & 0x00FF))
				pc += 4;
			pc += 2;
		break;

		// Opcode 5XY0 - Skip if V[x] == V[y]
		case 0x5000:
			if (V[(curr_opcode & 0x0F00 >> 8)] == V[(curr_opcode & 0x00F0 >> 4)])
				pc += 4;
			pc += 2;
		break;

		// Opcode 6XNN - Sets V[x] = NN
		case 0x6000:
			V[(curr_opcode & 0x0F00) >> 8] = curr_opcode & 0x00FF;
			pc += 2;
		break;

		// Opcode 7XNN - Adds NN to V[x]
		case 0x7000:
			V[(curr_opcode & 0x0F00) >> 8] += curr_opcode & 0x00FF;
			pc += 2;
		break;

		// Opcode 8XY0 to 8XYE have same first 4 bits
		case 0x8000:
			// we need to look at the last 4 bits
			switch (curr_opcode & 0x000F){
				// Opcode 8XY0 - Sets V[x] to the value of V[y]
				case 0x0000:
					V[(curr_opcode & 0x0F00 >> 8)] = V[(curr_opcode & 0x00F0 >> 4)];
					pc += 2;
				break;

				// Opcode 8XY1 - Sets V[x] to V[x] or V[y]
				case 0x0001:
					V[(curr_opcode & 0x0F00 >> 8)] |= V[(curr_opcode & 0x00F0 >> 4)];
					pc += 2;
				break;

				// Opcode 8XY2 - Sets V[x] to V[x] and V[y] 
				case 0x0002:
					V[(curr_opcode & 0x0F00 >> 8)] &= V[(curr_opcode & 0x00F0 >> 4)];
					pc += 2;
				break;

				// Opcode 8XY3 - Sets V[x] to V[x] xor V[y]
				case 0x0003:
					V[(curr_opcode & 0x0F00 >> 8)] ^= V[(curr_opcode & 0x00F0 >> 4)];
					pc += 2;
				break;

				// Opcode 8XY4 - Adds V[y] to V[x]
				case 0x0004:
					if (V[(curr_opcode & 0x00F0 >> 4)] > 0xFF - V[(curr_opcode & 0x0F00 >> 8)])
						V[0xF] = 1;
					else
						V[0xF] = 0;
					V[(curr_opcode & 0x0F00 >> 8)] += V[(curr_opcode & 0x00F0 >> 4)];
					pc += 2;
				break;

				// Opcode 8XY5 - Subtracts V[y] from V[x]
				case 0x0005:
					if (V[(curr_opcode & 0x00F0 >> 4)] > V[(curr_opcode & 0x0F00 >> 8)])
						V[0xF] = 0;
					else
						V[0xF] = 1;
					V[(curr_opcode & 0x0F00 >> 8)] -= V[(curr_opcode & 0x00F0 >> 4)];
					pc += 2;
				break;

				// Opcode 8XY6 - Sets V[x] to V[y] - V[x]
				case 0x0006:
					V[0xF] = V[(curr_opcode & 0x0F00 >> 8)] & 0x1;
					V[(curr_opcode & 0x0F00 >> 8)] >>= 1;
					pc += 2;
				break;

				// Opcode 8XY7:
				case 0x0007:
					if (V[(curr_opcode & 0x00F0 >> 4)] > V[(curr_opcode & 0x0F00 >> 8)])
						V[0xF] = 0;
					else
						V[0xF] = 1;
					V[(curr_opcode & 0x0F00 >> 8)] = V[(curr_opcode & 0x00F0 >> 4)] - V[(curr_opcode & 0x0F00 >> 8)];
					pc += 2;
				break;

				// Opcode 8XYE:
				case 0x000E:
					V[0xF] = V[(curr_opcode & 0x0F00 >> 8)] & 0x80;
					V[(curr_opcode & 0x0F00 >> 8)] <<= 1;
					pc += 2;
				break;
			}
		break;

		// Opcode 9XY0 - Skips the next instruction if VX doesn't equal VY
		case 0x9000:
			if (V[(curr_opcode & 0x0F00 >> 8)] != V[(curr_opcode & 0x00F0 >> 4)])
				pc += 4;
			pc += 2;
		break;

		// Opcode ANNN - Sets ir to the address NNN
		case 0xA000:
			ir = curr_opcode & 0x0FFF;
			pc += 2;
		break;

		// Opcode BNNN - Jumps to the address NNN plus V0
		case 0xB000:
			pc = V[0] + (curr_opcode & 0x0FFF);
		break;

		// Opcode CXNN - Sets VX to the result of a bitwise and operation on a random number 
		case 0xC000:
			V[(curr_opcode & 0x0F00 >> 8)] = ((rand()%255) & (curr_opcode & 0x00FF));
			pc += 2;
		break;

		// Opcode DXYN - Draws a sprite at coordinate (VX, VY)
		case 0xD000:{
			unsigned short x = (curr_opcode & 0x0F00 >> 8);
			unsigned short y = (curr_opcode & 0x00F0 >> 4);
			unsigned short height = (curr_opcode & 0x000F);
			unsigned short pixel_val;
			unsigned int draw_loc;

			V[0xF] = 0;
			for (int y_coord = 0; y_coord < height; y_coord++){
				pixel_val = memory[ir + y_coord];
				for (int x_coord = 0; x_coord < PIXEL_WIDTH; x_coord++){
					if((pixel_val & (0x80 >> x_coord)) != 0){
						draw_loc = x + x_coord + ((y + y_coord) * RESOLUTION_H);
						if (draw_loc == 1)
							V[0xF] = 1;
						draw_loc ^= 1;
					}
				}
			}
			draw_flag = 1;
			pc += 2;
		break;
		};

		// Opcodes EX9E and EXA1
		case 0xE000:
			// Opcode EX9E - Skips the next instruction if the key stored in VX is pressed
			switch (curr_opcode & 0x00F0){
				case 0x0090:
					if (keypad[V[(curr_opcode & 0x0F00 >> 8)]] == 1)
						pc += 4;
					else
						pc += 2;
			}
			break;

			// Opcode EXA1 - Skips the next instruction if the key stored in VX isn't pressed
			switch (curr_opcode & 0x00F0){
				case 0x0090:
					if (keypad[V[(curr_opcode & 0x0F00 >> 8)]] == 0)
						pc += 4;
					else
						pc += 2;
			}
			break;

		// Opcodes FX07 to FX65
		case 0xF000:
			switch (curr_opcode & 0x00FF){
				// Opcode FX07 - Sets VX to the value of the delay timer
				case 0x0007:
					V[(curr_opcode & 0x0F00 >> 8)] = delay_timer;
					pc += 2;
				break;

				// Opcode FX0A - A key press is awaited, and then stored in VX
				case 0x000A:
					// Keeps runnning until key is pressed
					while(1){
						// Find first key pressed
						for (int i = 0; i < KEYPAD_SIZE; i++){
							if (keypad[i] == 1){
								V[(curr_opcode & 0x0F00 >> 8)] = i;
								pc += 2;
								return;
							}
						}
					}
				break;

				// Opcode FX15 - Sets the delay timer to VX
				case 0x0015:
					delay_timer = V[(curr_opcode & 0x0F00 >> 8)];
					pc += 2;
				break;

				// Opcode FX18 - Sets the sound timer to VX
				case 0x0018:
					sound_timer = V[(curr_opcode & 0x0F00 >> 8)];
					pc += 2;
				break;

				// Opcode FX1E - Adds VX to IR
				case 0x001E:
					// V[F] set to 1 for overflow
					if(ir + V[(curr_opcode & 0x0F00) >> 8] > 0xFFF)
						V[0xF] = 1;
					else
						V[0xF] = 0;
					ir += V[(curr_opcode & 0x0F00 >> 8)];
					pc += 2;
				break;

				// Opcode FX29 - Sets I to the location of the sprite for the character in VX.
				case 0x0029:
					ir = V[(curr_opcode & 0x0F00) >> 8] * CHAR_SIZE;
					pc += 2;
				break;

				// Opcode FX33 - Stores the binary-coded decimal representation of VX
				case 0x0033:
					// Units
					memory[ir+2] = (V[(curr_opcode & 0x0F00) >> 8] % 100) % 10;		
					// Tens
					memory[ir+1] = (V[(curr_opcode & 0x0F00) >> 8] / 10) % 10;		
					// Hundreds	
					memory[ir] = V[(curr_opcode & 0x0F00) >> 8] / 100;
					pc += 2;
				break;

				// Opcode FX55 - Stores V0 to VX (including VX) in memory starting at address I
				case 0x0055:
					// V0 to VX
					for (int index = 0; index < (curr_opcode & 0x0F00) >> 8; index ++){
						memory[ir + index] = V[index];
					}
					pc += 2;
				break;

				// Opcode FX65 - Fills V0 to VX (including VX) with values from memory starting at address I.
				case 0x0065:
					// V0 to VX
					for (int index = 0; index < (curr_opcode & 0x0F00) >> 8; index ++){
						V[index] = memory[ir + index];
					}
					pc += 2;
				break;
			}
		// Update timers
		if(delay_timer > 0)
			delay_timer--;

		if(sound_timer > 0)
		{
			if(sound_timer == 1)
				printf("BEEP!\n");
			sound_timer--;
		}	
	}
}
