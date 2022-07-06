#pragma once
#include "Types.h"

#define NAME "SpyroEdit 2.3"
#define RELEASE 2
#define VERSION 2
#define SUBVERSION 3
#define INFORMATION "uwu?\n"
#define MEMINT(addr) *(int*) ((unsigned int) memory + (addr))
#define MEMUINT(addr) *(unsigned int*) ((unsigned int) memory + (addr))
#define MEMCHAR(addr) *(char*) ((unsigned int) memory + (addr))

#define MEMPTR(addr) (void*) (((char*) memory) + addr)

enum GameName {
	UNKNOWN_GAME = 0, 
	SPYRO1 = 1, 
	SPYRO2 = 2, 
	SPYRO3 = 3
};

enum GameVersion {
	UNKNOWN_VERSION, 
	ORIGINAL = 1, 
	GREATESTHITS = 2, 
	BETA = 3, 
	DEMO = 4
};

enum GameRegion {
	UNKNOWN_REGION, 
	NTSC = 1, 
	PAL = 2, 
	NTSCJP = 3
};

extern GameName game;
extern GameVersion gameVersion;
extern GameRegion gameRegion;

extern void* memory;
extern uint8* umem8;
extern uint16* umem16;
extern uint32* umem32;
const uint32 memory_size = 0x00200000;

extern bool keyDown[256];
extern bool keyPressed[256];

void MainLoop();
void UpdateKeys();
int Distance(int srcX, int srcY, int srcZ, int destX, int destY, int destZ);
