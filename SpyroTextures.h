#pragma once
#include "Types.h"

#define TEXTURESPERROW 8 // (in a texture export) number of textures in a single row
#define TEXTUREROWLENGTH ((TEXTURESPERROW) * 64)

#define TILETOX(a) (((a) & 1) << 5) // texture tile ID to offset (0: 0, 0 1: 0, 32 2: 32,0 3: 32,32)
#define TILETOY(a) (((a) & 2) << 4)

#define MAXAVGTEXCOLOURS 200

enum PaletteType {
	PT_LQ = 0,
	PT_HQ = 1
};

enum TextureEditFlags {
	TEF_SEPARATE         = 1,
	TEF_GENERATEPALETTES = 2,
	TEF_SHUFFLEPALETTES  = 4,
	TEF_AUTOLQ           = 8,
};

#pragma pack(push, 1)
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

struct TexCacheTile {
	uint32 bitmap[32 * 32];
	uint32 minX, minY, maxX, maxY; // full ABSOLUTE (vram coords) of this tile's known range of X and Y mapping, checked over time for moving textures
	uint32 sizeX, sizeY;
	
	uint32 texId;
	uint8 tileId;
};

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

// Matrix used for HQ quad tiling
struct Matrix {
	int xx, xy; // x = x * xx + y * xy
	int yx, yy; // y = x * yx + y * yy
};

// Object texture on the texmap
struct ObjTex {
	uint16 minX, maxX;
	uint16 minY, maxY;
	uint32 paletteX, paletteY;
	
	uint16 mapX, mapY; // position on ObjTexMap

	bool8 hasFade; // whether or not a fading texture palette is required
};

// Object texture map used for positioning, saving and loading object textures
struct ObjTexMap {
	uint16 width, height;

	ObjTex textures[2048];
	uint16 numTextures;
};
#pragma pack(pop)

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

void PinkMode(); // makes all level textures pink, because that is important
void ColorlessMode(); // removes all colour from the level, because it was too bright anyway
void CreepypastaMode(); // inverts all the colours, the games did it backwards the whole time
void IndieMode(); // removed all form of detail from the art, as the art is in storytelling, not good graphics, also this has nothing to do with our budget believe me

bool LoadBmp32(BmpInfo* bmpOut, const char* bmpName);
bool SaveBmp(BmpInfo* info, const char* bmpName);
void FreeBmp(BmpInfo* info);

extern TexDef* textures;
extern int32* numTextures;

extern LqTexDef* lqTextures; // Spyro 1 only
extern HqTexDef* hqTextures; // Spyro 1 only

extern uint8 avgTexColours[MAXAVGTEXCOLOURS][4][3]; // By texture, corner and colour channel

extern TexCache texCaches[256];