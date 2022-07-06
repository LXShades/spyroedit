#pragma once
#include "Main.h" // GPUSnapshot
#include "Types.h"

// GPU-plugin-compatible VRAM snapshot
struct GPUSnapshot {
	unsigned char vram[1024*1024];
	unsigned long ulControl[256];
	unsigned long ulStatus;
};

// VRAM manage class to make everything supa-EZ
class Vram {
public:
	Vram() : isValidIn(false), isValidOut(true) {}

	// Gets a VRAM snapshot and returns a pointer to it
	uint8* GetVram8();
	uint16* GetVram16();

	// Gets a VRAM line at a position
	uint8* GetVram8(int x, int y);
	uint16* GetVram16(int x, int y);

	// Gets a VRAM line at given texture palette positions
	uint16* GetHqPalette(uint16 hqPalette);
	uint16* GetLqPalette(uint16 lqPalette);

	// Called after each frame to flush pending caches
	void PostFrameUpdate();

	// Gets a full snapshot of the VRAM
	void GetSnapshot(GPUSnapshot* snapshotOut);

private:
	void ValidateSnapshot();

private:
	GPUSnapshot snapshot;

	bool8 isValidIn;
	bool8 isValidOut;
};

extern Vram vram;