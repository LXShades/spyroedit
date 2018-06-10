#include "GenObject.h"
#include "SpyroTextures.h"
#include "SpyroScene.h"
#include "SpyroGenesis.h"
#include "SpyroLiveGen.h"
#include "Window.h" // isGenesisPageValid

Scene scene;

extern bool colltreeInvalid;

uint32 FindFreeMemory(int sectorSize);

void Scene::OnLevelEntry() {
	// Initialise GenSectors and validate all
	if (spyroScene) {
		for (int s = 0; s < spyroScene->numSectors; s++)
			ConvertSpyroToGen(s);

		genScene->ValidateAllObjects();
	}

	// Make a snapshot of the initial scene
	originalScene.CopySnapshot(spyroScene);

	// Update UI
	isGenesisPageValid = false;
}

void Scene::ConvertSpyroToGen(int sectorId) {
	if (!genScene || !spyroScene) {
		return;
	}

	SceneSectorHeader* sector = spyroScene->sectors[sectorId];
	GenMod* genMod = genScene->GetModById(GENID_SCENESECTORMOD + sectorId);
	GenMesh* genSector = genMod ? genMod->GetMesh() : nullptr;

	if (!genSector)
		return;

	int numVerts = sector->numHpVertices;
	int hpVertexStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2;
	int hpColourStart = hpVertexStart + numVerts;
	int hpFaceStart = hpColourStart + sector->numHpColours * 2;
	int x = sector->xyPos >> 16, y = (sector->xyPos & 0xFFFF), z = ((sector->zPos >> 14) & 0xFFFF) >> 2;
	float fX = (float) x * WORLDSCALE, fY = (float) y * WORLDSCALE, fZ = (float) z * WORLDSCALE;
	bool flatSector = (sector->centreRadiusAndFlags >> 12 & 1);
	SceneFace* faces = (SceneFace*) &sector->data32[hpFaceStart];

	// Copy verts
	genSector->SetNum(GENMET_VERT, sector->numHpVertices);

	GenMeshVert* genVerts = genSector->LockVerts();
	for (int i = 0; i < sector->numHpVertices; i++) {
		uint32 curVertex = sector->data32[hpVertexStart + i];
		int curX = (curVertex >> 19 & 0x1FFC) >> 2, curY = (curVertex >> 8 & 0x1FFC) >> 2, curZ = (curVertex << 3 & 0x1FFC) >> 2;

		if (game == SPYRO1) {
			curX = (curVertex >> 19 & 0x1FFC) >> 2; curY = (curVertex >> 8 & 0x1FFC) >> 2; curZ = (curVertex << 3 & 0x1FFC) >> 3;
		}

		if (flatSector)
			curZ >>= 3;

		genVerts[i].pos.x = (float) curX * WORLDSCALE + fX;
		genVerts[i].pos.y = (float) curY * WORLDSCALE + fY;
		genVerts[i].pos.z = (float) curZ * WORLDSCALE + fZ;
	}

	genSector->UnlockVerts(genVerts);

	// Copy colours
	genSector->SetNum(GENMET_COLOUR, sector->numHpColours);
	
	genu32* genColours = genSector->LockColours();
	for (int i = 0; i < sector->numHpColours; i++)
		genColours[i] = sector->data32[hpColourStart + i] | 0xFF000000;
	genSector->UnlockColours(genColours);

	// Count UVs
	int32 uvByTexture[300];
	const int textureHeight = (*numTextures + TEXTURESPERROW - 1) / TEXTURESPERROW * 64;
	int numUsedTextures = 0;

	for (int i = 0; i < *numTextures; i++)
		uvByTexture[i] = -1;
	for (int i = 0; i < sector->numHpFaces; i++) {
		uint32 texId = faces[i].GetTexture();

		if (uvByTexture[texId] == -1) {
			uvByTexture[texId] = numUsedTextures * 4;
			numUsedTextures++;
		}
	}

	// Generate UVs
	genSector->SetNum(GENMET_UV, numUsedTextures * 4);

	GenVec2* genUvs = genSector->LockUvs();

	for (int i = 0; i < *numTextures; i++) {
		if (uvByTexture[i] != -1) {
			uint32 texX1 = (i * 64) % TEXTUREROWLENGTH, texX2 = texX1 + 64, 
				   texY1 = (i / TEXTURESPERROW) * 64, texY2 = texY1 + 64;

			int curUv = uvByTexture[i];
			genUvs[curUv + 0].x = (float) texX1 / (float) TEXTUREROWLENGTH;
			genUvs[curUv + 0].y = (float) texY1 / (float) textureHeight;
			genUvs[curUv + 1].x = (float) texX2 / (float) TEXTUREROWLENGTH;
			genUvs[curUv + 1].y = (float) texY1 / (float) textureHeight;
			genUvs[curUv + 2].x = (float) texX2 / (float) TEXTUREROWLENGTH;
			genUvs[curUv + 2].y = (float) texY2 / (float) textureHeight;
			genUvs[curUv + 3].x = (float) texX1 / (float) TEXTUREROWLENGTH;
			genUvs[curUv + 3].y = (float) texY2 / (float) textureHeight;
		}
	}
	genSector->UnlockUvs(genUvs);

	// Copy faces
	genSector->SetNum(GENMET_FACE, sector->numHpFaces);
	GenMeshFace* genFaces = genSector->LockFaces();

	for (int i = 0; i < sector->numHpFaces; i++) {
		uint32 texId = faces[i].GetTexture();
		int add = 0;

		if (!faces[i].GetFlip()) {
			genFaces[i].sides[0].vert = faces[i].verts[3];
			genFaces[i].sides[1].vert = faces[i].verts[2];
			genFaces[i].sides[2].vert = faces[i].verts[1];
			genFaces[i].sides[0].colour = faces[i].colours[3];
			genFaces[i].sides[1].colour = faces[i].colours[2];
			genFaces[i].sides[2].colour = faces[i].colours[1];
			genFaces[i].sides[0].uv = uvByTexture[texId];
			genFaces[i].sides[1].uv = uvByTexture[texId] + 1;
			genFaces[i].sides[2].uv = uvByTexture[texId] + 2;

			if (faces[i].verts[1] != faces[i].verts[0]) {
				genFaces[i].sides[3].vert = faces[i].verts[0];
				genFaces[i].sides[3].colour = faces[i].colours[0];
				genFaces[i].sides[3].uv = uvByTexture[texId] + 3;
				add = 1;
			}

			genFaces[i].numSides = 3 + add;
		} else {
			if (faces[i].verts[1] != faces[i].verts[0]) {
				genFaces[i].sides[0].vert = faces[i].verts[0];
				genFaces[i].sides[0].colour = faces[i].colours[0];
				genFaces[i].sides[0].uv = uvByTexture[texId] + 3;
				add = 1;
			}

			genFaces[i].sides[0+add].vert = faces[i].verts[1];
			genFaces[i].sides[1+add].vert = faces[i].verts[2];
			genFaces[i].sides[2+add].vert = faces[i].verts[3];
			genFaces[i].sides[0+add].colour = faces[i].colours[1];
			genFaces[i].sides[1+add].colour = faces[i].colours[2];
			genFaces[i].sides[2+add].colour = faces[i].colours[3];
			genFaces[i].sides[0+add].uv = uvByTexture[texId] + 2;
			genFaces[i].sides[1+add].uv = uvByTexture[texId] + 1;
			genFaces[i].sides[2+add].uv = uvByTexture[texId];
			genFaces[i].numSides = 3 + add;
		}
	}
	
	genSector->UnlockFaces(genFaces);
}

void RebuildCollisionTree();

void Scene::ConvertGenToSpyro(int sectorId) {
	if (!genScene || !spyroScene) {
		return;
	}

	GenMod* genMod = genScene->GetModById(GENID_SCENESECTORMOD + sectorId);
	GenMesh* genSector = genMod ? genMod->GetMesh() : nullptr;

	if (!genSector)
		return;

	SceneSectorHeader* sector = spyroScene->sectors[sectorId];
	bool flatSector = ((sector->centreRadiusAndFlags >> 12) & 1);
	int oldNumVerts = sector->numHpVertices, oldNumFaces = sector->numHpFaces;
	int numVerts = genSector->GetNumVerts(), numFaces = genSector->GetNumFaces(), numColours = genSector->GetNumColours();

	// Cap number of elements
	if (numVerts > 0xFF) numVerts = 0xFF;
	if (numFaces > 0xFF) numFaces = 0xFF;
	if (numColours > 0xFF) numColours = 0xFF;

	// Resize the sector if necessary
	if (sector->numHpVertices != numVerts || sector->numHpFaces != numFaces || sector->numHpColours != numColours) {
		SceneSectorHeader newHead = *sector;
	
		newHead.numHpVertices = numVerts;
		newHead.numHpFaces = numFaces;
		newHead.numHpColours = numColours;

		if (SceneSectorHeader* newSector = ResizeSector(sectorId, newHead))
			sector = newSector;
		else
			return;
	}

	// Begin sector refresh
	int hpVertexStart = sector->numLpVertices + sector->numLpColours + sector->numLpFaces * 2;
	int hpColourStart = hpVertexStart + numVerts;
	int hpFaceStart = hpColourStart + sector->numHpColours * 2;

	// UPDATE VERTICES ********
	int oldSectorX = sector->xyPos >> 16, oldSectorY = (sector->xyPos & 0xFFFF), oldSectorZ = ((sector->zPos >> 14) & 0xFFFF) >> 2;
	int newSectorX = 65535, newSectorY = 65535, newSectorZ = 65535;
	const GenMeshVert* genVerts = genSector->GetVerts();
	IntVector absoluteVerts[256];

	for (int i = 0; i < numVerts; i++) {
		absoluteVerts[i].x = (genVerts[i].pos.x / WORLDSCALE + 0.5f);
		absoluteVerts[i].y = (genVerts[i].pos.y / WORLDSCALE + 0.5f);
		absoluteVerts[i].z = (genVerts[i].pos.z / WORLDSCALE + 0.5f);
	}

	// Set new sector position according to the lowest vertex coordinates
	// Find the lowest coordinate for every vertex (to use as new sectorX/Y/Z)
	for (int i = 0; i < numVerts; i++) {
		if (absoluteVerts[i].x < newSectorX)
			newSectorX = absoluteVerts[i].x;
		if (absoluteVerts[i].y < newSectorY)
			newSectorY = absoluteVerts[i].y;
		if (absoluteVerts[i].z < newSectorZ)
			newSectorZ = absoluteVerts[i].z / 2 * 2; // fix splits =/
	}

	sector->xyPos = ((newSectorX & 0xFFFF) << 16) | (newSectorY & 0xFFFF);
	sector->zPos = (sector->zPos & 0x3FFF) | (((newSectorZ & 0xFFFF) << 14) << 2);

	int sectorX = sector->xyPos >> 16, sectorY = (sector->xyPos & 0xFFFF), sectorZ = ((sector->zPos >> 14) & 0xFFFF) >> 2;

	int32 minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX;
	int32 maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;
	for (int i = 0; i < numVerts; i++) {
		uint32 curVertex = sector->data32[hpVertexStart + i];
		int oldX = ((curVertex >> 19 & 0x1FFC) >> 3), oldY = ((curVertex >> 8 & 0x1FFC) >> 3), oldZ = ((curVertex << 3 & 0x1FFC) >> 3);
		int newX = absoluteVerts[i].x, newY = absoluteVerts[i].y, newZ = absoluteVerts[i].z;

		// Before we start: invalidate the collision tree if this vertex moved into a different collision area
		if (!colltreeInvalid && ((oldX<<1) + sectorX) >> 8 != newX >> 8 || ((oldY<<1) + sectorY) >> 8 != newY >> 8 || ((oldZ<<1) + sectorZ) >> 8 != newZ >> 8)
			colltreeInvalid = true;

		// Relocate the vertex
		curVertex = ((newX - sectorX) << 2 & 0x1FFC) << 19;
		curVertex |= ((newY - sectorY) << 2 & 0x1FFC) << 8;

		if (!flatSector && game != SPYRO1)
			curVertex |= ((newZ - sectorZ) << 2 & 0x1FFC) >> 3;
		else if (!flatSector && game == SPYRO1)
			curVertex |= ((newZ - sectorZ) << 2 & 0x1FFC) >> 2;
		else if (flatSector)
			curVertex |= ((newZ - sectorZ) << 5 & 0x1FFC) >> 3;

		sector->data32[hpVertexStart + i] = curVertex;

		// Find a matching LP vertex and move it too
		int closestLpVert = 0;
		int closestLpDist = 9999;
		for (int j = 0; j < sector->numLpVertices; j++) {
			int lpX = (sector->data32[j] >> 19 & 0x1FFC) >> 3, lpY = ((sector->data32[j] >> 8 & 0x1FFC) >> 3), lpZ = ((sector->data32[j] << 3 & 0x1FFC) >> 3);
			int xDist = lpX - oldX, yDist = lpY - oldY, zDist = lpZ - oldZ;
			int dist = xDist * xDist + yDist * yDist + zDist * zDist;

			if (dist > closestLpDist)
				continue;

			closestLpDist = dist;
			closestLpVert = j;
		}

		if (closestLpDist <= 4*4*4)
			sector->data32[closestLpVert] = curVertex;

		// Update vert coordinate boundaries
		if (newX < minX) minX = newX;
		if (newY < minY) minY = newY;
		if (newZ < minZ) minZ = newZ;
		if (newX > maxX) maxX = newX;
		if (newY > maxY) maxY = newY;
		if (newZ > maxZ) maxZ = newZ;
	}

	// Update the sector's centre info based on the boundaries measured
	sector->centreX = (minX + maxX) / 2;
	sector->centreY = (minY + maxY) / 2;
	sector->centreZ = (minZ + maxZ) / 2;
	sector->centreRadiusAndFlags = (sector->centreRadiusAndFlags & 0xF000) | ((Distance(minX, minY, minZ, maxX, maxY, maxZ) / 2) & 0xFFF);

	// UPDATE COLOURS
	const genu32* genColours = genSector->GetColours();
	for (int i = 0; i < numColours; i++) {
		sector->data32[hpColourStart + i] = genColours[i] & 0x00FFFFFF;
		sector->data32[hpColourStart + numColours + i] = genColours[i] & 0x00FFFFFF;
	}

	// UPDATE FACES
	SceneFace* faces = (SceneFace*) &sector->data32[hpFaceStart];
	const GenMeshFace* genFaces = genSector->GetFaces();
	int vertCap = numVerts ? numVerts : 1, colourCap = numColours ? numColours : 1;
	for (int i = 0; i < numFaces; i++) {
		int add = (genFaces[i].numSides == 3) ? 1 : 0;
		bool isFlipped = faces[i].GetFlip();
		for (int j = 0; j < genFaces[i].numSides; j++) {
			faces[i].verts[j + add] = genFaces[i].sides[3 - add - j].vert % vertCap;
			faces[i].colours[j + add] = genFaces[i].sides[3 - add - j].colour % colourCap;
		}

		if (genFaces[i].numSides == 3)
			faces[i].verts[0] = faces[i].verts[1];

		faces[i].word3 = 0x5732B824;
		faces[i].word4 = 0x577284AD;
		faces[i].SetFlip(false);
		faces[i].SetDepth(1);
		faces[i].SetTexture(1);
	}

	// UPDATE TEXTURES
	int numTexs = 0;
	float textureWidth = (float) TEXTURESPERROW; // width in texture units (64px)
	float textureHeight = (float) ((*numTextures + TEXTURESPERROW - 1) / TEXTURESPERROW); // height in texture units (64px)
	const GenVec2* genUvs = genSector->GetUvs();
	int numGenUvs = genSector->GetNumUvs();

	if (numTextures)
		numTexs = *numTextures;

	for (int i = 0; i < numFaces; i++) {
		// Find the average point of the face's UVs
		GenVec2 uv = {0.0f, 0.0f};
		float numUvs = 0.0f;

		for (int j = 0; j < genFaces[i].numSides; j++) {
			if (genFaces[i].sides[j].uv >= numGenUvs)
				continue;
			uv.x += genUvs[genFaces[i].sides[j].uv].x;
			uv.y += genUvs[genFaces[i].sides[j].uv].y;
			numUvs += 1.0f;
		}

		uv.x /= numUvs; uv.y /= numUvs;

		// Determine the average direction of each point from the centre point to decide whether the face should be flipped
		int avgDirection = 0;
		for (int j = 1; j < genFaces[i].numSides; j++) {
			if (genFaces[i].sides[j].uv >= numGenUvs || genFaces[i].sides[j - 1].uv >= numGenUvs)
				continue;
			
			float x = genUvs[genFaces[i].sides[j].uv].x, y = genUvs[genFaces[i].sides[j].uv].y, 
				lastX = genUvs[genFaces[i].sides[j - 1].uv].x, lastY = genUvs[genFaces[i].sides[j - 1].uv].y;

			if (atan2(y - uv.y, x - uv.x) < atan2(lastY - uv.y, lastX - uv.x))
				avgDirection++;
			else
				avgDirection--;
		}

		if (avgDirection > 0) {
			// Flip the face!
			faces[i].SetFlip(true);

			uint32 verts = faces[i].word1;
			uint32 colours = faces[i].word2;
			if (faces[i].verts[0] != faces[i].verts[1]) {
				faces[i].verts[0] = verts >> 24 & 0xFF;
				faces[i].verts[1] = verts >> 16 & 0xFF;
				faces[i].verts[2] = verts >> 8 & 0xFF;
				faces[i].verts[3] = verts & 0xFF;
				faces[i].colours[0] = colours >> 24 & 0xFF;
				faces[i].colours[1] = colours >> 16 & 0xFF;
				faces[i].colours[2] = colours >> 8 & 0xFF;
				faces[i].colours[3] = colours & 0xFF;
			} else {
				faces[i].verts[0] = verts >> 24 & 0xFF;
				faces[i].verts[1] = verts >> 24 & 0xFF;
				faces[i].verts[2] = verts >> 16 & 0xFF;
				faces[i].verts[3] = verts >> 8 & 0xFF;
				faces[i].colours[0] = colours >> 24 & 0xFF;
				faces[i].colours[1] = colours >> 24 & 0xFF;
				faces[i].colours[2] = colours >> 16 & 0xFF;
				faces[i].colours[3] = colours >> 8 & 0xFF;
			}
		}

		// Use that point to determine the texture
		int xTex = (int) (uv.x * textureWidth);
		int yTex = (int) (uv.y * textureHeight);
		int texId = yTex * TEXTURESPERROW + xTex;

		if (texId >= 0 && texId < numTexs)
			faces[i].SetTexture(texId);
		else
			faces[i].SetTexture(1);
	}

	// FIX GAPS
	int curFaceId = sector->GetFlistId();
	for (int f1 = 0; f1 < sector->numHpFaces; f1++) {
		for (int s1 = 0; s1 < 4; s1++) {
			int vert1 = faces[f1].verts[s1], vert2 = faces[f1].verts[(s1 + 1) & 3];

			for (int f2 = 0; f2 < sector->numHpFaces; f2++) {
				if (f2 == f1)
					continue;
				for (int s2 = 0; s2 < 4; s2++) {
					if ((faces[f2].verts[s2] == vert1 && faces[f2].verts[(s2 + 1) & 3] == vert2) || 
						(faces[f2].verts[s2] == vert2 && faces[f2].verts[(s2 + 1) & 3] == vert1)) {
						faces[f1].SetEdge((2 - s1) & 3, curFaceId + f2);
						goto NextSide;
					}
				}
			}

		NextSide:
			continue;
		}
	}

	curFaceId += sector->numHpFaces; // delete this line?

	// UPDATE COLLISION TRIANGLES ********
	CollTri* triangles = spyroCollision.triangles;
	uint16* triangleTypes = spyroCollision.surfaceType;
	int numTriangles = spyroCollision.numTriangles;

	// Our sector colltri cache
	CollSectorCache* sectorCache = &collisionCache.sectorCaches[sectorId];
	CollTriCache* cache = sectorCache->triangles;

	// If there are new faces added, steal some triangles from the unlinked cache and link them (assume the user wants them to be collidable)
	// TODO: Face IDs could be reordered at any time. Assuming they'll stay the same is bad... this OK?
	for (int face = oldNumFaces; face < numFaces; face++) {
		if (!collisionCache.numUnlinkedTriangles || sectorCache->numTriangles >= sizeof (sectorCache->triangles)/sizeof (sectorCache->triangles[0]))
			break; // no triangles left to steal!
		
		// For every triangle of this face (1 if the face is a triangle, 2 if it's a quad)
		for (int tri = 0, numTris = (faces[face].verts[0] == faces[face].verts[1] ? 1 : 2); tri < numTris; tri++) {
			if (!collisionCache.numUnlinkedTriangles || sectorCache->numTriangles >= sizeof (sectorCache->triangles)/sizeof (sectorCache->triangles[0]))
				break; // no triangles left to steal!
			
			// Take the least valuable triangle in the miscellaneous triangle list
			CollTriCache* stealMe = &collisionCache.unlinkedCaches[collisionCache.numUnlinkedTriangles - 1];
			CollTriCache* linkedTri = &cache[sectorCache->numTriangles];

			// Prepare the stolen triangle
			stealMe->faceIndex = face;

			if (triangleTypes) {
				triangleTypes[stealMe->triangleIndex] = 0;
				stealMe->triangleType = 0;
			}

			// Attach it to this face
			*linkedTri = *stealMe;

			if ((faces[face].GetFlip())) {
				if (tri == 0) {
					linkedTri->vert1Index = 1;
					linkedTri->vert2Index = 2;
					linkedTri->vert3Index = 3;
				} else if (tri == 1) {
					linkedTri->vert1Index = 3;
					linkedTri->vert2Index = 0;
					linkedTri->vert3Index = 1;
				}
			} else {
				if (tri == 0) {
					linkedTri->vert1Index = 3;
					linkedTri->vert2Index = 2;
					linkedTri->vert3Index = 1;
				} else if (tri == 1) {
					linkedTri->vert1Index = 1;
					linkedTri->vert2Index = 0;
					linkedTri->vert3Index = 3;
				}
			}

			// Decrement the unlinked triangle count after our thievery
			collisionCache.numUnlinkedTriangles--;
			sectorCache->numTriangles++;
		}
	}

	// Use the cache to move collision triangles
	bool doRefreshColltree = false;
	for (CollTriCache* cacheTri = cache, *e = &cache[sectorCache->numTriangles]; cacheTri < e; cacheTri++) {
		SceneFace* face = &sector->GetHpFaces()[cacheTri->faceIndex];
		const IntVector &vert1 = absoluteVerts[face->verts[cacheTri->vert1Index]], &vert2 = absoluteVerts[face->verts[cacheTri->vert2Index]], 
						&vert3 = absoluteVerts[face->verts[cacheTri->vert3Index]];
		CollTri* tri = &triangles[cacheTri->triangleIndex];

		if (!doRefreshColltree) {
			int16 offX = tri->xCoords & 0x3FFF, offY = tri->yCoords & 0x3FFF, offZ = tri->zCoords & 0x3FFF;
			int16 p1X = offX, p2X = bitss(tri->xCoords, 14, 9) + offX, p3X = bitss(tri->xCoords, 23, 9) + offX;
			int16 p1Y = offY, p2Y = bitss(tri->yCoords, 14, 9) + offY, p3Y = bitss(tri->yCoords, 23, 9) + offY;
			int16 p1Z = offZ, p2Z = bitsu(tri->zCoords, 16, 8) + offZ, p3Z = bitsu(tri->zCoords, 24, 8) + offZ;

			// Check if this triangle has moved outside its original colltree block
			if ((vert1.x >> 8) != (p1X >> 8) || (vert2.x >> 8) != (p2X >> 8) || (vert3.x >> 8) != (p3X >> 8) || 
				(vert1.y >> 8) != (p1Y >> 8) || (vert2.y >> 8) != (p2Y >> 8) || (vert3.y >> 8) != (p3Y >> 8) ||
				(vert1.z >> 8) != (p1Z >> 8) || (vert2.z >> 8) != (p2Z >> 8) || (vert3.z >> 8) != (p3Z >> 8)) {
				doRefreshColltree = true;
			}
		}

		tri->SetPoints(vert1.x, vert1.y, vert1.z, vert2.x, vert2.y, vert2.z, vert3.x, vert3.y, vert3.z);

	}

	// Refresh collision tree if polygons were moved out of their original block
	if (doRefreshColltree) {
		RebuildCollisionTree();
	}

	// Refresh UI
	isGenesisPageValid = false;
}

GenMesh* Scene::GetGenSector(int sectorIndex) {
	if (genScene) {
		if (GenMod* mod = genScene->GetModById(GENID_SCENESECTORMOD + sectorIndex)) {
			if (GenMesh* mesh = mod->GetMesh()) {
				return mesh;
			}
		}
	}

	return nullptr;
}

SceneSectorHeader* Scene::ResizeSector(int sectorId, const SceneSectorHeader& newHeader) {
	if (!spyroScene || !genScene)
		return nullptr;

	// Resizing the sector means finding some place to put it in memory
	// if the sector is smaller than it was before, this isn't a problem, we don't need to move it at all
	// it's a different story however if it's bigger, in which case it needs to be moved to what SpyroEdit believes is a free memory space
	SceneSectorHeader* header = spyroScene->sectors[sectorId];
	int curSize = header->GetSize();
	int newSize = newHeader.GetSize();
	uint32 newSpot = spyroScene->sectors[sectorId].address & 0x003FFFFF;
	uint8 backupSectorData[16384];
	SceneSectorHeader* backupSector = (SceneSectorHeader*) backupSectorData;

	if (newSize > curSize) {
		// First count the amount of space left in the main scene memory. If the user has deleted level segments we can defragment and use it.
		int usedSize = (spyroScene->numSectors * 4) + 0x0C;
		for (int i = 0; i < spyroScene->numSectors; i++) {
			usedSize += spyroScene->sectors[i]->GetSize();
		}
		usedSize -= header->GetSize();

		if (spyroScene->size >= usedSize + newSize) {
			// Defragment sectors while creating extra room for this one
			static uint8 backupSectors[0x00050000];
			static uint32 backupSectorOffsets[512];
			uint32 curOffset = 0;

			for (int i = 0; i < spyroScene->numSectors; i++) {
				int sectorSize = spyroScene->sectors[i]->GetSize();
				memcpy(&backupSectors[curOffset], spyroScene->sectors[i], sectorSize);

				backupSectorOffsets[i] = curOffset;

				if (i != sectorId)
					curOffset += sectorSize;
				else
					curOffset += newSize;
			}
			
			uint32 baseAddress = (uint8*)&spyroScene->sectors[spyroScene->numSectors] - umem8;
			memcpy(&umem8[baseAddress], backupSectors, curOffset);

			for (int i = 0; i < spyroScene->numSectors; i++)
				spyroScene->sectors[i].address = (baseAddress + backupSectorOffsets[i]) | 0x80000000;

			newSpot = spyroScene->sectors[sectorId].address & 0x003FFFFF;
			header = spyroScene->sectors[sectorId];
		} else {
			newSpot = FindFreeMemory(newSize); // Move the sector to a bigger memory area if possible
			
			if (!newSpot)
				return NULL; // no memory, cannot resize the sector to this size

			// Copy the old sector to the new place
			memcpy(&umem8[newSpot], header, header->GetSize());
			
			// Zero out the old sector. This enables us to reuse the memory if needed
			memset(header, 0, curSize);

			// Update pointer to sector
			spyroScene->sectors[sectorId].address = newSpot | 0x80000000;

			// Reassign header pointer
			header = (SceneSectorHeader*) &umem8[newSpot];
		}
	}
	
	// Backup the old header and data for reshuffle
	memcpy(backupSector, &header, header->GetSize());

	// Copy the new header
	memcpy((SceneSectorHeader*) header, &newHeader, 7 * 4);
	header->zTerminator = 0xFFFFFFFF;

	// Shuffle verts, faces etc
	uint32* basicUintShuffle[] = {backupSector->GetLpVertices(), header->GetLpVertices(), backupSector->GetLpColours(), header->GetLpColours(), 
								  backupSector->GetHpVertices(), header->GetHpVertices(), backupSector->GetHpColours(), header->GetHpColours()};
	int shuffleCount[] = {backupSector->numLpVertices, header->numLpVertices, backupSector->numLpColours, header->numLpColours, 
						  backupSector->numHpVertices, header->numHpVertices, backupSector->numHpColours, header->numHpColours};

	for (int j = 0; j < sizeof (basicUintShuffle) / sizeof (basicUintShuffle[0]) / 2; j++) {
		uint32* oldUints = basicUintShuffle[j * 2], *newUints = basicUintShuffle[j * 2 + 1];
		int oldCount = shuffleCount[j * 2], newCount = shuffleCount[j * 2 + 1];
		for (int i = 0; i < newCount; i++) {
			if (i < oldCount)
				newUints[i] = oldUints[i];
			else
				newUints[i] = 0;
		}
	}

	uint32* oldLpFaces = backupSector->GetLpFaces(), *newLpFaces = header->GetLpFaces();
	for (int i = 0; i < header->numLpFaces * 2; i++) {
		if (i < backupSector->numLpFaces * 2)
			newLpFaces[i] = oldLpFaces[i];
		else
			newLpFaces[i] = 0;
	}

	SceneFace* oldHpFaces = backupSector->GetHpFaces(), *newHpFaces = header->GetHpFaces();
	for (int i = 0; i < header->numHpFaces; i++) {
		if (i < backupSector->numHpFaces) {
			newHpFaces[i] = oldHpFaces[i];
		} else {
			newHpFaces[i].word1 = 0;
			newHpFaces[i].word2 = 0;
			newHpFaces[i].word3 = 0;
			newHpFaces[i].word4 = 0;
		}
	}

	if (newSize < curSize)
		// Zero out the free memory beyond this sector
		memset((void*) ((uintptr) header + newSize), 0, curSize - newSize);

	// Done!
	return header;
}

void Scene::UploadToGen() {
	// Send each sector as a GenState. Includes an instance, model and edit modifier
	for (int i = 0; i < spyroScene->numSectors; i++) {
		GenState meshState(0xFFFFFFFF, GENCOMMON_MULTIPLE);
		GenState combinedState(0, GENCOMMON_MULTIPLE);
		
		// Obtain states for instance, modifier and model from the global scene
		GenState instState = genScene->GetState(GENID_SCENESECTORINSTANCE + i, GENCOMMON_MULTIPLE);
		GenState modState = genScene->GetState(GENID_SCENESECTORMOD + i, GENCOMMON_MULTIPLE);
		GenState modelState = genScene->GetState(GENID_SCENESECTORMODEL + i, GENCOMMON_MULTIPLE);

		// Modifiers erm.... set the mesh states so that they have the, uh... same state as...what?
		if (GenMod* mod = genScene->GetModById(GENID_SCENESECTORMOD + i)) {
			meshState.SetInfo(mod->GetMeshId(), GENCOMMON_MULTIPLE);

			genScene->GetState(&meshState);
		}

		GenState* stateList[] = {&instState, &modState, &modelState, &meshState};

		combinedState.AddSubstates(stateList, 4);
		live->SendState(combinedState);
	}
}

void Scene::GenerateCrater(int centreX, int centreY, int centreZ) {
	if (!scene.spyroScene)
		return;
	
	const float range = 6.0f;
	const float depth = 0.7f;
	const float rimHeight = 1.1f;
	const float rimWidth = 3.0f;
	const float crushFactor = 1.0f, crushFactorOuter = 0.5f;
	float fX = (float) (centreX >> 4) * WORLDSCALE, fY = (float) (centreY >> 4) * WORLDSCALE, fZ = (float) (centreZ >> 4) * WORLDSCALE;

	for (int i = 0; i < scene.spyroScene->numSectors; i++) {
		GenMesh* genSector = scene.GetGenSector(i);
		GenMeshVert* genVerts = genSector->LockVerts();
		bool changeMade = false;

		for (int j = 0, e = genSector->GetNumVerts(); j < e; j++) {
			float diffX = genVerts[j].pos.x - fX, diffY = genVerts[j].pos.y - fY, diffZ = genVerts[j].pos.z - fZ;
			float dist = sqrt(diffX * diffX + diffY * diffY + diffZ * diffZ);

			if (dist <= range && diffZ <= 0.0f) {
				genVerts[j].pos.z = genVerts[j].pos.z + (fZ - genVerts[j].pos.z) * ((crushFactor * (1.0f-dist/range)) + (crushFactorOuter * (dist/range)));

				if (dist <= range - rimWidth)
					genVerts[j].pos.z -= cos(dist / (range - rimWidth) * 1.67f) * depth;
				else
					genVerts[j].pos.z += sin((dist - range - rimWidth) / rimWidth * 3.14f) * rimHeight;
				changeMade = true;
			}
		}

		if (changeMade) {
			GenMeshFace* genFaces = genSector->LockFaces();
			genu32* genColours = genSector->LockColours();
			genu32 numVerts = genSector->GetNumVerts(), numColours = genSector->GetNumColours();

			// Update colours then call UpdateSector
			float colourDist[256];
			memset(colourDist, 0, sizeof (colourDist));

			for (int j = 0, e = genSector->GetNumFaces(); j < e; j++) {
				for (int k = 0; k < genFaces[j].numSides; k++) {
					if (genFaces[j].sides[k].vert >= numVerts || genFaces[j].sides[k].colour >= numColours)
						continue;

					GenVec3* vert = &genVerts[genFaces[j].sides[k].vert].pos;
					float dist = sqrt((vert->x - fX) * (vert->x - fX) + (vert->y - fY) * (vert->y - fY));

					if (dist <= range && dist > colourDist[genFaces[j].sides[k].colour])
						colourDist[genFaces[j].sides[k].colour] = dist;
				}
			}

			for (int j = 0, e = genSector->GetNumColours(); j < e; j++) {
				if (colourDist[j] > 0.0f && colourDist[j] <= range) {
					int factor = 20 + (int) (colourDist[j] / range * 80.0f);
					int oldR = GETR32(genColours[j]), oldG = GETG32(genColours[j]), oldB = GETB32(genColours[j]);

					genColours[j] = MAKECOLOR32(oldR * factor / 100, oldG * factor / 100, oldB * factor / 100) | 0xFF000000;
				}
			}
			
			genSector->UnlockColours(genColours);
			genSector->UnlockFaces(genFaces);

			scene.ConvertGenToSpyro(i);
		}
		genSector->UnlockVerts(genVerts);
	}

	// Move nearby objects
	if (mobys) {
		for (int i = 0; mobys[i].state != -1; i++) {
			if (mobys[i].state < 0)
				continue;

			float x = (float) (mobys[i].x >> 4) * WORLDSCALE, y = (float) (mobys[i].y >> 4) * WORLDSCALE, z = (float) (mobys[i].z >> 4) * WORLDSCALE;
			float diffX = x - fX, diffY = y - fY, diffZ = z - fZ;
			float dist = sqrt(diffX * diffX + diffY * diffY + diffZ * diffZ);

			if (dist <= range) {
				z = z + (fZ - z) * ((crushFactor * (1.0f-dist/range)) + (crushFactorOuter * (dist/range)));

				if (dist <= range - rimWidth)
					z -= cos(dist / (range - rimWidth) * 1.67f) * depth;
				else
					z += sin((dist - range - rimWidth) / rimWidth * 3.14f) * rimHeight;

				mobys[i].z = (int) (z / WORLDSCALE * 16.0f);
			}
		}
	}
}

void Scene::Reset() {
	if (originalScene.scene && spyroScene) {
		originalScene.PasteSnapshot(spyroScene);

		for (int i = 0; i < spyroScene->numSectors; ++i) {
			ConvertSpyroToGen(i);
		}
	}
}

void Scene::Cleanup() {
	originalScene.Cleanup();
}

void Scene::SpyroSceneCache::CopySnapshot(const SpyroSceneHeader* sourceScene) {
	delete this->scene;
	this->scene = (SpyroSceneHeader*)operator new(sourceScene->size);

	// Todo: Check all addresses, making sure they're within range of the data block, and that the size of the data block is actually valid?
	memcpy(this->scene, sourceScene, sourceScene->size);
}

void Scene::SpyroSceneCache::PasteSnapshot(SpyroSceneHeader* destScene) {
	if (this->scene) {
		memcpy(destScene, this->scene, this->scene->size);

		// Todo: Make sure each sector is appropriately allocated, etc
	}
}

void Scene::SpyroSceneCache::Cleanup() {
	operator delete(scene);
	scene = nullptr;
}