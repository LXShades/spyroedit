#pragma once

#include "Types.h"
#include "Main.h"

#define WORLDSCALE 0.01f

#define MAKECOLOR16(r, g, b) ((((r) * 32 / 256) & 31) | (((g) * 32 / 256 & 31) << 5) | (((b) * 32 / 256 & 31) << 10) | 0x8000)
#define MAKECOLOR32(r, g, b) ((((r) & 0xFF)) | (((g) & 0xFF) << 8) | (((b) & 0xFF) << 16))
#define GETR16(clr) (((clr) & 31) * 264 / 32)
#define GETG16(clr) (((clr) >> 5 & 31) * 264 / 32)
#define GETB16(clr) (((clr) >> 10 & 31) * 264 / 32)
#define GETR32(clr) ((clr) & 0xFF)
#define GETG32(clr) (((clr) >> 8) & 0xFF)
#define GETB32(clr) (((clr) >> 16) & 0xFF)

#define TEXTURESPERROW 8 // (in a texture export) number of textures in a single row
#define TEXTUREROWLENGTH ((TEXTURESPERROW) * 64)

#define TILETOX(a) (((a) & 1) << 5) // texture tile ID to offset (0: 0, 0 1: 0, 32 2: 32,0 3: 32,32)
#define TILETOY(a) (((a) & 2) << 4)

#define MAXAVGTEXCOLOURS 200

// bit simplifiers
// get bits unsigned
#define bitsu(num, bitstart, numbits) ((num) >> (bitstart) & ~(0xFFFFFFFF << (numbits)))
// get bits signed
#define bitss(num, bitstart, numbits) (int) ((((num) >> (bitstart) & ~(0xFFFFFFFF << (numbits))) | (0 - (((num) >> ((bitstart)+(numbits)-1) & 1)) << (numbits))))
// build bits (use |)
#define bitsout(num, bitstart, numbits) (((num) & ~(0xFFFFFFFF << (numbits))) << (bitstart))

const uint16 BUT_TRIANGLE = 0x1000, BUT_SQUARE = 0x2000, BUT_X = 0x4000, BUT_CIRCLE = 0x8000;
const uint16 BUT_UP = 0x0010, BUT_RIGHT = 0x0020, BUT_DOWN = 0x0040, BUT_LEFT = 0x0080;
const uint16 BUT_L2 = 0x0100, BUT_R2 = 0x0200, BUT_L1 = 0x0400, BUT_R1 = 0x0800;
const uint16 BUT_SELECT = 0x0100, BUT_L3 = 0x0200, BUT_R3 = 0x0400, BUT_START = 0x0008;

#pragma pack(push, 1)
template <typename PointerType>
struct SpyroPointer { // Pointer type used to access PS1 memory pointers
	uint32 address;

	inline operator PointerType*() {
		return (PointerType*) ((uintptr)umem32 + (address & 0x003FFFFF));
	}

	inline PointerType* operator->() {
		return (PointerType*) ((uintptr)umem32 + (address & 0x003FFFFF));
	}

	template <typename T>
	inline explicit operator T*() {
		return (T*) ((uintptr)umem32 + (address & 0x003FFFFF));
	}
};

struct Angle {
	int8 x, y, z, unknown;
};

struct Animation {
	uint8 prevanim, nextanim, prevframe, nextframe;
};

struct IntVector {
	int16 x, y, z;
};

// X, Y, Z: 80073624, 80073628, 8007362C (signed 32)
// Main angle: 80073630
// Head angle: 80073634
// Spyro's animation: 30073638 (32)
// Head animations: 8007363C (32).
// Spyro (and head)'s animation progress: 80073640 (32).
// Hit (by others) flags: 80073648 (signed 32 but only last two bytes used)
// Collision stuff, maybe including collision angle. 80073658
// More collision stuff follows.
// State: 8007366C (32)
// 80073670(32): 1 when falling from a jump, 0 otherwise.
// Something like a state code: 80073674 (32) (Sparx grows a bubble when this is B).
// 80073678: Unknown, can be used to jump infinitely when frozen at FFFFFFFF or lower.
// Glide speed: 800736C4 (signed 32)
// Forward thrust (when charging?): 800736D0 (signed 32)
// Forward thrust (when moving): 800736D4 (signed 32)
struct Spyro {
	int32 x, y, z;

	Angle main_angle;
	Angle head_angle;

	Animation main_anim;
	Animation head_anim;

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

struct Moby { // Moby size: 0x58
	int32 typeData; // 0x00
	int32 unknown; // 0x04
	int32 collisionData;

	int32 x, y, z; // 0x0C

	uint32 attack_flags; // 0x18

	int8 _null[26]; // 0x1C

	int16 type; // 0x36

	int8 _dur[4]; // 0x38

	Animation anim; //0x3C

	int8 animspeed;   // 0x40
	int8 _unknown[2]; // 0x41
	int8 animRun;     // 0x43

	Angle angle; // 0x44

	int8 state; // 0x48

	int8 quack[3]; // 0x49

	int8 dist_draw; // 0x4C
	int8 dist_null; // 0x4D
	int8 dist_hp;   // 0x4E
	int8 scaleDown; // 0x4F (Default: 0x20)

	int8 dogtail[8]; // 0x54
};

enum power {
	PWR_ATTRACTION = 0x01,
	PWR_REPULSION = 0x02,
	PWR_GEMATTRACT = 0x04,
	PWR_FORCEFIELD = 0x08,
	PWR_SUPERBASH = 0x10,
	PWR_ULTRABASH = 0x20,
	PWR_HEADBASHPOCALYPSE = 0x40,
	PWR_BUTTERFLYBREATH = 0x80,
	PWR_DEATHSTARE = 0x0100,
	PWR_DEATHFIELD = 0x0200,
	PWR_REPELSTARE = 0x0400,
	PWR_ATTRACTSTARE = 0x0800,
	PWR_EXORCIST = 0x1000,
	PWR_TORNADO = 0x2000,
	PWR_TELEKINESIS = 0x4000,
	PWR_TRAIL = 0x8000,
	PWR_LOOKATSTUFF = 0x00010000,
	PWR_BARRELROLLS = 0x00020000,
	PWR_SANICROLLS  = 0x00040000,
	PWR_GIRAFFE     = 0x00080000,
	PWR_LUCIO       = 0x00100000
};

enum PaletteType {
	PT_LQ = 0,
	PT_HQ = 1
};

struct TexLq {
	uint8 xmin, ymin;
	uint8 palettex, palettey;
	uint8 xmax, ymax;
	uint8 region, alphaEtc;
};

struct TexHq {
	uint8 xmin, ymin;
	uint16 palette;
	uint8 xmax, ymax;
	uint8 region, unknown;

	inline int GetXMin() {
		return ((region * 128) % 2048) + xmin;
	}
	inline int GetYMin() {
		return ((region & 0x1F) / 16 * 256) + ymin;
	}
	inline int GetXMax() {
		return ((region * 128) % 2048) + xmax;
	}
	inline int GetYMax() {
		return ((region & 0x1F) / 16 * 256) + ymax;
	}
};

struct TexDef {
	TexLq lqData[2];
	TexHq hqData[4];
};

struct HqTexDef {
	uint32 unknown[2];
	TexHq hqData[4];
	TexHq hqDataClose[16];
};

struct LqTexDef {
	TexLq lqData[2];
};

struct SkyDef {
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

struct SceneFace;
struct SceneSectorHeader {
	uint16 centreY, centreX;
	uint16 centreRadiusAndFlags; // flags begin >>12
	uint16 centreZ;
	uint32 xyPos, zPos;
	uint8 numLpVertices, numLpColours, numLpFaces, lpUnknown;
	uint8 numHpVertices, numHpColours, numHpFaces, hpUnknown;
	uint32 zTerminator;

	union {
		uint32 data32[1]; uint16 data16[2]; uint8 data8[4];
	};

	int GetSize() const; // gets size in bytes
	inline int GetFlistId() const {
		return zPos & 0x1FFF; // since the per-face references max at 1FFF, this is a fairly safe assumption
	}
	inline void SetFlistId(uint32 val) {
		zPos = (zPos & (0xFFFFE000)) | (val & 0x1FFF);
	}
	
	inline uint32* GetLpVertices() {
		return data32;
	}
	inline uint32* GetLpColours() {
		return &data32[numLpVertices];
	}
	inline uint32* GetLpFaces() {
		return &data32[numLpVertices + numLpColours];
	}
	inline uint32* GetHpVertices() {
		return &data32[numLpVertices + numLpColours + numLpFaces * 2];
	}
	inline uint32* GetHpColours() {
		return &data32[numLpVertices + numLpColours + numLpFaces * 2 + numHpVertices];
	}
	inline SceneFace* GetHpFaces() {
		return (SceneFace*) &data32[numLpVertices + numLpColours + numLpFaces * 2 + numHpVertices + numHpColours * 2];
	}
};

struct SceneDef {
	uint32 size;
	uint32 iForget;
	uint32 numSectors;

	SpyroPointer<SceneSectorHeader> sectors[1];
};

extern int game;
struct SceneFace {
	union {
		uint8 verts[4]; // 0x00
		uint32 word1; // 0x00
	};
	union {
		uint8 colours[4]; // 0x04
		uint32 word2; // 0x04
	};
	uint32 word3; // 0x08
	uint32 word4; // 0x0C

	inline int GetTexture() const {
		if (game != SPYRO1)
			return word4 & 0x7F;
		else
			return word3 & 0x7F;
	}

	inline void SetTexture(int texture) {
		if (game != SPYRO1)
			word4 = (word4 & ~0x7F) | (texture & 0x7F);
		else
			word3 = (word3 & ~0x7F) | (texture & 0x7F);
	}

	inline bool GetFlip() const {
		if (game != SPYRO1)
			return (word4 >> 10) & 1;
		else
			return (word4 >> 1) & 1;
	}

	inline void SetFlip(bool flip) {
		if (game != SPYRO1)
			word4 = (word4 & ~0x400) | ((int) flip << 10);
		else
			word4 = (word4 & ~2) | ((int) flip << 1);
	}

	inline void SetDepth(int depth) {
		if (game == SPYRO1)
			word4 = (word4 & ~0xF8) | (depth << 3 & 0xF8);
	}

	inline int GetDepth() const {
		if (game == SPYRO1)
			return word4 >> 3 & 0x1F;
	}

	inline int GetEdge(int edgeId) const {
		if (game != SPYRO1) {
			switch (edgeId) {
				case 0: return word3 >> 19;
				case 1: return word3 >> 6 & 0x1FFF;
				case 2: return (word4 >> 6 & 0x1FC0) | (word3 & 0x3F);
				case 3: return word4 >> 19;
			}
		}
	}

	inline void SetEdge(int edgeId, uint32 val) {
		if (game != SPYRO1) {
			switch (edgeId) {
				case 0: word3 = (word3 & ~(0xFFFFFFFF << 19)) | ((val & 0x1FFF) << 19); break;
				case 1: word3 |= (val & 0x1FFF) << 6; break;
				case 2: 
					word3 = (word3 & ~0x3F) | (val & 0x3F);
					word4 = (word4 & ~0x7F000) | ((val & 0x1FC0) << 6);
					break;
				case 3: word4 = (word4 & ~(0xFFFFFFFF << 19)) | ((val & 0x1FFF) << 19); break;
			}
		}
	}
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
	uint32 iDontNeedThis[15]; // I don't need this

	SpyroPointer<SpyroAnimHeader> anims[1];
};

struct SpyroFrameInfo {
	uint32 unk1;
	uint32 unk2;
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

	inline void SetPoints(int myId, int p1X, int p1Y, int p1Z, int p2X, int p2Y, int p2Z, int p3X, int p3Y, int p3Z);
};

struct CollDef {
	uint32 numTriangles;
	uint32 numUnkn1;
	uint32 numUnkn2;
	SpyroPointer<uint16> blockTree;
	SpyroPointer<uint16> blocks;
	SpyroPointer<CollTri> triangleData;
	SpyroPointer<uint16> surfaceType;
	uint32 etc1;
	uint32 selfAddressUseless;
};

struct CollDefS1 {
	uint32 numTriangles;
	uint32 numUnkn1;
	SpyroPointer<uint16> blockTree;
	SpyroPointer<uint16> blocks;
	SpyroPointer<CollTri> triangleData;
	uint32 etc1;
	uint32 etc2;
};

struct LevelColours {
	uint8 fogR, fogG, fogB;
	int32 lightR, lightG, lightB;
};

struct ObjTex {
	uint16 minX, maxX;
	uint16 minY, maxY;
	uint32 paletteX, paletteY;
	
	uint16 mapX, mapY; // position on ObjTexMap

	bool8 hasFade; // whether or not a fading texture palette is required
};

struct ObjTexMap {
	uint16 width, height;

	ObjTex textures[2048];
	uint16 numTextures;
};

struct CollTriCache {
	int32 triangleIndex; // original index of the triangle in the Spyro 3's triangle list
	uint16 triangleType;
	CollTri trianglePoints;
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
	SEF_SKY,
	SEF_COLOURS,
	SEF_SCENERY,
	SEF_SETTINGS,
};

enum TextureEditFlags {
	TEF_SEPARATE         = 1,
	TEF_GENERATEPALETTES = 2,
	TEF_SHUFFLEPALETTES  = 4,
	TEF_AUTOLQ           = 8,
};

struct TexCacheTile {
	uint32 bitmap[32 * 32];
	uint32 minX, minY, maxX, maxY; // full ABSOLUTE (vram coords) of this tile's known range of X and Y mapping, checked over time for moving textures
	uint32 sizeX, sizeY;
	
	uint32 texId;
	uint8 tileId;
};

struct TexCache;
extern TexCache texCaches[256];
struct TexCache {
	TexCacheTile tiles[4];
	uint8 tileTransform[4];

	uint32 paletteId;
	bool8 ignore; // ignored textures won't be loaded into memory and their palettes will be preserved

	inline void Reset() {
		ignore = false;
		paletteId = 0;

		for (int i = 0; i < 4; i++) {
			tiles[i].minX = 0; tiles[i].maxX = 0;
			tiles[i].minY = 0; tiles[i].maxY = 0;
			tiles[i].sizeX = 0; tiles[i].sizeY = 0;
			tiles[i].texId = this - texCaches;
			tiles[i].tileId = i;
		}
	}
};

struct Matrix {
	int xx, xy; // x = x * xx + y * xy
	int yx, yy; // y = x * yx + y * yy
};

extern TexCache texCaches[256];
extern uint32 numTexCaches;
extern uint32 numPaletteCaches;

extern Matrix tileMatrices[8];

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

extern TexDef* textures;
extern int32* numTextures;

extern LqTexDef* lqTextures; // Spyro 1 only
extern HqTexDef* hqTextures; // Spyro 1 only

extern ObjTexMap objTexMap; // Object texture map generated by SpyroEdit based on moby textures (moby=obj)

extern uint32* levelNames;
extern int numLevelNames;

extern uint32 powers;
extern uint32 texEditFlags; // of the TextureEditFlags enum

extern SceneDef* sceneData;

extern SkyDef* skyData;
extern uint32* skyNumSectors;
extern uint32* skyBackColour;

extern uint8* sceneOcclusion;
extern uint8* skyOcclusion;

extern CollDef* collData;
extern CollDefS1* s1CollData;

extern CollisionCache collisionCache;

extern uint32 spyroModelPointer;
extern uint32 spyroDrawFuncAddress;
extern uint32 spyroDrawFuncAddressHack1;
extern uint32 spyroDrawFuncAddressHack2;

extern int palettes[1000]; // Byte positions of palettes
extern int paletteTypes[1000];
extern int numPalettes;

extern uint8 avgTexColours[MAXAVGTEXCOLOURS][4][3]; // By texture, corner and colour channel

// hacks
extern bool modifyCode; // Whether or not the plugin shall change the game's ASM
extern bool disableSceneOccl, disableSkyOccl; // Self-explanitory

void SpyroLoop(); // main loop
void PowersLoop(); // Spyro's powers applied here
void MultiplayerLoop(); // Spyro draw code overwritten and updated here (only if multiple players or recordings are in-game)

void DetectSpyroData(); // scans for and retrieves game data (pointers, etc). May not necessarily retrieve all data; unavailable data is typically reset to NULL or 0.

void SaveTextures(); // saves textures based on the current setting
void SaveTexturesAsSingle(); // Saves textures as a single 16-bit bmp file
void SaveTexturesAsMultiple(); // Saves textures into individual 8-bit HQ bmps and 4-bit LQ bmps
void SaveObjectTextures(); // Saves textures of the object model
void LoadTextures(); // Loads textures based on the current setting
void LoadTexturesAsSingle(); // Loads textures from a single 16 or 24-bit bmp file
void LoadTexturesAsMultiple(); // Loads textures from multiple 8-bit HQ bitmaps and 4-bit LQ bitmaps
void LoadTexturesAsMultipleUnpaletted(); // Loads textures from multiple 8/16/24-bit HQ bitmaps
void LoadObjectTextures(); // Loads textures of the object models

void UpdateObjTexMap();

void SaveSky(); // saves the sky model
void LoadSky(const char* customFilename = 0); // loads the sky model

void SaveColours(); // saves level colours--this might need changing once level geometry is replaceable
void LoadColours(); // loads level colours--this might need changing once level geometry is replaceable

void SetLevelColours(LevelColours* clrsIn); // tweaks level colours based on a colour multiplier
void GetLevelColours(LevelColours* clrsOut); // obtains level colours (does this even work? I forget)

void GetAvgTexColours(); // gets average level colours (incl. textures)

void TweakSky(int r, int g, int b);

void PinkMode(); // makes all level textures pink, because that is important
void ColorlessMode(); // removes all colour from the level, because it was too bright anyway
void CreepypastaMode(); // inverts all the colours, the games did it backwards the whole time
void IndieMode(); // removed all form of detail from the art, as the art is in storytelling, not good graphics, also this has nothing to do with our budget believe me

uint32 GetAverageLightColour();
uint32 GetAverageSkyColour();

void GetLevelFilename(char* filenameOut, SpyroEditFileType fileType, bool createFolders = false);

void UpdatePaletteList(); // updates the optimised palette list used for many texture mods

void GenerateLQTextures(); // auto-generated LQ textures from HQ textures
void CompleteLQPalettes(); // I forget

void MakePaletteFromColours(uint32* paletteOut, int desiredPaletteSize, uint32* colours, int numColours); // takes a full set of colours and makes a palette of desired size

float ToRad(int8 angle);
int8 ToAngle(float rad);

struct GPUSnapshot;
extern "C" void (__stdcall* Reset_Recompiler)(void);
extern "C" void GetSnapshot(GPUSnapshot* dataOut);
extern "C" void SetSnapshot(const GPUSnapshot* in);

inline void CollTri::SetPoints(int myId, int p1X, int p1Y, int p1Z, int p2X, int p2Y, int p2Z, int p3X, int p3Y, int p3Z) {
	if (p1Z <= p2Z && p1Z <= p3Z && 
		(p2X - p1X) >= -255 && (p2X - p1X) <= 255 && (p2Y - p1Y) >= -255 && (p2Y - p1Y) <= 255 &&
		(p3X - p1X) >= -255 && (p3X - p1X) <= 255 && (p3Y - p1Y) >= -255 && (p3Y - p1Y) <= 255) {

		xCoords = bitsout(p1X, 0, 14) | bitsout(p2X - p1X, 14, 9) | bitsout(p3X - p1X, 23, 9);
		yCoords = bitsout(p1Y, 0, 14) | bitsout(p2Y - p1Y, 14, 9) | bitsout(p3Y - p1Y, 23, 9);
		zCoords = bitsout(p1Z, 0, 16) | bitsout(p2Z - p1Z, 16, 8) | bitsout(p3Z - p1Z, 24, 8);
	} else if (p2Z <= p1Z && p2Z <= p3Z && 
		(p3X - p2X) >= -255 && (p3X - p2X) <= 255 && (p3Y - p2Y) >= -255 && (p3Y - p2Y) <= 255 &&
		(p1X - p2X) >= -255 && (p1X - p2X) <= 255 && (p1Y - p2Y) >= -255 && (p1Y - p2Y) <= 255) {

		xCoords = bitsout(p2X, 0, 14) | bitsout(p3X - p2X, 14, 9) | bitsout(p1X - p2X, 23, 9);
		yCoords = bitsout(p2Y, 0, 14) | bitsout(p3Y - p2Y, 14, 9) | bitsout(p1Y - p2Y, 23, 9);
		zCoords = bitsout(p2Z, 0, 16) | bitsout(p3Z - p2Z, 16, 8) | bitsout(p1Z - p2Z, 24, 8);
	} else if (p3Z <= p1Z && p3Z <= p2Z && 
		(p1X - p3X) >= -255 && (p1X - p3X) <= 255 && (p1Y - p3Y) >= -255 && (p1Y - p3Y) <= 255 &&
		(p2X - p3X) >= -255 && (p2X - p3X) <= 255 && (p2Y - p3Y) >= -255 && (p2Y - p3Y) <= 255) {

		xCoords = bitsout(p3X, 0, 14) | bitsout(p1X - p3X, 14, 9) | bitsout(p2X - p3X, 23, 9);
		yCoords = bitsout(p3Y, 0, 14) | bitsout(p1Y - p3Y, 14, 9) | bitsout(p2Y - p3Y, 23, 9);
		zCoords = bitsout(p3Z, 0, 16) | bitsout(p1Z - p3Z, 16, 8) | bitsout(p2Z - p3Z, 24, 8);
	}
}