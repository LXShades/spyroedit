#include "SpyroData.h"
#include "SpyroLiveGen.h"
#include "SpyroGenesis.h"
#include "SpyroScene.h"
#include "GenObject.h"
#include "SpyroTextures.h"

uint32 mobyVertAdjustTable[127] = {
	0xFE9FD3FA, 0xFF5FD3FA, 0x001FD3FA, 0x00DFD3FA, 0x019FD3FA, 0xFE9FEBFA, 0xFF5FEBFA, 0x001FEBFA, 0x00DFEBFA, 0x019FEBFA, 0xFE8003FA, 0xFF4003FA, 0x000003FA, 0x00C003FA,
	0x018003FA, 0xFE801BFA, 0xFF401BFA, 0x00001BFA, 0x00C01BFA, 0x01801BFA, 0xFE8033FA, 0xFF4033FA, 0x000033FA, 0x00C033FA, 0x018033FA, 0xFE9FD3FD, 0xFF5FD3FD, 0x001FD3FD,
	0x00DFD3FD, 0x019FD3FD, 0xFE9FEBFD, 0xFF5FEBFD, 0x001FEBFD, 0x00DFEBFD, 0x019FEBFD, 0xFE8003FD, 0xFF4003FD, 0x000003FD, 0x00C003FD, 0x018003FD, 0xFE801BFD, 0xFF401BFD,
	0x00001BFD, 0x00C01BFD, 0x01801BFD, 0xFE8033FD, 0xFF4033FD, 0x000033FD, 0x00C033FD, 0x018033FD, 0xFE9FD000, 0xFF5FD000, 0x001FD000, 0x00DFD000, 0x019FD000, 0xFE9FE800,
	0xFF5FE800, 0x001FE800, 0x00DFE800, 0x019FE800, 0xFE800000, 0xFF400000, 0x00000000, 0x00C00000, 0x01800000, 0xFE801800, 0xFF401800, 0x00001800, 0x00C01800, 0x01801800,
	0xFE803000, 0xFF403000, 0x00003000, 0x00C03000, 0x01803000, 0xFE9FD003, 0xFF5FD003, 0x001FD003, 0x00DFD003, 0x019FD003, 0xFE9FE803, 0xFF5FE803, 0x001FE803, 0x00DFE803,
	0x019FE803, 0xFE800003, 0xFF400003, 0x00000003, 0x00C00003, 0x01800003, 0xFE801803, 0xFF401803, 0x00001803, 0x00C01803, 0x01801803, 0xFE803003, 0xFF403003, 0x00003003,
	0x00C03003, 0x01803003, 0xFE9FD006, 0xFF5FD006, 0x001FD006, 0x00DFD006, 0x019FD006, 0xFE9FE806, 0xFF5FE806, 0x001FE806, 0x00DFE806, 0x019FE806, 0xFE800006, 0xFF400006,
	0x00000006, 0x00C00006, 0x01800006, 0xFE801806, 0xFF401806, 0x00001806, 0x00C01806, 0x01801806, 0xFE803006, 0xFF403006, 0x00003006, 0x00C03006, 0x01803006, 0x7E7F7F80,
	0x7C7C7D7D // Note: the last two values may be invalid
};

void UpdateObjTexMap();

void BuildGenMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrameIndex) {
	if (mobyModelId >= 768)
		return;
	if (!mobyModels[mobyModelId].address)
		return;

	UpdateObjTexMap();

	// Create the container model and mod
	GenModel* model = (GenModel*)genScene.CreateOrGetObject(GENID_MOBYMODELS + mobyModelId, GENOBJ_MODEL);
	GenMod* mod = (GenMod*)genScene.CreateOrGetObject(GENID_MOBYMODS + mobyModelId, GENOBJ_MOD);
	GenMesh* mesh = (GenMesh*)mod->GetMesh();

	if (!mesh || !mod || !model) {
		// Error?
		return;
	}

	mod->SetModType(MOD_SPYROMOBY);

	if (model->GetNumModifiers() == 0) {
		model->AddModifier(mod->GetId());
	}

	uint32* verts = NULL, *faces = NULL, *colours = NULL;
	uint16* blocks = NULL;
	uint8* adjusts = NULL;
	int numVerts = 0, numColours = 0;
	int maxNumFaces = 0;
	uint32 frameStartFlags = 0;
	float scale = MOBYPOSITIONSCALE;
	bool animated = (mobyModels[mobyModelId].address & 0x80000000);

	GenValueSetContainer<128000> modelData;
	mod->SetProperty("spyroBaseAddr", GenValueSet((gens32)mobyModels[mobyModelId].address));

	if (animated) {
		int size = (mobyModels[mobyModelId]->unkSomeOtherPlace & 0x003FFFFF) - (mobyModels[mobyModelId].address & 0x003FFFFF);
		if (size >= 0 && size < 128000) {
			modelData.numValues = size;
			modelData.type = GENTYPE_RAW;
			memcpy(modelData.byte, mobyModels[mobyModelId], size);
			mod->SetProperty("spyroModel", modelData);
		}
	} else {
		int size = (((SimpleModelHeader*)mobyModels[mobyModelId])->unkExtra & 0x003FFFFF) - (mobyModels[mobyModelId].address & 0x003FFFFF);
		if (size >= 0 && size < 128000) {
			modelData.numValues = size;
			modelData.type = GENTYPE_RAW;
			memcpy(modelData.byte, mobyModels[mobyModelId], size);
			mod->SetProperty("spyroModel", modelData);
		}
	}

	if (animated) {
		ModelHeader* model = mobyModels[mobyModelId];

		if (animId >= model->numAnims)
			return;
		
		ModelAnimHeader* anim = model->anims[animId];

		if (animFrameIndex >= anim->numFrames)
			return;

		ModelAnimFrameInfo* frame = &anim->frames[animFrameIndex];

		verts = anim->verts; faces = anim->faces; colours = anim->colours;
		numVerts = anim->numVerts; numColours = anim->numColours;
		maxNumFaces = faces[0] / 8;

		blocks = &anim->data[frame->blockOffset];
		adjusts = (uint8*)&blocks[frame->numBlocks];
		scale = (float) (MOBYPOSITIONSCALE) * ((float) (anim->vertScale + 1));
		frameStartFlags = frame->startFlags;
	} else {
		SimpleModelHeader* model = (SimpleModelHeader*) (mobyModels[mobyModelId]); // make sure the conversion works

		if (animId >= model->numStates)
			return;

		SimpleModelStateHeader* state = model->states[animId];
		verts = state->data32; faces = &state->data32[(state->hpFaceOff - 16) / 4]; colours = &state->data32[(state->hpColourOff - 16) / 4];
		numVerts = state->numHpVerts; numColours = state->numHpColours;
		maxNumFaces = faces[0] / 8;
		scale = MOBYPOSITIONSCALE * 4.0f;
	}

	if (numVerts >= 300 || numColours >= 300 || maxNumFaces >= 600)
		return;

	// Send frame vertices
	mesh->SetNum(GENMET_VERT, numVerts);
	GenMeshVert* genVerts = mesh->LockVerts();

	if (animated) {
		int curX = 0, curY = 0, curZ = 0;
		int blockX = 0, blockY = 0, blockZ = 0;
		int curBlock = 0;
		bool nextBlock = true, blockVert = false;
		int adjustVert = 0;
	
		// If flags & 3 == 0, Sheila will disrespect the first block entirely and start at the first vertex in her default pose
		// If flags & 3 == 1, Sheila will disrespect the first block and start at a fixed position, but her eyes will respect the first block and also the second?!
		// If flags & 3 == 2, Sheila will respect the first block
		// If flags & 3 == 3, Sheila will respect the first block but her eyes will skyrocket to a fixed position
		if (frameStartFlags & 2) {
			curX = 0; curY = 0; curZ = 0;
			nextBlock = true;
		} else {
			curX = bitss(verts[0], 21, 11);
			curY = bitss(verts[0], 10, 11);
			curZ = bitss(verts[0], 0, 10);
			curX = curY = curZ = 0;
			nextBlock = false;
		}

		for (int i = 0; i < numVerts; i++) {
			bool usedBlock = false;
			int x = bitss(verts[i], 21, 11), y = bitss(verts[i], 10, 11), z = bitss(verts[i], 0, 10);

			if (nextBlock) {
				blockVert = (blocks[curBlock] & 1) != 0;

				nextBlock = blockVert;
				if (blocks[curBlock] & 0x2) {
					// Long block
					uint32 pos = blocks[curBlock] | (blocks[curBlock + 1] << 16);

					int tZ = bitss(pos, 2, 10), tY = bitss(pos, 12, 10), tX = bitss(pos, 22, 10);

					curX = tX * 2; curY = -tY * 2; curZ = -tZ * 2;
					x = 0; y = 0; z = 0;
					curBlock += 2;
				} else {
					// Short block
					int tZ = bitss(blocks[curBlock], 1, 5), tY = bitss(blocks[curBlock], 5, 6), tX = bitss(blocks[curBlock], 10, 6);
					curX += tX * 2; curY += tY * 2; curZ += tZ * 2;
					curBlock += 1;
				}

				usedBlock = true;
			}
		
			if (!usedBlock && !blockVert) {
				uint32 adjust = mobyVertAdjustTable[(adjusts[adjustVert] >> 1)];
				int aX = bitss(adjust, 21, 11), aY = bitss(adjust, 10, 11), aZ = bitss(adjust, 0, 10);

				if (adjusts[adjustVert] & 1)
					nextBlock = true;

				x += aX; y += aY; z += aZ;
				adjustVert++;
			}

			curX += x; curY += y; curZ += z;
			genVerts[i].pos.Set((float)curX * scale, (float)curY * scale, (float)curZ * 2.0f * scale);
		}
	} else {
		for (int i = 0; i < numVerts; i++)
			genVerts[i].pos.Set((float) bitss(verts[i], 22, 10) * scale, (float) bitss(verts[i], 12, 10) * scale, (float) -bitss(verts[i], 0, 12) * scale / 2.0f);
	}

	mesh->UnlockVerts(genVerts);

	// Set model faces
	mesh->SetNum(GENMET_FACE, maxNumFaces);

	GenMeshFace* genFaces = mesh->LockFaces();
	int32 tempU[512], tempV[512];

	int numSides = 0, numUvs = 0, numUvIndices = 0, index = 1, maxIndex = faces[0] / 4 + 1;
	int curFace = 0;
	
	while (index < maxIndex) {
		GenMeshFace* genFace = &genFaces[curFace++];
		uint32 face = faces[index];
		uint32 clrs = faces[index + 1];
		int numFaceSides = (face & 0xFF) != ((face >> 8) & 0xFF) ? 4 : 3;

		if (numFaceSides == 4) {
			if (animated) {
				genFace->sides[0].vert = face >> 8 & 0xFF;
				genFace->sides[1].vert = face & 0xFF;
				genFace->sides[2].vert = face >> 16 & 0xFF;
				genFace->sides[3].vert = face >> 24 & 0xFF;
			} else {
				genFace->sides[0].vert = face >> 7 & 0x7F;
				genFace->sides[1].vert = face & 0x7F;
				genFace->sides[2].vert = face >> 14 & 0x7F;
				genFace->sides[3].vert = face >> 21 & 0x7F;
			}
			genFace->sides[0].colour = clrs >> 10 & 0x7F;
			genFace->sides[1].colour = clrs >> 3 & 0x7F;
			genFace->sides[2].colour = clrs >> 17 & 0x7F;
			genFace->sides[3].colour = clrs >> 24 & 0x7F;
		} else {
			genFace->sides[0].vert = face >> 24 & 0xFF;
			genFace->sides[1].vert = face >> 16 & 0xFF;
			genFace->sides[2].vert = face >> 8 & 0xFF;
			genFace->sides[0].colour = clrs >> 24 & 0x7F;
			genFace->sides[1].colour = clrs >> 17 & 0x7F;
			genFace->sides[2].colour = clrs >> 3 & 0x7F;
		}
		
		genFace->numSides = numFaceSides;

		if (clrs & 0x80000000) {
			uint32 word0 = faces[index], word1 = faces[index + 1], word2 = faces[index + 2], word3 = faces[index + 3], word4 = faces[index + 4];
			int p1U = word2 & 0xFF, p1V = (word2 >> 8) & 0xFF, p2U = word3 & 0xFF,         p2V = (word3 >> 8) & 0xFF, 
				p3U = word4 & 0xFF, p3V = (word4 >> 8) & 0xFF, p4U = (word4 >> 16) & 0xFF, p4V = (word4 >> 24) & 0xFF;
			int imageBlockX = ((word3 >> 16) & 0xF) * 0x40, imageBlockY = ((word3 >> 20) & 1) * 0x100;
			int p1X = p1U + (imageBlockX * 4), p1Y = p1V + imageBlockY, p2X = p2U + (imageBlockX * 4), p2Y = p2V + imageBlockY, p3X = p3U + (imageBlockX * 4),
				p3Y = p3V + imageBlockY, p4X = p4U + (imageBlockX * 4), p4Y = p4V + imageBlockY;
			bool gotTextures = false;

			for (int i = 0; i < objTexMap.numTextures; i++) {
				if (objTexMap.textures[i].minX <= p1X && objTexMap.textures[i].minY <= p1Y && objTexMap.textures[i].maxX >= p1X && objTexMap.textures[i].maxY >= p1Y) {
					ObjTex* tex = &objTexMap.textures[i];
					const int w = objTexMap.width, h = objTexMap.height;
					int us[4] = {(tex->mapX + (p1X - tex->minX)) * 65535 / w, (tex->mapX + (p2X - tex->minX)) * 65535 / w, 
							     (tex->mapX + (p4X - tex->minX)) * 65535 / w, (tex->mapX + (p3X - tex->minX)) * 65535 / w};
					int vs[4] = {(tex->mapY + (p1Y - tex->minY)) * 65535 / h, (tex->mapY + (p2Y - tex->minY)) * 65535 / h, 
								 (tex->mapY + (p4Y - tex->minY)) * 65535 / h, (tex->mapY + (p3Y - tex->minY)) * 65535 / h};
					int foundIndex[4] = {-1, -1, -1, -1};

					if (numFaceSides == 3) {
						//us[2] = us[3];
						//vs[2] = vs[3];
					}

					for (int j = 0; j < numUvs; j++) {
						for (int k = 0; k < 4; k++) {
							if (tempU[j] == us[k] && tempV[j] == vs[k])
								foundIndex[k] = j;
						}
					}

					for (int j = 0; j < numFaceSides; j++) {
						if (foundIndex[j] != -1)
							genFace->sides[j].uv = foundIndex[j];
						else {
							genFace->sides[j].uv = numUvs;
							tempU[numUvs] = us[j];
							tempV[numUvs] = vs[j];
							numUvs++;
						}
					}

					gotTextures = true;
					break;
				}
			}

			if (!gotTextures) {
				for (int i = 0; i < numFaceSides; i++)
					genFace->sides[i].uv = 0;
			}

			index += 5; // textured face, continue
		} else {
			for (int i = 0; i < numFaceSides; i++)
				genFace->sides[i].uv = 0;

			index += 2; // no textures, continue
		}
	}

	mesh->UnlockFaces(genFaces);

	// Set UVs according to the temporary UVs
	mesh->SetNum(GENMET_UV, numUvs);
	GenVec2* genUvs = mesh->LockUvs();

	for (int i = 0; i < numUvs; i++)
		genUvs[i].Set((float) tempU[i] / 65535, (float) tempV[i] / 65535);

	mesh->UnlockUvs(genUvs);

	// Set colours
	mesh->SetNum(GENMET_COLOUR, numColours);
	genu32* genColours = mesh->LockColours();

	for (int i = 0; i < numColours; i++)
		genColours[i] = colours[i] | 0xFF000000;

	mesh->UnlockColours(genColours);
}

#include <wchar.h>

void SendLiveGenMobyModel(int modelId, int mobyId = -1) {
	if (!live || !mobyModels)
		return;

	if (mobyId > -1) {
		// Send a specific moby with specific animations
		BuildGenMobyModel(modelId, mobys[mobyId].anim.nextAnim, mobys[mobyId].anim.nextFrame);
	} else {
		// Send a model with animation 0 and frame 0
		BuildGenMobyModel(modelId, 0, 0);
	}

	// Create dummy instance of model
	GenInst* inst = (GenInst*) genScene.CreateOrGetObject(GENID_MOBYPREVIEWINSTANCES + modelId, GENOBJ_INST);
	genwchar instName[256];

	swprintf(instName, L"MobyType %i/%04X; addr %08X", modelId, modelId, mobyModels[modelId].address & 0x003FFFFF);
	inst->SetModel(GENID_MOBYMODELS + modelId);
	inst->SetName(instName);
		
	// Send texture
	char filename[260];

	GetLevelFilename(filename, SEF_OBJTEXTURES);

	GenState texState(GENID_MOBYTEXTURES, GENMAT_SOURCEFILE); // todo genSceneify
	GenValueSet* elems = texState.Lock(GENTYPE_CSTRING, strlen(filename) + 1);
		
	strcpy(elems->s8, filename);
	texState.Unlock();

	live->SendState(texState.GetSubstate());

	// Send model states
	GenMod* mod = genScene.GetModById(GENID_MOBYMODS + modelId);
	GenObject* objList[] = {genScene.GetModelById(GENID_MOBYMODELS + modelId), mod, mod ? mod->GetMesh() : nullptr, inst};
	
	for (int i = 0; i < sizeof (objList) / sizeof (objList[0]); i++) {
		if (!objList[i])
			continue;

		live->SendState(objList[i]->GetState(GENCOMMON_MULTIPLE));
		objList[i]->ValidateAll();
	}
}

void SendLiveGenMobyInstances();

void SendLiveGenAllMobys() {
	if (!mobys || !mobyModels)
		return;

	bool doneTypes[768];
	memset(doneTypes, 0, sizeof (doneTypes));

	for (int i = 0; ; i++) {
		if (mobys[i].state == -1)
			break;
		if (mobys[i].state < 0)
			continue;

		if (mobys[i].type < 768 && !doneTypes[mobys[i].type]) {
			SendLiveGenMobyModel(mobys[i].type, i);
			doneTypes[mobys[i].type] = true;
		}
	}

	SendLiveGenMobyInstances();
}