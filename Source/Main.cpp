#include <cmath>

#include "SpyroData.h"
#include "Main.h"
#include "ObjectEdit.h"

GameName game = UNKNOWN_GAME;
GameVersion gameVersion = UNKNOWN_VERSION;
GameRegion gameRegion = UNKNOWN_REGION;

void* memory;
uint8* umem8;
uint16* umem16;
uint32* umem32;

bool keyDown[256];
bool keyPressed[256];

void MainLoop()
{
	if (!memory)
		return;

	UpdateKeys();
	
	ObjectEditorLoop();
	SpyroLoop();
}

void UpdateKeys()
{
	for (int i = 0; i < 256; i ++)
	{
		bool keyIsDown = (bool) (GetKeyState(i) >> 4);

		if (!keyDown[i] && keyIsDown)
			keyPressed[i] = true;
		else
			keyPressed[i] = false;

		keyDown[i] = keyIsDown;
	}
}

int Distance(int x1, int y1, int z1, int x2, int y2, int z2)
{
	return (int) (sqrt(((float) (x2 - x1) / 10000.0f * (x2 - x1)) + ((float) (y2 - y1) / 10000.0f * (y2 - y1)) + ((float) (z2 - z1) / 10000.0f * (z2 - z1))) * 100.0f);
}