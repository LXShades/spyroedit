#include <windows.h> // CALLBACK =/

#include "SpyroData.h"
#include "Main.h"
#include "ObjectEdit.h"

int game = UNKNOWN_GAME;
int gameVersion = UNKNOWN_VERSION, gameRegion = UNKNOWN_REGION;

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
	PowersLoop();
	SpyroLoop();
}

void UpdateKeys()
{
	for (int i = 0; i < 256; i ++)
	{
		bool keyIsDown = (bool) (GetKeyState(i) >> 4);

		if (! keyDown[i] && keyIsDown)
			keyPressed[i] = true;
		else
			keyPressed[i] = false;

		keyDown[i] = keyIsDown;
	}
}

#include <cmath>
int Distance(int x1, int y1, int z1, int x2, int y2, int z2)
{
	return (int) (sqrt(((float) (x2 - x1) / 10000.0f * (x2 - x1)) + ((float) (y2 - y1) / 10000.0f * (y2 - y1)) + ((float) (z2 - z1) / 10000.0f * (z2 - z1))) * 100.0f);
}