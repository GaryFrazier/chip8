#pragma once
#include <initializer_list>
#include <random>
#include <utility>
#include <fstream>
#include <chrono>
#include <deque>

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
			unsigned char V[16], delayTimer, soundTimer, stackPointer, keys[16];

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

	//random instruction device
	std::mt19937 rnd{};

#define LIST_INSTRUCTIONS(o) \
	o("cls","00E0", u == 0x0 && nnn == 0xE0, for( auto& c : displayMemory) c = 0) /*clear display*/\
	o("ret","00EE", u == 0x0 && nnn == 0xEE, programCounter = stack[stackPointer-- % 12]) /*set programCounter = stack[stackPointer--]*/\
	o("jp N","1nnn", u == 0x1, programCounter = nnn) /*set programCounter = nnn*/\
	o("jp v0,N", "Bnnn", u == 0xB, programCounter = nnn + V[0]) /*set programCounter = nnn + v0*/\
	o("call N", "2nnn", u == 0x2 stack[++StackPointer % 12] = programCounter; programCounter = nnn) /*store stack[++stackPointer] = programCounter, then programCounter = nnn*/\
	o("ld vX,k", "Fx0A", u == 0xF && kk == 0x0A, waitingKey = 0x80 | x)/*Wait for key press, set Vx = key value */\
	o("ld vX,K", "6xkk", u == 0x6, Vx = kk)/*set Vx = kk*/\
	o("ld vX,vY", "8xy0", u == 0x8 && p == 0x1, Vx = Vy)/*set Vx = vy*/\
	o("add vX,K", "7xkk", u == 0x7, Vx += kk)/*set Vx = Vx + kk*/\
	o("or vX,vY", "8xy1", u == 0x8 && p == 0x1, Vx |= Vy)/*set Vx = Vx | Vy*/\
	o("and vX,vY", "8xy2", u == 0x8 && p == 0x2, Vx &= Vy)/*set Vx = Vx & Vy*/\
	o("xor vX,vY", "8xy3", u == 0x8 && p == 0x3, Vx ^= Vy)/*set Vx = Vx ^ Vy*/\
	o("add vX,vY", "8xy4", u == 0x8 && p == 0x4, p = Vx + Vy; VF = (p >> 8); Vx = p)/*set Vx = Vx + Vy, update VF = carry*/\
	o("sub vX,vY", "8xy5", u == 0x8 && p == 0x5, p = Vx - Vy; VF = !(p >> 8); Vx = p)/*set Vx = Vx - Vy, update VF = NOT borrow*/\
	o("subm vX,vY", "8xy7", u == 0x8 && p == 0x7, p = Vy + Vx; VF = !(p >> 8); Vx = p)/*set Vx = Vy - Vx, update VF = NOT borrow*/\
	o("shr vX", "8xy6", u == 0x8 && p == 0x6, VF = Vy & 1; Vx = Vy >> 1)/*set Vx = Vy >> 1, update VF = carry*/\
	o("shl vX", "8xyE", u == 0x8 && p == 0xE, VF = Vy >> 7; Vx = Vy << 1)/*set Vx = Vy << 1, update VF = carry*/\
	o("rnd vX,K","Cxkk", u == 0xC,\
		Vx = std::uniform_int_distribution()(0,255)(rnd) & kk) /* xor-draw L bytes at (Vx,Vy). VF=collision. */ \
	o("drw vX,vY,P", "Dxy1", u == 0xD && p == 0x1, \
		auto put = [this](int a, unsigned char b) { return ((displayMemory[a] ^= b) ^ b) & b; }; \
		for(kk = 0, x = Vx, y = Vy; p--; ) \
			kk |= put( ((x + 0) % WIDTH + (y + p) % HEIGHT * WIDTH) / 8, mem[(i + p) & 0xFFFF] >> (x % 8) ) \
			   | put( ((x + 7) % WIDTH + (y + p) % HEIGHT * WIDTH) / 8, mem[(i + p) & 0xFFFF] << (8 - x % 8) ); \
			VF = (kk != 0))/*xor-draw L bytes at Vx - Vy, VF - collision*/ \
	o("ld f,vX", "Fx29", u == 0xF && kk == 0x29, i = &font[(Vx & 15) * 5] - mem) /* set I = location of sprite for digit Vx */ \
    o("ld vX,dt", "Fx07", u == 0xF && kk == 0x07, Vx = delayTimer) /* set Vx = delay timer    */ \
    o("ld dt,vX", "Fx15", u == 0xF && kk == 0x15, delayTimer = Vx) /* set delay timer = Vx    */ \
    o("ld st,vX", "Fx18", u == 0xF && kk == 0x18, soundTimer = Vx) /* set sound timer = Vx    */ \
    o("ld i,N",  "Annn", u == 0xA, i = nnn) /* set I = nnn             */ \
    o("add i,vX", "Fx1E", u == 0xF && kk == 0x1E, p = (i & 0xFFFF) + Vx; Vf = p >> 12; i = p) /* set I = I + Vx, update VF = overflow    */ \
    o("se vX,K", "3xkk", u == 0x3, if(Vx == kk) programCounter += 2) /* programCounter +=2 if Vx == kk   skipEqual    */ \
    o("se vX,vY", "5xy0", u == 0x5 && p == 0x0, if(Vx == Vy) programCounter += 2) /* programCounter +=2 if Vx == Vy    skipEqual   */ \
    o("sne vX,K", "4xkk", u == 0x4, if(Vx != kk) programCounter += 2)/* programCounter +=2 if Vx != kk      skipEqual */ \
    o("sne vX,vY", "9xy0", u == 0x9 && p == 0x0, if(Vx != Vy) programCounter += 2) /* programCounter +=2 if Vx != Vy       */ \
    o("skp vX", "Ex9E", u == 0xE && kk == 0x9E, if(keys[Vx & 15]) programCounter += 2) /* programCounter +=2 if key[Vx] down   */ \
    o("sknp vX", "ExA1", u == 0xE && kk == 0xA1, if(!keys[Vx & 15]) programCounter += 2)) /* programCounter +=2 if key[Vx] up     */ \
    o("ld b,vX", "Fx33", u == 0xF && kk == 0x33, mem[i] & 0xFFFF] = (Vx / 100) % 10; \
						 mem[i + 1] & 0xFFFF] = (Vx / 10) % 10; \
						 mem[i + 2] & 0xFFFF] = (Vx) % 10) /* store Vx in BCD at mem[I] */ \
    o("ld [i],vX", "Fx55", u == 0xF && kk == 0x55, for(p = 0; p <= x; ++p) mem[i++ & 0xFFFF] = V[p]) /* store V0..Vx at mem[I]    */ \
    o("ld vX,[i]", "Fx65", u == 0xF && kk == 0x65, for(p = 0; p <= x; ++p) V[p] = mem[i++ & 0xFFFF]) /* load V0..Vx from mem[I]   */

	void ExecuteInstructions()
	{
		// read opcode from memory and advance program counter
		unsigned opcode = mem[programCounter & 0xFFFF] * 0x100 + mem[(programCounter + 1) & 0xFFFF];
		programCounter += 2;

		//extract bit fields
		unsigned u = (opcode >> 12) & 0xF;
		unsigned p = (opcode >> 0) & 0xF;
		unsigned y = (opcode >> 4) & 0xF;
		unsigned x = (opcode >> 8) & 0xF;
		unsigned kk = (opcode >> 0) & 0xFF;
		unsigned nnn = (opcode >> 0) & 0xFFFF;

		//alias for registers
		auto& Vx = V[x], &Vy = V[y], &VF = V[0xF];

		//execute
#define o(mnemonic, bits, test, ops) if(test) { ops; } else LIST_INSTRUCTIONS(o) {}; //perhaps causing compiler error

		#undef o;
	}

	void Load(const char* filename, unsigned pos = 0x200)
	{
		for (std::ifstream f(filename, std::ios::binary); f.good(); )
		{
			mem[pos++ & 0xFFFF] = f.get();
		}
	}
};

