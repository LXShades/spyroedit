#include "Vram.h"

Vram vram;

// Accessor functions for the PlayStation VRAM
extern "C" void GetSnapshot(GPUSnapshot* dataOut);
extern "C" void SetSnapshot(const GPUSnapshot* in);

// Gets a VRAM snapshot and returns a pointer to it
uint8* Vram::GetVram8() {
	ValidateSnapshot();

	isValidOut = false; // could we add a const version of this function that guarantees no revalidation?

	return (uint8*)snapshot.vram;
}

uint16* Vram::GetVram16() {
	ValidateSnapshot();

	isValidOut = false;

	return (uint16*)snapshot.vram;
}

// Gets a VRAM line at a position
uint8* Vram::GetVram8(int x, int y) {
	ValidateSnapshot();

	isValidOut = false;

	return (uint8*)snapshot.vram;
}

uint16* Vram::GetVram16(int x, int y) {	
	ValidateSnapshot();

	isValidOut = false;

	return &((uint16*)snapshot.vram)[y * 1024 + x];
}
	
uint16* Vram::GetHqPalette(uint16 palette) {
	ValidateSnapshot();

	isValidOut = false;

	return &((uint16*)snapshot.vram)[palette * 16];
}

uint16* Vram::GetLqPalette(uint16 palette) {
	uint8 x = palette >> 8;
	uint8 y = palette & 0xFF;

	return &((uint16*)snapshot.vram)[(x * 16 * 2 + y * 4 * 2048) / 2];
}

// Called after each frame to flush the cache
void Vram::PostFrameUpdate() {
	if (!isValidOut) {
		// Update the in-game memory
		SetSnapshot(&snapshot);

		isValidOut = true;
	}

	isValidIn = false;
}

void Vram::GetSnapshot(GPUSnapshot* snapshotOut) {
	::GetSnapshot(snapshotOut);
}

void Vram::ValidateSnapshot() {
	if (!isValidIn) {
		GetSnapshot(&snapshot);
		isValidIn = true;
	}
}