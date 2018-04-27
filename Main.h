#pragma once
#include "Types.h"

#define NAME "SpyroEdit 2.0"
#define RELEASE 2
#define VERSION 2
#define SUBVERSION 0
#define INFORMATION "What is there to say about me?\n" "Well, I'm made of lots of little ones and zeroes called 'bits'...\n" \
                    "They uh... they make me do stuff.\n" "LXShadow made the stuff, so don't ask me about it...\n" \
                    "It's his job to do all the weird mathemalogics. I'm just here to execute it.\n" \
                    "Stop asking these questions! It's driving me insane!\n" "I'M JUST A PLUGIN. USE ME, AND ASK NO MORE QUESTIONS!\n" \
                    "Now just click OK and leave me alone!"

#define MEMINT(addr) *(int*) ((unsigned int) memory + (addr))
#define MEMUINT(addr) *(unsigned int*) ((unsigned int) memory + (addr))
#define MEMCHAR(addr) *(char*) ((unsigned int) memory + (addr))

#define MEMPTR(addr) (void*) (((char*) memory) + addr)

// GPU-plugin-compatible VRAM snapshot
struct GPUSnapshot {
	unsigned char vram[1024*1024];
	unsigned long ulControl[256];
	unsigned long ulStatus;
};

enum gname {UNKNOWN_GAME = 0, SPYRO1 = 1, SPYRO2 = 2, SPYRO3 = 3};
enum gversion {UNKNOWN_VERSION, ORIGINAL = 1, GREATESTHITS = 2, BETA = 3, DEMO = 4};
enum gregion {UNKNOWN_REGION, NTSC = 1, PAL = 2, NTSCJP = 3};

extern int game;
extern int gameVersion, gameRegion;

extern void* memory;
extern uint8* umem8;
extern uint16* umem16;
extern uint32* umem32;
const uint32 memory_size = 0x00200000;

extern bool keyDown[256];
extern bool keyPressed[256];

extern GPUSnapshot vramSs;

void MainLoop();

void UpdateKeys();

int Distance(int srcX, int srcY, int srcZ, int destX, int destY, int destZ);
