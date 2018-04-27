#include "SpyroData.h"
#include "Main.h"

#include <Windows.h>
#include <cstdio> // sprintf

struct BmpInfo {
	uint32 width, height;
	uint8 bitsPerColour;

	union {
		uint32* data32;
		uint16* data16;
		uint8* data8;
	};
	RGBQUAD palette[256]; // only applicable if bitsPerColour <= 8
};

TexCache texCaches[256];
uint32 numTexCaches = 0;
uint32 numPaletteCaches = 0;

RGBQUAD paletteClrs[256];
Matrix tileMatrices[8] = {
	// 0
	1, 0,
	0, 1,
	// 1
	0, 1,
	1, 0, 
	// 2
	-1, 0,
	0, -1,
	// 3
	0, -1,
	1, 0,
	// 4
	0, 1,
	1, 0,
	// 5
	-1, 0,
	0, 1,
	// 6
	0, -1,
	-1, 0,
	// 7
	1, 0,
	0, -1
};

GPUSnapshot vramSs;

int palettes[1000]; // Byte positions of palettes
int paletteTypes[1000];
int numPalettes = 0;

ObjTexMap objTexMap;

uint8 avgTexColours[MAXAVGTEXCOLOURS][4][3]; // By texture, corner and colour channel

void SaveTexturesAsSingle();
void LoadObjectTextures();
void UpdateObjTexMap();

bool LoadBmp32(BmpInfo* bmpOut, const char* bmpName);
bool SaveBmp(BmpInfo* info, const char* bmpName);
void FreeBmp(BmpInfo* info);

void SaveTextures() {
	if (texEditFlags & TEF_SEPARATE)
		SaveTexturesAsMultiple();
	else
		SaveTexturesAsSingle();
}

void SaveTexturesAsMultiple() {
	uint32* uintmem = (uint32*) memory;

	if (!textures && !lqTextures)
		return;

	GetSnapshot(&vramSs);

	uint16* vram16 = (uint16*) vramSs.vram;
	uint8* vram8 = (uint8*) vramSs.vram;
	char lqFolderName[MAX_PATH], hqFolderName[MAX_PATH];

	GetLevelFilename(lqFolderName, SEF_TEXTUREFOLDERLQ, true);
	GetLevelFilename(hqFolderName, SEF_TEXTUREFOLDERHQ, true);

	for (int i = 0; i < *numTextures; i++) {
		TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;
		TexLq* curLq = textures ? textures[i].lqData : lqTextures[i].lqData;

		// Setup the bmp
		BmpInfo bmpInfo;
		uint8 bmpData[64*64];

		bmpInfo.bitsPerColour = 8;
		bmpInfo.width = 64;
		bmpInfo.height = 64;
		bmpInfo.data8 = bmpData;

		// Read texture tiles by palette
		uint16 palettesDone[4];
		int numPalettesDone = 0;
		for (int pal = 0; pal < 4; pal++) {
			for (int j = 0; j < numPalettesDone; j++) {
				if (palettesDone[j] == curHq[pal].palette)
					goto SkipPalette; // Already done palette
			}

			int paletteByteStart = curHq[pal].palette * 32;
			uint16* palette = &vram16[paletteByteStart / 2];
			int numTilesDone = 0;

			// Read palette data
			for (int j = 0; j < 256; j++) {
				bmpInfo.palette[j].rgbRed = GETR16(palette[j]);
				bmpInfo.palette[j].rgbGreen = GETG16(palette[j]);
				bmpInfo.palette[j].rgbBlue = GETB16(palette[j]);
				bmpInfo.palette[j].rgbReserved = 0;
			}

			for (int tile = 0; tile < 4; tile++) {
				const int xStartTable[4] = {0, 32, 0, 32}, yStartTable[4] = {0, 0, 32, 32};

				if (curHq[tile].palette != curHq[pal].palette) {
					// Fill this tile with the zero-colour in the bmp
					for (int y = 0; y < 32; y++) {
						for (int x = 0; x < 32; x++)
							bmpData[(y + yStartTable[tile]) * 64 + x + xStartTable[tile]] = 0;
					}
					continue;
				}

				int destXStart = xStartTable[tile], destYStart = yStartTable[tile];
				int srcXStart = curHq[tile].GetXMin(), srcYStart = curHq[tile].GetYMin();

				for (int y = 0; y < 32; y ++) {
					for (int x = 0; x < 32; x ++)
						bmpData[(destYStart + y) * 64 + (destXStart + x)] = vram8[(y + srcYStart) * 2048 + srcXStart + x];
				}

				numTilesDone++;
			}

			// Write the texture file
			char filename[MAX_PATH];

			sprintf(filename, "%s\\Pal%04X_Tex%04X.bmp", hqFolderName, curHq[pal].palette, i);
	
			SaveBmp(&bmpInfo, filename);
			SkipPalette:;
		}

		// Write low-res texture
		bmpInfo.bitsPerColour = 4;
		bmpInfo.width = 32;
		bmpInfo.height = 32;

		int paletteByteStart = curLq[0].palettex * 16 * 2 + curLq[0].palettey * 4 * 2048;
		uint16* palette = &vram16[paletteByteStart / 2];

		for (int j = 0; j < 16; j++) {
			bmpInfo.palette[j].rgbRed = GETR16(palette[j]);
			bmpInfo.palette[j].rgbGreen = GETG16(palette[j]);
			bmpInfo.palette[j].rgbBlue = GETB16(palette[j]);
			bmpInfo.palette[j].rgbReserved = 0;
		}

		int startx = (curLq[0].region * 256) % 2048, starty = ((curLq[0].region & 0x1F) / 16 * 256);
		int x1, y1, x2, y2;

		x1 = 2048+startx + curLq[0].xmin;
		x2 = 2048+startx + curLq[0].xmax;
		y1 = starty + curLq[0].ymin;
		y2 = starty + curLq[0].ymax;

		for (int y = 0; y < 32; y++) {
			for (int x = 0; x < 16; x++)
				bmpData[y * 16 + x] = (vram8[x1/2 + x + (y1 + y) * 2048] >> 4 & 0x0F) | (vram8[x1/2 + x + (y1 + y) * 2048] << 4 & 0xF0);
		}
		
		char filename[MAX_PATH];
		sprintf(filename, "%s\\Pal%02X%02X_Tex%04X.bmp", lqFolderName, curLq[0].palettex, curLq[0].palettey, i);

		SaveBmp(&bmpInfo, filename);
	}
}

void SaveTexturesAsSingle() {
	uint32* uintmem = (uint32*) memory;

	if (!textures && !hqTextures)
		return;

	// Get VRAM snapshot
	GetSnapshot(&vramSs);

	// Setup vars for faster save
	uint16* vram16 = (uint16*) vramSs.vram;
	uint8* vram8 = (uint8*) vramSs.vram;

	// Get textures filename
	char filename[MAX_PATH];

	GetLevelFilename(filename, SEF_TEXTURES, true);

	// Calculate the expected size of the bitmap
	int bmpWidth = TEXTURESPERROW * 64, bmpHeight = ((*numTextures + TEXTURESPERROW - 1) / TEXTURESPERROW) * 64;
	int curX = 0, curY = 0;
	int bmpPitch = ((bmpWidth * 2 + 1) / 2 * 2); // pitch includes padding

	uint16* bmpData = (uint16*) malloc(bmpHeight * bmpPitch);
	memset(bmpData, 0, bmpHeight * bmpPitch);

	// Create bitmap header
	BmpInfo bmpInfo;

	bmpInfo.width = bmpWidth; bmpInfo.height = bmpHeight;
	bmpInfo.data16 = bmpData;
	bmpInfo.bitsPerColour = 16;

	// Begin creation of bitmap
	for (int i = 0; i < *numTextures; i ++) {
		// The hi-res texture write begins here
		TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;
		TexLq* curLq = textures ? textures[i].lqData : lqTextures[i].lqData;

		for (int tile = 0; tile < 4; tile++) {
			const int xStartTable[4] = {0, 32, 0, 32}, yStartTable[4] = {0, 0, 32, 32};
			int paletteByteStart = curHq[tile].palette * 32;
			uint16* palette = &vram16[paletteByteStart / 2];
			int numTilesDone = 0;
			int destXStart = xStartTable[tile], destYStart = yStartTable[tile];

			// Read palette data
			for (int k = 0; k < 256; k ++) {
				uint16 clr = palette[k];
				paletteClrs[k].rgbRed = GETR16(clr);
				paletteClrs[k].rgbGreen = GETG16(clr);
				paletteClrs[k].rgbBlue = GETB16(clr);
				paletteClrs[k].rgbReserved = 0;
			}

			// Rotate or flip
			int orientation = curHq[tile].unknown>>4 & 7;
			int xx = tileMatrices[orientation].xx, xy = tileMatrices[orientation].xy;
			int yx = tileMatrices[orientation].yx, yy = tileMatrices[orientation].yy;
			int srcXStart = curHq[tile].GetXMin(), srcYStart = curHq[tile].GetYMin();

			if (xx < 0 || xy < 0)
				srcXStart += 31;
			if (yx < 0 || yy < 0)
				srcYStart += 31;

			// Write colours
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 32; x++) {
					RGBQUAD* clr = &paletteClrs[vram8[(x * yx + y * yy + srcYStart) * 2048 + srcXStart + x * xx + y * xy]];
					bmpData[(destYStart + y + curY) * bmpPitch / 2 + (destXStart + x + curX)] = 
						(((clr->rgbRed * 32 / 256) & 31) << 10) | (((clr->rgbGreen * 32 / 256) & 31) << 5) | (((clr->rgbBlue * 32 / 256) & 31));
				}
			}

			numTilesDone++;
		}

		curX += 64;

		if (curX >= bmpWidth) {
			curX = 0;
			curY += 64;
		}
	}

	// Write the texture file
	SaveBmp(&bmpInfo, filename);

	free(bmpData);
}
	/* Notes on alpha:
	- textures.lqData[0].alphaEtc needs & 0x80
	- alpha'd colours need 0x8000 */
	/*for (int i = 0; i < numTexs; i++) {
		textures[i].lqData[0].alphaEtc |= 0x80;
		textures[i].lqData[1].alphaEtc |= 0x80;
		for (int j = 0; j < 256; j++) {
			vram16[textures[i].hqData[0].palette * 32 / 2 + j] |= 0x8000;
			vram16[textures[i].hqData[1].palette * 32 / 2 + j] |= 0x8000;
			vram16[textures[i].hqData[2].palette * 32 / 2 + j] |= 0x8000;
			vram16[textures[i].hqData[3].palette * 32 / 2 + j] |= 0x8000;
			vram16[(textures[i].lqData[0].palettex * 16 * 2 + textures[i].lqData[0].palettey * 4 * 2048)/2 + j] |= 0x8000;
			vram16[(textures[i].lqData[1].palettex * 16 * 2 + textures[i].lqData[1].palettey * 4 * 2048)/2 + j] |= 0x8000;
		}
	}

	SetSnapshot(&vramSs);
	return;*/

void LoadTexturesIntoMemory();

void LoadTextures() {
	if (!textures && (!hqTextures || !lqTextures))
		return;

	// Reset cache ignore flag
	for (int i = 0; i < *numTextures; i++)
		texCaches[i].ignore = false;

	// Load textures from file(s) into texture caches
	if (texEditFlags & TEF_SEPARATE)
		LoadTexturesAsMultiple();
	else
		LoadTexturesAsSingle();

	// Ignore textures set to a certain colour
	for (int i = 0; i < *numTextures; i++) {
		int greenCount = 0;

		for (int t = 0; t < 4; t++) {
			uint32* bitmap = texCaches[i].tiles[t].bitmap;
			for (int c = 0; c < 32 * 32; c++) {
				if ((bitmap[c] >> 8) & 0xFF >= 253 && !(bitmap[c] & 0x00FF00FF))
					greenCount++;
			}

			if (greenCount >= 32 * 32 / 10)
				texCaches[i].ignore = true;
		}
	}
	
	// Load the textures into memory!
	LoadTexturesIntoMemory();

	if (texEditFlags & TEF_AUTOLQ)
		GenerateLQTextures();
}

void LoadTexturesAsSingle() {
	if (!textures && !hqTextures)
		return;

	// Get the texture file name
	char filename[MAX_PATH];
	GetLevelFilename(filename, SEF_TEXTURES);
	
	// Open the texture file
	BmpInfo bmp;
	
	if (!LoadBmp32(&bmp, filename))
		return;

	// Limit the number of textures
	int texPerRow = bmp.width / 64, numRows = bmp.height / 64;
	int numTexs = *numTextures;

	if (numTexs > texPerRow * numRows)
		numTexs = texPerRow * numRows; // avoid exceeding the BMP's limits
	
	for (int i = 0; i < numTexs; i++) {
		int texX = (i % texPerRow) * 64, texY = (i / texPerRow) * 64;
		TexCache* cache = &texCaches[i];

		for (int tile = 0; tile < 4; tile++) {
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 32; x++)
					cache->tiles[tile].bitmap[y * 32 + x] = bmp.data32[(texY + TILETOY(tile) + y) * bmp.width + texX + TILETOX(tile) + x] & 0x00FFFFFF;
			}
		}
	}

	FreeBmp(&bmp);
}

void LoadTexturesAsMultiple() {
	if (!textures && (!lqTextures || !hqTextures))
		return;

	int numTexs = *numTextures;
	int paletteByteStart = 0;
	char hqFolder[MAX_PATH], lqFolder[MAX_PATH];

	GetLevelFilename(hqFolder, SEF_TEXTUREFOLDERHQ);
	GetLevelFilename(lqFolder, SEF_TEXTUREFOLDERLQ);

	for (int i = 0; i < numTexs && i < numTexCaches; i++) {
		// Read each tile from each possible file (note: not very efficient as this can open a file multiple times)
		TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;
		for (int tile = 0; tile < 4; tile++) {
			char filename[MAX_PATH];

			sprintf(filename, "%s\\Pal%04X_Tex%04X.bmp", hqFolder, curHq[tile].palette, i);
	
			// Open the texture file
			BmpInfo bmp;
	
			if (!LoadBmp32(&bmp, filename)) {
				texCaches[i].ignore = true;
				continue;
			}
			if (bmp.width != 64 || bmp.height != 64) {
				texCaches[i].ignore = true;
				FreeBmp(&bmp);
				continue;
			}

			// Copy bitmap data to tile cache
			uint32* bitmap = texCaches[i].tiles[tile].bitmap;
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 32; x++)
					bitmap[y * 32 + x] = bmp.data32[((y + TILETOY(tile)) * 64) + x + TILETOX(tile)];
			}

			FreeBmp(&bmp);
		}
	}
}

void LoadTexturesIntoMemory() {
	if (!textures && (!lqTextures || !hqTextures))
		return;

	// Setup VRAM
	GetSnapshot(&vramSs);
	uint16* vram16 = (uint16*) vramSs.vram;
	uint8* vram8 = (uint8*) vramSs.vram;

	// Limit the number of textures
	int numTexs = *numTextures;

	if (numTexs > numTexCaches)
		numTexs = numTexCaches;

	// Make new palettes
	uint16 uniquePaletteIds[512];
	uint32 avgColourPalette[512];
	bool8 paletteIgnored[512] = {0};
	int numModdablePalettes = 0;
	int numTotalPalettes = 0;

	// Count number of unique palettes
	for (int i = 0; i < numTexs; i++) {
		TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;

		for (int tile = 0; tile < 4; tile++) {
			int palette = curHq[tile].palette;
			int foundPalette;

			for (foundPalette = 0; foundPalette < numTotalPalettes; foundPalette++) {
				if (uniquePaletteIds[foundPalette] == palette)
					break;
			}

			if (foundPalette == numTotalPalettes)
				uniquePaletteIds[numTotalPalettes++] = palette;

			if (texCaches[i].ignore)
				paletteIgnored[foundPalette] = true;
		}
	}

	// Count number of unique palettes that aren't being ignored
	for (int i = 0; i < numTotalPalettes; i++) {
		if (!paletteIgnored[i])
			numModdablePalettes++;
	}

	// Collect all colours in the textures and build a level-wide, median-colour palette
	if ((texEditFlags & TEF_SHUFFLEPALETTES) && (texEditFlags & TEF_GENERATEPALETTES)) {
		uint32 completedTiles[2048];
		int numCompletedTiles = 0;

		// Find unique texture tiles
		TexCacheTile* uniqueTiles[512];
		uint32 uniqueTilePos[512];
		bool uniqueTileInPalette[512] = {0};
		uint32 numUniqueTiles = 0;

		for (int i = 0; i < numTexs; i++) {
			TexHq* hqData = textures ? textures[i].hqData : hqTextures[i].hqData;

			if (texCaches[i].ignore)
				continue;

			for (int tile = 0; tile < 4; tile++) {
				uint32 pos = hqData[tile].xmin + hqData[tile].ymin * 2048 + hqData[tile].region * 2048 * 512;
				for (int j = 0; j < numUniqueTiles; j++) {
					if (uniqueTilePos[j] == pos)
						goto Skip;
				}

				uniqueTilePos[numUniqueTiles] = pos;
				uniqueTiles[numUniqueTiles] = &texCaches[i].tiles[tile];
				numUniqueTiles++;
			}

			Skip:
			continue;
		}

		// Find the texture tile that is most different from all other tiles
		uint32 maxDiff = 0;
		uint32 maxDiffTile = 0;
		for (int i = 0; i < numUniqueTiles; i++) {
			uint32 totalDiff = 0;
			uint8* bmp = (uint8*) uniqueTiles[i]->bitmap;

			for (int j = 0; j < numUniqueTiles; j++) {
				if (j == i)
					continue;

				uint8* bmp2 = (uint8*) uniqueTiles[j]->bitmap;

				for (int c = 0; c < 32 * 32 * 4; c += 4) {
					if (bmp[c] > bmp2[c])
						totalDiff += bmp[c] - bmp2[c];
					else
						totalDiff += bmp2[c] - bmp[c];
					if (bmp[c+1] > bmp2[c+1])
						totalDiff += bmp[c+1] - bmp2[c+1];
					else
						totalDiff += bmp2[c+1] - bmp[c+1];
					if (bmp[c+2] > bmp2[c+2])
						totalDiff += bmp[c+2] - bmp2[c+2];
					else
						totalDiff += bmp2[c+2] - bmp[c+2];
				}
			}

			if (totalDiff > maxDiff) {
				maxDiff = totalDiff;
				maxDiffTile = i;
			}
		}

		TexCacheTile* paletteTiles[256] = {uniqueTiles[maxDiffTile]};
		uint32 numPaletteTiles = 1;
		uniqueTileInPalette[maxDiffTile] = true;
		
		// Recursively find the most 'differing' tiles compared to the ones in paletteTiles and add them to paletteTiles
		// This should hopefully result in a maximally diverse palette
		while (numPaletteTiles < numModdablePalettes) {
			uint32 maxMinDiff = 0, maxMinDiffTile = 0;

			for (int i = 0; i < numUniqueTiles; i++) {
				if (uniqueTileInPalette[i])
					continue;

				uint32 minDiff = 0xFFFFFFFF;
				uint8* bmp = (uint8*) uniqueTiles[i]->bitmap;

				for (int j = 0; j < numPaletteTiles; j++) {
					uint8* bmp2 = (uint8*) paletteTiles[j]->bitmap;
					uint32 totalDiff = 0;

					for (int c = 0; c < 32 * 32 * 4; c += 8 + (8*32)) {
						if (bmp[c] > bmp2[c])
							totalDiff += bmp[c] - bmp2[c];
						else
							totalDiff += bmp2[c] - bmp[c];
						if (bmp[c+1] > bmp2[c+1])
							totalDiff += bmp[c+1] - bmp2[c+1];
						else
							totalDiff += bmp2[c+1] - bmp[c+1];
						if (bmp[c+2] > bmp2[c+2])
							totalDiff += bmp[c+2] - bmp2[c+2];
						else
							totalDiff += bmp2[c+2] - bmp[c+2];
					}

					if (totalDiff < minDiff)
						minDiff = totalDiff;
				}

				if (minDiff > maxMinDiff) {
					maxMinDiff = minDiff;
					maxMinDiffTile = i;
				}
			}

			paletteTiles[numPaletteTiles++] = uniqueTiles[maxMinDiffTile];
			uniqueTileInPalette[maxMinDiffTile] = true;
		}

		// Scan through all textures, and their tiles, and determine their new colour palettes
		for (int i = 0; i < numUniqueTiles; i++) {
			uint8* bmp = (uint8*) uniqueTiles[i]->bitmap;
			uint32 palMinDiff = 0xFFFFFFFF;
			uint32 palMin = 0;
				
			for (int j = 0; j < numPaletteTiles; j++) {
				uint8* bmp2 = (uint8*) paletteTiles[j]->bitmap;
				uint32 totalDiff = 0;

				for (int c = 0; c < 32 * 32 * 4; c += 8 + (8*32)) {
					if (bmp[c] > bmp2[c]) totalDiff += bmp[c] - bmp2[c];
					else totalDiff += bmp2[c] - bmp[c];
					if (bmp[c+1] > bmp2[c+1]) totalDiff += bmp[c+1] - bmp2[c+1];
					else totalDiff += bmp2[c+1] - bmp[c+1];
					if (bmp[c+2] > bmp2[c+2]) totalDiff += bmp[c+2] - bmp2[c+2];
					else totalDiff += bmp2[c+2] - bmp[c+2];
				}

				if (totalDiff < palMinDiff) {
					palMinDiff = totalDiff;
					palMin = j;
				} else if (paletteTiles[j] == uniqueTiles[i]) {
					palMinDiff = 0;
					palMin = j;
				}
			}

			TexHq* curHq = textures ? textures[uniqueTiles[i]->texId].hqData : hqTextures[uniqueTiles[i]->texId].hqData;
			int curPal = 0;

			for (int j = 0; j < numTotalPalettes; j++) {
				if (paletteIgnored[j])
					continue;

				if (curPal == palMin) {
					curHq[uniqueTiles[i]->tileId].palette = uniquePaletteIds[curPal];
					break;
				}
				curPal++;
			}
		}
	}

	// Build the palettes themselves and reassign the colours of each tile using it!
	uint32 tempColours[64*64*30];
	for (int pal = 0; pal < numTotalPalettes; pal++) {
		if (paletteIgnored[pal])
			continue;

		uint32 paletteColours[256];
		int numTempColours = 0;
		uint16* palette = &vram16[uniquePaletteIds[pal] * 32 / 2];

		// Collect colours for shared palette
		if (texEditFlags & TEF_GENERATEPALETTES) {
			for (int j = 0; j < numTexs; j++) {
				TexHq* curHq = textures ? textures[j].hqData : hqTextures[j].hqData;

				for (int tile = 0; tile < 4; tile++) {
					if (curHq[tile].palette != uniquePaletteIds[pal])
						continue;

					for (int y = 0; y < 32; y++) {
						for (int x = 0; x < 32; x++) {
							uint32 colour = texCaches[j].tiles[tile].bitmap[y * 32 + x];

							for (int k = 0; k < numTempColours; k++) {
								if (tempColours[k] == colour)
									goto Skip2;
							}

							tempColours[numTempColours++] = colour;
							Skip2:;
						}
					}
				}
			}
		
			// Build palette
			MakePaletteFromColours(paletteColours, 256, tempColours, numTempColours);

			// Copy 32-bit palette colours to 16-bit memory
			for (int j = 0; j < 256; j++) {
				palette[j] = MAKECOLOR16(GETR32(paletteColours[j]), GETG32(paletteColours[j]), GETB32(paletteColours[j]));

				if (palette[j] == 0x8000)
					palette[j] = 0;
			}
		}

		// Reassign the texture tiles with the palette
		uint32 completedTiles[2048];
		uint32 completedTilePalettes[2048];
		int numCompletedTiles = 0;
		for (int j = 0; j < numTexs; j++) {
			TexHq* curHq = textures ? textures[j].hqData : hqTextures[j].hqData;

			if (texCaches[j].ignore)
				continue; // save some processing time

			for (int tile = 0; tile < 4; tile++) {
				uint32* bitmap = texCaches[j].tiles[tile].bitmap;
				int vramXStart = texCaches[j].tiles[tile].minX, vramYStart = texCaches[j].tiles[tile].minY;
				bool alreadyExists = false;

				for (int c = 0; c < numCompletedTiles; c++) {
					if (vramXStart + vramYStart * 4096 == completedTiles[c]) { // tile is shared with a tile that's already been reassigned
						curHq[tile].palette = completedTilePalettes[c]; // just set the palette
						alreadyExists = true;
						break;
					}
				}

				if (curHq[tile].palette != uniquePaletteIds[pal] || alreadyExists)
					continue;

				int orientation = curHq[tile].unknown>>4 & 7;
				int xx = tileMatrices[orientation].xx, xy = tileMatrices[orientation].xy;
				int yx = tileMatrices[orientation].yx, yy = tileMatrices[orientation].yy;
				int vramYEnd = texCaches[j].tiles[tile].maxY;
				bool movingX = (texCaches[j].tiles[tile].maxX - texCaches[j].tiles[tile].minX > 32),
					 movingY = (texCaches[j].tiles[tile].maxY - texCaches[j].tiles[tile].minY > 32);

				if (xx < 0 || xy < 0)
					vramXStart += 31;
				if (yx < 0 || yy < 0)
					vramYStart += 31;

				for (int y = 0; y < 32; y++) {
					for (int x = 0; x < 32; x++) {
						uint32 colour = bitmap[y * 32 + x], clrR = GETR32(colour), clrG = GETG32(colour), clrB = GETB32(colour);

						int palId = 0, palClosestDist = 256 * 256 * 3;
						for (int j = 0; j < 256; j++) {
							int rDelta = GETR16(palette[j]) - clrR, gDelta = GETG16(palette[j]) - clrG, bDelta = GETB16(palette[j]) - clrB;
							int dist = rDelta * rDelta + gDelta * gDelta + bDelta * bDelta;

							if (dist < palClosestDist) {
								palId = j;
								palClosestDist = dist;
							}
						}

						vram8[((x * yx + y * yy) + vramYStart) * 2048 + vramXStart + (x * xx + y * xy)] = palId;

						if (movingY) {
							if (((x * yx + y * yy) + vramYStart) + 32 <= vramYEnd)
								vram8[((x * yx + y * yy) + vramYStart + 32) * 2048 + vramXStart + (x * xx + y * xy)] = palId;
							if (((x * yx + y * yy) + vramYStart) + 64 <= vramYEnd)
								vram8[((x * yx + y * yy) + vramYStart + 64) * 2048 + vramXStart + (x * xx + y * xy)] = palId;
						}
					}
				}

				completedTilePalettes[numCompletedTiles] = curHq[tile].palette;
				completedTiles[numCompletedTiles] = vramXStart + vramYStart * 4096;
				numCompletedTiles++;
			}
		}
	}

	if (hqTextures) {
		for (int i = 0; i < *numTextures; i++) {
			for (int j = 0; j < 16; j++) {
				int tileX = (j % 4), tileY = (j / 4);
				TexHq* sourceTile = &hqTextures[i].hqData[(tileY / 2) * 2 + (tileX / 2)];
				int xStart = sourceTile->xmin + (sourceTile->xmax - sourceTile->xmin + 1) / 2 * (tileX & 1), xEnd = sourceTile->xmin + (sourceTile->xmax - sourceTile->xmin + 1) / 2 * ((tileX & 1) + 1) - 1;
				int yStart = sourceTile->ymin + (sourceTile->ymax - sourceTile->ymin + 1) / 2 * (tileY & 1), yEnd = sourceTile->ymin + (sourceTile->ymax - sourceTile->ymin + 1) / 2 * ((tileY & 1) + 1) - 1;
				hqTextures[i].hqDataClose[j].palette = sourceTile->palette;
			}
		}
	}

	SetSnapshot(&vramSs);
}

/*void LoadTexturesIntoMemory() {
	if (!textures && (!lqTextures || !hqTextures))
		return;

	// Setup VRAM
	GetSnapshot(&vramSs);
	uint16* vram16 = (uint16*) vramSs.vram;
	uint8* vram8 = (uint8*) vramSs.vram;

	// Limit the number of textures
	int numTexs = *numTextures;

	if (numTexs > numTexCaches)
		numTexs = numTexCaches;

	// Make new palettes
	uint16 uniquePaletteIds[512];
	uint32 avgColourPalette[512];
	bool8 paletteIgnored[512] = {0};
	int numModdablePalettes = 0;
	int numTotalPalettes = 0;

	// Count number of unique palettes
	for (int i = 0; i < numTexs; i++) {
		TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;

		for (int tile = 0; tile < 4; tile++) {
			int palette = curHq[tile].palette;
			int foundPalette;

			for (foundPalette = 0; foundPalette < numTotalPalettes; foundPalette++) {
				if (uniquePaletteIds[foundPalette] == palette)
					break;
			}

			if (foundPalette == numTotalPalettes)
				uniquePaletteIds[numTotalPalettes++] = palette;

			if (texCaches[i].ignore)
				paletteIgnored[foundPalette] = true;
		}
	}

	// Count number of unique palettes that aren't being ignored
	for (int i = 0; i < numTotalPalettes; i++) {
		if (!paletteIgnored[i])
			numModdablePalettes++;
	}

	// Collect all colours in the textures and build a level-wide, median-colour palette
	uint32* tempColours = (uint32*) malloc(numTexs * 64 * 64 * 4);
	int numTempColours = 0;
	
	if ((texEditFlags & TEF_SHUFFLEPALETTES) && (texEditFlags & TEF_GENERATEPALETTES)) {
		uint32 completedTiles[2048];
		int numCompletedTiles = 0;

		// Collect all tiles. I tried adding code to prevent tiles from being re-used, but that seemed to make uglier palettes
		for (int i = 0; i < numTexs; i++) {
			TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;

			for (int tile = 0; tile < 4; tile++) {
				for (int y = 0; y < 32; y++) {
					for (int x = 0; x < 32; x++)
						tempColours[numTempColours++] = texCaches[i].tiles[tile].bitmap[y * 32 + x];
				}
			}
		}

		// Make the palette
		MakePaletteFromColours(avgColourPalette, numModdablePalettes, tempColours, numTempColours);

		// Scan through all textures, and their tiles, and determine their new colour palettes
		for (int i = 0; i < numTexs; i++) {
			TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;

			if (texCaches[i].ignore)
				continue;

			for (int tile = 0; tile < 4; tile++) {
				int incidence[256] = {0};

				for (int y = 0; y < 32; y++) {
					for (int x = 0; x < 32; x++) {
						uint32 colour = texCaches[i].tiles[tile].bitmap[y * 32 + x], clrR = GETR32(colour), clrG = GETG32(colour), clrB = GETB32(colour);
						int palId = 0, palClosestDist = 256 * 256 * 256;

						for (int j = 0; j < numTotalPalettes; j++) {
							if (paletteIgnored[j])
								continue;

							int rDelta = GETR32(avgColourPalette[j]) - clrR, gDelta = GETG32(avgColourPalette[j]) - clrG, bDelta = GETB32(avgColourPalette[j]) - clrB;
							int dist = rDelta * rDelta + gDelta * gDelta + bDelta * bDelta;

							if (dist < palClosestDist) {
								palId = j;
								palClosestDist = dist;
							}
						}

						incidence[palId]++;
					}
				}

				// Count the most common palette
				int palId = 0, maxNumIncidences = 0;
				for (int j = 0; j < numTotalPalettes; j++) {
					if (incidence[j] > maxNumIncidences) {
						maxNumIncidences = incidence[j];
						palId = j;
					}
				}

				curHq[tile].palette = uniquePaletteIds[palId];
			}
		}
	}

	// Build the palettes themselves and reassign the colours of each tile using it!
	for (int pal = 0; pal < numTotalPalettes; pal++) {
		if (paletteIgnored[pal])
			continue;

		uint32 paletteColours[256];
		int numTempColours = 0;
		uint16* palette = &vram16[uniquePaletteIds[pal] * 32 / 2];

		// Collect colours for shared palette
		if (texEditFlags & TEF_GENERATEPALETTES) {
			for (int j = 0; j < numTexs; j++) {
				TexHq* curHq = textures ? textures[j].hqData : hqTextures[j].hqData;

				for (int tile = 0; tile < 4; tile++) {
					if (curHq[tile].palette != uniquePaletteIds[pal])
						continue;

					for (int y = 0; y < 32; y++) {
						for (int x = 0; x < 32; x++) {
							uint32 colour = texCaches[j].tiles[tile].bitmap[y * 32 + x];

							for (int k = 0; k < numTempColours; k++) {
								if (tempColours[k] == colour)
									goto Skip2;
							}

							tempColours[numTempColours++] = colour;
							Skip2:;
						}
					}
				}
			}
		
			// Build palette
			MakePaletteFromColours(paletteColours, 256, tempColours, numTempColours);

			// Copy 32-bit palette colours to 16-bit memory
			for (int j = 0; j < 256; j++) {
				palette[j] = MAKECOLOR16(GETR32(paletteColours[j]), GETG32(paletteColours[j]), GETB32(paletteColours[j]));

				if (palette[j] == 0x8000)
					palette[j] = 0;
			}
		}

		// Reassign the texture tiles with the palette
		uint32 completedTiles[2048];
		uint32 completedTilePalettes[2048];
		int numCompletedTiles = 0;
		for (int j = 0; j < numTexs; j++) {
			TexHq* curHq = textures ? textures[j].hqData : hqTextures[j].hqData;

			if (texCaches[j].ignore)
				continue; // save some processing time

			for (int tile = 0; tile < 4; tile++) {
				uint32* bitmap = texCaches[j].tiles[tile].bitmap;
				int vramXStart = texCaches[j].tiles[tile].minX, vramYStart = texCaches[j].tiles[tile].minY;
				bool alreadyExists = false;

				for (int c = 0; c < numCompletedTiles; c++) {
					if (vramXStart + vramYStart * 4096 == completedTiles[c]) { // tile is shared with a tile that's already been reassigned
						curHq[tile].palette = completedTilePalettes[c]; // just set the palette
						alreadyExists = true;
						break;
					}
				}

				if (curHq[tile].palette != uniquePaletteIds[pal] || alreadyExists)
					continue;

				int orientation = curHq[tile].unknown>>4 & 7;
				int xx = tileMatrices[orientation].xx, xy = tileMatrices[orientation].xy;
				int yx = tileMatrices[orientation].yx, yy = tileMatrices[orientation].yy;
				int vramYEnd = texCaches[j].tiles[tile].maxY;
				bool movingX = (texCaches[j].tiles[tile].maxX - texCaches[j].tiles[tile].minX > 32),
					 movingY = (texCaches[j].tiles[tile].maxY - texCaches[j].tiles[tile].minY > 32);

				if (xx < 0 || xy < 0)
					vramXStart += 31;
				if (yx < 0 || yy < 0)
					vramYStart += 31;

				for (int y = 0; y < 32; y++) {
					for (int x = 0; x < 32; x++) {
						uint32 colour = bitmap[y * 32 + x], clrR = GETR32(colour), clrG = GETG32(colour), clrB = GETB32(colour);

						int palId = 0, palClosestDist = 256 * 256 * 3;
						for (int j = 0; j < 256; j++) {
							int rDelta = GETR16(palette[j]) - clrR, gDelta = GETG16(palette[j]) - clrG, bDelta = GETB16(palette[j]) - clrB;
							int dist = rDelta * rDelta + gDelta * gDelta + bDelta * bDelta;

							if (dist < palClosestDist) {
								palId = j;
								palClosestDist = dist;
							}
						}

						vram8[((x * yx + y * yy) + vramYStart) * 2048 + vramXStart + (x * xx + y * xy)] = palId;

						if (movingY) {
							if (((x * yx + y * yy) + vramYStart) + 32 <= vramYEnd)
								vram8[((x * yx + y * yy) + vramYStart + 32) * 2048 + vramXStart + (x * xx + y * xy)] = palId;
							if (((x * yx + y * yy) + vramYStart) + 64 <= vramYEnd)
								vram8[((x * yx + y * yy) + vramYStart + 64) * 2048 + vramXStart + (x * xx + y * xy)] = palId;
						}
					}
				}

				completedTilePalettes[numCompletedTiles] = curHq[tile].palette;
				completedTiles[numCompletedTiles] = vramXStart + vramYStart * 4096;
				numCompletedTiles++;
			}
		}
	}

	if (hqTextures) {
		for (int i = 0; i < *numTextures; i++) {
			for (int j = 0; j < 16; j++) {
				int tileX = (j % 4), tileY = (j / 4);
				TexHq* sourceTile = &hqTextures[i].hqData[(tileY / 2) * 2 + (tileX / 2)];
				int xStart = sourceTile->xmin + (sourceTile->xmax - sourceTile->xmin + 1) / 2 * (tileX & 1), xEnd = sourceTile->xmin + (sourceTile->xmax - sourceTile->xmin + 1) / 2 * ((tileX & 1) + 1) - 1;
				int yStart = sourceTile->ymin + (sourceTile->ymax - sourceTile->ymin + 1) / 2 * (tileY & 1), yEnd = sourceTile->ymin + (sourceTile->ymax - sourceTile->ymin + 1) / 2 * ((tileY & 1) + 1) - 1;
				hqTextures[i].hqDataClose[j].palette = sourceTile->palette;
			}
		}
	}

	SetSnapshot(&vramSs);

	free(tempColours);
}*/

void SaveObjectTextures() {
	GetSnapshot(&vramSs);

	UpdateObjTexMap();

	uint16* vram16 = (uint16*) vramSs.vram;
	uint8* vram8 = (uint8*) vramSs.vram;
	const int vramWidth = 1024; // vram width in u16s
	int bmpWidth = objTexMap.width, bmpHeight = objTexMap.height;
	int bmpPitch = ((bmpWidth * 2 + 1) / 2 * 2);
	uint16* bmpData = (uint16*) malloc(bmpHeight * bmpPitch * 2);

	// Clear bmp
	for (int i = 0, e = bmpWidth * bmpHeight; i < e; i++)
		bmpData[i] = MAKECOLOR16(255, 0, 255);

	// Draw white box to bmp
	for (int y = 0; y < 32 && y < bmpHeight; y++) {
		for (int x = 0; x < 32; x++)
			bmpData[y * bmpWidth + x] = 0xFFFF;
	}

	// Draw textures to bmp
	for (int i = 0; i < objTexMap.numTextures; i++) {
		ObjTex* tex = &objTexMap.textures[i];
		uint16* palette = &vram16[tex->paletteX + tex->paletteY * vramWidth];

		int bmpX = tex->mapX, bmpY = tex->mapY;
		for (int y = 0, texHeight = tex->maxY - tex->minY + 1; y < texHeight; y++) {
			int rowStart = (bmpY + y) * (bmpPitch / 2) + bmpX;
			for (int x = 0, texWidth = tex->maxX - tex->minX + 1; x < texWidth; x++) {
				uint16 clr;

				if (x & 1)
					clr = palette[(vram8[(tex->minY + y) * 2048 + (tex->minX + x)/2] >> 4) & 0x0F];
				else
					clr = palette[vram8[(tex->minY + y) * 2048 + (tex->minX + x)/2] & 0x0F];

				int r = GETR16(clr), g = GETG16(clr), b = GETB16(clr);
				bmpData[rowStart + x] = (((r * 32 / 256) & 31) << 10) | (((g * 32 / 256) & 31) << 5) | (((b * 32 / 256) & 31));
			}
		}
	}

	// Write bitmap
	BmpInfo info;
	info.bitsPerColour = 16;
	info.width = bmpWidth;
	info.height = bmpHeight;
	info.data16 = bmpData;
	
	char filename[256];
	GetLevelFilename(filename, SEF_OBJTEXTURES, true);
	SaveBmp(&info, filename);

	free(bmpData);
}

void LoadObjectTextures() {
	GetSnapshot(&vramSs);

	UpdateObjTexMap();

	BmpInfo bmp;
	char filename[256];
	GetLevelFilename(filename, SEF_OBJTEXTURES);

	if (!LoadBmp32(&bmp, filename))
		return;
	
	uint16* vram16 = (uint16*) vramSs.vram;
	uint8* vram8 = (uint8*) vramSs.vram;
	const int vramWidth = 1024;

	// Iterate through every palette (NOTE: despite this for loop's appearance, we intend to iterate through palettes first, not textures)
	uint32* colours = (uint32*) malloc(256 * 4);

	for (int i = 0; i < objTexMap.numTextures; i++) {
		ObjTex* tex = &objTexMap.textures[i];
		int paletteId = tex->paletteX + tex->paletteY * vramWidth;
		int numColours = 0;

		for (int j = 0; j < i; j++) {
			if (objTexMap.textures[j].paletteX + objTexMap.textures[j].paletteY * vramWidth == paletteId)
				goto NextPalette; // We've already done this palette
		}

		// New unique palette. Collect up the colours from the relevant textures in the bmp
		for (int j = i; j < objTexMap.numTextures; j++) {
			if (objTexMap.textures[j].paletteX + objTexMap.textures[j].paletteY * vramWidth != paletteId)
				continue;
			if (objTexMap.textures[j].mapX + (objTexMap.textures[j].maxX - objTexMap.textures[j].minX) >= bmp.width || 
				objTexMap.textures[j].mapY + (objTexMap.textures[j].maxY - objTexMap.textures[j].minY) >= bmp.height)
				continue;

			ObjTex* tex = &objTexMap.textures[j];
			for (int x = tex->mapX; x < tex->mapX + tex->maxX - tex->minX; x++) {
				for (int y = tex->mapY; y < tex->mapY + tex->maxY - tex->minY; y++) {
					uint32 colour = bmp.data32[y * bmp.width + x];
					bool colourExists = false;

					for (int c = 0; c < numColours; c++) {
						if (colours[c] == colour) {
							colourExists = true;
							break;
						}
					}

					if (!colourExists) {
						colours[numColours++] = colour;
						if (!(numColours % 256))
							colours = (uint32*) realloc(colours, (numColours + 256) * 4);
					}
				}
			}
		}

		// Make a palette out of the colours we grabbed
		uint32 palette[16];
		MakePaletteFromColours(palette, 16, colours, numColours);

		// Set the palette into vram! (with fades)
		uint16* vramPalette = &vram16[tex->paletteX + tex->paletteY * vramWidth];

		int fadeLevels = (tex->hasFade ? 8 : 1);
		for (int f = 0; f < fadeLevels; f++) {
			for (int p = 0; p < 16; p++) {
				vramPalette[vramWidth * f + p] = MAKECOLOR16(
					GETR32(palette[p]) + (127 - GETR32(palette[p])) * f / 8, 
					GETG32(palette[p]) + (127 - GETG32(palette[p])) * f / 8, 
					GETB32(palette[p]) + (127 - GETB32(palette[p])) * f / 8);

				if (vramPalette[vramWidth * f + p] == 0x8000)
					vramPalette[vramWidth * f + p] = 0;
			}
		}

		// Match bmp texture colours with the palette and set into vram!
		for (int j = i; j < objTexMap.numTextures; j++) {
			ObjTex* tex = &objTexMap.textures[j];

			if (tex->paletteX + tex->paletteY * vramWidth != paletteId)
				continue;
			if (objTexMap.textures[j].mapX + (objTexMap.textures[j].maxX - objTexMap.textures[j].minX) >= bmp.width || 
				objTexMap.textures[j].mapY + (objTexMap.textures[j].maxY - objTexMap.textures[j].minY) >= bmp.height)
				continue;

			for (int x = 0, width = tex->maxX - tex->minX + 1; x < width; x++) {
				for (int y = 0, height = tex->maxY - tex->minY + 1; y < height; y++) {
					uint32 bmpColour = bmp.data32[(tex->mapY + y) * bmp.width + (tex->mapX + x)];
					int shortestDist = 1000000;
					int paletteId = 0;

					for (int c = 0; c < 16; c++) {
						int rDelta = GETR32(palette[c]) - GETR32(bmpColour), gDelta = GETG32(palette[c]) - GETG32(bmpColour), bDelta = GETB32(palette[c]) - GETB32(bmpColour);
						int dist = rDelta * rDelta + gDelta * gDelta + bDelta * bDelta;

						if (dist < shortestDist) {
							shortestDist = dist;
							paletteId = c;
						}
					}

					if (x & 1)
						vram8[(tex->minY + y) * 2048 + (tex->minX + x)/2] = (vram8[(tex->minY + y) * 2048 + (tex->minX + x)/2] & 0x0F) | (paletteId << 4);
					else
						vram8[(tex->minY + y) * 2048 + (tex->minX + x)/2] = (vram8[(tex->minY + y) * 2048 + (tex->minX + x)/2] & 0xF0) |  paletteId;
				}
			}
		}

		NextPalette:
		continue;
	}

	free(colours);
	FreeBmp(&bmp);

	SetSnapshot(&vramSs);
}

void GenerateLQTextures() {
	if (!textures && !hqTextures)
		return;
	
	GetSnapshot(&vramSs);

	uint16* vram16 = (uint16*) vramSs.vram;
	uint8* vram8 = (uint8*) vramSs.vram;

	UpdatePaletteList();

	for (int pal = 0; pal < numPalettes; pal++) {
		if (paletteTypes[pal] != PT_LQ)
			continue;

		uint32 newPalette[16];
		uint32 texData[10][32*32];
		int texIds[10];
		int numTexs = 0;

		// Generate 24-bit LQ tex data from the HQ textures using this palette
		for (int j = 0; j < *numTextures && numTexs < 10; j++) {
			TexHq* curHq = textures ? textures[j].hqData : hqTextures[j].hqData;
			TexLq* curLq = textures ? textures[j].lqData : lqTextures[j].lqData;
			
			if (curLq->palettex * 16 * 2 + curLq->palettey * 4 * 2048 != palettes[pal])
				continue;
			if (texCaches[j].ignore)
				continue;

			uint32* curTexData = texData[numTexs];
			for (int tile = 0; tile < 4; tile++) {
				int paletteByteStart = curHq[tile].palette * 32;
				uint16* palette = &vram16[paletteByteStart / 2];

				int destXStart = TILETOX(tile)/2, destYStart = TILETOY(tile)/2;
				for (int y = 0; y < 16; y++) {
					for (int x = 0; x < 16; x++) {
						uint8* hqLine1 = (uint8*)&texCaches[j].tiles[tile].bitmap[(y * 2) * 32];
						uint8* hqLine2 = (uint8*)&texCaches[j].tiles[tile].bitmap[(y * 2 + 1) * 32];

						// Interpolate-scale
						// Note: this is the first time and hopefully last time I've ever used negative array indices (in the case that srcX is negative)
						curTexData[(destYStart + y) * 32 + destXStart + x] = MAKECOLOR32(
								(hqLine1[x*8] + hqLine1[x*8 + 4] + hqLine2[x*8] + hqLine2[x*8 + 4]) / 4,
								(hqLine1[x*8 + 1] + hqLine1[x*8 + 5] + hqLine2[x*8 + 1] + hqLine2[x*8 + 5]) / 4, 
								(hqLine1[x*8 + 2] + hqLine1[x*8 + 2] + hqLine2[x*8 + 6] + hqLine2[x*8 + 6]) / 4);
					}
				}
			}

			texIds[numTexs] = j;
			numTexs ++;
		}

		// Scan through the newly-generated LQ textures, collecting colours from all of them combined
		uint32 colours[2048];
		int numColours = 0;
		for (int t = 0; t < numTexs; t++) {
			for (int j = 0; j < 32*32 && numColours < 2048; j++) {
				for (int k = 0; k < numColours; k++) {
					if (texData[t][j] == colours[k])
						goto ColourAlreadyExists;
				}
				
				colours[numColours++] = texData[t][j];
				ColourAlreadyExists:;
			}
		}

		// Make the colour palette for the new texture
		MakePaletteFromColours(newPalette, 16, colours, numColours);

		if (numColours > 16)
			numColours = 16; // Numcolours has been reduced (not always the case, but here so)

		// Translate the palette to 16-bit and copy to the game's vram
		uint8* paletteR = &((uint8*)newPalette)[0], * paletteG = &((uint8*)newPalette)[1], * paletteB = &((uint8*)newPalette)[2];

		for (int i = 0; i < numColours; i ++) {
			vram16[palettes[pal]/2+i] = MAKECOLOR16(paletteR[i*4], paletteG[i*4], paletteB[i*4]);
			if (vram16[palettes[pal]/2+i] == 0x8000)
				vram16[palettes[pal]/2+i] = 0x0000;
		}

		// Convert all LQ texture data to the new palette
		for (int t = 0; t < numTexs; t++) {
			TexLq* curLq = textures ? textures[texIds[t]].lqData : lqTextures[texIds[t]].lqData;

			int startx = (curLq[0].region * 256) % 2048, starty = ((curLq[0].region & 0x1F) / 16 * 256);
			int x1, y1, x2, y2;

			x1 = 2048 + startx + curLq[0].xmin;
			x2 = 2048 + startx + curLq[0].xmax;
			y1 = starty + curLq[0].ymin;
			y2 = starty + curLq[0].ymax;

			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 32; x++) {
					// Grab our 32bit texture data and compare against the palette
					int colourR = texData[t][y*32+x] & 0xFF, colourG = texData[t][y*32+x] >> 8 & 0xFF, colourB = texData[t][y*32+x] >> 16 & 0xFF;
					int bestClr = 0;
					uint32 bestDist = 0xFFFFFFFF;
					for (int p = 0; p < numColours; p++) {
						int rDist = colourR - paletteR[p*4], gDist = colourG - paletteG[p*4], bDist = colourB - paletteB[p*4];
						int dist = rDist * rDist + gDist * gDist + bDist * bDist;

						if (dist < bestDist) {
							bestDist = dist;
							bestClr = p;
						}
					}

					// Store the result in the actual LQ texture data
					if (!(x&1))
						vram8[x1/2 + x/2 + (y1 + y) * 2048] = bestClr;
					else
						vram8[x1/2 + x/2 + (y1 + y) * 2048] |= bestClr << 4;
				}
			}
		}
	}
	
	SetSnapshot(&vramSs);
	CompleteLQPalettes();
}

void GetAvgTexColours()
{
	for (int i = 0; i < MAXAVGTEXCOLOURS; i ++)
	{
		for (int j = 0; j < 4; j ++)
			avgTexColours[i][j][0] = avgTexColours[i][j][1] = avgTexColours[i][j][2] = 0;
	}

	if (textures||lqTextures)
	{
		GetSnapshot(&vramSs);

		uint16* vram16 = (uint16*) vramSs.vram;
		uint8* vram8 = (uint8*) vramSs.vram;
		for (int i = 0; i < *numTextures && i < MAXAVGTEXCOLOURS; i ++)
		{
			TexHq* curHq = textures[i].hqData;

			if (!textures) curHq = hqTextures[i].hqData;

			for (int tile = 0; tile < 4; tile ++)
			{
				int paletteByteStart = curHq[tile].palette * 32;
				uint16* palette = &vram16[paletteByteStart / 2];
				int srcXStart = curHq[tile].GetXMin(), srcYStart = curHq[tile].GetYMin();
				
				int avgR = 0, avgG = 0, avgB = 0;
				for (int y = 0; y < 32; y ++)
				{
					uint8* line = &vram8[(y + srcYStart) * 2048 + srcXStart];
					for (int x = 0; x < 32; x ++)
					{
						avgR += (palette[line[x]] & 31) * 255 / 31;
						avgG += (palette[line[x]] >> 5 & 31) * 255 / 31;
						avgB += (palette[line[x]] >> 10 & 31) * 255 / 31;
					}
				}

				avgTexColours[i][tile][0] = avgR / (32*32) & 0xFF;
				avgTexColours[i][tile][1] = avgG / (32*32) & 0xFF;
				avgTexColours[i][tile][2] = avgB / (32*32) & 0xFF;
			}
		}
	}
}

void UpdatePaletteList()
{
	if (!textures&&!hqTextures)
		return;

	numPalettes = 0;

	int numTexs = *numTextures;
	bool found;
	for (int i = 0; i < numTexs; i++) {
		TexHq* curHq = textures[i].hqData;
		TexLq* curLq = textures[i].lqData;

		if (!textures) {
			curHq = hqTextures[i].hqData;
			curLq = lqTextures[i].lqData;
		}

		int lqPalette = curLq[0].palettex * 16 * 2 + curLq[0].palettey * 4 * 2048;
		int hqPalette[4] = {curHq[0].palette * 32, curHq[1].palette * 32, curHq[2].palette * 32, curHq[3].palette * 32};

		found = false;
		for (int j = 0; j < numPalettes; j++) {
			if (palettes[j] == lqPalette) {
				found = true;
				break;
			}
		}

		if (!found) {
			palettes[numPalettes] = lqPalette;
			paletteTypes[numPalettes ++] = PT_LQ;
		}

		for (int j = 0; j < 4; j++) {
			found = false;
			for (int k = 0; k < numPalettes; k++) {
				if (palettes[k] == hqPalette[j]) {
					found = true;
					break;
				}
			}

			if (!found) {
				palettes[numPalettes] = hqPalette[j];
				paletteTypes[numPalettes ++] = PT_HQ;
			}
		}
	}
}

void UpdateObjTexMap() {
	const int vramWidth = 1024, vramHeight = 512; // vram dimensions in u16s
	
	// Reset texmap vars
	objTexMap.width = 512;
	objTexMap.height = 32; // Unknown for now
	objTexMap.numTextures = 0;

	// Read texture data from object models in scene
	for (int i = 0; i < 768; i++) {
		uint32 modelAddress = mobyModels[i].address;
		bool hasAnims = (modelAddress & 0x80000000) != 0;
		if (!modelAddress)
			continue;

		modelAddress &= 0x003FFFFF;
		if ((umem32[modelAddress/4] & 0xFFFFFF00) || !umem32[modelAddress/4] || 
			(!hasAnims && (umem32[modelAddress/4+4] >> 24 != 0x80 || umem32[modelAddress/4+5] >> 24 != 0x80)) ||
			(hasAnims && (umem32[modelAddress/4+13] >> 24 != 0x80 || umem32[modelAddress/4+14] >> 24 != 0x80 || umem32[modelAddress/4+15] >> 24 != 0x80) && i > 0) || 
			(i == 0 && (umem32[modelAddress/4+15] >> 24 != 0x80 || umem32[modelAddress/4+14] != 0x1714)))
			continue;

		uint32* faces;
		if (hasAnims && i != 0)
			faces = &umem32[(umem32[modelAddress/4+14] & 0x003FFFFF) / 4];
		else if (!hasAnims)
			faces = &umem32[(umem32[modelAddress/4+4] & 0x003FFFFF) / 4];
		else if (i == 0)
			faces = ((SpyroModelHeader*)mobyModels[0])->anims[0]->faces;

		int faceIndex = 1;
		uint32 maxIndex = faces[0] / 4 + 1;

		if (maxIndex > 0x800)
			continue;

		while (faceIndex < maxIndex) {
			if ((faces[faceIndex + 1] & 0x80000000) || i == 0 /* special case for the Spyro model: always 20-byte faces */) {
				uint32 word0 = faces[faceIndex], word1 = faces[faceIndex + 1], word2 = faces[faceIndex + 2], word3 = faces[faceIndex + 3], word4 = faces[faceIndex + 4];
				int p1U = bitsu(word2, 0, 8), p1V = bitsu(word2, 8, 8), p2U = bitsu(word3, 0, 8),  p2V = bitsu(word3, 8, 8),
					p3U = bitsu(word4, 0, 8), p3V = bitsu(word4, 8, 8), p4U = bitsu(word4, 16, 8), p4V = bitsu(word4, 24, 8);
				int paletteX = bitsu(word2, 16, 6) * 0x10, paletteY = bitsu(word2, 22, 10) * 1; /* note: was paletteY = bitsu(word2,24,8) * 4 */
				int imageBlockX = ((word3 >> 16) & 0xF) * 0x40, imageBlockY = ((word3 >> 20) & 1) * 0x100;
				int minU = 256, minV = 256, maxU = 0, maxV = 0;

				if (paletteX + 16 > vramWidth || paletteY + 8 > vramHeight) {
					faceIndex += 5;
					continue;
				}

				if (p1U < minU) minU = p1U; if (p1V < minV) minV = p1V;
				if (p2U < minU) minU = p2U; if (p2V < minV) minV = p2V;
				if (p3U < minU) minU = p3U; if (p3V < minV) minV = p3V;
				if (p4U < minU) minU = p4U; if (p4V < minV) minV = p4V;
				if (p1U > maxU) maxU = p1U; if (p1V > maxV) maxV = p1V;
				if (p2U > maxU) maxU = p2U; if (p2V > maxV) maxV = p2V;
				if (p3U > maxU) maxU = p3U; if (p3V > maxV) maxV = p3V;
				if (p4U > maxU) maxU = p4U; if (p4V > maxV) maxV = p4V;
			
				bool foundMark = false;
				int minX = minU + (imageBlockX * 4), maxX = maxU + (imageBlockX * 4); // *4 to convert 16bit->4bit units
				int minY = minV + imageBlockY, maxY = maxV + imageBlockY;

				for (ObjTex* tex = objTexMap.textures, *eTex = &objTexMap.textures[objTexMap.numTextures]; tex < eTex; tex++) {
					if (tex->paletteX != paletteX || tex->paletteY != paletteY)
						continue;

					if ((minX >= tex->minX && minX <= tex->maxX && minY >= tex->minY && minY <= tex->maxY) || 
						(maxX >= tex->minX && maxX <= tex->maxX && minY >= tex->minY && minY <= tex->maxY) || 
						(minX >= tex->minX && minX <= tex->maxX && maxY >= tex->minY && maxY <= tex->maxY) || 
						(maxX >= tex->minX && maxX <= tex->maxX && maxY >= tex->minY && maxY <= tex->maxY))
					{
						foundMark = true;
						if (minX < tex->minX) tex->minX = minX;
						if (minY < tex->minY) tex->minY = minY;
						if (maxX > tex->maxX) tex->maxX = maxX;
						if (maxY > tex->maxY) tex->maxY = maxY;

						break;
					}
				}

				if (!foundMark) {
					ObjTex* tex = &objTexMap.textures[objTexMap.numTextures++];
					tex->minX = minX; tex->maxX = maxX;
					tex->minY = minY; tex->maxY = maxY;
					tex->paletteX = paletteX; tex->paletteY = paletteY;

					tex->hasFade = (i != 0);
				}

				faceIndex += 5;
			} else {
				faceIndex += 2;
				continue;
			}
		}
	}

	// Map the textures
	int bmpX = 32, bmpY = 0;
	int maxRowHeight = 0;

	for (int i = 0; i < objTexMap.numTextures; i++) {
		ObjTex* tex = &objTexMap.textures[i];

		if (bmpX + tex->maxX - tex->minX + 1 > objTexMap.width) {
			bmpX = 0;
			bmpY += maxRowHeight;
			maxRowHeight = 0;
		}

		objTexMap.textures[i].mapX = bmpX;
		objTexMap.textures[i].mapY = bmpY;

		bmpX += tex->maxX - tex->minX + 1;

		if (tex->maxY - tex->minY + 1 > maxRowHeight)
			maxRowHeight = tex->maxY - tex->minY + 1;
	}

	objTexMap.height = bmpY + maxRowHeight;
}

void CompleteLQPalettes()
{
	GetSnapshot(&vramSs);
	UpdatePaletteList();

	uint16* vram16 = (uint16*) vramSs.vram;
	for (int i = 0; i < numPalettes; i ++)
	{
		if (paletteTypes[i] != PT_LQ)
			continue;

		uint16* palette = &vram16[palettes[i] / 2];
		for (int y = 1; y < 16; y ++)
		{
			for (int x = 0; x < 16; x ++)
			{
				int r = (palette[x] & 31) * 255 / 31, g = (palette[x] >> 5 & 31) * 255 / 31, b = (palette[x] >> 10 & 31) * 255 / 31;
				palette[y * 1024 + x] = MAKECOLOR16(r + (127 - r) * y / 15, g + (127 - g) * y / 15, b + (127 - b) * y / 15);
			}
		}
	}

	SetSnapshot(&vramSs);
}

// BMP stuff
bool LoadBmp32(BmpInfo* bmpOut, const char* bmpName) {
	HANDLE bmpIn = CreateFile(bmpName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (bmpIn == INVALID_HANDLE_VALUE)
		return false;

	BITMAPFILEHEADER head;
	BITMAPINFOHEADER info;
	DWORD readBytes1, readBytes2, readBytes3;
	int bmpTop, bmpDir;
	uint32* bmpData = NULL;
	uint8* bmpLine = NULL;
	uint32* tempColours = NULL;

	ReadFile(bmpIn, &head, sizeof (BITMAPFILEHEADER), &readBytes1, NULL);
	ReadFile(bmpIn, &info, sizeof (BITMAPINFOHEADER), &readBytes2, NULL);

	if (readBytes1 != sizeof (head) || readBytes2 != sizeof (info) || (info.biBitCount != 8 && info.biBitCount != 16 && info.biBitCount != 24 && info.biBitCount != 32))
		goto Fail;

	int texPerRow = info.biWidth / 64, numRows = info.biHeight / 64;
	int width = info.biWidth, height = info.biHeight;
	int bitCount = info.biBitCount;

	if (height > 0) {
		bmpTop = info.biHeight - 1;
		bmpDir = -1;
	} else {
		bmpTop = 0;
		bmpDir = 1;
		height = -height;
		numRows = -numRows;
	}
	
	// Read the bitmap data
	bmpData = (uint32*) malloc(width * height * 4);
	bmpLine = (uint8*) malloc(width * bitCount / 8);

	if (bitCount == 32) {
		for (int i = 0; i < height; i++) {
			ReadFile(bmpIn, bmpLine, width * 4, &readBytes3, NULL);

			if (readBytes3 != width * 4)
				goto Fail;

			for (int j = 0, dataStart = (bmpTop + (i * bmpDir)) * width; j < width; j++)
				bmpData[dataStart + j] = (((uint32*)bmpLine)[j] & 0xFF00FF00) | (bmpLine[j*4] << 16) | (bmpLine[j*4+2]);
		}
	} else if (bitCount == 24) {
		for (int i = 0; i < height; i++) {
			ReadFile(bmpIn, bmpLine, width * 3, &readBytes3, NULL);

			if (readBytes3 != width * 3)
				goto Fail;

			for (int j = 0, dataStart = (bmpTop + (i * bmpDir)) * width; j < width; j++)
				bmpData[dataStart + j] = MAKECOLOR32(bmpLine[j * 3 + 2], bmpLine[j * 3 + 1], bmpLine[j * 3]); // RGB reversed
		}
	} else if (bitCount == 16) {
		for (int i = 0; i < height; i++) {
			ReadFile(bmpIn, bmpLine, width * 2, &readBytes3, NULL);

			if (readBytes3 != width * 2)
				goto Fail;

			for (int j = 0, dataStart = (bmpTop + (i * bmpDir)) * width; j < width; j++)
				bmpData[dataStart + j] = MAKECOLOR32(GETB16(((uint16*) bmpLine)[j]), GETG16(((uint16*) bmpLine)[j]), GETR16(((uint16*) bmpLine)[j])); // RGB reversed
		}
	} else if (bitCount == 8) {
		RGBQUAD palette[256];

		ReadFile(bmpIn, palette, 256 * sizeof (RGBQUAD), &readBytes3, NULL);

		if (readBytes3 != 256 * sizeof (RGBQUAD))
			goto Fail;

		for (int i = 0; i < height; i++) {
			ReadFile(bmpIn, bmpLine, width, &readBytes3, NULL);

			if (readBytes3 != width)
				goto Fail;

			for (int j = 0, dataStart = (bmpTop + (i * bmpDir)) * width; j < width; j++)
				bmpData[dataStart + j] = MAKECOLOR32(palette[bmpLine[j]].rgbRed, palette[bmpLine[j]].rgbGreen, palette[bmpLine[j]].rgbBlue);
		}
	}

	free(bmpLine);

	bmpOut->width = width;
	bmpOut->height = height;
	bmpOut->bitsPerColour = 32;
	bmpOut->data32 = bmpData;

	CloseHandle(bmpIn);
	return true;

Fail:
	free(bmpData);
	free(bmpLine);
	CloseHandle(bmpIn);
	return false;
}

bool SaveBmp(BmpInfo* info, const char* bmpName) {
	DWORD nil;
	BITMAPFILEHEADER fh;
	BITMAPINFO bi;
	int bmpPitch = 0, paletteNum = 0;

	if (info->bitsPerColour == 4) {
		bmpPitch = info->width / 2;
		paletteNum = 16;
	} else if (info->bitsPerColour == 8) {
		bmpPitch = info->width;
		paletteNum = 256;
	} else if (info->bitsPerColour == 16)
		bmpPitch = (info->width * 2 + 1) * 2 / 2;
	else if (info->bitsPerColour == 24)
		bmpPitch = (info->width + 3) * 4 / 4;
	else
		return false;

	memset(&bi.bmiHeader, 0, sizeof (BITMAPINFO));
	bi.bmiHeader.biSize = sizeof (BITMAPINFO)-4;
	bi.bmiHeader.biSizeImage = 0;//totalWidth * bmpPitch;
	bi.bmiHeader.biWidth = info->width;
	bi.bmiHeader.biHeight = -(int)info->height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = info->bitsPerColour;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biXPelsPerMeter = 0;
	bi.bmiHeader.biYPelsPerMeter = 0;
	bi.bmiHeader.biClrUsed = paletteNum;
	bi.bmiHeader.biClrImportant = 0;

	fh.bfSize = sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFO)-4 + info->height * bmpPitch;
	fh.bfOffBits = sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFO)-4;
	fh.bfType = 'B' | ('M' << 8);
	fh.bfReserved1 = fh.bfReserved2 = 0;

	HANDLE bmpOut = CreateFile(bmpName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	WriteFile(bmpOut, &fh, sizeof (BITMAPFILEHEADER), &nil, NULL);
	WriteFile(bmpOut, &bi, sizeof (BITMAPINFO)-4, &nil, NULL);
	WriteFile(bmpOut, info->palette, paletteNum * sizeof (RGBQUAD), &nil, NULL);
	WriteFile(bmpOut, info->data8, bmpPitch * info->height, &nil, NULL);
	CloseHandle(bmpOut);

	return true;
}

void FreeBmp(BmpInfo* info) {
	free(info->data32);
	info->data32 = NULL;
}