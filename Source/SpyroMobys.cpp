#include <Windows.h>
#include <string>
#include "SpyroData.h"

void UpdateSpyroDestroyer() {
	if (!spyro || !mobyModels[0].address) {
		return;
	}

	SpyroModelHeader* model = (SpyroModelHeader*)mobyModels[0];
	SpyroAnimHeader* anim = model->anims[spyro->anim.nextAnim];
	SpyroFrameInfo* frame = &anim->frames[spyro->anim.nextFrame];

	uint32* spyroVerts = anim->verts, *spyroFaces = anim->faces, *spyroColours = anim->colours;
	uint16* blocks = &anim->data[frame->blockOffset / 2];
	uint8* adjusts = &((uint8*)anim->data)[frame->blockOffset + ((frame->word1 >> 10) & 0x3FF)];
	IntVector vertPositions[80] = {0};
	IntVector _initialVertPositions[80] = {0};
	IntVector* initialVertPositions = &_initialVertPositions[1];

#define longblock(x, y, z, bit) *(uint32*)(&blocks[(blockIndex += 2) - 2]) = bitsout(z, 2, 10) | bitsout(y, 12, 10) | bitsout(x, 22, 10) | 2 | (bit&1)
#define shortblock(x, y, z, bit) blocks[blockIndex++] = bitsout(z, 2, 4) | bitsout(y, 6, 5) | bitsout(x, 11, 5) | (bit&1)

	/* unk1 & 1: whether anim starts on first block */

	spyro->anim.nextAnim = 0;
	spyro->anim.nextFrame = 0;
	spyro->anim.prevAnim = 0;
	spyro->anim.prevFrame = 0;

	uint32 frameAddr = (uintptr)frame - (uintptr)umem32;
	uint32 blockAddr = (uintptr)blocks - (uintptr)umem32;
	frame->unk1 |= 1;
	anim->numFrames = 1;
	spyroFaces[0] = 0;

	uint8 colour;
	uint8* adjust = adjusts;
	uint16* block = blocks;
	uint32* vert = spyroVerts;
	IntVector pos = {0, 0, 0};

	for (int i = 0; i < anim->numVerts; ++i) {
		spyroVerts[i] = 0;
	}

	auto writeblock = [&](uint32 val, bool isLarge) {
		*(block++) = val & 0xFFFF;
		if (isLarge) {
			*(block++) = (val >> 16) & 0xFFFF;
		}
	};

	auto writebase = [&](int32 x, int32 y, int32 z, bool bit) {
		// Write position
		//*block = 1;
		*vert = bitsout(x - pos.x, 21, 11) | bitsout(y - pos.y, 10, 11) | bitsout(z - pos.z, 0, 10);

		// Read back position
		pos = {(int16)x, (int16)y, (int16)z};
		/*if (!(*block & 1)) {
			*adjust |= 1;
			uint32 adjustment = mobyVertAdjustTable[*(adjust++) >> 1];
			pos.x += bitss(adjustment, 21, 11);
			pos.y += bitss(adjustment, 10, 11);
			pos.z += bitss(adjustment, 0, 10);
		}*/

		vertPositions[vert - spyroVerts] = pos;
		initialVertPositions[vert - spyroVerts] = pos;

		// Progress
		vert++;
	};

	auto writerelative = [&](int32 x, int32 y, int32 z, bool bit) {
		// Write position
		*block = bitsout((z - pos.z)/4, 2, 4) | bitsout((y - pos.y)/4, 6, 5) | bitsout((x - pos.x)/4, 11, 5) | (bit>0);

		// Read back position
		pos = {(int16)pos.x + bitss(*block, 11, 5)*4, (int16)pos.y + bitss(*block, 6, 5)*4, (int16)pos.z + bitss(*block, 2, 4)*4};
		/*if (!(*block & 1)) {
			*adjust |= 1;
			uint32 adjustment = mobyVertAdjustTable[*(adjust++) >> 1];
			pos.x += bitss(adjustment, 21, 11);
			pos.y += bitss(adjustment, 10, 11);
			pos.z += bitss(adjustment, 0, 10);
		}*/

		vertPositions[vert - spyroVerts] = pos;

		// Progress
		block++;
		vert++;
	};

	auto writeabsolute = [&](int32 x, int32 y, int32 z, bool bit) {
		// Write position
		*(uint32*)block = bitsout(-z/2, 2, 10) | bitsout(-y/2, 12, 10) | bitsout(x/2, 22, 10) | 2 | (bit>0);
		
		// Read back position
		pos = {bitss(*(uint32*)block, 22, 10)*2, -bitss(*(uint32*)block, 12, 10)*2, -bitss(*(uint32*)block, 2, 10)*2}; /* for Z, both (2,10)*2 and (1,11) are seemingly equivalent */
		/*if (!(*block & 1)) {
			*adjust |= 1;
			uint32 adjustment = mobyVertAdjustTable[*(adjust++) >> 1];
			pos.x += bitss(adjustment, 21, 11);
			pos.y += bitss(adjustment, 10, 11);
			pos.z += bitss(adjustment, 0, 10);
		}*/

		vertPositions[vert - spyroVerts] = pos;

		// Progress
		vert++;
		block += 2;
	};

	auto writeface = [&]() {
		uint8 colour = spyroFaces[0] / 8;
		int numVerts = vert - spyroVerts;
		int faceIndex = 1 + spyroFaces[0] / 4;
		spyroFaces[faceIndex] = ((numVerts - 4) << 8) | (numVerts - 3) | ((numVerts - 2) << 16) | ((numVerts - 1) << 24);
		//spyroFaces[1+spyroFaces[0] / 4 + 1] = (colour << 10) | (colour << 3) | (colour << 17) | (colour << 24);
		spyroFaces[faceIndex + 1] = (1 << 3) | (0 << 10) | (2 << 17) | (3 << 24);
		spyroFaces[faceIndex + 2] = 0;
		spyroFaces[faceIndex + 3] = 0;
		spyroFaces[faceIndex + 4] = 0;
		spyroFaces[0] += 20;
	};
	auto duplicate = [&](int offsetX, int offsetY, int offsetZ) {
		int numFaces = spyroFaces[0] / 8;
		int numVerts = vert - spyroVerts;
		for (int i = 0; i < numFaces; ++i) {
			uint8 v1 = spyroFaces[1+i*2] >> 8, v2 = spyroFaces[1+i*2], v3 = spyroFaces[1+i*2] >> 16, v4 = spyroFaces[1+i*3] >> 24;
			writeabsolute(vertPositions[v1].x + offsetX, vertPositions[v1].y + offsetY, vertPositions[v1].z + offsetZ, 1);
			writeabsolute(vertPositions[v2].x + offsetX, vertPositions[v2].y + offsetY, vertPositions[v2].z + offsetZ, 1);
			writeabsolute(vertPositions[v3].x + offsetX, vertPositions[v3].y + offsetY, vertPositions[v3].z + offsetZ, 1);
			writeabsolute(vertPositions[v4].x + offsetX, vertPositions[v4].y + offsetY, vertPositions[v4].z + offsetZ, 1);
			writeface();
		}
	};
	auto duplicateasbase = [&](int offsetX, int offsetY, int offsetZ) {
		int numFaces = spyroFaces[0] / 8;
		int numVerts = vert - spyroVerts;
		writeabsolute(0, 0, 0, true);
		for (int i = 0; i < numFaces; ++i) {
			uint8 v1 = spyroFaces[1+i*2] >> 8, v2 = spyroFaces[1+i*2], v3 = spyroFaces[1+i*2] >> 16, v4 = spyroFaces[1+i*3] >> 24;
			writebase(vertPositions[v1].x + offsetX, vertPositions[v1].y + offsetY, vertPositions[v1].z + offsetZ, 1);
			writebase(vertPositions[v2].x + offsetX, vertPositions[v2].y + offsetY, vertPositions[v2].z + offsetZ, 1);
			writebase(vertPositions[v3].x + offsetX, vertPositions[v3].y + offsetY, vertPositions[v3].z + offsetZ, 1);
			writebase(vertPositions[v4].x + offsetX, vertPositions[v4].y + offsetY, vertPositions[v4].z + offsetZ, 1);
			writeface();
		}
	};

	// In Base Verts: X is positive. Y is positive. Z is positive.
	// In Absolute Blocks: X is positive. Y is negative. Z is negative.
	// Reference square (XY)
	int random[] = {345, 2346, 26, 357, 4567, 45, 347, 24577, 24537, 34574, 48, 3265, 37756, 456734, 245674, 4275, 36, 4467};
	int curRand = 0;
	auto getrandom = [&](int min, int max){return min + random[(curRand++)%sizeof(random)/sizeof(random[0])] % (max + 1 - min);};

	// if unk1 | 1, the first vertex can be a writeabsolute
	// else, the first vertex is always the first vertex in the vert list

	writebase(-64, 64, 64, 1);
	writebase(-64, -64, 64, 1);
	writebase(-64, -64, 0, 1);
	writebase(-64, 64, 0, 1);
	writeface();

	writebase(-64, 64, 0, 1);
	writebase(-64, -64, 0, 1);
	writebase(-64, -64, -64, 1);
	writebase(-64, 64, -64, 1);
	writeface();;
	
	writeblock(0, true);
	writeblock(0, true);

	/* Interesting notes: 
		If I make a bunch of relative verts, and I duplicate it as a base, it shifts compared to duplicating it absolutely
	*/

	for (int i = 0; i < vert - spyroVerts; ++i) {
		adjusts[i] = 0x7C;
	}
	adjusts[0] = 0x7D;
	adjusts[1] = 0x7D;
	adjusts[2] = 0x7D;
	adjusts[3] = 0x7D;
	adjusts[4] = 0x7C;
	adjusts[5] = 0x7C;
	adjusts[6] = 0x7C;
	adjusts[7] = 0x7C;
	adjusts[8] = 0x7C;

	spyroColours[0] = 0x000000FF; // AABBGGRR depiction
	spyroColours[1] = 0x0000FF00;
	spyroColours[2] = 0x00FF0000;
	spyroColours[3] = 0x00000000;

}

/* Behold the Spyro model extraction code that doesn't work and I'mma fix 
	if (!spyro || !mobyModels[0].address)
		return;

	SpyroModelHeader* model = (SpyroModelHeader*)mobyModels[0];
	SpyroAnimHeader* anim = model->anims[spyro->main_anim.nextanim];
	SpyroFrameInfo* frame = &anim->frames[spyro->main_anim.nextframe];

	uint16* blocks = NULL;
	uint8* adjusts = NULL;
	uint32* verts, *faces, *colours;
	int numVerts = 0, numColours = 0;
	int maxNumFaces = 0;
	float scale = MOBYPOSSCALE;
	int animId = spyro->main_anim.nextanim, animFrameIndex = spyro->main_anim.nextframe;

	verts = anim->verts; faces = anim->faces; colours = anim->colours;
	numVerts = anim->numVerts; numColours = anim->numColours;
	maxNumFaces = faces[0] / 8;

	if (numVerts >= 256 || numColours >= 256 || maxNumFaces >= 900)
		return;

	// Get frame vertices
	Vec3* drawVerts = writeGameState->spyro.model.verts;
	
	for (int i = 0; i < numVerts; i++)
		drawVerts[i].Zero();

	float animProgress = (float)(spyro->animprogress & 0x0F) / (float)(spyro->animprogress >> 4 & 0x0F);

	for (int frameIndex = 0; frameIndex < 2; frameIndex++) {
		int curX = 0, curY = 0, curZ = 0;
		int blockX = 0, blockY = 0, blockZ = 0;
		int headX = 0, headY = 0, headZ = 0;
		int headVert = 0;
		int curBlock = 0;
		bool nextBlock = true, blockVert = false;
		int adjustVert = 0;
	
		float lerp = (frameIndex == 0 ? 1.0f - animProgress : animProgress);

		if (lerp == 0.0f)
			continue;
			
		model = (SpyroModelHeader*)mobyModels[0];
		anim = model->anims[frameIndex == 0 ? spyro->main_anim.prevanim : spyro->main_anim.nextanim];
		frame = &anim->frames[frameIndex == 0 ? spyro->main_anim.prevframe : spyro->main_anim.nextframe];

		verts = anim->verts;
		blocks = &anim->data[frame->dataOffset / 2];
		adjusts = &((uint8*)anim->data)[frame->dataOffset + ((frame->word1 >> 10) & 0x3FF)];
		headVert = anim->headVertStart;
		headX = bitss(frame->headPos, 21, 11); headY = bitss(frame->headPos, 10, 11); headZ = bitss(frame->headPos, 0, 10) / 2;

		if (0  & 2) { // TODO..??
			continue;
			curX = 0; curY = 0; curZ = 0;
			nextBlock = true;
		} else {
			curX = bitss(verts[0], 21, 11);
			curY = bitss(verts[0], 10, 11);
			curZ = bitss(verts[0], 0, 10);
			curX = curY = curZ = 0;
			nextBlock = false;
		}

		nextBlock = false; // spyro seems to start w/o block
		for (int i = 0; i < numVerts; i++) {
			bool usedBlock = false;
			int x = bitss(verts[i], 21, 11), y = bitss(verts[i], 10, 11), z = bitss(verts[i], 0, 10);

			x = sra(verts[i], 21);
			y = sra(verts[i] << 11, 21);
			z = sra(verts[i] << 22, 21);

#if 1
			if (nextBlock) {
				blockVert = (blocks[curBlock] & 1) != 0;

				nextBlock = blockVert;
				if (blocks[curBlock] & 0x2) {
					// Long block
					uint32 pos = blocks[curBlock] | (blocks[curBlock + 1] << 16);

					int tZ = bitss(pos, 1, 11), tY = bitss(pos, 12, 11) * 2, tX = bitss(pos, 21, 11);

					curX = tX; curY = tY; curZ = tZ;

					int16 lower = blocks[curBlock], upper = blocks[curBlock + 1];
					if (upper >= 0x8000)
						upper |= 0xFFFF0000;

					curZ = sra(lower << 20, 21);
					lower &= 0xF000;
					curY = ((uint16)lower >> 11) | sra(upper << 26, 21);
					curX = sra(upper, 5);

					x = 0; y = 0; z = 0;
					curBlock += 2;
				} else {
					// Short block
					// x += ? >> 9, y -= ? << 21 >> 25, z -= ? << 26 >> 27 << 2
					int tZ = bitss(blocks[curBlock], 1, 3), tY = bitss(blocks[curBlock], 4, 7), tX = bitss(blocks[curBlock], 9, 7);

					tX = sra((int16)blocks[curBlock], 9);
					tY = sra(blocks[curBlock] << 21, 25);
					tZ = sra(blocks[curBlock] << 26, 27);

					curX += tX; curY -= tY; curZ -= tZ * 4;
					curBlock += 1;
				}

				usedBlock = true;
			}
		
			if (!usedBlock && !blockVert) {
				int32 adjusted = mobyVertAdjustTable[(adjusts[adjustVert] >> 1)] + verts[i];

				if (adjusts[adjustVert] & 1)
					nextBlock = true;
				
				x = sra(adjusted, 21);
				y = sra(adjusted << 11, 21);
				z = sra(adjusted << 22, 21);
				adjustVert++;
			}
#endif
			if (i == headVert) {
				break;
				curX = headX; curY = headY; curZ = -headZ;
			}

			curX += x; curY -= y; curZ -= z; // x = fwd/back y = left/right z = up/down
			//if (i == 0) {curX = 0x3f; curY = 0x66; curZ = 0x16E;}
			drawVerts[i] += Vec3((float)curX * scale * lerp, (float)curY * scale * lerp, (float)-curZ * scale * lerp);
		}
	}

	// Set model faces
	int32 tempU[512], tempV[512];

	int numSides = 0, numUvs = 0, numUvIndices = 0, index = 1, maxIndex = faces[0] / 4 + 1;
	int curFace = 0;
	
	while (index < maxIndex) {
		DrawFace* drawFace = &writeGameState->spyro.model.faces[curFace++];
		uint32 face = faces[index];
		uint32 clrs = faces[index + 1];
		int numFaceSides = ((face & 0xFF) != ((face >> 8) & 0xFF) ? 4 : 3);
		
		if (numFaceSides == 4) {
			drawFace->verts[0] = face >> 8 & 0xFF;
			drawFace->verts[1] = face & 0xFF;
			drawFace->verts[2] = face >> 16 & 0xFF;
			drawFace->verts[3] = face >> 24 & 0xFF;
			
			drawFace->colours[0] = clrs >> 10 & 0x7F;
			drawFace->colours[1] = clrs >> 3 & 0x7F;
			drawFace->colours[2] = clrs >> 17 & 0x7F;
			drawFace->colours[3] = clrs >> 24 & 0x7F;
		} else {
			drawFace->verts[0] = face >> 24 & 0xFF;
			drawFace->verts[1] = face >> 16 & 0xFF;
			drawFace->verts[2] = face >> 8 & 0xFF;
			drawFace->colours[0] = clrs >> 24 & 0x7F;
			drawFace->colours[1] = clrs >> 17 & 0x7F;
			drawFace->colours[2] = clrs >> 10 & 0x7F;
		}
		
		for (int i = 0; i < numFaceSides; i++) {
			if (drawFace->colours[i] >= numColours)
				drawFace->colours[i] = 0;
		}
		drawFace->numSides = numFaceSides;

		if (1) {
			uint32 word0 = faces[index], word1 = faces[index + 1], word2 = faces[index + 2], word3 = faces[index + 3], word4 = faces[index + 4];
			int p1U = bitsu(word4, 16, 8), p1V = bitsu(word4, 24, 8), p2U = bitsu(word4, 0, 8), p2V = bitsu(word4, 8, 8), 
				p3U = bitsu(word3, 0, 8),  p3V = bitsu(word3, 8, 8),  p4U = bitsu(word2, 0, 8), p4V = bitsu(word2, 8, 8);
			int imageBlockX = ((word3 >> 16) & 0xF) * 0x40, imageBlockY = ((word3 >> 20) & 1) * 0x100;
			int p1X = p1U + (imageBlockX * 4), p1Y = p1V + imageBlockY, p2X = p2U + (imageBlockX * 4), p2Y = p2V + imageBlockY, p3X = p3U + (imageBlockX * 4),
				p3Y = p3V + imageBlockY, p4X = p4U + (imageBlockX * 4), p4Y = p4V + imageBlockY;
			bool gotTextures = false;

			for (int i = 0; i < objTexMap.numTextures; i++) {
				if (objTexMap.textures[i].minX <= p1X && objTexMap.textures[i].minY <= p1Y && objTexMap.textures[i].maxX >= p1X && objTexMap.textures[i].maxY >= p1Y) {
					ObjTex* tex = &objTexMap.textures[i];
					const int w = objTexMap.width, h = objTexMap.height;
					int pX[4] = {p2X, p1X, p3X, p4X}, pY[4] = {p2Y, p1Y, p3Y, p4Y};
					//int pX[4] = {p1X, p3X, p2X, p4X}, pY[4] = {p1Y, p3Y, p2Y, p4Y};

					for (int j = 0; j < numFaceSides; j++) {
						drawFace->u[j] = (float) (tex->mapX + pX[j] - objTexMap.textures[i].minX) / 512.0f;
						drawFace->v[j] = (float) (tex->mapY + pY[j] - objTexMap.textures[i].minY) / 512.0f;
					}

					gotTextures = true;
					break;
				}
			}

			if (!gotTextures) {
				for (int i = 0; i < numFaceSides; i++) {
					drawFace->u[i] = 8.0f / (float) 512.0f;
					drawFace->v[i] = 8.0f / (float) 512.0f;
				}
			}

			index += 5; // textured face, continue
		} else {
			for (int i = 0; i < numFaceSides; i++) {
				drawFace->u[i] = 8.0f / (float) 512.0f;
				drawFace->v[i] = 8.0f / (float) 512.0f;
			}

			index += 2; // no textures, continue
		}
	}

	// Set colours
	uint32* drawColours = writeGameState->spyro.model.colours;

	for (int i = 0; i < numColours; i++)
		drawColours[i] = colours[i] | 0xFF000000;
	*/

void SaveMobys() {
	if (!mobys) {
		return;
	}

	char filename[MAX_PATH];
	GetLevelFilename(filename, SEF_MOBYLAYOUT, true);
	HANDLE file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return;

	std::string fileData;

	fileData.reserve(15200);

	for (int i = 0; i < 500; ++i) {
		if (mobys[i].state == -1) {
			break;
		}

		char tempBuffer[250];
		sprintf(tempBuffer, "X: %i, Y: %i, Z: %i, RotX: %i, RotY: %i, RotZ: %i\x0D\x0A", mobys[i].x, mobys[i].y, mobys[i].z, mobys[i].angle.x, mobys[i].angle.y, mobys[i].angle.z);

		fileData.append(tempBuffer);
	}
	
	DWORD written;
	WriteFile(file, fileData.c_str(), fileData.length(), &written, nullptr);

	CloseHandle(file);
}

void LoadMobys() {
	if (!mobys) {
		return;
	}

	char filename[MAX_PATH];
	GetLevelFilename(filename, SEF_MOBYLAYOUT, true);
	HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return;

	char* fileData = new char[GetFileSize(file, nullptr) + 1];
	DWORD read;
	ReadFile(file, fileData, GetFileSize(file, nullptr), &read, nullptr);
	fileData[GetFileSize(file, nullptr)] = '\0';

	std::string fileString(fileData);
	int index = 0;

	for (int i = 0; i < 500; ++i) {
		if (mobys[i].state == -1) {
			break;
		}
		
		index = fileString.find_first_not_of(" \r\n", index);

		while (index >= 0 && fileString[index] != '\r' && fileString[index] != '\n' && fileString[index]) {
			int tagEnd = fileString.find_first_of(":", index);
			int valueEnd = fileString.find_first_of(",\r\n", tagEnd);
			const char* const dur = &fileString.c_str()[index];
			auto interpretParams = [&](auto& nameArray, auto& ptrArray) {
				// What the heck am I writing
				for (int i = 0; i < nameArray.size(); ++i) {
					if (!fileString.compare(index, tagEnd - index, nameArray.begin()[i])) {
						*(ptrArray.begin()[i]) = std::stoi(fileString.substr(fileString.find_first_not_of(" ", tagEnd + 1), valueEnd - index));
						break;
					}
				}
			};
			
			// omg isn't this too easy now? C++11 is magic
			interpretParams(std::initializer_list<const char*>{"X", "Y", "Z"}, std::initializer_list<int*>{&mobys[i].x, &mobys[i].y, &mobys[i].z});
			interpretParams(std::initializer_list<const char*>{"RotX", "RotY", "RotZ"}, std::initializer_list<int8*>{&mobys[i].angle.x, &mobys[i].angle.y, &mobys[i].angle.z});
			
			int lastIndex = index;
			index = fileString.find_first_not_of(", ", valueEnd);

			const char* debug = &fileString.c_str()[lastIndex];
			const char* debug2 = &fileString.c_str()[index];
		
			if (index < 0) {
				goto EndFunction;
			}
		}

		if (index < 0 || (fileString[index] != '\n' && fileString[index] != '\r')) {
			break;
		}
	}

	EndFunction:;
	delete[] fileData;

	CloseHandle(file);
}