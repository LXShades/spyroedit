#pragma once

#include "Types.h"
#include "Main.h"

#define WORLDSCALE 0.01f

const uint16 BUT_TRIANGLE = 0x1000, BUT_SQUARE = 0x2000, BUT_X = 0x4000, BUT_CIRCLE = 0x8000;
const uint16 BUT_UP = 0x0010, BUT_RIGHT = 0x0020, BUT_DOWN = 0x0040, BUT_LEFT = 0x0080;
const uint16 BUT_L2 = 0x0100, BUT_R2 = 0x0200, BUT_L1 = 0x0400, BUT_R1 = 0x0800;
const uint16 BUT_SELECT = 0x0100, BUT_L3 = 0x0200, BUT_R3 = 0x0400, BUT_START = 0x0008;

// Spyro struct definitions follow!
#pragma pack(push, 1)

// Direct pointer wrapper for PS1 pointers
template <typename PointerType = void>
struct SpyroPointer {
	uint32 address;

	SpyroPointer() = default;
	SpyroPointer(uint32 initialValue) {
		address = initialValue;
	}

	static void* SpyroToLocal(uint32 spyro) {
		return (uint32)umem32 + (spyro & 0x001FFFFF);
	}

	static uint32 LocalToSpyro(void* spyro) {
		return ((uintptr)spyro - (uintptr)umem32) | 0x80000000;
	}

	static SpyroPointer FromLocal(void* address) {
		address = 0x80000000 | ((uintptr)address - (uintptr)umem32);
	}

	bool IsValid() const {
		return (address & ~0x80000000) > 0x00001000 && (address & ~0x80000000) < 0x00200000;
	}

	inline operator PointerType*() {
		if (address & 0x003FFFFF) {
			return (PointerType*) ((uintptr)umem32 + (address & 0x003FFFFF));
		} else {
			return nullptr;
		}
	}

	inline SpyroPointer operator=(PointerType* pointer) {
		address = ((uintptr)pointer - (uintptr)umem32) | 0x80000000;
		return *this;
	}

	inline PointerType* operator->() {
		return (PointerType*) ((uintptr)umem32 + (address & 0x003FFFFF));
	}

	inline PointerType** operator&() {
		return (PointerType**)this;
	}

	template <typename T>
	inline explicit operator T*() {
		if (address & 0x003FFFFF) {
			return (T*) ((uintptr)umem32 + (address & 0x003FFFFF));
		} else {
			return nullptr;
		}
	}
};

struct Angle {
	int8 x, y, z, unknown;
};

struct Animation {
	uint8 prevAnim, nextAnim, prevFrame, nextFrame;
};

struct IntVector {
	int16 x, y, z;
};

struct Spyro {
	int32 x, y, z;

	Angle angle;
	Angle headAngle;

	Animation anim;
	Animation headAnim;

	uint32 animprogress;

	char unknown[0x10];
	char maybemore[0x10];
};

struct SpyroExtended3 {
	int32 x, y, z;

	Angle main_angle;
	Angle head_angle;

	Animation main_anim;
	Animation head_anim;

	uint32 animprogress;

	int8 unknown[0x10];
	int8 maybemore[0x10];
	int8 derp[0x08];

	int32 state;

	uint32 lawl[4];

	int32 xAngle, yAngle, zAngle;

	uint8 lurp[0x12C];

	int32 headXAngle, headYAngle, headZAngle;
};

extern SpyroPointer<struct Moby>* mobyCollisionRegions;

struct Moby { // Moby size: 0x58
	int32 extraData; // 0x00
	SpyroPointer<Moby> nextCollisionChain; // 0x04
	int32 collisionData;

	int32 x, y, z; // 0x0C

	uint32 attackFlags; // 0x18

	int8 _null[24]; // 0x1C

	int16 collisionRegion; // 0x34
	int16 type; // 0x36

	int8 _dur[4]; // 0x38

	Animation anim; //0x3C

	uint8 animProgress;   // 0x40
	int8 _unknown[2]; // 0x41
	int8 animRun;     // 0x43

	Angle angle; // 0x44

	int8 state; // 0x48

	int8 quack[3]; // 0x49

	int8 distDraw; // 0x4C
	int8 distUnknown; // 0x4D
	int8 distHp;   // 0x4E
	int8 scaleDown; // 0x4F (Default: 0x20)

	int8 _unkA; // 0x50
	int8 _unkB; // 0x51
	int8 levelSectorIndex; // 0x52
	int8 _unkC; // 0x53

	uint32 colour; // 0x54

	void SetPosition(int newX, int newY, int newZ) {
		// Update the collision region
		int16 oldCollisionRegion = (x >> 13) + ((y >> 13) << 5);
		int16 newCollisionRegion = (newX >> 13) + ((newY >> 13) << 5);

		if (newCollisionRegion != oldCollisionRegion && collisionRegion << 2 >= 0 && mobyCollisionRegions && state >= 0) {
			// Remove this object from the old collision chain
			SpyroPointer<Moby>* nextItemInChain;
			nextItemInChain = (SpyroPointer<Moby>*)&mobyCollisionRegions[oldCollisionRegion];

			while (nextItemInChain->address && *nextItemInChain != this) {
				nextItemInChain = (SpyroPointer<Moby>*)&(*nextItemInChain)->nextCollisionChain;
			}

			if (nextItemInChain->address) {
				*nextItemInChain = this->nextCollisionChain;

				// Add this object to the new collision chain
				this->nextCollisionChain = mobyCollisionRegions[newCollisionRegion];
				mobyCollisionRegions[newCollisionRegion] = this;
	
				this->collisionRegion = newCollisionRegion;
			}
		}

		if (newX != x || newY != y || newZ != z) {
			// For now, throw the object into 'just render me like one of your French girls' mode, but only if it actually moved
			levelSectorIndex = 0xFF;

			// Update the coordinates
			x = newX;
			y = newY;
			z = newZ;
		}
	}
};

struct SpyroSky {
	uint32 size;
	uint32 backColour;
	uint32 numSectors;

	uint32 data[1];
};

struct SkySectorHeader {
	uint8 unknown00[12];
	uint8 numVertices, numColours;
	uint8 unknown0E[2];
	uint8 unknown10[4];

	uint32 data32[1];
};

struct SkySectorHeaderS1 {
	uint8 unknown00[12];
	uint8 numVertices, unknown0D;
	uint8 unknown0E[2];
	uint16 numTriangles;
	uint16 numColours;
	uint32 zTerminator;

	uint32 data32[1];
};

struct ModelAnimFrameInfo {
	uint16 blockOffset; // offset of block start (in blocks/ *2 bytes)
	uint8 numBlocks; // number of blocks
	uint8 startFlags; // flags
	uint32 shadowStuff; // shadow stuff yay
};

struct ModelAnimHeader {
	// There seems to be more than 1 type of anim header so let's start with this one
	uint8 numFrames, numColours, vertScale, unkc;
	uint8 numVerts, unkd, unke, unkf;
	uint32 unkg;
	SpyroPointer<uint32> verts;
	SpyroPointer<uint32> faces;
	SpyroPointer<uint32> colours;
	SpyroPointer<uint32> lpFaces;
	SpyroPointer<uint32> lpColours;
	SpyroPointer<uint16> data;
	ModelAnimFrameInfo frames[1]; // frames--may be more than 1!
};

struct ModelHeader { // header for animated models (|0x80000000 in the model list)
	uint32 numAnims; // number of animations for this object/model
	uint32 unkFSequence[4]; // unknown sequence of mostly FFs and occasionally a value
	uint32 unkExtra; // sometimes equals an address, don't know what it means
	uint32 unkSeven[7]; // seven more unknowns woot
	uint32 unkSomeOtherPlace; // pointer to something
	SpyroPointer<uint32> faces; // pointer to the default faces of the model, although all animations typically have their own pointer seemingly to the same place
	SpyroPointer<ModelAnimHeader> anims[1]; // expandable array of pointers to each animation (may be more than 1)
};

struct SimpleModelStateHeader {
	uint8 numHpColours, numHpVerts, unk1, unk2;
	uint32 allUnks;
	uint16 hpFaceOff, hpColourOff; // HP offsets relative to the header
	uint16 lpFaceOff, lpColourOff; // LP offsets ditto

	union {
		uint32 data32[1]; // Offset to index: (offset - 16) / 4
		uint16 data16[1]; // Offset to index: (offset - 16) / 2
		uint8 data8[1]; // Offset to index: (offset - 16)
	};
};

struct SimpleModelHeader { // header for non-animated models (|0x80000000 in the model list)
	uint32 numStates; // number of different states/model appearances in this model
	uint32 unkFSequence;
	uint32 unkExtra;
	uint32 unkZero;
	SpyroPointer<uint32> faces; // pointer to the default faces w/ the same formatting as animated models
	SpyroPointer<SimpleModelStateHeader> states[1]; // expandable array of pointers to each model state (may be more than 1)
};

struct SpyroAnimHeader;
struct SpyroFrameInfo;
struct SpyroModelHeader {
	uint32 numAnims;
	uint32 iDontNeedThis[14]; // I don't need this

	SpyroPointer<SpyroAnimHeader> anims[1];
};

struct SpyroFrameInfo {
	uint16 blockOffset, unk1;
	uint32 word1;
	uint32 headPos; // xxxxxxxxxxxyyyyyyyyyyyzzzzzzzzzz: z = up y = left x = forward
	uint32 shadowLeft;
	uint32 shadowRight;
};

struct SpyroAnimHeader {
	uint8 numFrames, numColours, headVertStart, numVerts;
	SpyroPointer<uint32> verts;
	SpyroPointer<uint32> faces;
	SpyroPointer<uint32> colours;
	SpyroPointer<uint16> data;

	SpyroFrameInfo frames[1];
};

struct CollTri {
	uint32 xCoords;
	uint32 yCoords;
	uint32 zCoords;

	inline void SetPoints(int p1X, int p1Y, int p1Z, int p2X, int p2Y, int p2Z, int p3X, int p3Y, int p3Z);
};

class SpyroCollision {
public:
	union {
		void* address;

		struct SpyroCollisionS1 {
			uint32 numTriangles;
			uint32 numUnkn1;
			SpyroPointer<uint16> blockTree;
			SpyroPointer<uint16> blocks;
			SpyroPointer<CollTri> triangleData;
			uint32 etc1;
			uint32 etc2;
		}* s1;

		struct SpyroCollisionS2S3 {
			uint32 numTriangles;
			uint32 numUnkn1;
			uint32 numUnkn2;
			SpyroPointer<uint16> blockTree;
			SpyroPointer<uint16> blocks;
			SpyroPointer<CollTri> triangleData;
			SpyroPointer<uint16> surfaceType;
			uint32 etc1;
			uint32 selfAddressUseless;
		}* s2s3;
	};

	GameName game;

public:
	uint32 numTriangles;
	SpyroPointer<uint16> blockTree;
	SpyroPointer<uint16> blocks;
	SpyroPointer<CollTri> triangles;
	SpyroPointer<uint16> surfaceType;

	bool IsValid() {
		return (address != nullptr);
	}

	void SetAddress(void* address, GameName game) {
		this->address = address;
		this->game = game;

		if (address) {
			if (game == SPYRO1) {
				numTriangles = s1->numTriangles;
				triangles = s1->triangleData;
				blockTree = s1->blockTree;
				blocks = s1->blocks;
			} else {
				numTriangles = s2s3->numTriangles;
				blocks = s2s3->blocks;
				blockTree = s2s3->blockTree;
				triangles = s2s3->triangleData;
				surfaceType = s2s3->surfaceType;
			}
		}
	}

	void Reset() {
		*this = SpyroCollision{0};
	}

	void CopyToGame() {
		if (!address)
			return;

		if (game == SPYRO1) {
			s1->numTriangles = numTriangles;
			s1->blockTree = blockTree;
			s1->blocks = blocks;
		} else {
			s2s3->numTriangles = numTriangles;
			s2s3->blockTree = blockTree;
			s2s3->triangleData = triangles;
			s2s3->surfaceType = surfaceType;
		}
	}
};

struct LevelColours {
	uint8 fogR, fogG, fogB;
	int32 lightR, lightG, lightB;
};

struct CollTriCache {
	int32 triangleIndex; // original index of the triangle in the Spyro 3's triangle list
	uint8 faceIndex; // ID of face connected to this colltri, if applicable
	uint8 vert1Index : 2; // Indices of the face that make this triangle
	uint8 vert2Index : 2;
	uint8 vert3Index : 2;
};

struct CollSectorCache {
	CollTriCache triangles[512];
	uint16 numTriangles;
};

struct CollisionCache {
	CollSectorCache sectorCaches[256];
	CollTriCache unlinkedCaches[50000]; // Collision triangles, ordered from most important to least important (guessed)
	uint32 numUnlinkedTriangles;
};

struct SpyroCamera {
	uint32 x1, y1, z1;
	uint32 x2, y2, z2;

	uint32 unk;
	uint16 unketc, vAngle1;
	uint16 hAngle1, unkderp;
	uint32 mode;
	uint32 unk2[4];

	uint32 x3, y3, z3;
	uint32 unk3;
	uint32 x4, y4, z4;

	uint32 angle;
};

struct VecU16 {
	uint16 x, y, z;
};

struct TriangleU16 {
	union {
		VecU16 points[3];
		struct {
			uint16 x1, y1, z1;
			uint16 x2, y2, z2;
			uint16 x3, y3, z3;
		};
	};
};
#pragma pack(pop)

enum SpyroGameState {
	GAMESTATE_NOTFOUND = -1,
	GAMESTATE_UNKNOWN = 0,
	GAMESTATE_LOADINGLEVEL, // tends to begin as soon as you enter a portal (note: this state covers two in-game values 5 and 7, 5 in a loading screen and 7 in a portal)
	GAMESTATE_POSTLOADINGLEVEL, // between the fade-out and fade-in of level load
	GAMESTATE_LOADINGMINIGAME, // lasts until the end of the fade-in and beginning of player control
	GAMESTATE_INLEVEL,
	GAMESTATE_TALKING,
	GAMESTATE_CUTSCENE, // lasts until end of fadeout
	GAMESTATE_PAUSED
};

enum SpyroEditFileType {
	SEF_TEXTURES,
	SEF_TEXTUREFOLDERLQ,
	SEF_TEXTUREFOLDERHQ,
	SEF_OBJTEXTURES,
	SEF_RENDERTEXTURES,
	SEF_RENDEROBJTEXTURES,
	SEF_SKY,
	SEF_COLOURS,
	SEF_GEOMETRY,
	SEF_MOBYLAYOUT,
	SEF_SETTINGS,
};

extern Spyro* spyro;
extern SpyroExtended3* spyroExt;
extern void *wadstart;

extern uint16 joker;
extern uint16 jokerPressed;
extern uint16* jokerPtr;

extern uint32 usedMem[0x00200000 / 1024 / 32]; // Bit-based array of memory flagged as used. Reset on level load

extern int32* level;
extern uint8* levelArea;
extern SpyroGameState gameState;

extern Moby* mobys;
extern int32* numMobys;

extern SpyroPointer<ModelHeader>* mobyModels;

extern uint32* levelNames;
extern int numLevelNames;

extern uint32 texEditFlags; // of the TextureEditFlags enum

extern SpyroSky* skyData;
extern uint32* skyNumSectors;
extern uint32* skyBackColour;

extern uint8* sceneOcclusion;
extern uint8* skyOcclusion;

extern SpyroCamera* spyroCamera;

extern SpyroCollision spyroCollision;

extern CollisionCache collisionCache;

extern uint32 spyroModelPointer;
extern uint32 spyroDrawFuncAddress;
extern uint32 spyroDrawFuncAddressHack1;
extern uint32 spyroDrawFuncAddressHack2;

extern int palettes[1000]; // Byte positions of palettes
extern int paletteTypes[1000];
extern int numPalettes;

// hacks
extern bool modifyCode; // Whether or not the plugin shall change the game's ASM
extern bool disableSceneOccl, disableSkyOccl; // Self-explanitory

void SpyroLoop(); // main loop
void MultiplayerLoop(); // Spyro draw code overwritten and updated here (only if multiple players or recordings are in-game)

void UpdateSpyroPointers(); // scans for and retrieves game data (pointers, etc). May not necessarily retrieve all data; unavailable data is typically reset to NULL or 0.

void SaveSky(); // saves the sky model
void LoadSky(const char* customFilename = 0); // loads the sky model

void SaveColours(); // saves level colours--this might need changing once level geometry is replaceable
void LoadColours(); // loads level colours--this might need changing once level geometry is replaceable

void SaveMobys(); // saves moby positions (but not ordering, IDs etc--might not be cross-version-compatible)
void LoadMobys();

void SaveAllMods();
void LoadAllMods();

void SetLevelColours(LevelColours* clrsIn); // tweaks level colours based on a colour multiplier
void GetLevelColours(LevelColours* clrsOut); // obtains level colours (does this even work? I forget)

void GetAvgTexColours(); // gets average level colours (incl. textures)

void TweakSky(int r, int g, int b);

uint32 GetAverageLightColour();
uint32 GetAverageSkyColour();

void GetLevelFilename(char* filenameOut, SpyroEditFileType fileType, bool createFolders = false);

void UpdatePaletteList(); // updates the optimised palette list used for many texture mods

void GenerateLQTextures(); // auto-generated LQ textures from HQ textures
void CompleteLQPalettes(); // I forget

void MakePaletteFromColours(uint32* paletteOut, int desiredPaletteSize, uint32* colours, int numColours); // takes a full set of colours and makes a palette of desired size

bool IsModelValid(int modelId, int animIndex = -1);

float ToRad(int8 angle);
int8 ToAngle(float rad);

extern "C" void (__stdcall* Reset_Recompiler)(void);

inline void CollTri::SetPoints(int p1X, int p1Y, int p1Z, int p2X, int p2Y, int p2Z, int p3X, int p3Y, int p3Z) {
	uint32 preservedUnknownBits = zCoords & 0xC000;

	if (p1Z <= p2Z && p1Z <= p3Z && 
		(p2X - p1X) >= -255 && (p2X - p1X) <= 255 && (p2Y - p1Y) >= -255 && (p2Y - p1Y) <= 255 &&
		(p3X - p1X) >= -255 && (p3X - p1X) <= 255 && (p3Y - p1Y) >= -255 && (p3Y - p1Y) <= 255) {

		xCoords = bitsout(p1X, 0, 14) | bitsout(p2X - p1X, 14, 9) | bitsout(p3X - p1X, 23, 9);
		yCoords = bitsout(p1Y, 0, 14) | bitsout(p2Y - p1Y, 14, 9) | bitsout(p3Y - p1Y, 23, 9);
		zCoords = bitsout(p1Z, 0, 14) | bitsout(p2Z - p1Z, 16, 8) | bitsout(p3Z - p1Z, 24, 8);
	} else if (p2Z <= p1Z && p2Z <= p3Z && 
		(p3X - p2X) >= -255 && (p3X - p2X) <= 255 && (p3Y - p2Y) >= -255 && (p3Y - p2Y) <= 255 &&
		(p1X - p2X) >= -255 && (p1X - p2X) <= 255 && (p1Y - p2Y) >= -255 && (p1Y - p2Y) <= 255) {

		xCoords = bitsout(p2X, 0, 14) | bitsout(p3X - p2X, 14, 9) | bitsout(p1X - p2X, 23, 9);
		yCoords = bitsout(p2Y, 0, 14) | bitsout(p3Y - p2Y, 14, 9) | bitsout(p1Y - p2Y, 23, 9);
		zCoords = bitsout(p2Z, 0, 14) | bitsout(p3Z - p2Z, 16, 8) | bitsout(p1Z - p2Z, 24, 8);
	} else if (p3Z <= p1Z && p3Z <= p2Z && 
		(p1X - p3X) >= -255 && (p1X - p3X) <= 255 && (p1Y - p3Y) >= -255 && (p1Y - p3Y) <= 255 &&
		(p2X - p3X) >= -255 && (p2X - p3X) <= 255 && (p2Y - p3Y) >= -255 && (p2Y - p3Y) <= 255) {

		xCoords = bitsout(p3X, 0, 14) | bitsout(p1X - p3X, 14, 9) | bitsout(p2X - p3X, 23, 9);
		yCoords = bitsout(p3Y, 0, 14) | bitsout(p1Y - p3Y, 14, 9) | bitsout(p2Y - p3Y, 23, 9);
		zCoords = bitsout(p3Z, 0, 14) | bitsout(p1Z - p3Z, 16, 8) | bitsout(p2Z - p3Z, 24, 8);
	}

	zCoords |= preservedUnknownBits;
}