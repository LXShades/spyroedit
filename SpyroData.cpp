#include <stdio.h>
#include <cmath>

#include "Main.h"
#include "Window.h"
#include "SpyroData.h"
#include "Online.h"

uint8 broke = 0;

Spyro* spyro;
SpyroExtended3* spyroExt;
int32 *spyro_x, *spyro_y, *spyro_z;
int8 *spyro_nextanim, *spyro_lastanim, *spyro_nextframe, *spyro_lastframe;
int32 *spyro_angle, *head_angle;
int32 *spyro_anim, *head_anim;
int32 *spyro_animprogress;
void *wadstart;

uint32 powers = 0;
uint32 texEditFlags = TEF_GENERATEPALETTES | TEF_SHUFFLEPALETTES | TEF_AUTOLQ;

int32* level;
int32 lastLevel;
uint8* levelArea;
SpyroGameState gameState;
SpyroGameState lastGameState;

uint16 joker, jokerPressed;
uint16* jokerPtr;

Moby* mobys;
int32* numMobys;

TexDef* textures;
int32* numTextures;

LqTexDef* lqTextures;
HqTexDef* hqTextures;

SpyroPointer<ModelHeader>* mobyModels;

SceneDef* sceneData;

CollisionCache collisionCache;

SkyDef* skyData;
uint32* skyNumSectors;
uint32* skyBackColour;

uint8* sceneOcclusion;
uint8* skyOcclusion;

CollDef* collData;
CollDefS1* s1CollData;

uint32* levelNames;
int numLevelNames;

uint32 backupColoursHp[256][512];
uint32 backupColoursLp[256][256];

uint32 usedMem[0x00200000 / 1024 / 32]; // Bit-based array of memory flagged as used. Reset on level load

bool disableSceneOccl = false;
bool disableSkyOccl = false;

int levelTextureHash = 0;

uint32 spyroModelPointer = 0;
uint32 spyroDrawFuncAddress = 0;
uint32 spyroDrawFuncAddressHack1 = 0;
uint32 spyroDrawFuncAddressHack2 = 0;

const uint32 drawPlayerFunctionAddress = 0x001F8200; // Default 0x001F8200
const uint32 drawPlayerAddress = drawPlayerFunctionAddress + 0xA0;

const char* spyro1LevelNames[] = {
	"Artisans", "Stone Hill", "Dark Hollow", "Town Square", "Toasty", "Sunny Flight", 
	"Peace Keepers", "Dry Canyon", "Cliff Town", "Ice Cavern", "Doctor Shemp", "Night Flight", 
	"Magic Crafters", "Alpine Ridge", "High Caves", "Wizard Peak", "Blowhard", "Crystal Flight", 
	"Beast Makers", "Terrace Village", "Misty Bog", "Tree Tops", "Metalhead", "Wild Flight", 
	"Dream Weavers", "Dark Passage", "Lofty Castle", "Haunted Towers", "Jacques", "Icy Flight", 
	"Gnorc Gnexus", "Gnorc Cove", "Twilight Harbour", "Gnasty Gnorc", "Gnasty's Loot"};

Player recordedFrames[60 * 30];
int numRecordedFrames = 0;

bool recording = false;
bool playing = false;
int playingFrame = 0;

GPUSnapshot hurr;

void UpdateLiveGen();
void LiveGenOnLevelEntry();
void SpyroOnLevelEntry();

void SpyroLoop() {
	DetectSpyroData();

	// Call OnEnterLevel if it's believed that the player has just entered a level
	bool texturesChanged = false;
	if ((textures || (lqTextures && hqTextures)) && level) {
		int newTexHash = 0;

		for (int i = 0; i < *numTextures; i++)
			newTexHash += textures ? (  textures[i].hqData[0].region * 4 +   textures[i].hqData[0].xmin * 3) : 
									 (hqTextures[i].hqData[0].region * 4 + hqTextures[i].hqData[0].xmin * 3);
		newTexHash += *level * 257328;

		if (newTexHash != levelTextureHash) {
			texturesChanged = true;
			levelTextureHash = newTexHash;
		}
	}

	if ((gameState == GAMESTATE_INLEVEL || gameState == GAMESTATE_POSTLOADINGLEVEL || gameState == GAMESTATE_PAUSED) && 
		(((lastGameState == GAMESTATE_LOADINGLEVEL || lastGameState == GAMESTATE_LOADINGMINIGAME)) || (level && *level != lastLevel) || texturesChanged)) {
		SpyroOnLevelEntry();

		if (level)
			lastLevel = *level;
	}

	switch (game) {
		case SPYRO1:
			UpdateLiveGen();
			//Spyro1BetaTestFunction();
			break;
		case SPYRO2:
		case SPYRO3:
			NetworkLoop();
			MultiplayerLoop();
			PowersLoop();
			UpdateLiveGen();
			break;
	}

	// Check level and gamestate changes
	lastGameState = gameState;
	if (level)
		lastLevel = *level;
	else
		lastLevel = -1;

	// Disable/restore occlusion
	if (sceneOcclusion) {
		static uint32 sceneOcclRestoreVal = 0, sceneOcclRestoreAddress = 0;

		if (disableSceneOccl) {
			if (*((uint32*) sceneOcclusion) != 0xFFFFFFFF) {
				sceneOcclRestoreVal = *((uint32*) sceneOcclusion);
				*((uint32*) sceneOcclusion) = 0xFFFFFFFF;
			}
		} else {
			if (*((uint32*) sceneOcclusion) == 0xFFFFFFFF && sceneOcclRestoreVal)
				*((uint32*) sceneOcclusion) = sceneOcclRestoreVal;
		}
	}

	if (skyOcclusion) {
		static uint32 skyOcclRestoreVal = 0, skyOcclRestoreAddress = 0;

		if (disableSkyOccl) {
			if (*((uint32*) skyOcclusion) != 0xFFFFFFFF) {
				skyOcclRestoreVal = *((uint32*) skyOcclusion);
				*((uint32*) skyOcclusion) = 0xFFFFFFFF;
			}
		} else {
			if (*((uint32*) skyOcclusion) == 0xFFFFFFFF && skyOcclRestoreVal)
				*((uint32*) skyOcclusion) = skyOcclRestoreVal;
		}
	}

	// Freeze moving textures at their base
	if (textures || (lqTextures && hqTextures)) {
		for (int i = 0; i < *numTextures; i++) {
			TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;
			for (int tile = 0; tile < 4; tile++) {
				if (curHq[tile].GetXMin() < texCaches[i].tiles[tile].minX)
					texCaches[i].tiles[tile].minX = curHq[tile].GetXMin() & ~7;
				if (curHq[tile].GetYMin() < texCaches[i].tiles[tile].minY)
					texCaches[i].tiles[tile].minY = curHq[tile].GetYMin() & ~7;
				if (curHq[tile].GetXMax() > texCaches[i].tiles[tile].maxX)
					texCaches[i].tiles[tile].maxX = curHq[tile].GetXMax() + 7 & ~7;
				if (curHq[tile].GetYMax() > texCaches[i].tiles[tile].maxY)
					texCaches[i].tiles[tile].maxY = curHq[tile].GetYMax() + 7 & ~7;
			}
		}
	}
}

void SpyroOnLevelEntry() {
	LiveGenOnLevelEntry();

	// Backup level colours (no more colour loss)
	if (sceneData) {
		for (int i = 0; i < sceneData->numSectors; i++) {
			uint32* colours = sceneData->sectors[i]->GetHpColours(), *lpColours = sceneData->sectors[i]->GetLpColours();
			uint32 numColours = sceneData->sectors[i]->numHpColours, numLpColours = sceneData->sectors[i]->numLpColours;

			for (int j = 0, e = numColours * 2; j < e; j++)
				backupColoursHp[i][j] = colours[j];
			for (int j = 0; j < numLpColours; j++)
				backupColoursLp[i][j] = lpColours[j];
		}
	}

	// Auto-load level textures
	// Update the texture cache
	GetSnapshot(&vramSs);
	
	uint8* vram8 = (uint8*) &vramSs.vram;
	uint16* vram16 = (uint16*) &vramSs.vram;

	numTexCaches = *numTextures;
	for (int i = 0; i < *numTextures; i++) {
		TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;
		TexLq* curLq = textures ? textures[i].lqData : lqTextures[i].lqData;

		texCaches[i].Reset();

		for (int tile = 0; tile < 4; tile++) {
			int paletteByteStart = curHq[tile].palette * 32;
			uint16* palette = &vram16[paletteByteStart / 2];
			int numTilesDone = 0;
			uint32 colours[256];
			TexCacheTile* tileCache = &texCaches[i].tiles[tile];

			// Read palette data
			for (int j = 0; j < 256; j++)
				colours[j] = MAKECOLOR32(GETR16(palette[j]), GETG16(palette[j]), GETB16(palette[j]));

			int orientation = curHq[tile].unknown>>4 & 7;
			int xx = tileMatrices[orientation].xx, xy = tileMatrices[orientation].xy;
			int yx = tileMatrices[orientation].yx, yy = tileMatrices[orientation].yy;
			int srcXStart = curHq[tile].GetXMin(), srcYStart = curHq[tile].GetYMin();
			int xAdd = 0, yAdd = 0;

			if (xx < 0 || xy < 0)
				xAdd = 31;
			if (yx < 0 || yy < 0)
				yAdd = 31;

			// Read colours into cache tile
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 32; x++)
					tileCache->bitmap[(x * yx + y * yy + yAdd) * 32 + x * xx + y * xy + xAdd] = colours[vram8[(y + srcYStart) * 2048 + srcXStart + x]];
			}

			tileCache->minX = curHq[tile].GetXMin();
			tileCache->maxX = curHq[tile].GetXMax();
			tileCache->minY = curHq[tile].GetYMin();
			tileCache->maxY = curHq[tile].GetYMax();
			tileCache->sizeX = tileCache->maxX - tileCache->minX + 1;
			tileCache->sizeY = tileCache->maxY - tileCache->minY + 1;
		}
	}
			
	// If autoload is enabled, do the thing
	if (SendMessage(checkbox_autoLoad, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		LoadTextures();
		LoadObjectTextures();
		LoadColours();
		LoadSky();
	}
}

#define BUILDADDR(immUpper, immSignedLower) (((((immUpper) & 0xFFFF) << 16) + (int16) ((immSignedLower) & 0xFFFF)) & 0x003FFFFF)
#define STRIPADDR(addr) ((addr) & 0x003FFFFF)

enum SpyroDataPlacesOfInterest {POI_SPYRO, POI_MOBYS, POI_TEXTURES, POI_LQTEXTURES, POI_HQTEXTURES, POI_SKY, POI_LEVEL, POI_LEVELAREA, POI_LEVELNAMES, POI_SCENE, POI_COLLISION, 
								POI_SCENEOCCL, POI_SKYOCCL, POI_MOBYMODELS, POI_SPYROMODEL, POI_SPYRODRAW, POI_GAMESTATE, POI_JOKER, POI_NUMTYPES};

uint32 spyroPois[POI_NUMTYPES]; // List of Spyro 'places of interest' e.g. areas of code with reference to a required set of data, such as the level scenery
								// This list is re-used and/or updated on each Spyro data scan. This allows a game disc swap to take place comfortably
								// List values are in indices of umem32
								// The Temmie Shop theme is now stuck in my head. pOI!!!!!

void DetectSpyroData() {
	// Find the variables we need for Spyro hacking. If this function has been called before, spyroPois is a helpful list of previously-found memory areas.
	// If not, the memory is re-scanned from minMemScan to maxMemScan.
	// During this function all Spyro variables are reset, this is mostly for safety but also because some variables, such as mobys, will change between levels.
	const uint32 minMemScan = 0x00010000;
	const uint32 maxMemScan = 0x00070000;
	uint32* uintmem = (uint32*) memory;
	int8* bytemem = (int8*) memory;
	uint16 lastJoker = joker;

	mobys = NULL; numMobys = NULL;
	textures = NULL; numTextures = NULL;
	lqTextures = NULL; hqTextures = NULL;
	level = NULL; spyro = NULL;
	skyData = NULL; skyNumSectors = NULL; skyBackColour = NULL;
	levelNames = NULL; numLevelNames = 0;
	sceneData = NULL; collData = NULL; s1CollData = NULL;
	sceneOcclusion = NULL; skyOcclusion = NULL;
	spyroDrawFuncAddress = 0;
	//spyroModelPointer = 0;
	joker = 0; jokerPtr = NULL;
	game = UNKNOWN_GAME;
	gameRegion = UNKNOWN_REGION;
	gameVersion = UNKNOWN_VERSION;
	gameState = GAMESTATE_NOTFOUND;
	mobyModels = NULL;
	levelArea = NULL;

	uint32 lastPois[POI_NUMTYPES];
	for (int poi = 0; poi < POI_NUMTYPES; poi++) {
		lastPois[poi] = spyroPois[poi];
		spyroPois[poi] = 0; // spyroPois are reset in this loop due to some detections setting multiple pois in the next loop
	}

	for (int poi = 0; poi <= POI_NUMTYPES; poi++) {
		uint32 memStart = poi < POI_NUMTYPES ? lastPois[poi]   : minMemScan / 4;
		uint32 memEnd = poi < POI_NUMTYPES ? lastPois[poi] + 1 : maxMemScan / 4;

		if (poi == POI_NUMTYPES) {
			// Check if we have obtained all needed variables. If so, there's no need to scan the entire memory area
			if (spyroPois[POI_SPYRO] && spyroPois[POI_MOBYS] && (spyroPois[POI_TEXTURES] || (spyroPois[POI_LQTEXTURES] && spyroPois[POI_HQTEXTURES])) && 
				spyroPois[POI_SKY] && spyroPois[POI_LEVEL] && spyroPois[POI_SCENE] && spyroPois[POI_COLLISION] && spyroPois[POI_GAMESTATE] && spyroPois[POI_JOKER] && 
				(spyroPois[POI_LEVELAREA] || game != SPYRO3)) {

				break;
			}
		}

		for (int i = memStart; i < memEnd; i++) {
			if (!mobys) {
				// Moby list (Spyro 3)
				uint32 mobyAddr = 0;

				if ((uintmem[i+0] & 0xFFFF0000) == 0x8C420000u && 
					(uintmem[i+1] & 0xFFFF0000) == 0x3C030000u && 
					(uintmem[i+2] & 0xFFFF0000) == 0x8C630000u && 
					uintmem[i+3] == 0x00000000                 && 
					uintmem[i+4] == 0x00431023                 && 
					uintmem[i+5] == 0x28420A01) {

					mobyAddr = BUILDADDR(uintmem[i + 8], uintmem[i + 9]);
				}

				// Moby list (Spyro 2)
				if (uintmem[i+0] == 0x8E700004 && uintmem[i+1] == 0x26630008 && (uintmem[i+2] & 0xFFFF0000) == 0x3C010000 && 
					(uintmem[i+3] & 0xFFFF0000) == 0xAC320000) {

					mobyAddr = BUILDADDR(uintmem[i + 2], uintmem[i + 3]);
				}

				// Moby list (Spyro 1)
				if (uintmem[i+0] == 0x26100004 && uintmem[i+1] == 0x8E110000 && uintmem[i+2] == 0x26100004 && uintmem[i+3] == 0x3C042492 && 
					(uintmem[i+4] & 0xFFFF0000) == 0x3C010000 && (uintmem[i+5] & 0xFFFF0000) == 0xAC300000) {

					mobyAddr = BUILDADDR(uintmem[i + 4], uintmem[i + 5]);
				}

				if (mobyAddr > 0 && mobyAddr < 0x00200000) {
					if ((uintmem[mobyAddr / 4] & 0x003FFFFF) < 0x00200000 && uintmem[mobyAddr / 4] > 0) {
						mobys = (Moby*) &bytemem[uintmem[mobyAddr / 4] & 0x003FFFFF];
						numMobys = (int*) &bytemem[(uintmem[mobyAddr / 4] & 0x003FFFFF) - 4];
						spyroPois[POI_MOBYS] = i;
					}
				}
			}

			// Texture list (Spyro 2 and Spyro 3)
			if (!textures) {
				if (uintmem[i+0] == 0x8DA40004 && (uintmem[i+1] & 0xFFFF0000) == 0x3C010000 && (uintmem[i+2] & 0xFFFF0000) == 0x24210000 && uintmem[i+3] == 0x8C210020) {
					uint32 addr = BUILDADDR(uintmem[i + 1], uintmem[i + 2]);
				
					if (uintmem[addr / 4 + 8] >= 0x80000000 && uintmem[addr / 4 + 8] < 0x80200000) {
						textures = (TexDef*) &bytemem[STRIPADDR(uintmem[addr / 4 + 8])];
						numTextures = (int*) &bytemem[addr + 0x24];
						spyroPois[POI_TEXTURES] = i;
					}
				}
			}

			if (!sceneData) {
				// Scene data and collision data(Spyro 2 and Spyro 3)
				if (uintmem[i+0] >> 16 == 0x3C06 && uintmem[i+1] >> 16 == 0x24C6 && uintmem[i+2] == 0x8CC60028 && uintmem[i+3] == 0x302300FF && uintmem[i+4] == 0x00011202) {
					uint32 addr = BUILDADDR(uintmem[i], uintmem[i + 1]);
					uint32 collAddr = ((uintmem[i] << 16) + (int16) (uintmem[i+1] & 0xFFFF) + (int16) (uintmem[i+2] & 0xFFFF)) & 0x003FFFFF;

					if ((uintmem[addr/4] & 0x003FFFFF) > 0 && (uintmem[addr/4] & 0x003FFFFF) < 0x00200000) {
						sceneData = (SceneDef*) &bytemem[uintmem[addr/4] - 0x0C & 0x003FFFFF];
						collData = (CollDef*) &bytemem[uintmem[collAddr/4] & 0x003FFFFF];
						sceneOcclusion = (uint8*)&bytemem[uintmem[addr/4+2] & 0x003FFFFF];
						skyOcclusion = (uint8*)&bytemem[uintmem[addr/4+3] & 0x003FFFFF];
						spyroPois[POI_SCENE] = i;
						spyroPois[POI_COLLISION] = i;
						spyroPois[POI_SCENEOCCL] = i;
						spyroPois[POI_SKYOCCL] = i;
					}
				}

				// Scene data, texture data and urp collision data (Spyro 1)
				if (uintmem[i+0] == 0x02023021 && uintmem[i+1] == 0x00c08021 && uintmem[i+2] == 0x24C60004 && uintmem[i+3] == 0x8CC20000 && uintmem[i+4] == 0x24C60004 && 
					uintmem[i+5] >> 16 == 0x3C01 && uintmem[i+6] >> 16 == 0xAC26) {

					uint32 addr = BUILDADDR(uintmem[i + 5], uintmem[i + 6]);

					if ((uintmem[addr/4] & 0x003FFFFF) > 0 && (uintmem[addr/4] & 0x003FFFFF) < 0x00200000) {
						sceneData = (SceneDef*) &bytemem[uintmem[addr/4] - 0x0C & 0x003FFFFF];
						spyroPois[POI_SCENE] = i;
					}

					if ((uintmem[addr/4+6] & 0x003FFFFF) > 0 && (uintmem[addr/4+6] & 0x003FFFFF) < 0x00200000) {
						lqTextures = (LqTexDef*) &bytemem[uintmem[addr/4+6] & 0x003FFFFF];
						spyroPois[POI_LQTEXTURES] = i;
					}

					if ((uintmem[addr/4+7] & 0x003FFFFF) > 0 && (uintmem[addr/4+7] & 0x003FFFFF) < 0x00200000) {
						hqTextures = (HqTexDef*) &bytemem[uintmem[addr/4+7] & 0x003FFFFF];
						spyroPois[POI_HQTEXTURES] = i;
					}

					if ((uintmem[addr/4+11] & 0x003FFFFF) > 0 && (uintmem[addr/4+11] & 0x003FFFFF) < 0x00200000) {
						s1CollData = (CollDefS1*) &bytemem[uintmem[addr/4+11] & 0x003FFFFF];
						spyroPois[POI_COLLISION] = i;
					}

					numTextures = (int*)&uintmem[addr/4+8];
					game = SPYRO1;
				}
			}

			if (!skyData) {
				// Sky data (Spyro 2 and Spyro 3)
				if (uintmem[i+0] == 0x48C03800 && (uintmem[i+1] >> 16) == 0x3C01 && (uintmem[i+2] >> 16) == 0x2421 && uintmem[i+3] == 0x8C3F0000 && uintmem[i+4] == 0x8C3D0004) {
					uint32 addr = BUILDADDR(uintmem[i + 1], uintmem[i + 2]);
					if ((uintmem[addr/4+1] & 0x003FFFFF) > 0 && (uintmem[addr/4+1] & 0x003FFFFF) < 0x00200000) {
						skyData = (SkyDef*) &bytemem[STRIPADDR(uintmem[addr/4+1] - 0x0C)];
						skyNumSectors = (uint32*) &uintmem[addr/4];
						skyBackColour = (uint32*) &uintmem[addr/4+2];
						spyroPois[POI_SKY] = i;
					}
				}

				// Sky data (Spyro 1)
				if (uintmem[i+0] == 0xACA20000 && uintmem[i+1] == 0x8E020000 && uintmem[i+2] == 0x26100004 && uintmem[i+3] >> 16 == 0x3C01 && 
					uintmem[i+4] >> 16 == 0xAC30 && uintmem[i+8] == 0x00001821) {

					uint32 addr = BUILDADDR(uintmem[i + 3], uintmem[i + 4]);
					if ((uintmem[addr/4] & 0x003FFFFF) > 0 && (uintmem[addr/4] & 0x003FFFFF) < 0x00200000) {
						skyData = (SkyDef*) &bytemem[STRIPADDR(uintmem[addr/4] - 0x0C)];
						skyNumSectors = (uint32*) &uintmem[addr/4-1];
						skyBackColour = (uint32*) &skyData->backColour;
						spyroPois[POI_SKY] = i;
					}
				}
			}

			// Spyro's variables (Spyro 2 and 3) ----------
			if (!spyro) {
				if ((uintmem[i+0] >> 16) == 0x27BD && 
					(uintmem[i+1] >> 16) == 0xAFB2 && 
					 uintmem[i+2] == 0x00809021    && 
					(uintmem[i+3] >> 16) == 0xAFBF && 
					(uintmem[i+4] >> 16) == 0xAFB1 && 
					(uintmem[i+5] >> 24) == 0x0C   && 
					(uintmem[i+6] >> 16) == 0xAFB0 && 
					(uintmem[i+7] >> 16) == 0x3C10 && 
					(uintmem[i+9] >> 16) == 0x3C05) {

					spyro = (Spyro*) &bytemem[BUILDADDR(uintmem[i + 9], uintmem[i + 10])];
					spyroExt = (SpyroExtended3*) spyro;
					spyroPois[POI_SPYRO] = i;
				}
			}

			// Level ID ----------
			if (!level) {
				// Spyro 3
				if (uintmem[i+0] == 0x2604000A && uintmem[i+1] >> 16 == 0x3C06 && uintmem[i+2] >> 16 == 0x8CC6 && uintmem[i+3] >> 16 == 0x3C01 && uintmem[i+4] == 0x00260821) {
					level = (int32*) &bytemem[BUILDADDR(uintmem[i + 1], uintmem[i + 2])];
					spyroPois[POI_LEVEL] = i;
				}

				// Spyro 2
				if (uintmem[i+0] >> 16 == 0x3C02 && uintmem[i+1] >> 16 == 0x8C42 && uintmem[i+2] == 0x00031983 && uintmem[i+3] == 0x00021080 && 
					uintmem[i+4] == 0x00451021 && uintmem[i+5] == 0x00431021) {
					level = (int32*) &bytemem[BUILDADDR(uintmem[i], uintmem[i + 1])];
					spyroPois[POI_LEVEL] = i;
				}

				// Spyro 1
				if (uintmem[i+0]  == 0x10400023 && uintmem[i+1] == 0x00000000 && uintmem[i+2] >> 16 == 0x3C05 && uintmem[i+3] >> 16 == 0x24A5) {
					level = (int32*) &bytemem[BUILDADDR(uintmem[i+4], uintmem[i+5])];
					spyroPois[POI_LEVEL] = i;
				}
			}

			// Level area (Spyro 3 only)
			if (!levelArea) {
				if (uintmem[i] >> 16 == 0x3C03 && uintmem[i+1] >> 16 == 0x8C63 && uintmem[i+2] == 0x00021100 && uintmem[i+3] == 0x00441021 && uintmem[i+10] == 0x0043102A) {
					levelArea = &umem8[BUILDADDR(uintmem[i], uintmem[i+1])];
					spyroPois[POI_LEVELAREA] = i;
				}
			}

			// Level name list
			if (!levelNames) {
				if (uintmem[i] >= 0 && (uintmem[i] & 0x7FFFFFFF) < 0x00200000 - 20) {
					if (bytemem[uintmem[i] & 0x7FFFFFFF] == 'S' && bytemem[(uintmem[i] & 0x7FFFFFFF) + 1] == 'u') {
						// Level names (Spyro 2)
						if (!strcmp(&bytemem[uintmem[i] & 0x7FFFFFFF], "Summer Forest")) {
							levelNames = &uintmem[i];
							numLevelNames = 29;
							game = SPYRO2;
							spyroPois[POI_LEVELNAMES] = i;
						}
						// Level names (Spyro 3)
						else if (!strncmp(&bytemem[uintmem[i] & 0x7FFFFFFF], "Sunrise Spring", 14)) {
							levelNames = &uintmem[i];
							numLevelNames = 37;
							game = SPYRO3;

							if (uintmem[i] == 0x80011B24) {
								gameRegion = PAL;
								gameVersion = ORIGINAL;
							}

							spyroPois[POI_LEVELNAMES] = i;
						}
					}
				}
			}

			// Moby models (Spyro 2 & 3)
			if (!mobyModels) {
				if (uintmem[i] >> 16 == 0x3C02 && uintmem[i+1] >> 16 == 0x8C42 && uintmem[i+2] == 0 && uintmem[i+3] == 0x8C420000 && uintmem[i+4] == 0x24E70001 &&
					uintmem[i+5] == 0x00E2102A && uintmem[i+7] == 0x24A50004) {
					spyroPois[POI_MOBYMODELS] = i;
					mobyModels = (SpyroPointer<ModelHeader>*)&umem8[BUILDADDR(uintmem[i], uintmem[i+1])];
				}
			}

			// Spyro draw function address (Spyro 3)
			if (!spyroDrawFuncAddress) {
				if (uintmem[i] == 0x2021D000 && uintmem[i+1] == 0x48811000 && uintmem[i+2] == 0x20391400) {
					uint32 funcAddress = 0;
					int maxGoBack = 30;

					for (int goBack = 1; goBack < maxGoBack; goBack++) {
						if ((uintmem[i - goBack] >> 16) == 0x3C01 && (uintmem[i - goBack + 4] >> 20) == 0xAC3) {
							funcAddress = ((i - goBack) * 4) | 0x80000000;
							break;
						}
					}

					if (funcAddress) {
						int maxGoForward = 0x200;

						spyroDrawFuncAddress = funcAddress; // Got the draw func address but addresshack1 and 2 are not guaranteed since they are lost when the hack is applied
						spyroPois[POI_SPYRODRAW] = i;

						for (int addr = (funcAddress & 0x003FFFFF) / 4, e = (funcAddress & 0x003FFFFF) / 4 + maxGoForward; addr < e; addr++) {
							// Look for the addresses to modify during the multi-Spyro draw code
							if (((uintmem[addr] >> 16) == 0x3C1F && ((uintmem[addr + 1] >> 16) == 0x27FF) || (uintmem[addr + 1] >> 16) == 0x37FF)) {
								if (!spyroDrawFuncAddressHack1) spyroDrawFuncAddressHack1 = (addr * 4) | 0x80000000;
								else if (!spyroDrawFuncAddressHack2) spyroDrawFuncAddressHack2 = (addr * 4) | 0x80000000;
							}

							// Also look for Spyro's model address
							if (uintmem[addr] >> 16 == 0x3C1D && uintmem[addr + 1] >> 16 == 0x27BD) {
								;/*spyroModelPointer = BUILDADDR(uintmem[addr], uintmem[addr + 1]);

								if (spyroModelPointer >= 0x00ffc7ff)
									spyroModelPointer = spyroModelPointer;*/
							}
						}
					}
				}
			}

			// GameState
			if (gameState == GAMESTATE_NOTFOUND) {
				// Spyro 1
				if (uintmem[i] >> 16 == 0x3C01 && uintmem[i+1] >> 16 == 0xAC22 && uintmem[i+3] == 0x2402000C && uintmem[i+4] >> 16 == 0x3C03 && uintmem[i+5] >> 16 == 0x8C63 && 
					uintmem[i+6] == 0x00000000) {
					uint32 value = uintmem[(BUILDADDR(uintmem[i+4], uintmem[i+5]) & 0x003FFFFF) / 4];

					gameState = GAMESTATE_UNKNOWN;
				
					const SpyroGameState states[] = {
						GAMESTATE_INLEVEL, GAMESTATE_LOADINGLEVEL, GAMESTATE_PAUSED, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, 
						GAMESTATE_UNKNOWN, GAMESTATE_POSTLOADINGLEVEL, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, GAMESTATE_LOADINGLEVEL /* balloonist */};

					if (value < sizeof (states) / sizeof (states[0]))
						gameState = states[value];
					
					spyroPois[POI_GAMESTATE] = i;
				}


				// Spyro 2/3
				if (uintmem[i] == 0x00101040 && uintmem[i+1] == 0x00501021 && uintmem[i+2] == 0x00021080 && uintmem[i+3] == 0x00501023 && uintmem[i+4] == 0x00022880 && game != SPYRO1) {
					uint32 foundAddr = 0;
					for (int j = i + 5; j < i + 15; j++) {
						if (uintmem[j] >> 16 == 0x3C03 && uintmem[j + 1] >> 16 == 0x8C63 && uintmem[j + 2] == 0x24020003) {
							foundAddr = BUILDADDR(uintmem[j], uintmem[j + 1]);
						}
					}

					if (foundAddr) {
						uint32 value = uintmem[foundAddr / 4];

						gameState = GAMESTATE_UNKNOWN; // promoted from not-found to unknown!

						const SpyroGameState states[] = {
							GAMESTATE_INLEVEL, GAMESTATE_TALKING, GAMESTATE_UNKNOWN, GAMESTATE_POSTLOADINGLEVEL, GAMESTATE_PAUSED, GAMESTATE_LOADINGLEVEL, GAMESTATE_CUTSCENE, // 0-6
							GAMESTATE_LOADINGLEVEL, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, GAMESTATE_UNKNOWN, GAMESTATE_LOADINGLEVEL, GAMESTATE_LOADINGMINIGAME};

						if (value < sizeof (states) / sizeof (states[0]))
							gameState = states[value];
						
						spyroPois[POI_GAMESTATE] = i;
					}
				}
			}

			// Joker
			if (!jokerPtr) {
				if (uintmem[i] == 0x1440000D && uintmem[i+1] == 0x240200FF && uintmem[i+5] == 0x14620006 &&
					uintmem[i+2] >> 16 == 0x3C03 && uintmem[i+3] >> 16 == 0x9063) {
					jokerPtr = (uint16*)&umem8[BUILDADDR(uintmem[i+2], uintmem[i+3]) & 0x003FFFFF];
					joker = ~(*jokerPtr);
					spyroPois[POI_JOKER] = i;
				}
			}
		}
	}

	// Update jokerPressed
	jokerPressed = (joker ^ lastJoker) & joker;
}

void MakeCrater(int craterX, int craterY, int craterZ);
void PowersLoop() {
	if (!mobys || !spyro)
		return;

	static int shockwaveDist = 0;
	static int headbashingPlayer = -1;
	static int tkObject = -1;
	static int tkLockDist = 0;
	uint32* uintmem = (uint32*) memory;

	int numPlayersTotal = player_count;

	if (playing)
		numPlayersTotal ++; // Include recorded player

	for (int j = 0; j < numPlayersTotal; j++) {
		Spyro* curSpyro;
		uint32 curPowers;
			
		if (playing && j == numPlayersTotal - 1) { // HACK - use recorded player
			curSpyro = &recordedFrames[playingFrame].spyro;
			curPowers = recordedFrames[playingFrame].powers;
		} else if (j == playerId) { // Local player
			curSpyro = spyro;
			curPowers = powers;
		} else { // Another player
			curSpyro = &players[j].spyro;
			curPowers = players[j].powers;
		}

		if (!curPowers)
			continue; // No powers to use!

		if ((curPowers & (PWR_SUPERBASH|PWR_ULTRABASH|PWR_HEADBASHPOCALYPSE))) {
			if (headbashingPlayer != -1) {
				if (shockwaveDist > 15000)
					headbashingPlayer = -1; // Finished!

				shockwaveDist += 1000;
			}

			int numHeadbashFrames = 1;
			if (mobyModels && mobyModels[0].address && mobyModels[0]->anims[46].address)
				numHeadbashFrames = mobyModels[0]->anims[46]->numFrames;

			if (curSpyro->main_anim.nextanim == 46 && 
				curSpyro->main_anim.nextframe >= 12 * numHeadbashFrames / 22 && curSpyro->main_anim.nextframe <= 14 * numHeadbashFrames / 22 && headbashingPlayer == -1) {
				headbashingPlayer = j;
				shockwaveDist = 1000;
				
				if (curPowers & PWR_HEADBASHPOCALYPSE)
					MakeCrater(curSpyro->x, curSpyro->y, curSpyro->z);
			}
		}

		// Iterate all game objects (mobys)
		for (int i = 0; i < 500; i++) {
			if (mobys[i].state == -1)
				break;

			if (mobys[i].state < 0)
				continue;

#define RECALCDIST dist = Distance(mobys[i].x, mobys[i].y, mobys[i].z, curSpyro->x, curSpyro->y, curSpyro->z)
			bool killable = (mobys[i].type != 206); // Question mark jars can freeze Spyro if destroyed from a distance
			int dist = Distance(mobys[i].x, mobys[i].y, mobys[i].z, curSpyro->x, curSpyro->y, curSpyro->z);
			float angle = atan2((float) (mobys[i].y - curSpyro->y), (float) (mobys[i].x - curSpyro->x));
			uint8 ang1 = (uint8) ToAngle(angle);
			uint8 ang2 = (uint8) (curSpyro->main_angle.z + curSpyro->head_angle.z) & 0xFF;
			int16 angDifference = 0;
			if (ang1 > ang2)      angDifference = ang1 - ang2;
			else if (ang2 > ang1) angDifference = ang2 - ang1;
			if (angDifference > 128)
				angDifference = 256 - angDifference;

			if ((curPowers & PWR_ATTRACTION) && dist < 10000 && dist > 1000) {
				float attractAmt = dist < 400 ? (float) dist : 400.0f;

				if (dist - attractAmt < 1000)
					attractAmt = dist - 1000;

				float dir = atan2((float) -(mobys[i].y - curSpyro->y), (float) -(mobys[i].x - curSpyro->x));
				mobys[i].x += cos(dir) * attractAmt;
				mobys[i].y += sin(dir) * attractAmt;
				mobys[i].z -= sin(atan2((float) (mobys[i].z - curSpyro->z), dist)) * attractAmt;

				RECALCDIST;
			}

			if ((curPowers & PWR_REPELSTARE) && dist < 10000 && angDifference < 64) {
				float repelForce = (8000.0f - (float) dist) / 10.0f * (1.0f - angDifference / 64.0f);

				mobys[i].x += cos(angle) * repelForce;
				mobys[i].y += sin(angle) * repelForce;
					
				RECALCDIST;
			}

			if ((curPowers & PWR_ATTRACTSTARE) && dist < 10000 && angDifference < 5/* && mobys[i].type == 1*/) { // GEM HACK
				float attractForce = dist < 400 ? (float) dist : 400.0f;

				if (dist - attractForce < 600)
					attractForce = dist - 600;

				mobys[i].x -= cos(angle) * attractForce;
				mobys[i].y -= sin(angle) * attractForce;
				mobys[i].z -= sin(atan2((float) (mobys[i].z - curSpyro->z), dist)) * attractForce;
					
				RECALCDIST;
			}
				
			if ((curPowers & PWR_GEMATTRACT) && mobys[i].type == 1 && dist < 100000) {
				float attractAmt = (100000.0f - (float) dist) / 10.0f;
				//float attractAmt = dist < 400 ? (float) dist : 400.0f;

				if (attractAmt > 400.0f) attractAmt = 400.0f;

				if (attractAmt > dist)
					attractAmt = dist;

				float dir = atan2((float) -(mobys[i].y - curSpyro->y), (float) -(mobys[i].x - curSpyro->x));
				mobys[i].x += cos(dir) * attractAmt;
				mobys[i].y += sin(dir) * attractAmt;
				mobys[i].z -= sin(atan2((float) (mobys[i].z - curSpyro->z), dist)) * attractAmt;
					
				RECALCDIST;
			}

			if (((curPowers & PWR_SUPERBASH) || (curPowers & PWR_ULTRABASH)) && headbashingPlayer == j && killable) {
				if (dist < shockwaveDist || (curPowers & PWR_ULTRABASH))
					mobys[i].attack_flags = 0x00950000;
			}

			if ((curPowers & PWR_BUTTERFLYBREATH) && mobys[i].attack_flags == 0x00010000 && mobys[i].type != 16 && 
				((! (uintmem[0x0006F9F8 / 4] < 300 && mobys[i].type == 527)) || game != SPYRO3)) { // SHEILA HACK
				// Butterfly ID: 16
				Animation null = {0, 0, 0, 0};
				uint32* uintmem = (uint32*) memory;

				mobys[i].typeData = 0x80000000;
				mobys[i].collisionData = 0;
				mobys[i].anim = null;
				mobys[i].anim.nextframe = 1;
				mobys[i].type = 16;
				mobys[i].state = 0;
				//mobys[i]._unknown[0] = 0;
				//mobys[i]._unknown[1] = 0;
				mobys[i].animspeed = 0x30;
				mobys[i].animRun = 0xFF;

				uintmem[0x00000000/4] = mobys[i].x;
				uintmem[0x00000004/4] = mobys[i].y;
				uintmem[0x00000008/4] = mobys[i].z + 10000;
				uintmem[0x0000000c/4] = 0x05dc0001;
				uintmem[0x00000010/4] = 0x1cc80000;
				uintmem[0x00000014/4] = 0x00000000;
			}

			// DEATH STARE, deserves more code; doesn't need it though!
			if ((curPowers & PWR_DEATHSTARE) && dist < 10000 && killable && angDifference <= 5)
				mobys[i].attack_flags = 0x00950000; // SuperFlame: 00950000 Frozen Altars Laser: 10000000

			if ((curPowers & PWR_DEATHFIELD) && dist < 10000 && killable)
				mobys[i].attack_flags = 0x00950000;

			if ((curPowers & PWR_REPULSION) && mobys[i].type != 120 && dist <= 8000) { // Don't repel Sparx
				float repelForce = (8000.0f - (float) dist) / 10.0f;

				mobys[i].x += cos(angle) * repelForce;
				mobys[i].y += sin(angle) * repelForce;
					
				RECALCDIST;
			}

			if ((curPowers & PWR_FORCEFIELD) && mobys[i].type != 120 && dist < 2250) {
				mobys[i].x += cos(angle) * (2250 - dist);
				mobys[i].y += sin(angle) * (2250 - dist);
					
				RECALCDIST;
			}

			if ((curPowers & PWR_TORNADO) && dist < 10000 && mobys[i].type != 120) {
				mobys[i].x += cos(angle - 3.141592f / 2.0f) * 600;
				mobys[i].y += sin(angle - 3.141592f / 2.0f) * 600;

				// Don't throw them out of orbit, now!
				int newDist = Distance(curSpyro->x, curSpyro->y, curSpyro->z, mobys[i].x, mobys[i].y, mobys[i].z);
				mobys[i].x = (mobys[i].x - curSpyro->x) * dist / newDist + curSpyro->x;
				mobys[i].y = (mobys[i].y - curSpyro->y) * dist / newDist + curSpyro->y;
					
				//RECALCDIST; No need; we're locking the distance
			}

			if ((curPowers & PWR_TELEKINESIS) && dist < 15000 && angDifference <= 5 && uintmem[0x00071340/4] == 7 && // Note: Only currently compatible with PAL (camera mode code)
				mobys[i].type != 1002 && game == SPYRO3 && gameRegion == PAL) {

				if (tkObject == -1) {
					tkObject = i;
					tkLockDist = dist;
				} else {
					if (dist < Distance(curSpyro->x, curSpyro->y, curSpyro->z, mobys[tkObject].x, mobys[tkObject].y, mobys[tkObject].z)) {
						tkObject = i;
						tkLockDist = dist;
					}
				}
			}
		} // for loop
	
		if (tkObject != -1) {
			if (((curPowers & PWR_TELEKINESIS) && uintmem[0x00071340/4] != 7) || mobys[tkObject].state < 0) // Note: Only currently compatible with PAL (camera mode code)
				tkObject = -1; // Reset TK object
			else {
				// Move object
				int dist = Distance(mobys[tkObject].x, mobys[tkObject].y, mobys[tkObject].z, curSpyro->x, curSpyro->y, curSpyro->z);
				float angleRad = ToRad((uint8) curSpyro->head_angle.z + (uint8) curSpyro->main_angle.z);
				float verAngleRad = ToRad((uint8) curSpyro->head_angle.y + (uint8) curSpyro->main_angle.y);
				mobys[tkObject].x = curSpyro->x + cos(angleRad) * tkLockDist * cos(verAngleRad);
				mobys[tkObject].y = curSpyro->y + sin(angleRad) * tkLockDist * cos(verAngleRad);
				mobys[tkObject].z = curSpyro->z + tkLockDist * sin(verAngleRad);
			}
		}
	}

	// Single-player powers
	int mobyCount = 0;
	static int barrelRollTime = 0, barrelRollDirection = 0;
	static int backflipTime = 0;
	static int upsideDownTime = 0;
	bool sonicSpinningLastFrame = false;

	if ((powers & PWR_LOOKATSTUFF) && game == SPYRO3 && gameRegion == PAL) {
		if (mobys && spyro) {
			int closestMoby = -1;
			int closestMobyDist = 5000;
			for (int i = 0; ; i++) {
				if (mobys[i].state == -1)
					break;
				mobyCount++;

				if (mobys[i].state >= 0 && mobys[i].type != 120 && mobys[i].type < 1000) {
					int dist = Distance(spyro->x, spyro->y, spyro->z, mobys[i].x, mobys[i].y, mobys[i].z);
					int head = (int) ((atan2(mobys[i].y - spyro->y, mobys[i].x - spyro->x)) * 8192.0f / 3.14f) - (spyroExt->zAngle * 0x4000 / 0x1000);

					while (head >= 8192) head -= 16384;
					while (head <= -8192) head += 16384;

					if (head > -5000 && head < 5000 && dist < closestMobyDist) {
						closestMoby = i;
						closestMobyDist = dist;
					}
				}
			}

			if (closestMoby != -1) {
				SpyroExtended3* spyroExt = (SpyroExtended3*) spyro;
				spyroExt->headZAngle = (int) ((atan2(mobys[closestMoby].y - spyro->y, mobys[closestMoby].x - spyro->x)) * 8192.0f / 3.14f) - 
					(spyroExt->zAngle * 0x4000 / 0x1000);
				spyroExt->headYAngle = (int) ((atan2(mobys[closestMoby].z - spyro->z + 500, Distance(mobys[closestMoby].x, mobys[closestMoby].y, 0, spyro->x, spyro->y, 0)) * 
					8192.0f / 3.14f));

				while (spyroExt->headZAngle >= 8192) spyroExt->headZAngle -= 16384;
				while (spyroExt->headZAngle <= -8192) spyroExt->headZAngle += 16384;
			}
		}
	}

	if (powers & PWR_BARRELROLLS) {
		if ((joker & BUT_X) && (joker & (BUT_LEFT | BUT_RIGHT)) && !barrelRollTime) {
			if (joker & BUT_RIGHT) barrelRollDirection = 0;
			if (joker & BUT_LEFT) barrelRollDirection = 1;

			barrelRollTime = 1;
		}

		if (spyro->main_anim.nextanim != 0x11 && spyro->main_anim.nextanim != 0x21)
			barrelRollTime = 0;

		if (barrelRollTime) {
			if (barrelRollDirection == 0) spyroExt->xAngle = (barrelRollTime * 0x1000 / 40) & 0x0FFF;
			else if (barrelRollDirection == 1) spyroExt->xAngle = -(barrelRollTime * 0x1000 / 40) & 0x0FFF;
			spyro->z += (int) (sin((float) barrelRollTime / 40.0f * 6.28f) * 75.0f);
			spyro->main_angle.x = spyroExt->xAngle * 255 / 0x1000;

			barrelRollTime++;
			if (barrelRollTime >= 40)
				barrelRollTime = 0;
		}
	}

	if (powers & PWR_SANICROLLS) {
		if (spyro->main_anim.nextanim == 0x16 || spyro->main_anim.nextanim == 0x06) {
			spyroExt->yAngle -= 400;
			sonicSpinningLastFrame = true;
		} else if (sonicSpinningLastFrame) {
			sonicSpinningLastFrame = false;
			spyroExt->yAngle = 0;
		}
	}

	if ((powers & PWR_GIRAFFE) && gameState == GAMESTATE_INLEVEL && mobyModels && mobyModels[0].address) {
		SpyroModelHeader* spyroModel = (SpyroModelHeader*) mobyModels[0];

		if (spyroModel->anims[spyro->main_anim.nextanim].address) {
			for (int i = 0; i < spyroModel->anims[spyro->main_anim.nextanim]->numFrames; i++) {
				SpyroFrameInfo* frame = &spyroModel->anims[spyro->main_anim.nextanim]->frames[i];

				frame->headPos = (frame->headPos & ~0x00000FFF) | 0x480;
			}
		}
	}

	if ((powers & PWR_LUCIO) && gameState == GAMESTATE_INLEVEL && mobyModels && mobyModels[0].address) {
		SpyroModelHeader* spyroModel = (SpyroModelHeader*) mobyModels[0];
		static float boop = 0.0f;
		static DWORD startTime = GetTickCount();

		boop = ((GetTickCount() - startTime) % 1000) / 1000.0f * 6.28f;

		if (spyroModel->anims[spyro->main_anim.nextanim].address) {
			for (int i = 0; i < spyroModel->anims[spyro->main_anim.nextanim]->numFrames; i++) {
				SpyroFrameInfo* frame = &spyroModel->anims[spyro->main_anim.nextanim]->frames[i];

				frame->headPos = bitsout((int) (sin(boop) * 100.0f) + 120, 21, 11) | 
								 bitsout((int) (cos(boop) * 100.0f), 10, 11) | 
								 bitsout((int) (-(1.0f + cos(boop*2.0f)) * 25.0f) + 8, 0, 10);
			}
		}
	}
}

#define BACKUPLENGTH 600

Spyro spyroTrail[BACKUPLENGTH];

// Note: Make sure drawPlayerAddress won't be less than 0 after a subtraction of sizeof (Spyro), and no more than FFFF after an addition of sizeof (Spyro).
// Draw code: the function that'll draw multiple Spyros
uint32 draw_code[] = {
/*0x27BDFFF0, 
0xAFBF0000, 
0xAFB00004, 
0x3410 | (drawPlayerAddress & 0xFFFF), // this is changed, number of players and stuff goes here
0x3C023C1F, 
0x34420000 | (drawPlayerAddress >> 16) | 0x8000, 
0x00000000, // this to be changed: lui v1
0x00000000, // this to be changed: ori v1,?
0x00000000, // this to be changed sw ?,0(v1)
0x00000000, // this to be changed: sw ?,$?(v1)
0x3C0237FF, 
0x00501025, 
0x00000000, // this to be changed sw ?,4(v1)
0x00000000, // this to be changed: sw $?(v1) 
0x00000000, // this is changed, draw function call goes here
0xAFB00008, 
0x8FB00008, 
0x26100000 | sizeof (Spyro), // this is changed too
(0x34020000 | (drawPlayerAddress & 0xFFFF)) + (sizeof (Spyro) * 10), // this is also changed
0x1602FFF0, 
0x00000000, 
0x00000000,
0x00000000,
0x3C023C1F,
0x34420000, // this needs to be changed: ori v0,v0,$(spyroUpper)
0xAC620000,
0xAC620458,
0x3C0237FF,
0x34420000, // this needs to be changed: ori v0,v0,$(spyroLower)
0xAC620004,
0xAC62045C,
0x00000000, // this needs to be changed: Draw function goes here
0x00000000,
0x8FB00004, 
0x8FBF0000, 
0x03E00008, 
0x27BD0010, */

0x27BDFFF8,
0xAFBF0000,
0xAFB00004,
0x34108400, // ori s0,zero,$[drawPlayerAddress & 0xFFFF]
0x3C023C1F, 
0x3442801F, // ori v0,v0,$[drawPlayerAddress >> 16]
0x3C038004, // lui v1,$(drawHack1 >> 16)
0x34634160, // ori v1,v1,$(drawHack1 & 0xFFFF)
0xAC620000,
0xAC620458,
0x3C0237FF,
0x00501025,
0xAC620004,
0xAC62045C,
0x0C011047, // jal $(drawPlayerFunction)
0x00000000,
0x26100000 | sizeof (Spyro), // addiu s0,s0,$[sizeof (Spyro)]
0x3402FFFF, // ori v0,zero,$&drawPlayers[numExtraPlayers]
0x1602FFF1,
0x3C038004, // lui v1,$(drawHack1 >> 16)
0x34634160, // ori v1,v1,$(drawHack1 & 0xFFFF)
0x3C023C1F,
0x34428006, // ori v0,v0,$[spyro.address >> 16]
0xAC620000,
0xAC620458,
0x3C0237FF,
0x3442DBF8, // ori v0,v0,$[spyro.address & 0xFFFF]
0xAC620004,
0xAC62045C,
0x0C011047, // jal $(drawPlayerFunction)
0x00000000,
0x8FB00004,
0x8FBF0000,
0x03E00008,
0x27BD0008,
};

void MultiplayerLoop() {
	if (!spyroDrawFuncAddress)
		return; // not compatible with this version =(
	if (gameState != GAMESTATE_INLEVEL && gameState != GAMESTATE_TALKING && gameState != GAMESTATE_POSTLOADINGLEVEL)
		return;

	uint32* uintmem = (uint32*) memory;
	uint8* bytemem = (uint8*) memory;

	static Savestate argh;
	static bool gotArgh = false;

	if (keyPressed['I'] && keyDown[VK_CONTROL] && !recording) {
		recording = true;
		numRecordedFrames = 0;
	}

	if (keyPressed['O'] && keyDown[VK_CONTROL])
		recording = playing = false;

	if (keyPressed['P'] && keyDown[VK_CONTROL] && !playing) {
		playing = true;
		playingFrame = 0;
	}

	// Exorcist mode. (PAL ONLY)
	if ((powers & PWR_EXORCIST) && gameRegion == PAL) {
		uintmem[0x00016CFC / 4] = 0x00000000;
		uintmem[0x00016D1C / 4] = 0x00000000;
		uintmem[0x00016E30 / 4] = 0x00000000;
		uintmem[0x00016E48 / 4] = 0x00000000;
	}

	// MULTIPLAYER CODE
	// Store Spyro's movement data
	for (int i = BACKUPLENGTH - 1; i > 0; i --)
		spyroTrail[i] = spyroTrail[i - 1];
	spyroTrail[0] = *spyro;

	// Draw the local player
	Spyro* drawSpyros = (Spyro*) &uintmem[drawPlayerAddress / 4];
	int curOtherSpyro = 0;

	// Draw the other online players.
	for (int i = 0; i < player_count; i ++) {
		Spyro* curDrawSpyro = &drawSpyros[curOtherSpyro];
		*curDrawSpyro = players[i].spyro;

		// Check player data--it must be ensured that they don't use invalid or unavailable animations
		if (spyroModelPointer) {
			uint32 spyroModel = STRIPADDR(uintmem[spyroModelPointer / 4]);
			uint8* animList[] = {&curDrawSpyro->main_anim.nextanim, &curDrawSpyro->main_anim.prevanim, &curDrawSpyro->head_anim.nextanim, &curDrawSpyro->head_anim.prevanim};
			uint8* frameList[] = {&curDrawSpyro->main_anim.nextframe, &curDrawSpyro->main_anim.prevframe, &curDrawSpyro->head_anim.nextframe, &curDrawSpyro->head_anim.prevframe};
			
			for (int i = 0; i < sizeof (animList) / sizeof (animList[0]); i++) {
				if (!uintmem[spyroModel / 4 + *animList[i]])
					*animList[i] = 0; // set anim to 0 which is most likely to actually exist
			}

			for (int i = 0; i < sizeof (frameList) / sizeof (frameList[0]); i++) {
				if (uintmem[spyroModel / 4 + *animList[i]]) {
					uint32 animHeader = uintmem[STRIPADDR(uintmem[spyroModel / 4 + *animList[i]]) / 4];

					if (*frameList[i] >= (animHeader & 0xFF))
						*frameList[i] = (animHeader & 0xFF) - 1;
				}
			}
		}

		if (i != playerId)
			curOtherSpyro++;
	}

	int numExtraSpyros = player_count - 1;
	
	// Draw trail
	if (powers & PWR_TRAIL) {
		for (int i = numExtraSpyros; i <= 5; i ++) {
			drawSpyros[i] = spyroTrail[(i * 10)];
			numExtraSpyros++;
		}
	}

	// Update and draw recordings
	if (recording) {
		recordedFrames[numRecordedFrames].powers = powers;
		recordedFrames[numRecordedFrames].spyro = *spyro;

		numRecordedFrames ++;

		if (numRecordedFrames >= 30 * 60)
			recording = false;
	}

	if (playing) {
		drawSpyros[numExtraSpyros++] = recordedFrames[playingFrame++].spyro;

		if (playingFrame >= numRecordedFrames)
			playing = false;
	}

	if (numExtraSpyros > 0 && spyroDrawFuncAddress) {
		// Extend/partly disable Spyro draw distance limit (NOTE: PAL only address!)
		if (gameVersion == PAL && game == SPYRO3) {
			uintmem[0x0003f334 / 4] = 0x00000000;
			uintmem[0x0003f344 / 4] = 0x00000000;
			uintmem[0x0003f360 / 4] = 0x00000000;
			uintmem[0x0003f368 / 4] = 0x00000000;
		}

		// Set draw code hook
		uint32 drawCall = 0x0C000000 | ((spyroDrawFuncAddress & 0x003FFFFF) / 4);
		for (int i = 0; i < 0x000C0000 / 4; i++) {
			if (uintmem[i] == drawCall)
				uintmem[i] = 0x0C000000 | (drawPlayerFunctionAddress / 4);
		}

		// Copy the player draw code over
		memcpy(&uintmem[drawPlayerFunctionAddress / 4], draw_code, sizeof (draw_code));

		// Update dynamic draw code for reasons
		bool do_reset = true;
		uint32* funcmem = &uintmem[(drawPlayerFunctionAddress / 4)];

		funcmem[3] = 0x34100000 | (drawPlayerAddress & 0x0000FFFF);
		
		funcmem[5] = 0x34420000 | (drawPlayerAddress >> 16);
		funcmem[6] = 0x3C030000 | (spyroDrawFuncAddressHack1 >> 16);
		funcmem[7] = 0x34630000 | (spyroDrawFuncAddressHack1 & 0xFFFF);

		if (spyroDrawFuncAddressHack1)
			funcmem[8] = 0xAC620000;
		if (spyroDrawFuncAddressHack2)
			funcmem[9] = 0xAC620000 | ((spyroDrawFuncAddressHack2 - spyroDrawFuncAddressHack1) & 0xFFFF);
		
		if (spyroDrawFuncAddressHack1)
			funcmem[12] = 0xAC620004;
		if (spyroDrawFuncAddressHack2)
			funcmem[13] = 0xAC620000 | (((spyroDrawFuncAddressHack2 - spyroDrawFuncAddressHack1) + 4) & 0xFFFF);

		funcmem[14] = drawCall;
		funcmem[17] = 0x34020000 | ((drawPlayerAddress & 0xFFFF) + (numExtraSpyros * sizeof (Spyro)));

		funcmem[19] = funcmem[6];
		funcmem[20] = funcmem[7];

		funcmem[22] = 0x34428000 | ((uint32) ((uint8*)spyro - umem8) >> 16);
		funcmem[26] = 0x34420000 | ((uint32) ((uint8*)spyro - umem8) & 0xFFFF);

		funcmem[29] = drawCall;

		// Reset the recompiler (currently done every frame because I don't remember)
		if (do_reset && Reset_Recompiler)
			Reset_Recompiler();
	} else if (!numExtraSpyros && spyroDrawFuncAddressHack1 && 
			uintmem[(spyroDrawFuncAddressHack1 & 0x003FFFFF) / 4 + 1] != (0x27FF0000 | ((uint32) ((uint8*)spyro - umem8) & 0xFFFF)) || 1) {
		uint32 hackAddrs[2] = {spyroDrawFuncAddressHack1 & 0x003FFFFF, spyroDrawFuncAddressHack2 & 0x003FFFFF};
		uint32 spyroAddress = (uint32)((uint8*)spyro - umem8) | 0x80000000;

		for (int i = 0; i < 2; i++) {
			if (hackAddrs[i]) {
				if (uintmem[hackAddrs[i] / 4 + 1] != (0x27FF0000 | (spyroAddress & 0xFFFF))) {
					uintmem[hackAddrs[i] / 4] = 0x3C1F0000 | ((spyroAddress & 0xFFFF) >= 0x8000 ? (spyroAddress >> 16) + 1 : (spyroAddress >> 16));
					uintmem[hackAddrs[i] / 4 + 1] = 0x27FF0000 | (spyroAddress & 0xFFFF);
				}
			}
		}

		uint32 hackDrawCall = 0x0C000000 | (drawPlayerFunctionAddress / 4);
		uint32 revertDrawCall = 0x0C000000 | ((spyroDrawFuncAddress & 0x003FFFFF) / 4);
		for (int i = 0; i < 0x000C0000 / 4; i++) {
			if (uintmem[i] == hackDrawCall)
				uintmem[i] = revertDrawCall;
		}

		Reset_Recompiler();
	}
}

void ColorlessMode()
{
	if (!textures&&!hqTextures)
		return;

	UpdatePaletteList();
	GetSnapshot(&vramSs);

	uint16* vram16 = (uint16*) vramSs.vram;
	for (int i = 0; i < numPalettes; i ++)
	{
		if (paletteTypes[i] == PT_HQ)
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 256; j ++)
			{
				int lum = 0;
				if ((palette[j] & 31) > lum) lum = (palette[j] & 31);
				if ((palette[j] >> 5 & 31) > lum) lum = (palette[j] >> 5 & 31);
				if ((palette[j] >> 10 & 31) > lum) lum = (palette[j] >> 10 & 31);
				palette[j] = lum | (lum << 5) | (lum << 10) | 0x8000;
			}
		}
		else
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 16; j ++)
			{
				uint16* clr = &palette[j];
				int lum = 0;
				if ((*clr & 31) > lum) lum = (*clr & 31);
				if ((*clr >> 5 & 31) > lum) lum = (*clr >> 5 & 31);
				if ((*clr >> 10 & 31) > lum) lum = (*clr >> 10 & 31);
				*clr = lum | (lum << 5) | (lum << 10) | 0x8000;
			}
		}
	}
	SetSnapshot(&vramSs);
	CompleteLQPalettes();
}

void PinkMode()
{
	if (!textures&&!hqTextures)
		return;
	
	UpdatePaletteList();
	GetSnapshot(&vramSs);

	uint16* vram16 = (uint16*) vramSs.vram;
	for (int i = 0; i < numPalettes; i ++)
	{
		if (paletteTypes[i] == PT_HQ)
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 256; j ++)
			{
				uint16* clr = &palette[j];
				int lum = 0;
				if ((*clr & 31) > lum) lum = (*clr & 31);
				if ((*clr >> 5 & 31) > lum) lum = (*clr >> 5 & 31);
				if ((*clr >> 10 & 31) > lum) lum = (*clr >> 10 & 31);
				lum = lum * 255 / 31;

				*clr = MAKECOLOR16(255, 150 + (lum * (255 - 160 - 30) / 255), 210 + (lum * (255 - 230 - 10) / 255));
			}
		}
		else
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 16; j ++)
			{
				uint16* clr = &palette[j];
					
				int lum = 0;
				if ((*clr & 31) > lum) lum = (*clr & 31);
				if ((*clr >> 5 & 31) > lum) lum = (*clr >> 5 & 31);
				if ((*clr >> 10 & 31) > lum) lum = (*clr >> 10 & 31);
				lum = lum * 255 / 31;

				*clr = MAKECOLOR16(255, 150 + (lum * (255 - 160 - 30) / 255), 210 + (lum * (255 - 230 - 10) / 255));
			}
		}
	}

	SetSnapshot(&vramSs);
	CompleteLQPalettes();
}

void IndieMode()
{
	if (!sceneData||(!textures&&!hqTextures))
		return;

	UpdatePaletteList();
	GetSnapshot(&vramSs);

	uint16* vram16 = (uint16*) vramSs.vram;
	for (int i = 0; i < numPalettes; i ++)
	{
		if (paletteTypes[i] == PT_HQ)
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 256; j ++)
				palette[j] = 0xFFFF;
		}
		else
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 16; j ++)
				palette[j] = 0xFFFF;
		}
	}
	SetSnapshot(&vramSs);
	CompleteLQPalettes();

	uint8* bytemem = (uint8*) memory;
	for (int i = 0; i < sceneData->numSectors; i ++) {
		SceneSectorHeader* sector = sceneData->sectors[i];
		int lpColourStart = sector->numLpVertices;
		int lpFaceStart = sector->numLpVertices + sector->numLpColours;
		int hpVertexStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2;
		int hpColourStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices;
		int hpFaceStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices + sector->numHpColours * 2;

		// Set light
		for (int j = 0; j < sector->numHpColours; j ++)
			sector->data32[hpColourStart + j] = (((sector->data32[hpColourStart + j + sector->numHpColours])&0xFF)/2) | 
			((((sector->data32[hpColourStart + j + sector->numHpColours])>>8&0xFF)/2)<<8) | 
			((((sector->data32[hpColourStart + j + sector->numHpColours])>>16&0xFF)/2) << 16);
	}
}

void CreepypastaMode()
{
	if (!textures)
		return;
	
	UpdatePaletteList();
	GetSnapshot(&vramSs);

	uint16* vram16 = (uint16*) vramSs.vram;
	for (int i = 0; i < numPalettes; i ++)
	{
		if (paletteTypes[i] == PT_HQ)
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 256; j ++)
			{
				uint16* clr = &palette[j];

				*clr = (31 - (*clr & 31)) | ((31 - (*clr >> 5 & 31)) << 5) | ((31 - (*clr >> 10 & 31)) << 10) | 0x8000;
			}
		}
		else
		{
			uint16* palette = &vram16[palettes[i] / 2];
			for (int j = 0; j < 16; j ++)
			{
				uint16* clr = &palette[j];
					
				*clr = (31 - (*clr & 31)) | ((31 - (*clr >> 5 & 31)) << 5) | ((31 - (*clr >> 10 & 31)) << 10) | 0x8000;
			}
		}
	}

	SetSnapshot(&vramSs);
	CompleteLQPalettes();
}

void SaveSky()
{
	if (!skyData)
		return;
	if (skyData->numSectors >= 0x100 || skyData->size >= 0x20000)
		return; // Something went wrong

	char filename[MAX_PATH];
	DWORD nil;

	GetLevelFilename(filename, SEF_SKY, true);
	
	HANDLE skyOut = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (skyOut == INVALID_HANDLE_VALUE)
		return;

	uint32 startAddr = (uintptr)skyData - (uintptr)memory;
	WriteFile(skyOut, skyData, sizeof (SkyDef) - 4, &nil, NULL);


	for (int i = 0; i < skyData->numSectors; i ++)
	{
		uint32 offset = (skyData->data[i] & 0x003FFFFF) - startAddr;
		WriteFile(skyOut, &offset, 4, &nil, NULL);
	}

	WriteFile(skyOut, &skyData->data[skyData->numSectors], skyData->size - sizeof (SkyDef) - 4 - skyData->numSectors * 4 + 0x0C, &nil, NULL);

	CloseHandle(skyOut);
}

void LoadSky(const char* useFilename) {
	if (!skyData)
		return;

	if (!useFilename) {
		char filename[MAX_PATH];

		GetLevelFilename(filename, SEF_SKY);
		useFilename = filename;
	}

	HANDLE skyIn = CreateFile(useFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SkyDef tempSkyData;

	if (skyIn == INVALID_HANDLE_VALUE)
		return;

	DWORD high, size = GetFileSize(skyIn, &high);
	if (size > skyData->size + 4) {
		if (MessageBox(hwndEditor, "Warning: This sky is bigger than the original sky. This may cause errors or crashes. Continue?", 
						"The sky's the limit, unfortunately", MB_YESNO) == IDNO)
			return;
	}

	uint32 startAddr = ((uintptr)skyData - (uintptr)memory) | 0x80000000;
	DWORD nil;

	ReadFile(skyIn, &tempSkyData, sizeof (SkyDef) - 4, &nil, NULL);
	
	if (tempSkyData.numSectors >= 0x100 || tempSkyData.size >= 0x20000) {
		CloseHandle(skyIn);
		return; // Something went wrong
	}

	uint32 originalSize = skyData->size; // Preserve the original size so we can better keep track of memory constraints
	*skyData = tempSkyData;
	skyData->size = originalSize;

	uint8* bytemem = (uint8*) memory;
	for (int i = 0; i < skyData->numSectors; i++) {
		uint32 offset;
		ReadFile(skyIn, &offset, 4, &nil, NULL);

		uint32 address = offset + startAddr;
		skyData->data[i] = address;

		/*if (game == SPYRO1)
		{
			SkySectorHeaderS1* sector = (SkySectorHeaderS1*) &bytemem[address & 0x003FFFFF];
			SetFilePointer(skyIn, sizeof (SkyDef) - 4 + offset, NULL, FILE_BEGIN);
			
			ReadFile(skyIn, sector, sizeof (SkySectorHeaderS1), &nil, NULL);
		}*/
	}

	//if (game != SPYRO1)
		ReadFile(skyIn, &skyData->data[skyData->numSectors], skyData->size - sizeof (SkyDef) - 4 - skyData->numSectors * 4 + 0x0C, &nil, NULL);

	// Don't forget to update the game's other sky variables
	*skyNumSectors = skyData->numSectors;
	*skyBackColour = skyData->backColour;

	CloseHandle(skyIn);
}

void SaveColours()
{
	if (!sceneData)
		return;

	char filename[MAX_PATH];
	DWORD nil;

	GetLevelFilename(filename, SEF_COLOURS, true);

	HANDLE clrOut = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (clrOut == INVALID_HANDLE_VALUE)
		return;

	WriteFile(clrOut, &sceneData->numSectors, 4, &nil, NULL);

	uint8* bytemem = (uint8*) memory;
	for (int i = 0; i < sceneData->numSectors; i ++)
	{
		SceneSectorHeader* sector = sceneData->sectors[i];
		int lpColourStart = sector->numLpVertices;
		int hpColourStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices;

		WriteFile(clrOut, &sector->numLpColours, 1, &nil, NULL);
		WriteFile(clrOut, &sector->numHpColours, 1, &nil, NULL);

		for (int i = 0; i < sector->numLpColours; i ++)
			WriteFile(clrOut, &sector->data32[lpColourStart + i], 4, &nil, NULL);

		for (int i = 0; i < sector->numHpColours * 2; i ++)
			WriteFile(clrOut, &sector->data32[hpColourStart + i], 4, &nil, NULL);
	}

	CloseHandle(clrOut);
}

void LoadColours()
{
	if (!sceneData)
		return;

	char filename[MAX_PATH];
	DWORD nil;

	GetLevelFilename(filename, SEF_COLOURS);

	HANDLE clrIn = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (clrIn == INVALID_HANDLE_VALUE)
		return;

	uint32 numSectors = 0;
	ReadFile(clrIn, &numSectors, 4, &nil, NULL);

	if (numSectors != sceneData->numSectors)
		return;

	uint8* bytemem = (uint8*) memory;
	for (int i = 0; i < sceneData->numSectors; i ++) {
		SceneSectorHeader* sector = sceneData->sectors[i];
		int lpColourStart = sector->numLpVertices;
		int hpColourStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices;
		int numLpColours = 0, numHpColours = 0;

		ReadFile(clrIn, &numLpColours, 1, &nil, NULL);
		ReadFile(clrIn, &numHpColours, 1, &nil, NULL);

		if (numLpColours != sector->numLpColours || numHpColours != sector->numHpColours)
			return;

		for (int i = 0; i < numLpColours; i ++)
			ReadFile(clrIn, &sector->data32[lpColourStart + i], 4, &nil, NULL);

		for (int i = 0; i < numHpColours * 2; i ++)
			ReadFile(clrIn, &sector->data32[hpColourStart + i], 4, &nil, NULL);
	}

	CloseHandle(clrIn);
}

void SetLevelColours(LevelColours* clrsIn) {
	if (!sceneData)
		return;

	GetAvgTexColours();

	uint8* bytemem = (uint8*) memory;
	int fogR = clrsIn->fogR, fogG = clrsIn->fogG, fogB = clrsIn->fogB;
	int extraLightR = clrsIn->lightR, extraLightG = clrsIn->lightG, extraLightB = clrsIn->lightB;

	// Refresh sector lighting
	for (int i = 0; i < sceneData->numSectors; i++) {
		SceneSectorHeader* sector = sceneData->sectors[i];
		int lpColourStart = sector->numLpVertices;
		int lpFaceStart = sector->numLpVertices + sector->numLpColours;
		int hpVertexStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2;
		int hpColourStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices;
		int hpFaceStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices + sector->numHpColours * 2;

		// Set light tweaks
		for (int j = 0; j < sector->numHpColours; j++) {
			int r = backupColoursHp[i][j] & 0xFF, g = backupColoursHp[i][j] >> 8 & 0xFF, b = backupColoursHp[i][j] >> 16 & 0xFF;
			
			r = r * extraLightR / 100; g = g * extraLightG / 100; b = b * extraLightB / 100;
			if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
			if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;

			sector->data32[hpColourStart + j] = r | (g << 8) | (b << 16);
		}

		// Set fade-out colours based on average texture colour and fog.
		for (int j = 0; j < sector->numHpFaces; j++) {
			uint32 faceVerts = sector->data32[hpFaceStart + j * 4], faceVertClrs = sector->data32[hpFaceStart + j * 4 + 1];
			uint8 verts[4] = {faceVerts >> 24 & 0xFF, faceVerts >> 16 & 0xFF, faceVerts >> 8 & 0xFF, faceVerts & 0xFF};
			uint8 vertClrs[8] = {faceVertClrs >> 24 & 0xFF, faceVertClrs >> 16 & 0xFF, faceVertClrs >> 8 & 0xFF, faceVertClrs & 0xFF};
			int numSides = verts[2] == verts[3] ? 3 : 4;
			uint32 texId = sector->data32[hpFaceStart + j * 4 + 3] & 0x7F;
			
			if (game == SPYRO1)
				texId = sector->data32[hpFaceStart + j * 4 + 2] & 0x7F;

			for (int k = 0; k < numSides; k++) {
				uint32 lightClr = sector->data32[hpColourStart + vertClrs[k]];
				int curTexSide = k;
				int lightR = lightClr & 0xFF, lightG = lightClr >> 8 & 0xFF, lightB = lightClr >> 16 & 0xFF;
				int texR = avgTexColours[texId][curTexSide][0], texG = avgTexColours[texId][curTexSide][1], texB = avgTexColours[texId][curTexSide][2];
				int outR, outG, outB;

				outR = texR + texR * (lightR - 127) / 127; outG = texG + texG * (lightG - 127) / 127; outB = texB + texB * (lightB - 127) / 127;
				if (outR < 0) outR = 0; if (outG < 0) outG = 0; if (outB < 0) outB = 0;
				if (outR > 255) outR = 255; if (outG > 255) outG = 255; if (outB > 255) outB = 255;

				outR = outR + outR * (fogR - 127) / 127; outG = outG + outG * (fogG - 127) / 127; outB = outB + outB * (fogB - 127) / 127;
				if (outR < 0) outR = 0; if (outG < 0) outG = 0; if (outB < 0) outB = 0;
				if (outR > 255) outR = 255; if (outG > 255) outG = 255; if (outB > 255) outB = 255;

				sector->data32[hpColourStart + sector->numHpColours + vertClrs[k]] = outR | (outG << 8) | (outB << 16);
			}
		}

		// Find matching vertices in LP and HP lists, and use them to generate LP colours
		uint16 newAvgColours[0xFF][3];
		uint16 newAvgColourCount[0xFF];
		memset(newAvgColours, 0, sizeof (newAvgColours));
		memset(newAvgColourCount, 0, sizeof (newAvgColourCount));

		for (int j = 0; j < sector->numLpFaces; j ++) {
			uint32 faceVerts = sector->data32[lpFaceStart + j * 2], faceVertClrs = sector->data32[lpFaceStart + j * 2 + 1];
			uint8 verts[4] = {faceVerts >> 24 & 0x7F, faceVerts >> 17 & 0x7F, faceVerts >> 10 & 0x7F, faceVerts >> 3 & 0x7F};
			uint8 vertClrs[4] = {faceVertClrs >> 25 & 0x7F, faceVertClrs >> 18 & 0x7F, faceVertClrs >> 11 & 0x7F, faceVertClrs >> 4 & 0x7F};
			int numSidesLp = verts[3] == verts[2] ? 3 : 4;
			int faceX = 0, faceY = 0, faceZ = 0; // Average X/Y of this face
			
			if (game == SPYRO1) {
				vertClrs[0] = (faceVertClrs >> 23 & 0x1F8) >> 3; vertClrs[1] = (faceVertClrs >> 17 & 0x1F8) >> 3;
				vertClrs[2] = (faceVertClrs >> 11 & 0x1F8) >> 3; vertClrs[3] = (faceVertClrs >> 5 & 0x1F8) >> 3;
			}

			// Calculate this face avg position
			for (int vert = 0; vert < numSidesLp; vert ++) {
				uint32 curVertex = sector->data32[verts[vert]];
				faceX += ((curVertex >> 19 & 0x1FFC) >> 3);
				faceY += ((curVertex >> 8 & 0x1FFC) >> 3);
				faceZ += ((curVertex << 3 & 0x1FFC) >> 3);
			}

			faceX /= numSidesLp; faceY /= numSidesLp; faceZ /= numSidesLp;

			// For every vertex in this face, find a matching HP face vertex. If there's more than one, the one with a face closest to this face's avg will be picked.
			for (int vert = 0; vert < numSidesLp; vert++) {
				uint32 curVertex = sector->data32[verts[vert]];
				int curX = (curVertex >> 19 & 0x1FFC) >> 3, curY = (curVertex >> 8 & 0x1FFC) >> 3, curZ = (curVertex << 3 & 0x1FFC) >> 3;
				uint32 closestVertexDist = 1000000, closestVertexColour = 0xFFFFFFFF;
				uint32 closestFaceDist = 10000000;

				// Find an HP face that has the closest midpoint to the LP one and matches the current vertex
				for (int k = 0; k < sector->numHpFaces; k++) {
					uint32 faceVerts = sector->data32[hpFaceStart + k * 4], faceVertClrs = sector->data32[hpFaceStart + k * 4 + 1];
					uint8 hpVerts[4] = {faceVerts >> 24 & 0xFF, faceVerts >> 16 & 0xFF, faceVerts >> 8 & 0xFF, faceVerts & 0xFF};
					uint8 hpVertClrs[4] = {faceVertClrs >> 24 & 0xFF, faceVertClrs >> 16 & 0xFF, faceVertClrs >> 8 & 0xFF, faceVertClrs & 0xFF};
					int hpFaceX = 0, hpFaceY = 0, hpFaceZ = 0;
					int numSidesHp = hpVerts[2] == hpVerts[3] ? 3 : 4;
					uint32 curClosestVertexColour = 0, curClosestVertexDist = closestVertexDist+1;

					// Get the face's midpoint while finding the closest vertex to this one
					for (int hSide = 0; hSide < numSidesHp; hSide++) {
						uint32 v = sector->data32[hpVertexStart + hpVerts[hSide]];
						int x = (v >> 19 & 0x1FFC) >> 3, y = (v >> 8 & 0x1FFC) >> 3, z = (v << 3 & 0x1FFC) >> 3;
						int xDiff = x - curX, yDiff = y - curY, zDiff = z - curZ;
						uint32 vertDist = (xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff);
						
						hpFaceX += x; hpFaceY += y; hpFaceZ += z;

						if (vertDist < curClosestVertexDist) {
							curClosestVertexDist = vertDist;
							curClosestVertexColour = hpVertClrs[hSide];
						}
					}

					hpFaceX /= numSidesHp; hpFaceY /= numSidesHp; hpFaceZ /= numSidesHp;
					int xDiff = hpFaceX - faceX, yDiff = hpFaceY - faceY, zDiff = hpFaceZ - faceZ;
					uint32 faceDist = (xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff);

					if ((curClosestVertexDist <= closestVertexDist) && faceDist < closestFaceDist) {
						closestFaceDist = faceDist;
						closestVertexDist = curClosestVertexDist;
						closestVertexColour = curClosestVertexColour;
					}
				}

				// Finally, add the colour to its place on the average colour list!
				if (vertClrs[vert] < sector->numLpColours && sector->numHpColours && closestVertexColour != 0xFFFFFFFF) {
					uint32 clr = sector->data32[hpColourStart + sector->numHpColours + closestVertexColour];
					newAvgColours[vertClrs[vert]][0] += clr & 0xFF;
					newAvgColours[vertClrs[vert]][1] += clr >> 8 & 0xFF;
					newAvgColours[vertClrs[vert]][2] += clr >> 16 & 0xFF;
					newAvgColourCount[vertClrs[vert]] ++;
				}
			}
		}

		// Finally, set the new colours
		for (int j = 0; j < sector->numLpColours; j++) {
			if (newAvgColourCount[j]) {
				int c = newAvgColourCount[j];
				sector->data32[lpColourStart + j] = MAKECOLOR32(newAvgColours[j][0] / c, newAvgColours[j][1] / c, newAvgColours[j][2] / c);
			}
			//else
			//	sector->data32[lpColourStart + j] = MAKECOLOR32(0, 0, 0);
		}
	}
}

void GetLevelColours(LevelColours* clrsOut) {
	if (!sceneData)
		return;

	GetAvgTexColours();

	uint8* bytemem = (uint8*) memory;
	
	int mainAvgFogR = 0, mainAvgFogG = 0, mainAvgFogB = 0;
	int mainAvgFogDiv = 0;
	for (int i = 0; i < sceneData->numSectors; i ++) {
		SceneSectorHeader* sector = sceneData->sectors[i];
		int hpVertexStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2;
		int hpColourStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices;
		int hpFaceStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices + sector->numHpColours * 2;

		int avgFogR = 0, avgFogG = 0, avgFogB = 0;
		int avgFogDiv = 0;
		for (int j = 0; j < sector->numHpFaces; j ++) {
			uint32 faceVerts = sector->data32[hpFaceStart + j * 4], faceVertClrs = sector->data32[hpFaceStart + j * 4 + 1];
			uint8 verts[4] = {faceVerts >> 24 & 0xFF, faceVerts >> 16 & 0xFF, faceVerts >> 8 & 0xFF, faceVerts & 0xFF};
			uint8 vertClrs[8] = {faceVertClrs >> 24 & 0xFF, faceVertClrs >> 16 & 0xFF, faceVertClrs >> 8 & 0xFF, faceVertClrs & 0xFF};
			int numSides = verts[2] == verts[3] ? 3 : 4;
			uint32 texId = sector->data32[hpFaceStart + j * 4 + 3] & 0x7F;
			
			if (game == SPYRO1)
				texId = sector->data32[hpFaceStart + j * 4 + 2] & 0x7F;

			for (int k = 0; k < numSides; k ++) {
				uint32 lightClr = sector->data32[hpColourStart + vertClrs[k]];
				uint32 fadeClr = sector->data32[hpColourStart + sector->numHpColours + vertClrs[k]];
				int lightR = lightClr & 0xFF, lightG = lightClr >> 8 & 0xFF, lightB = lightClr >> 16 & 0xFF;
				int curTexSide = k;
				int texR = avgTexColours[texId][curTexSide][0], texG = avgTexColours[texId][curTexSide][1], texB = avgTexColours[texId][curTexSide][2];
				int outR, outG, outB;
				
				outR = texR + texR * (lightR - 127) / 127; outG = texG + texG * (lightG - 127) / 127; outB = texB + texB * (lightB - 127) / 127;
				if (outR < 0) outR = 0; if (outG < 0) outG = 0; if (outB < 0) outB = 0;
				if (outR > 255) outR = 255; if (outG > 255) outG = 255; if (outB > 255) outB = 255;

				int fogR = 255, fogG = 255, fogB = 255;
				if (outR) fogR = (fadeClr & 0xFF)*127/outR;
				if (outG) fogG = (fadeClr >> 8& 0xFF)*127/outG;
				if (outB) fogB = (fadeClr >> 16& 0xFF)*127/outB;
				avgFogR += fogR; avgFogG += fogG; avgFogB += fogB;
			}
			
			avgFogDiv += numSides;
		}

		if (avgFogDiv) {
			mainAvgFogR += avgFogR / avgFogDiv;
			mainAvgFogG += avgFogG / avgFogDiv;
			mainAvgFogB += avgFogB / avgFogDiv;
			mainAvgFogDiv ++;
		}
	}

	clrsOut->fogR = mainAvgFogR / mainAvgFogDiv; clrsOut->fogG = mainAvgFogG / mainAvgFogDiv; clrsOut->fogB = mainAvgFogB / mainAvgFogDiv;
	clrsOut->lightB = 127; clrsOut->lightG = 127; clrsOut->lightR = 127;
}

void TweakSky(int tweakR, int tweakG, int tweakB) {
	if (!skyData)
		return;

	uint8* bytemem = (uint8*) memory;
	for (int i = 0; i < skyData->numSectors; i++) {
		int numColours = 0, numVertices = 0;
		uint32* data32 = NULL;

		if (game == SPYRO1) {
			SkySectorHeaderS1* sector = (SkySectorHeaderS1*) &bytemem[skyData->data[i] & 0x003FFFFF];
			numColours = sector->numColours; numVertices = sector->numVertices; data32 = sector->data32;
		} else {
			SkySectorHeader* sector = (SkySectorHeader*) &bytemem[skyData->data[i] & 0x003FFFFF];
			numColours = sector->numColours; numVertices = sector->numVertices; data32 = sector->data32;
		}

		for (int j = 0; j < numColours; j++) {
			uint32 clr = data32[numVertices + j];
			int r = clr & 0xFF, g = clr >> 8 & 0xFF, b = clr >> 16 & 0xFF, a = clr >> 24 & 0xFF;

			r = r + r * (tweakR - 100) / 100; g = g + g * (tweakG - 100) / 100; b = b + b * (tweakB - 100) / 100;
			if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
			if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;

			data32[numVertices + j] = r | (g << 8) | (b << 16) | (a << 24);
		}
	}

	// Back colour tweak
	int r = *skyBackColour & 0xFF, g = *skyBackColour >> 8 & 0xFF, b = *skyBackColour >> 16 & 0xFF, a = *skyBackColour >> 24 & 0xFF;

	r = r + r * (tweakR - 100) / 100; g = g + g * (tweakG - 100) / 100; b = b + b * (tweakB - 100) / 100;
	if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
	if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;

	*skyBackColour = r | (g << 8) | (b << 16) | (a << 24);
	skyData->backColour = *skyBackColour;
}

uint32 GetAverageLightColour()
{
	if (!sceneData)
		return 0;

	GetAvgTexColours();

	uint8* bytemem = (uint8*) memory;
	
	int mainAvgClrR = 0, mainAvgClrG = 0, mainAvgClrB = 0;
	int mainAvgClrDiv = 0;
	for (int i = 0; i < sceneData->numSectors; i ++) {
		SceneSectorHeader* sector = sceneData->sectors[i];
		int hpVertexStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2;
		int hpColourStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices;
		int hpFaceStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2 + sector->numHpVertices + sector->numHpColours * 2;

		int avgClrR = 0, avgClrG = 0, avgClrB = 0;
		int avgClrDiv = 0;
		for (int j = 0; j < sector->numHpFaces; j ++)
		{
			uint32 faceVerts = sector->data32[hpFaceStart + j * 4], faceVertClrs = sector->data32[hpFaceStart + j * 4 + 1];
			uint8 verts[4] = {faceVerts >> 24 & 0xFF, faceVerts >> 16 & 0xFF, faceVerts >> 8 & 0xFF, faceVerts & 0xFF};
			uint8 vertClrs[8] = {faceVertClrs >> 24 & 0xFF, faceVertClrs >> 16 & 0xFF, faceVertClrs >> 8 & 0xFF, faceVertClrs & 0xFF};
			int numSides = verts[2] == verts[3] ? 3 : 4;
			uint32 texId = sector->data32[hpFaceStart + j * 4 + 3] & 0x7F;

			for (int k = 0; k < numSides; k ++)
			{
				uint32 lightClr = sector->data32[hpColourStart + vertClrs[k]];
				uint32 fadeClr = sector->data32[hpColourStart + sector->numHpColours + vertClrs[k]];
				int lightR = lightClr & 0xFF, lightG = lightClr >> 8 & 0xFF, lightB = lightClr >> 16 & 0xFF;
				int curTexSide = k;
				int texR = avgTexColours[texId][curTexSide][0], texG = avgTexColours[texId][curTexSide][1], texB = avgTexColours[texId][curTexSide][2];
				int outR, outG, outB;
				
				outR = texR + texR * (lightR - 127) / 127; outG = texG + texG * (lightG - 127) / 127; outB = texB + texB * (lightB - 127) / 127;
				if (outR < 0) outR = 0; if (outG < 0) outG = 0; if (outB < 0) outB = 0;
				if (outR > 255) outR = 255; if (outG > 255) outG = 255; if (outB > 255) outB = 255;

				avgClrR += outR; avgClrG += outG; avgClrB += outB;
			}
			
			avgClrDiv += numSides;
		}

		if (avgClrDiv)
		{
			mainAvgClrR += avgClrR / avgClrDiv;
			mainAvgClrG += avgClrG / avgClrDiv;
			mainAvgClrB += avgClrB / avgClrDiv;
			mainAvgClrDiv ++;
		}
	}

	if (mainAvgClrDiv)
	{
		mainAvgClrR /= mainAvgClrDiv;
		mainAvgClrG /= mainAvgClrDiv;
		mainAvgClrB /= mainAvgClrDiv;
	}

	return mainAvgClrR | (mainAvgClrG << 8) | (mainAvgClrB << 16);
}

uint32 GetAverageSkyColour()
{
	if (!skyData)
		return 0;

	uint8* bytemem = (uint8*) memory;
	int mainAvgClrR = 0, mainAvgClrG = 0, mainAvgClrB = 0;
	int mainAvgClrDiv = 0;
	for (int i = 0; i < skyData->numSectors; i ++)
	{
		int numColours = 0, numVertices = 0;
		int avgClrR = 0, avgClrG = 0, avgClrB = 0;
		uint32* data32 = NULL;

		if (game == SPYRO1)
		{
			SkySectorHeaderS1* sector = (SkySectorHeaderS1*) &bytemem[skyData->data[i] & 0x003FFFFF];
			numColours = sector->numColours; numVertices = sector->numVertices; data32 = sector->data32;
		}
		else
		{
			SkySectorHeader* sector = (SkySectorHeader*) &bytemem[skyData->data[i] & 0x003FFFFF];
			numColours = sector->numColours; numVertices = sector->numVertices; data32 = sector->data32;
		}

		for (int j = 0; j < numColours; j ++)
		{
			uint32 clr = data32[numVertices + j];
			int r = clr & 0xFF, g = clr >> 8 & 0xFF, b = clr >> 16 & 0xFF, a = clr >> 24 & 0xFF;

			avgClrR += r; avgClrG += g; avgClrB += b;
		}

		if (numColours)
		{
			mainAvgClrR += avgClrR / numColours;
			mainAvgClrG += avgClrG / numColours;
			mainAvgClrB += avgClrB / numColours;
			mainAvgClrDiv ++;
		}
	}

	if (mainAvgClrDiv)
	{
		mainAvgClrR /= mainAvgClrDiv;
		mainAvgClrG /= mainAvgClrDiv;
		mainAvgClrB /= mainAvgClrDiv;
	}

	return mainAvgClrR | (mainAvgClrG << 8) | (mainAvgClrB << 16);
}

struct ColourBox
{
	int minR, minG, minB;
	int maxR, maxG, maxB;
};

void MakePaletteFromColours(uint32* paletteOut, int desiredPaletteSize, uint32* colours, int numColours) {
	ColourBox boxes[256];
	uint8* clrR = &((uint8*)colours)[0], *clrG = &((uint8*)colours)[1], *clrB = &((uint8*)colours)[2];
	int numBoxes = 1;
	int quadNumColours = numColours * 4;
	uint32* coloursArrangedByR = (uint32*) malloc(numColours * 4), *coloursArrangedByG = (uint32*) malloc(numColours * 4), *coloursArrangedByB = (uint32*) malloc(numColours * 4);
	bool hasBlack = false;

	// Setup initial box
	int startMinR = 255, startMinG = 255, startMinB = 255, startMaxR = 0, startMaxG = 0, startMaxB = 0;
	for (int i = 0; i < quadNumColours; i += 4) {
		if (clrR[i] < startMinR) startMinR = clrR[i]; if (clrR[i] > startMaxR) startMaxR = clrR[i];
		if (clrG[i] < startMinG) startMinG = clrG[i]; if (clrG[i] > startMaxG) startMaxG = clrG[i];
		if (clrB[i] < startMinB) startMinB = clrB[i]; if (clrB[i] > startMaxB) startMaxB = clrB[i];
	}

	boxes[0].minR = startMinR; boxes[0].maxR = startMaxR;
	boxes[0].minG = startMinG; boxes[0].maxG = startMaxG;
	boxes[0].minB = startMinB; boxes[0].maxB = startMaxB;

	if (startMinR == 0 && startMinG == 0 && startMinB == 0)
		hasBlack = true;

	// Arrange colours on all axes
	uint8* axes[] = {clrR, clrG, clrB};
	uint32* arrangedAxes[] = {coloursArrangedByR, coloursArrangedByG, coloursArrangedByB};
	for (int a = 0; a < 3; a++) {
		uint8* axisClrs = axes[a];
		uint32* arrangedAxis = arrangedAxes[a];

		int curArranged = 0;

		for (int j = 0; j < 32; j++) { /* hack: speedup by arranging with 5-bit precision instead of 8-bit */
			for (int k = 0; k < quadNumColours; k += 4) {
				if (axisClrs[k] >> 3 == j)
					arrangedAxis[curArranged++] = colours[k / 4];
			}
		}
	}

	// Make boxes
	uint8* arrangedColours = (uint8*)malloc(numColours);
	while (numBoxes < desiredPaletteSize) {
		for (int i = 0; i < numBoxes; i ++) {
			ColourBox* curBox = &boxes[i];
			ColourBox* nextBox = &boxes[numBoxes + i];

			// Find the longest axis
			int axisId = 0;
			if (curBox->maxG - curBox->minG > curBox->maxR - curBox->minR) axisId = 1;
			if (curBox->maxB - curBox->minB > curBox->maxG - curBox->minG) axisId = 2;
			
			// Setup axis variables (arrAxis = the colour channel of the longest axis for this box, from the colours arranged by the longest axis for this box)
			uint32* arrangedAxes[] = {coloursArrangedByR, coloursArrangedByG, coloursArrangedByB};
			uint32* arrangedAxis = arrangedAxes[axisId];
			uint8* arrAxis = &((uint8*) arrangedAxis)[axisId];
			uint8* arrR = &((uint8*) arrangedAxis)[0], *arrG = &((uint8*) arrangedAxis)[1], *arrB = &((uint8*) arrangedAxis)[2];

			uint8 minR = curBox->minR, maxR = curBox->maxR, minG = curBox->minG, maxG = curBox->maxG, minB = curBox->minB, maxB = curBox->maxB;

			// Get colours arranged along the longest axis
			int numArrangedColours = 0;
			
			for (int k = 0; k < quadNumColours; k += 4) {
				if (arrR[k] < minR || arrR[k] > maxR || arrG[k] < minG || arrG[k] > maxG || arrB[k] < minB || arrB[k] > maxB) // Colour is outside of box!
					continue;

				arrangedColours[numArrangedColours ++] = arrAxis[k];
			}

			// Get the median of the colours along this axis
			int median = 0;

			if (numArrangedColours > 0) {
				if (!(numArrangedColours & 1))
					median = (arrangedColours[numArrangedColours / 2 - 1] + arrangedColours[numArrangedColours / 2]) / 2;
				else
					median = arrangedColours[numArrangedColours / 2];
			}

			// Split this box into two by the median. nextBox will be on the greater side of the median; curBox on the smaller side			
			*nextBox = *curBox;
			switch (axisId) {
				case 0:
					curBox->maxR = median;
					nextBox->minR = median;
					break;
				case 1:
					curBox->maxG = median;
					nextBox->minG = median;
					break;
				case 2:
					curBox->maxB = median;
					nextBox->minB = median;
					break;
			}

			// Recalculate the boundaries of both boxes
			ColourBox* temp[2] = {curBox, nextBox};
			for (int j = 0; j < 2; j ++) {
				ColourBox* box = temp[j];
				int minR = 255, minG = 255, minB = 255, maxR = 0, maxG = 0, maxB = 0;

				for (int k = 0; k < quadNumColours; k += 4) {
					if (clrR[k] > box->maxR || clrR[k] < box->minR || clrG[k] > box->maxG || clrG[k] < box->minG || clrB[k] > box->maxB || clrB[k] < box->minB)
						continue;

					if (clrR[k] < minR) minR = clrR[k]; if (clrR[k] > maxR) maxR = clrR[k];
					if (clrG[k] < minG) minG = clrG[k]; if (clrG[k] > maxG) maxG = clrG[k];
					if (clrB[k] < minB) minB = clrB[k]; if (clrB[k] > maxB) maxB = clrB[k];
				}

				box->minR = minR; box->minG = minG; box->minB = minB;
				box->maxR = maxR; box->maxG = maxG; box->maxB = maxB;
			}
		}

		numBoxes *= 2;
	}
	free(arrangedColours);

	// Set the colours into the palette
	for (int i = 0; i < numBoxes; i ++) {
		int minR = boxes[i].minR, minG = boxes[i].minG, minB = boxes[i].minB, maxR = boxes[i].maxR, maxG = boxes[i].maxG, maxB = boxes[i].maxB;
		int avgR = 0, avgG = 0, avgB = 0;
		int numAvg = 0;

		for (int j = 0; j < quadNumColours; j += 4) {
			if (clrR[j] > maxR || clrR[j] < minR || clrG[j] < minG || clrG[j] > maxG || clrB[j] < minB || clrB[j] > maxB)
				continue; // Not in this box

			avgR += clrR[j];
			avgG += clrG[j];
			avgB += clrB[j];
			numAvg ++;
		}

		if (!numAvg) // This should never happen but hey-ho
			continue;
		
		avgR /= numAvg; avgG /= numAvg; avgB /= numAvg;

		// PLOT TWIST: Instead of using the pure average of the colours, find the used colour closest to it
		int bestClr = 0;
		uint32 bestDist = 0xFFFFFFFF;

		for (int j = 0; j < quadNumColours; j += 4) {
			if (clrR[j] > maxR || clrR[j] < minR || clrG[j] < minG || clrG[j] > maxG || clrB[j] < minB || clrB[j] > maxB)
				continue; // Not in this box

			int rDist = clrR[j] - avgR, gDist = clrG[j] - avgG, bDist = clrB[j] - avgB;
			if (rDist < 0) rDist = -rDist; if (gDist < 0) gDist = -gDist; if (bDist < 0) bDist = -bDist;
			int dist = rDist;

			if (gDist > dist) dist = gDist;
			if (bDist > dist) dist = bDist;

			if (dist < bestDist) {
				bestDist = dist;
				bestClr = j;
			}
		}

		paletteOut[i] = clrR[bestClr] | (clrG[bestClr] << 8) | (clrB[bestClr] << 16);
	}

	// Free resources
	free(coloursArrangedByR);
	free(coloursArrangedByG);
	free(coloursArrangedByB);

	// If black bias, add black colour to the palette
	bool addingBlack = false;
	int finalPaletteSize = desiredPaletteSize;
	if (hasBlack) {
		addingBlack = true;
		for (int i = 0; i < numBoxes; i++) {
			if (paletteOut[i] == 0)
				addingBlack = false;
		}

		if (addingBlack)
			finalPaletteSize -= 1;
	}

	// Shrink down the palette if the resulting number of boxes (always a power of 2) is bigger than the number of colours requested
	while (numBoxes > finalPaletteSize) {
		int closestDist = 256 * 256 * 256, combine1 = 0, combine2 = 0;

		for (int i = 0; i < numBoxes; i++) {
			if ((paletteOut[i] & 0x7FFF) == 0)
				continue; // don't touch the transparency colour

			for (int j = i + 1; j < numBoxes; j++) { // only check colours above i so that we don't repeat searches (i.e. with i and j switched)
				if ((paletteOut[j] & 0x7FFF) == 0)
					continue;

				int deltaR = GETR32(paletteOut[i]) - GETR32(paletteOut[j]), deltaG = GETG32(paletteOut[i]) - GETG32(paletteOut[j]),
					deltaB = GETB32(paletteOut[i]) - GETB32(paletteOut[j]);
				int dist = deltaR * deltaR + deltaG * deltaG + deltaB * deltaB;

				if (dist < closestDist) {
					combine1 = i;
					combine2 = j;
					closestDist = dist;
				}
			}
		}

		// Combine the palette
		int incidence1 = 0, incidence2 = 0;

		for (int i = 0; i < numColours; i++) {
			if ((colours[i] & 0x00FFFFFF) == paletteOut[combine1])
				incidence1++;
			if ((colours[i] & 0x00FFFFFF) == paletteOut[combine2])
				incidence2++;
		}

		if (incidence2 > incidence1)
			paletteOut[combine1] = paletteOut[combine2];
		/*paletteOut[combine1] = MAKECOLOR32((GETR32(paletteOut[combine1]) + GETR32(paletteOut[combine2])) / 2, 
			(GETG32(paletteOut[combine1]) + GETG32(paletteOut[combine2])) / 2, 
			(GETB32(paletteOut[combine1]) + GETB32(paletteOut[combine2])) / 2);*/

		// remove combine2 entirely
		numBoxes--;
		for (int i = combine2; i < numBoxes; i++)
			paletteOut[i] = paletteOut[i + 1];
	}

	if (addingBlack && finalPaletteSize > 0)
		paletteOut[finalPaletteSize++] = 0;
}

float ToRad(int8 angle)
{
	uint8 uAngle = *((uint8*) &angle);
	return (float) uAngle / 256.0f * 6.283f;
}

int8 ToAngle(float rad)
{
	uint8 uAngle = (uint8) (rad * 256.0f / 6.283f);
	return *(int8*) &uAngle;
}

void GetLevelFilename(char* filenameOut, SpyroEditFileType fileType, bool createFolders) {
	const char* levelName = "*UnknownLevel*";
	char rootDir[MAX_PATH];

	if (level && levelNames && *level < numLevelNames)
		levelName = (const char*) &umem8[levelNames[*level] & 0x003FFFFF];
	else if (game == SPYRO1 && level && *level >= 0 && *level < 35)
		levelName = spyro1LevelNames[*level];

	// Get root directory
	GetModuleFileName(GetModuleHandle(NULL), rootDir, MAX_PATH);

	int p = 0;
	for (p = 0; rootDir[p]; p++);
	for (; p > 0 && rootDir[p] != '\\' && rootDir[p] != '/'; p--);

	if (p >= 0)
		rootDir[p] = '\0';
	else {
		rootDir[0] = '.';
		rootDir[1] = '\0';
	}

	// Generate file/folder name
	switch (fileType) {
		case SEF_TEXTURES:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Textures.bmp", rootDir, levelName); break;
		case SEF_TEXTUREFOLDERHQ:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Textures HQ\\", rootDir, levelName); break;
		case SEF_TEXTUREFOLDERLQ:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Textures LQ\\", rootDir, levelName); break;
		case SEF_OBJTEXTURES:
			if (levelArea && levelArea > 0) {
				sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Object Textures (Area %i).bmp", rootDir, levelName, *levelArea); break;
			} else {
				sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Object Textures.bmp", rootDir, levelName); break;
			}
		case SEF_COLOURS:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Colours.clr", rootDir, levelName); break;
		case SEF_SKY:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Sky.sky", rootDir, levelName); break;
		case SEF_SCENERY:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Scene.scn", rootDir, levelName); break;
		case SEF_SETTINGS:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Settings.ini", rootDir, levelName); break;
		default:
			sprintf(filenameOut, "%s\\SpyroEdit\\%s\\Programmer's Blunder.derp.thisisanerror", rootDir, levelName); break;
	}

	if (createFolders) {
		char levelFolder[MAX_PATH];
		sprintf(levelFolder, "%s\\SpyroEdit\\%s\\", rootDir, levelName);

		CreateDirectory(".\\SpyroEdit\\", NULL);
		CreateDirectory(levelFolder, NULL);

		if (fileType == SEF_TEXTUREFOLDERHQ || fileType == SEF_TEXTUREFOLDERLQ)
			CreateDirectory(filenameOut, NULL);
	}
}

int SceneSectorHeader::GetSize() const
{
	return (7 + numLpVertices + numLpColours + numLpFaces * 2 + numHpVertices + numHpColours * 2 + numHpFaces * 4) * 4;
}

/*
Replace all JALs to the function to a JAL to this...

addiu sp, sp, $FFF0
sw ra, $0000(sp)
sw s0, $0004(sp)

ori s0, zero, $F200

LoopStart:
lui v1, $0004
lui v0, $3C1F
ori v0, v0, $8020
sw v0, $F004(v1)
sw v0, $F45C(v1)
lui v0, $27FF
or v0, v0, s0
sw v0, $F008(v1)
sw v0, $F460(v1)

jal $8003efc0
sw s0, $0008(sp)

lw s0, $0008(sp)
addiu s0, s0, $0030 // How much to increase the address (may be changed)

ori v0, zero, $F290 // At what address to stop (may also be changed)
bne s0, v0, $00000010
nop

lw s0, $0004(sp)
lw ra, $0000(sp)
jr ra
addiu sp, sp, $0010


*/
