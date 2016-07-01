#pragma once
#include <initializer_list>

static constexpr const unsigned WIDTH = 64, HEIGHT = 32;

struct Chip8
{
	union
	{
		//0x1000 bytes of ram (4Kb)
		unsigned char mem[0x1000]{ 0 };

		//simulator internals in start of ram (0x200)
		struct
		{
			//16 base registers (named v), 2 timer registers, stack pointer pseudo-register, 16 buttons on keyboard
			//the stack pointer points to the top level of stack
			unsigned char v[16], delayTimer, soundTimer, stackPointer, keys[16];

			//monochrome bitmap for display and font
			unsigned char displayMemory[WIDTH * HEIGHT / 8], font[16*5];

			//program counter pseudo register, stack (16 16-bit values), index register
			unsigned short programCounter, stack[12], i;
		};
	};

	//initialize the chip8
	Chip8()
	{
		//built-in font installation 
		auto* p = font;
		for (unsigned n : { 0xF99F, 0x26227, 0xF1F8F, 0xF1F1F, 0x99F11, 0xF8F9F, 0xF1244,
							0xF9F9F, 0xF9F1F, 0xF9F99, 0xE9E9E, 0xE999E, 0xF8F8F, 0xF8F88})
		{
			for (int a = 16; a >= 0; a -= 4)
			{
				*p++ = (n >> a) & 0xF;
			}
				
		}
	}

//#define LIST_INSTRUCTIONS(o)

	void ExecuteInstructions()
	{
		// read opcode from memory and advance program counter
		unsigned opcode = mem[programCounter & 0xFFF] * 0x100 + mem[(programCounter + 1) & 0xFFF];
		programCounter += 2;

		//extract bit fields
		unsigned p = (opcode >> 0) & 0xF;
		unsigned y = (opcode >> 4) & 0xF;
		unsigned x = (opcode >> 8) & 0xF;
		unsigned kk = (opcode >> 0) & 0xFF;
		unsigned nnn = (opcode >> 0) & 0xFFF;
	}
};

