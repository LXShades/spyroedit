#pragma once
#include "Main.h" // GPUSnapshot
#include "Types.h"

Vram vram;

class Vram {
public:
	// Gets a VRAM snapshot and returns a pointer to it
	uint8* GetVram8();
	uint16* GetVram16();

	// Gets a VRAM line at a position
	uint8* GetVram8(int x, int y);
	uint16* GetVram16(int x, int y);

	// Gets a VRAM line at given texture palette positions
	uint16* GetHqPalette(uint16 hqPalette);
	uint16* GetLqPalette(uint16 lqPalette);

	// Called before each frame to update the VRAM cache
	void PreFrameUpdate();
	
	// Called after each frame to flush the cache
	void PostFrameUpdate();

private:
	void ValidateSnapshot();

private:
	GPUSnapshot snapshot;

	bool8 isValidIn;
	bool8 isValidOut;
};