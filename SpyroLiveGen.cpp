#include "Window.h"
#include "SpyroData.h"
#include "Main.h"

#include "GenLive.h"
#include "GenObject.h"

#include <cstdio>
#include <cwchar> // for instance name

#define GENID_SCENESECTORMODEL 1000
#define GENID_SCENESECTORMOD 1256
#define GENID_SCENESECTORINSTANCE 2537
#define GENID_SCENETEXTURE 3000
#define GENID_COLLISIONDATAMOD 9001
#define GENID_COLLISIONDATAMODEL 9002
#define GENID_COLLISIONDATAINSTANCE 9003
#define GENID_GENERICMOBYMOD 11221
#define GENID_GENERICMOBYMODEL 11222
#define GENID_GENERICMOBYMODELMOD 11223
#define GENID_MOBYMODELS 534000
#define GENID_MOBYPREVIEWINSTANCES 536000
#define GENID_MOBYINSTANCES 538000
#define GENID_MOBYMODS 540000
#define GENID_MOBYTEXTURES 542000
#define GENID_SPYROMODEL 22330
#define GENID_SPYROMODELMOD 22331
#define GENID_SPYROINSTANCE 22334

#define MOBYPOSSCALE 0.000625f
#define MOBYSPHERESIZE 0.15f

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

ILiveGen* live;

GenScene genScene;
GenModel* genSectors[256];

GenMesh* genMobys[768];
GenMod* genMobyMods[768];
GenModel* genMobyModels[768];

bool genSceneCreated = false;

extern HWND edit_genIp;

void SendLiveGenMobys();
void SetupCollisionLinks();
void RebuildCollisionTree();
void UpdateFlaggedMemory();
SceneSectorHeader* ResizeSector(int sectorId, const SceneSectorHeader* newHead);

void BuildSpyroSector(int sectorId);
void BuildGenSector(int sectorId);
void BuildGenMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrame);
void BuildSpyroMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrame);

DWORD lastMobyUpdateTime = 0;

bool colltreeInvalid = false; // set to true to rebuild colltris at the end of the frame

// Receive scene collision from Genesis
// Regenerate scene collision

// Auto-gen full collision and send

/* Joker list
80071834
80071844
80071854
800749BC
800749CC
800749DC
800749EC */

bool ConnectLiveGen() {
	if (live) {
		DestroyLiveGen(live);
		live = NULL;
	}

	// Connect
	live = CreateLiveGen();

	if (!live)
		return false;

	LgAddress addr;
	char addrText[20] = {0};

	SendMessageA(edit_genIp, WM_GETTEXT, 20, (LPARAM) addrText);
	addrText[19] = 0;

	addr.SetIp(addrText);
	addr.port = 6253;

	if (!live->Connect(&addr)) {
		DestroyLiveGen(live);
		live = NULL;
		return false;
	}

	return true;
}

#include <cmath> // look at everything mode
void MakeIceWall(int, int, int);

void UpdateLiveGen() {
	//if (((jokerPressed & BUT_L2) || (jokerPressed & BUT_R2)) && (joker & (BUT_L2 | BUT_R2)) == (BUT_L2 | BUT_R2))
	//	MakeIceWall(spyro->x, spyro->y, spyro->z);
	UpdateFlaggedMemory();

	if (colltreeInvalid) {
		RebuildCollisionTree();
		colltreeInvalid = false;
	}

	if (!live)
		return;

	live->Update();

	// Receive updates from Genesis and send them to genScene
	GenState liveStatePack, liveStatePack2;
	while (live->GetState(&liveStatePack, NULL, LGALLNODES))
		genScene.SetState(liveStatePack.GetStateData());

	// Update and validate objects that have been changed since the receive
	// They may or may not be invalidated later on, but by the end of this function they must be validated so that they're not re-sent to Genesis unless desired
	// That isn't actually programmed to happen anyway but uhhh it's on the list? Maybe!?!!?!? WHAT AM I DOING
	int test = genScene.GetNumObjects();
	for (GenObject* obj = genScene.GetNextChangedObject(NULL); obj != NULL; obj = genScene.GetNextChangedObject(obj)) {
		genid objId = obj->GetId();
		GenObjType objType = obj->GetObjType();

		// Update scene sector if applicable
		// Check if model has changed (e.g. mods changed, consequently mesh has changed)
		if (objId >= GENID_SCENESECTORMODEL && objId <= GENID_SCENESECTORMODEL + 256 && objType == GENOBJ_MODEL) {
			// in case the mods have changed
			BuildSpyroSector(objId - GENID_SCENESECTORMODEL);

			obj->ValidateAll();
		}

		// Check if a model's mesh has changed
		if (objType == GENOBJ_MESH && sceneData) {
			for (int i = 0; i < sceneData->numSectors; i++) {
				if (GenModel* model = (GenModel*) genScene.GetObjectById(GENID_SCENESECTORMODEL + i)) {
					genid id1 = obj->GetId(), id2 = model->GetMeshId();
					if (obj == model->GetMesh())
						BuildSpyroSector(i);
				}
			}

			obj->ValidateAll();
		}

		// Update moby if applicable
		if (objId >= GENID_MOBYINSTANCES && objId <= GENID_MOBYINSTANCES + 300 && objType == GENOBJ_INST) {
			int mobyId = objId - GENID_MOBYINSTANCES;
			float zSubtract = MOBYSPHERESIZE;
			
			if (mobys[mobyId].type < 768 && mobyModels && (mobyModels[mobys[mobyId].type].address & 0x80000000))
				zSubtract = 0.0f;

			const GenTransform* trans = ((GenInst*)obj)->GetTransform();
			mobys[mobyId].angle.z = (int) (trans->localRotation.z / 6.28f * 255.0f);
			mobys[mobyId].x = (int) (trans->localTranslation.x / MOBYPOSSCALE);
			mobys[mobyId].y = (int) (trans->localTranslation.y / MOBYPOSSCALE);
			mobys[mobyId].z = (int) ((trans->localTranslation.z - zSubtract) / MOBYPOSSCALE);
			
			obj->ValidateAll();
		}

		// Move mobys
		if (objId >= GENID_MOBYMODELS && objId <= GENID_MOBYMODELS + 768) {
			GenModel* genModel = genScene.GetModelById(objId);

			if (genModel) {
				// Update mesh in case the mods changed
				genMobys[objId - GENID_MOBYMODELS] = genModel->GetMesh();

				if (genModel->GetMesh())
					obj->ValidateAll();
			}
		}

		// Move Spyro
		if (objId == GENID_SPYROINSTANCE) {
			const GenTransform* trans = ((GenInst*)obj)->GetTransform();
			spyro->x = (int) (trans->localTranslation.x / MOBYPOSSCALE);
			spyro->y = (int) (trans->localTranslation.y / MOBYPOSSCALE);
			spyro->z = (int) ((trans->localTranslation.z - MOBYSPHERESIZE) / MOBYPOSSCALE);
			obj->ValidateAll();
		}

		obj->ValidateAll();
		/*for (int i = 0; i < 768; i++) {
			if (obj == genMobys[i])
				BuildSpyroMobyModel(i, 0, 0);
		}*/
	}

/*			} else if (stateType == GENMOD_MESHDATA && stateId == GENID_COLLISIONDATAMOD && collData) {
				// COLLISION DATA RECEIVE
				CollTri* tris = (CollTri*) &((uint8*) memory)[collData->triangleData & 0x003FFFFF];
				int numTris = collData->numTriangles;

				GenState meshState;

				if (!liveState.GetState(&meshState, 0))
					continue;

				for (int i = 0, e = meshState.GetNumStates(); i < e; i++) {
					const GenStateData* curState;

					if (!(curState = meshState.GetStateData(i)))
						continue;

					genData = &curState->data;

					if (!genData) continue;

					switch (curState->info.type) {
						case GENMESH_VERTS: {
							int max = numTris;
							const GenVec3* verts = genData->vec3;

							if (genData->numElements / 3 < max)
								max = genData->numElements / 3;

							for (int tri = 0; tri < max; tri++) {
								tris[tri].SetPoints(tri, (int) (verts[tri*3].x / WORLDSCALE), (int) (verts[tri*3].y / WORLDSCALE), (int) (verts[tri*3].z / WORLDSCALE), 
									(int) (verts[tri*3+1].x / WORLDSCALE), (int) (verts[tri*3+1].y / WORLDSCALE), (int) (verts[tri*3+1].z / WORLDSCALE), 
									(int) (verts[tri*3+2].x / WORLDSCALE), (int) (verts[tri*3+2].y / WORLDSCALE), (int) (verts[tri*3+2].z / WORLDSCALE));
							}

							for (int tri = max; tri < collData->numTriangles; tri++) // blank out leftover triangles in memory
								tris[tri].xCoords = tris[tri].yCoords = tris[tri].zCoords = 0;

							if (max != collData->numTriangles)
								RebuildCollisionTree(); // some IDs have probably changed
							break;
						}
					}
				}
			}
		}
	}*/

	DWORD curTime = GetTickCount();

	if (curTime - lastMobyUpdateTime >= 1000) {
		//SendLiveGenMobys();
		lastMobyUpdateTime = curTime;
	}
}

char rawDataHackyStacky[sizeof (GenElements) + 256 * 4 * sizeof (gens32)];
SceneSectorHeader* levelOnStart = NULL;

void LiveGenOnLevelEntry() {
	if (!genSceneCreated) {
		genScene.Create();
		genScene.SetId(0xDEAFD00D);

		// Create Gen scene sectors and pointers and stuffs
		for (int i = 0; i < 256; i++) {
			GenModel* model = (GenModel*) genScene.CreateObject(GENID_SCENESECTORMODEL + i, GENOBJ_MODEL);
			GenMod* mod = (GenMod*) genScene.CreateObject(GENID_SCENESECTORMOD + i, GENOBJ_MOD);
			GenInst* inst = (GenInst*) genScene.CreateObject(GENID_SCENESECTORINSTANCE + i, GENOBJ_INST);
			
			genSectors[i] = model;
			model->AddModifier(mod->GetId());
			mod->SetModType(MOD_EDITMESH);
			inst->SetModel(GENID_SCENESECTORMODEL + i);
			inst->SetMaterial(GENID_SCENETEXTURE);
		}

		int nums[GENOBJ_NUMTYPES] = {0};
		for (int i = 0; i < genScene.GetNumObjects(); i++) {
			nums[genScene.GetObjectByIndex(i)->GetObjType()]++;
		}

		genSceneCreated = true;
	}

	// Initialise gensectors and validate all
	if (sceneData) {
		for (int s = 0; s < sceneData->numSectors; s++)
			BuildGenSector(s);

		genScene.ValidateAllObjects();
	}

	// Create the links between collision data and scenery data
	SetupCollisionLinks();

	// Reset memory pool
	memset(usedMem, 0, sizeof (usedMem));
}

void SendLiveGenScene() {
	if (!live)
		return;
	
	GenElements* data = (GenElements*) rawDataHackyStacky; // use a fixed-size stack portion for all our data today~

	// Send scene texture
	data->type = GENTYPE_CSTRING;
	GetLevelFilename(data->s8, SEF_TEXTURES);
	data->numElements = strlen(data->s8) + 1;
	
	if (FILE* texExists = fopen(data->s8, "rb"))
		fclose(texExists); // no need to save texture file; it already exists
	else
		SaveTexturesAsSingle(); // save the textures

	live->SendStateDirect(GENID_SCENETEXTURE, GENMAT_SOURCEFILE, data);

	// Send scenery data
	// Obtain a GenState for the entire scene and send all at once
	// This might be slow for now, compression and memory allocation optimisation may come in the future
	// I CHANGED MY MIND. It's more helpful to see the level while it transfer, so fragment it
	for (int i = 0; i < sceneData->numSectors; i++) {
		GenState instState(GENID_SCENESECTORINSTANCE + i, GENCOMMON_MULTIPLE), modState(GENID_SCENESECTORMOD + i, GENCOMMON_MULTIPLE), 
				modelState(GENID_SCENESECTORMODEL + i, GENCOMMON_MULTIPLE);
		GenState meshState(0xFFFFFFFF, GENCOMMON_MULTIPLE);
		GenState combinedState(0, GENCOMMON_MULTIPLE);
		GenState* stateList[] = {&instState, &modState, &modelState, &meshState};

		for (int i = 0; i < 3; i++)
			genScene.GetState(stateList[i]);

		if (GenMod* mod = genScene.GetModById(GENID_SCENESECTORMOD + i)) {
			meshState.SetInfo(mod->GetMeshId(), GENCOMMON_MULTIPLE);

			genScene.GetState(&meshState);
		}

		combinedState.AddStates(stateList, 4);
		live->SendState(&combinedState);
	}
}

void SendLiveGenCollision() {
	if (!live || (!collData && !s1CollData))
		return;

	uint32* data;
	int numTriangles;

	if (!s1CollData) {
		data = (uint32*) collData->triangleData;
		numTriangles = collData->numTriangles;
	} else {
		data = (uint32*) s1CollData->triangleData;
		numTriangles = s1CollData->numTriangles;
	}

	// Setup data structs
	GenState posState(GENID_COLLISIONDATAMOD, GENMESH_VERTS), indexState(GENID_COLLISIONDATAMOD, GENMESH_FACEVERTINDEX), 
			 modelIdState(GENID_COLLISIONDATAMODEL, GENMODEL_MODIDS), instIdState(GENID_COLLISIONDATAINSTANCE, GENINST_MODELID), 
			modTypeState(GENID_COLLISIONDATAMOD, GENMOD_TYPE);
	GenElements* posData, *indexData, *colourData, * modelid, *instid, *modTypeData;

	posData = posState.Lock(GENTYPE_VEC3, numTriangles * 3);
	indexData = indexState.Lock(GENTYPE_S32, numTriangles * 3);
	modelid = modelIdState.Lock(GENTYPE_ID, 1);
	instid = instIdState.Lock(GENTYPE_ID, 1);
	modTypeData = modTypeState.Lock(GENTYPE_U16, 1);
	
	// Set IDs
	instid->id[0] = GENID_COLLISIONDATAMODEL;
	modelid->id[0] = GENID_COLLISIONDATAMOD;
	modTypeData->u16[0] = MOD_EDITMESH;

	// Set vert positions
	// Get verts
	GenVec3* verts = posData->vec3;
	gens32* faceIndices = indexData->s32;
	uint16* refs = s1CollData ? s1CollData->blocks : collData->blocks;
	int blckStart[100] = {0};//((collData->triangleData & 0x003FFFFF) - (collData->blocks & 0x003FFFFF)) / 2;
	genu32 blckClrs[3] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000};
	int blck = 0;

	for (int i = 0; blck < 100; i++) {
		if (refs[i] & 0x8000)
			blckStart[blck++] = i;
	}

	for (int part = 0, e = 3*numTriangles; part < e; part += 3) {
		int16 offX = data[part+0] & 0x3FFF, offY = data[part+1] & 0x3FFF, offZ = data[part+2] & 0x3FFF;
		int16 p1X = offX, p2X = bitss(data[part+0], 14, 9) + offX, p3X = bitss(data[part+0], 23, 9) + offX;
		int16 p1Y = offY, p2Y = bitss(data[part+1], 14, 9) + offY, p3Y = bitss(data[part+1], 23, 9) + offY;
		int16 p1Z = offZ, p2Z = bitsu(data[part+2], 16, 8) + offZ, p3Z = bitsu(data[part+2], 24, 8) + offZ;

		verts[part+0].x = (float) p1X * WORLDSCALE; verts[part+0].y = (float) p1Y * WORLDSCALE; verts[part+0].z = (float) p1Z * WORLDSCALE;
		verts[part+1].x = (float) p2X * WORLDSCALE; verts[part+1].y = (float) p2Y * WORLDSCALE; verts[part+1].z = (float) p2Z * WORLDSCALE;
		verts[part+2].x = (float) p3X * WORLDSCALE; verts[part+2].y = (float) p3Y * WORLDSCALE; verts[part+2].z = (float) p3Z * WORLDSCALE;

		faceIndices[part] = part; faceIndices[part+1] = part + 1; faceIndices[part+2] = -(part + 2) - 1;

		// get a colour intensity
		/*genu32 colour = 0xFF000000;
		for (int i = 0; i < 3; i++)
		{
			for (int j = blckStart[i]; j < blckStart[i+1]; j++)
			{
				if ((refs[j] & 0x7FFF) == part / 3)
				{
					colour += blckClrs[i%3];
					colour |= 0xFF000000;
				}
			}
		}
		if (colour == 0xFF000000) colour = 0xFFFFFFFF;
		colourData->u32[part] = colourData->u32[part+1] = colourData->u32[part+2] = colour;*/
	}
	
	// Unlock all data
	posState.Unlock();
	indexState.Unlock();
	modelIdState.Unlock();
	instIdState.Unlock();
	modTypeState.Unlock();

	// Send mesh data
	GenState meshState(GENID_COLLISIONDATAMOD, GENMOD_MESHDATA);

	meshState.AddState(&posState);
	meshState.AddState(&indexState);

	live->SendState(&meshState);

	// Send ID data
	live->SendState(&instIdState);

	live->SendState(&modelIdState);

	// Send mod type
	live->SendState(&modTypeState);
}

int lastMobyX[1000], lastMobyY[1000], lastMobyZ[1000];

void SendLiveGenMobys() {
	if (!live)
		return;

	SaveObjectTextures();

	// TEST -- Send objects!
	if (mobys && numMobys) {
		GenElements* data = (GenElements*) rawDataHackyStacky;

		// Moby model
		data->type = GENTYPE_U16;
		data->numElements = 1;
		data->u16[0] = MOD_BASESPHERE;
		live->SendStateDirect(GENID_GENERICMOBYMOD, GENMOD_TYPE, data);

		GenExElements ex(GENTYPE_PROP);
		GenProp* prop = (GenProp*)ex.AddElement(GenProp::GetSize(GENTYPE_F32, 1));
		strcpy(prop->name, "radius");
		prop->value.type = GENTYPE_F32;
		prop->value.numElements = 1;
		prop->value.f32[0] = MOBYSPHERESIZE;

		data->type = GENTYPE_RAW;
		data->numElements = ex.GetRawSize();
		memcpy(data->raw, ex.GetRaw(), ex.GetRawSize());
		live->SendStateDirect(GENID_GENERICMOBYMOD, GENMOD_PROPS, data);

		data->type = GENTYPE_ID;
		data->numElements = 1;
		data->id[0] = GENID_GENERICMOBYMOD;
		live->SendStateDirect(GENID_GENERICMOBYMODEL, GENMODEL_MODIDS, data);

		for (int i = 0; (uintptr) &mobys[i] + sizeof (mobys[i]) < (uintptr) memory + 0x00200000; i++) {
			if (mobys[i].state == -1)
				break; // last moby
			else if (mobys[i].state < 0)
				continue; // dead moby
			//if (mobys[i].x == lastMobyX[i] && mobys[i].y == lastMobyY[i] && mobys[i].z == lastMobyZ[i])
			//	continue; // moby hasn't changed

			float zAdd = MOBYSPHERESIZE;
			data->type = GENTYPE_ID;
			data->numElements = 1;
			data->id[0] = GENID_GENERICMOBYMODEL;
			if (mobys[i].type < 768 && genScene.GetModelById(GENID_MOBYMODELS + mobys[i].type)) {
				data->id[0] = GENID_MOBYMODELS + mobys[i].type;
				zAdd = 0.0f;
			}
			live->SendStateDirect(GENID_MOBYINSTANCES + i, GENINST_MODELID, data);

			data->type = GENTYPE_TRANSFORM;
			data->numElements = 1;
			data->transform[0].Reset();
			data->transform[0].localRotation.z = (float) mobys[i].angle.z / 255.0f * 6.28f;
			data->transform[0].localTranslation.x = (float) mobys[i].x * MOBYPOSSCALE;
			data->transform[0].localTranslation.y = (float) mobys[i].y * MOBYPOSSCALE;
			data->transform[0].localTranslation.z = (float) mobys[i].z * MOBYPOSSCALE + zAdd;
			live->SendStateDirect(GENID_MOBYINSTANCES + i, GENINST_TRANSFORM, data);

			char name[250];

			sprintf(name, "moby %i (%08X)", i, (uintptr) &mobys[i] - (uintptr) memory);
			data->type = GENTYPE_CSTRING;
			data->numElements = strlen(name) + 1;
			strcpy(data->s8, name);
			live->SendStateDirect(GENID_MOBYINSTANCES + i, GENINST_NAME, data);

			data->type = GENTYPE_ID;
			data->numElements = 1;
			data->id[0] = GENID_MOBYTEXTURES;
			live->SendStateDirect(GENID_MOBYINSTANCES + i, GENINST_MATIDS, data);

			lastMobyX[i] = mobys[i].x;
			lastMobyY[i] = mobys[i].y;
			lastMobyZ[i] = mobys[i].z;
		}
	}
}

void SendLiveGenSpyro() {
	if (!live || !spyroModelPointer)
		return;

	GenState modelState(GENID_SPYROMODEL, GENMODEL_MODIDS);
	GenElements* elems = modelState.Lock(GENTYPE_ID, 1);
	elems->id[0] = GENID_SPYROMODELMOD;
	modelState.Unlock();

	GenState modState(GENID_SPYROMODELMOD, GENMOD_TYPE);

	elems = modState.Lock(GENTYPE_U16, 1);
	elems->u16[0] = MOD_EDITMESH;
	modState.Unlock();
	
	int animId = 16;
	uint32 animAddr = umem32[(umem32[(spyroModelPointer&0x003FFFFF)/4]&0x003FFFFF)/4+animId] & 0x003FFFFF;

	if (!animAddr)
		return;

	uint8 numVerts = umem8[animAddr + 3];
	uint32* verts = &umem32[(umem32[animAddr / 4 + 1] & 0x003FFFFF) / 4];
	uint32* faces = &umem32[(umem32[animAddr / 4 + 2] & 0x003FFFFF) / 4];
	GenState vertState(0, GENMESH_VERTS);
	elems = vertState.Lock(GENTYPE_VEC3, numVerts);

	int lastX = 0, lastY = 0, lastZ = 0;
	for (int i = 0; i < numVerts; i++) {
		int x = bitss(verts[i], 21, 11), y = bitss(verts[i], 10, 11), z = bitss(verts[i], 0, 10);
		elems->vec3[i].x = (lastX + x) / 1024.0f;
		elems->vec3[i].y = (lastY + y) / 1024.0f;
		elems->vec3[i].z = (lastZ + z) * 2.0f / 1024.0f;
		lastX += x; lastY += y; lastZ += z;
	}
	vertState.Unlock();

	int numFaces = umem32[0x001E12F4/4] / 0x14;
	GenState faceState(0, GENMESH_FACEVERTINDEX);
	elems = faceState.Lock(GENTYPE_S16, numFaces * 4);

	int numSides = 0;
	for (int i = 0; i < numFaces; i++) {
		uint32 face = umem32[(0x001E12F4 + 4 + i * 0x14) / 4];

		if ((face & 0xFF) == ((face >> 8) & 0xFF)) {
			elems->s16[numSides++] = face >> 24 & 0xFF;
			elems->s16[numSides++] = face >> 16 & 0xFF;
			elems->s16[numSides++] = -(int)(face >> 8 & 0xFF) - 1;
		} else {
			elems->s16[numSides++] = face >> 24 & 0xFF;
			elems->s16[numSides++] = face >> 16 & 0xFF;
			elems->s16[numSides++] = face & 0xFF;
			elems->s16[numSides++] = -(int)(face >> 8 & 0xFF) - 1;
		}
	}

	elems->numElements = numSides;
	faceState.Unlock();

	GenState instState(GENID_SPYROINSTANCE, GENINST_MODELID);
	elems = instState.Lock(GENTYPE_ID, 1);
	elems->id[0] = GENID_SPYROMODEL;
	instState.Unlock();

	GenState modMeshState(GENID_SPYROMODELMOD, GENMOD_MESHDATA);

	modMeshState.AddState(&vertState);
	modMeshState.AddState(&faceState);

	// Send all states
	live->SendState(&modState);
	live->SendState(&modMeshState);
	live->SendState(&modelState);
	live->SendState(&instState);
}

void SendLiveGenMobyModel(int modelId, int mobyId = -1) {
	if (!live || !mobyModels)
		return;

	if (mobyId > -1)
		BuildGenMobyModel(modelId, mobys[mobyId].anim.nextanim, mobys[mobyId].anim.nextframe);
	else
		BuildGenMobyModel(modelId, 0, 0);

	if (genMobys[modelId]) {
		// Create dummy instance of model
		if (!genScene.GetInstById(GENID_MOBYPREVIEWINSTANCES + modelId)) {
			GenInst* inst = (GenInst*) genScene.CreateObject(GENID_MOBYPREVIEWINSTANCES + modelId, GENOBJ_INST);

			inst->SetModel(GENID_MOBYMODELS + modelId);

			genwchar str[256];

			swprintf(str, L"Moby %i/%04X %08X", modelId, modelId, mobyModels[modelId].address & 0x003FFFFF);
			inst->SetName(str);
		}
		
		// Send texture
		char filename[MAX_PATH];

		GetLevelFilename(filename, SEF_OBJTEXTURES);

		GenState texState(GENID_MOBYTEXTURES, GENMAT_SOURCEFILE); // todo genSceneify
		GenElements* elems = texState.Lock(GENTYPE_CSTRING, strlen(filename) + 1);
		
		strcpy(elems->s8, filename);
		texState.Unlock();

		live->SendState(texState.GetStateData());

		// Send model states
		GenState state;
		GenObject* objList[] = {genMobyModels[modelId], genMobyMods[modelId], genScene.GetInstById(GENID_MOBYPREVIEWINSTANCES + modelId)};
		
		for (int i = 0; i < sizeof (objList) / sizeof (objList[0]); i++) {
			if (!objList[i])
				continue;

			state.SetInfo(objList[i]->GetId(), GENCOMMON_MULTIPLE);

			objList[i]->GetState(&state);

			live->SendState(state.GetStateData());
			objList[i]->ValidateAll();
		}
	}
}

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

	SendLiveGenMobys();
}

uint32 FindFreeMemory(int sectorSize) {
	// Find the longest trail of free memory
	int curTrailLength = 0;
	int biggestTrailId = -1;
	int biggestTrailLength = 0;
	for (int i = 0x00020000/1024; i < (0x00200000 / 1024); i++) {
		if (!(usedMem[i / 32] & (1 << (i & 31)))) {
			curTrailLength++;
		} else {
			if (curTrailLength > biggestTrailLength) {
				biggestTrailLength = curTrailLength;
				biggestTrailId = i - curTrailLength;
			}

			curTrailLength = 0;
		}
	}

	if (biggestTrailLength > 2 && biggestTrailLength * 1024 >= sectorSize) {
		return (uint32) biggestTrailId * 1024;
	} else
		return NULL; // probably isn't gonna work out
}

void UpdateFlaggedMemory() {
	for (int i = 0; i < 0x00200000; i += 1024) {
		for (uint32* mem = &umem32[i / 4], *e = &umem32[(i + 1024) / 4]; mem < e; mem++) {
			if (*mem) {
				usedMem[i / 1024 / 32] |= (1 << ((i / 1024) & 31));
				break;
			}
		}
	}
}

void BuildSpyroSector(int sectorId) {
	if (sectorId >= sceneData->numSectors || sectorId < 0)
		return;
	if (!genSectors[sectorId])
		return;

	GenMesh* genSector = genSectors[sectorId]->GetMesh();

	if (!genSector)
		return;

	SceneSectorHeader* sector = sceneData->sectors[sectorId];
	bool flatSector = ((sector->centreRadiusAndFlags >> 12) & 1);
	int oldNumVerts = sector->numHpVertices, oldNumFaces = sector->numHpFaces;
	int numVerts = genSector->GetNumVerts(), numFaces = genSector->GetNumFaces(), numColours = genSector->GetNumColours();

	// 000A96D0 flipped triangle in Midday Gardens

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

		if (SceneSectorHeader* newSector = ResizeSector(sectorId, &newHead))
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
	for (int i = 0; i < numFaces; i++) {
		int add = (genFaces[i].numSides == 3) ? 1 : 0;
		bool isFlipped = faces[i].GetFlip();
		for (int j = 0; j < genFaces[i].numSides; j++) {
			faces[i].verts[j + add] = genFaces[i].sides[3 - add - j].vert;
			faces[i].colours[j + add] = genFaces[i].sides[3 - add - j].colour;
		}

		if (genFaces[i].numSides == 3)
			faces[i].verts[0] = faces[i].verts[1];

		// Safety
		for (int j = 0; j < 4; j++) {
			if (numVerts)
				faces[i].verts[j] %= numVerts;
			else
				faces[i].verts[j] = 0;
			if (numColours)
				faces[i].colours[j] %= numColours;
			else
				faces[i].colours[j] = 0;
		}

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
	CollTri* triangles;
	uint16* triangleTypes = nullptr;
	int numTriangles = 0;

	if (collData) {
		triangles = collData->triangleData;
		triangleTypes = collData->surfaceType;
		numTriangles = collData->numTriangles;
	} else if (s1CollData) {
		triangles = s1CollData->triangleData;
		numTriangles = s1CollData->numTriangles;
	}
	
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
			CollTriCache* stealMe = &collisionCache.unlinkedCaches[collisionCache.numUnlinkedTriangles];
			CollTriCache* linkedTri = &cache[sectorCache->numTriangles];

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

			linkedTri->faceIndex = face;
			linkedTri->triangleType = 0;

			if (triangleTypes) {
				triangleTypes[linkedTri->triangleIndex] = 0;
				linkedTri->triangleType = 0;
			}

			// Decrement the unlinked triangle count after our thievery
			collisionCache.numUnlinkedTriangles--;
			sectorCache->numTriangles++;

		}
	}

	// Use the cache to move collision triangles
	for (CollTriCache* cacheTri = cache, *e = &cache[sectorCache->numTriangles]; cacheTri < e; cacheTri++) {
		SceneFace* face = &sector->GetHpFaces()[cacheTri->faceIndex];
		const IntVector &vert1 = absoluteVerts[face->verts[cacheTri->vert1Index]], &vert2 = absoluteVerts[face->verts[cacheTri->vert2Index]], 
						&vert3 = absoluteVerts[face->verts[cacheTri->vert3Index]];

		triangles[cacheTri->triangleIndex].SetPoints(0, vert1.x, vert1.y, vert1.z, vert2.x, vert2.y, vert2.z, vert3.x, vert3.y, vert3.z);
	}
}

void BuildGenSector(int sectorId) {
	if (sectorId >= sceneData->numSectors || sectorId < 0 || !genSectors[sectorId])
		return;

	SceneSectorHeader* sector = sceneData->sectors[sectorId];
	GenMesh* genSector = genSectors[sectorId]->GetMesh();

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

void BuildGenMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrameIndex) {
	if (mobyModelId >= 768)
		return;
	if (!mobyModels[mobyModelId].address)
		return;

	UpdateObjTexMap();

	// Create the container model and mod
	if (!genMobyModels[mobyModelId]) {
		genMobyModels[mobyModelId] = (GenModel*)genScene.CreateObject(GENID_MOBYMODELS + mobyModelId, GENOBJ_MODEL);
		if (!genMobyMods[mobyModelId]) {
			genMobyMods[mobyModelId] = (GenMod*) genScene.CreateObject(GENID_MOBYMODS + mobyModelId, GENOBJ_MOD);
			genMobyMods[mobyModelId]->SetModType(MOD_EDITMESH);

			genMobys[mobyModelId] = genMobyMods[mobyModelId]->GetMesh();
		}

		genMobyModels[mobyModelId]->AddModifier(GENID_MOBYMODS + mobyModelId);
	}

	if (!genMobys[mobyModelId])
		return; // failed to create moby model

	GenMesh* genMoby = genMobys[mobyModelId];
	uint32* verts = NULL, *faces = NULL, *colours = NULL;
	uint16* blocks = NULL;
	uint8* adjusts = NULL;
	int numVerts = 0, numColours = 0;
	int maxNumFaces = 0;
	uint32 frameStartFlags = 0;
	float scale = MOBYPOSSCALE;
	bool animated = (mobyModels[mobyModelId].address & 0x80000000);

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
		scale = (float) (MOBYPOSSCALE) * ((float) (anim->vertScale + 1));
		frameStartFlags = frame->startFlags;
	} else {
		SimpleModelHeader* model = (SimpleModelHeader*) (mobyModels[mobyModelId]); // make sure the conversion works

		if (animId >= model->numStates)
			return;

		SimpleModelStateHeader* state = model->states[animId];
		verts = state->data32; faces = &state->data32[(state->hpFaceOff - 16) / 4]; colours = &state->data32[(state->hpColourOff - 16) / 4];
		numVerts = state->numHpVerts; numColours = state->numHpColours;
		maxNumFaces = faces[0] / 8;
		scale = MOBYPOSSCALE * 4.0f;
	}

	if (numVerts >= 300 || numColours >= 300 || maxNumFaces >= 600)
		return;

	// Send frame vertices
	genMoby->SetNum(GENMET_VERT, numVerts);
	GenMeshVert* genVerts = genMoby->LockVerts();

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
					int tZ = bitss(blocks[curBlock], 2, 4), tY = bitss(blocks[curBlock], 6, 5), tX = bitss(blocks[curBlock], 11, 5);
					curX += tX * 4; curY += tY * 4; curZ += tZ * 4;
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

	genMoby->UnlockVerts(genVerts);

	// Set model faces
	genMoby->SetNum(GENMET_FACE, maxNumFaces);

	GenMeshFace* genFaces = genMoby->LockFaces();
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

	genMoby->UnlockFaces(genFaces);

	// Set UVs according to the temporary UVs
	genMoby->SetNum(GENMET_UV, numUvs);
	GenVec2* genUvs = genMoby->LockUvs();

	for (int i = 0; i < numUvs; i++)
		genUvs[i].Set((float) tempU[i] / 65535, (float) tempV[i] / 65535);

	genMoby->UnlockUvs(genUvs);

	// Set colours
	genMoby->SetNum(GENMET_COLOUR, numColours);
	genu32* genColours = genMoby->LockColours();

	for (int i = 0; i < numColours; i++)
		genColours[i] = colours[i] | 0xFF000000;

	genMoby->UnlockColours(genColours);
}

void BuildSpyroMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrameIndex) {
	GenMesh* genMoby = genMobys[mobyModelId];

	if (!(mobyModels[mobyModelId].address & 0x80000000))
		return;

	ModelHeader* model = mobyModels[mobyModelId];

	if (animId >= model->numAnims)
		return;

	ModelAnimHeader* anim = model->anims[animId];
	ModelAnimFrameInfo* frame = &anim->frames[animFrameIndex];

	uint32* verts = anim->verts, *faces = anim->faces, *colours = anim->colours;
	uint16* blocks = &anim->data[frame->blockOffset];
	uint8* adjusts = (uint8*) &blocks[frame->numBlocks];
	int numVerts = anim->numVerts, numColours = anim->numColours;
	int maxNumFaces = faces[0] / 8;
	const float scale = (float) (MOBYPOSSCALE) * ((float) (anim->vertScale + 1));

	// Set colours
	genu32* genColours = genMoby->LockColours();
	genu32 genNumColours = genMoby->GetNumColours();

	for (int i = 0; i < anim->numColours && i < genNumColours; i++)
		anim->colours[i] = genColours[i] & 0x00FFFFFF;

	genMoby->UnlockColours(genColours);
}

void MakeCrater(int craterX, int craterY, int craterZ) {
	if (!sceneData)
		return;
	
	const float range = 6.0f;
	const float depth = 0.7f;
	const float rimHeight = 1.1f;
	const float rimWidth = 3.0f;
	const float crushFactor = 1.0f, crushFactorOuter = 0.5f;
	float fX = (float) (craterX >> 4) * WORLDSCALE, fY = (float) (craterY >> 4) * WORLDSCALE, fZ = (float) (craterZ >> 4) * WORLDSCALE;
	for (int i = 0; i < sceneData->numSectors; i++) {
		BuildGenSector(i);

		GenMesh* genSector = NULL;
		if (genSectors[i]) {
			genSector = genSectors[i]->GetMesh();
			if (!genSector)
				continue;
		}

		GenMeshVert* genVerts = genSector->LockVerts();
		bool changeMade = false;
		for (int j = 0, e = genSector->GetNumVerts(); j < e; j++) {
			float diffX = genVerts[j].pos.x - fX, diffY = genVerts[j].pos.y - fY, diffZ = genVerts[j].pos.z - fZ;
			float dist = sqrt(diffX * diffX + diffY * diffY + diffZ * diffZ);

			if (dist <= range) {
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
			BuildSpyroSector(i);
		}
		genSector->UnlockVerts(genVerts);
	}

	RebuildCollisionTree();

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

void MakeIceWall(int _x, int _y, int _z) {
	if (!sceneData)
		return;

	int sX = _x >> 4, sY = _y >> 4, sZ = _z >> 4;
	float x = (float) (_x >> 4) * WORLDSCALE, y = (float) (_y >> 4) * WORLDSCALE, z = (float) (_z >> 4) * WORLDSCALE;
	uint64 maxX = 0, maxY = 0, maxZ = 0;
	uint32 closestId = 0;
	uint64 closestDist = 0xFFFFFFFFFFFFFFFFll;

	for (int i = 0; i < sceneData->numSectors; i++) {
		SceneSectorHeader* sector = sceneData->sectors[i];
		int sectorX = sector->xyPos >> 16, sectorY = (sector->xyPos & 0xFFFF), sectorZ = ((sector->zPos >> 14) & 0xFFFF) >> 2;
		uint64 dist = (sectorX - sX) * (sectorX - sX) + (sectorY - sY) * (sectorY - sY) + (sectorZ - sZ) * (sectorZ - sZ);

		if (dist < closestDist && sectorX < sX && sectorY < sY && sectorZ < sZ) {
			closestId = i;
			closestDist = dist;
		}
	}

	// Ice wall, coming right up
	SceneSectorHeader* sector = sceneData->sectors[closestId];
	GenMesh* genSector = NULL;

	if (genSectors[closestId]) {
		genSector = genSectors[closestId]->GetMesh();

		if (!genSector)
			return;
	}

	int startVert = genSector->GetNumVerts(), startFace = genSector->GetNumFaces();
	
	if (startVert + 4 < 256 && startFace + 1 < 256) {
		BuildGenSector(closestId);
		
		genSector->SetNum(GENMET_FACE, startFace + 1);
		genSector->SetNum(GENMET_VERT, startVert + 4);
		GenMeshVert* genVerts = genSector->LockVerts();
		GenMeshFace* genFaces = genSector->LockFaces();
		
		genVerts[startVert + 0].pos.Set(x - 1.0f, y - 1.0f, z + 0.25f);
		genVerts[startVert + 1].pos.Set(x - 1.0f, y + 1.0f, z + 0.25f);
		genVerts[startVert + 2].pos.Set(x + 1.0f, y + 1.0f, z + 0.25f);
		genVerts[startVert + 3].pos.Set(x + 1.0f, y - 1.0f, z + 0.25f);
		
		for (int i = 0; i < 4; i++) {
			genFaces[startFace].sides[i].vert = startVert + i;
			genFaces[startFace].sides[i].colour = 0;
			genFaces[startFace].sides[i].uv = 0;
		}
		
		genFaces[startFace].numSides = 4;

		BuildSpyroSector(closestId);

		genSector->UnlockVerts(genVerts);
		genSector->UnlockFaces(genFaces);
	}
}

void MoveSceneData(SceneSectorHeader* sector, int start, int size, int moveOffset) {
	if (moveOffset > 0) {
		int difference = moveOffset;

		for (int i = start + size + difference - 1, e = start + difference; i >= e; i--)
			sector->data32[i] = sector->data32[i - difference];
	} else if (moveOffset < 0) {
		int difference = -moveOffset;

		for (int i = start - difference; i < start + size - difference; i++)
			sector->data32[i] = sector->data32[i + difference];
	}
}

SceneSectorHeader* ResizeSector(int sectorId, const SceneSectorHeader* newHead) {
	if (!sceneData)
		return NULL;
	if (sectorId >= sceneData->numSectors)
		return NULL;

	// Resizing the sector means finding some place to put it in memory
	// if the sector is smaller than it was before, this isn't a problem, we don't need to move it at all
	// it's a different story however if it's bigger, in which case it needs to be moved to what SpyroEdit believes is a free memory space
	SceneSectorHeader* head = sceneData->sectors[sectorId];
	int curSize = head->GetSize();
	int newSize = newHead->GetSize();
	uint32 newSpot = sceneData->sectors[sectorId].address & 0x003FFFFF;
	uint8 backupSectorData[16384];
	SceneSectorHeader* backupSector = (SceneSectorHeader*) backupSectorData;

	if (newSize > curSize) {
		// First count the amount of space left in the main scene memory. If the user has deleted level segments we can defragment and use it.
		int usedSize = (sceneData->numSectors * 4) + 0x0C;
		for (int i = 0; i < sceneData->numSectors; i++) {
			usedSize += sceneData->sectors[i]->GetSize();
		}
		usedSize -= head->GetSize();

		if (sceneData->size >= usedSize + newSize) {
			// Defragment sectors while creating extra room for this one
			static uint8 backupSectors[0x00050000];
			static uint32 backupSectorOffsets[512];
			uint32 curOffset = 0;

			for (int i = 0; i < sceneData->numSectors; i++) {
				int sectorSize = sceneData->sectors[i]->GetSize();
				memcpy(&backupSectors[curOffset], sceneData->sectors[i], sectorSize);

				backupSectorOffsets[i] = curOffset;

				if (i != sectorId)
					curOffset += sectorSize;
				else
					curOffset += newSize;
			}
			
			uint32 baseAddress = (uint8*)&sceneData->sectors[sceneData->numSectors] - umem8;
			memcpy(&umem8[baseAddress], backupSectors, curOffset);

			for (int i = 0; i < sceneData->numSectors; i++)
				sceneData->sectors[i].address = (baseAddress + backupSectorOffsets[i]) | 0x80000000;

			newSpot = sceneData->sectors[sectorId].address & 0x003FFFFF;
			head = sceneData->sectors[sectorId];
		} else {
			newSpot = FindFreeMemory(newSize); // Move the sector to a bigger memory area if possible
			
			if (!newSpot)
				return NULL; // no memory, cannot resize the sector to this size

			// Copy the old sector to the new place
			memcpy(&umem8[newSpot], head, head->GetSize());
			
			// Zero out the old sector. This enables us to reuse the memory if needed
			memset(head, 0, curSize);

			// Update pointer to sector
			sceneData->sectors[sectorId].address = newSpot | 0x80000000;

			// Reassign header pointer
			head = (SceneSectorHeader*) &umem8[newSpot];
		}
	}
	
	// Backup the old header and data for reshuffle
	memcpy(backupSector, &head, head->GetSize());

	// Copy the new header
	memcpy((SceneSectorHeader*) head, newHead, 7 * 4);
	head->zTerminator = 0xFFFFFFFF;

	// Shuffle verts, faces etc
	uint32* basicUintShuffle[] = {backupSector->GetLpVertices(), head->GetLpVertices(), backupSector->GetLpColours(), head->GetLpColours(), 
								  backupSector->GetHpVertices(), head->GetHpVertices(), backupSector->GetHpColours(), head->GetHpColours()};
	int shuffleCount[] = {backupSector->numLpVertices, head->numLpVertices, backupSector->numLpColours, head->numLpColours, 
						  backupSector->numHpVertices, head->numHpVertices, backupSector->numHpColours, head->numHpColours};

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

	uint32* oldLpFaces = backupSector->GetLpFaces(), *newLpFaces = head->GetLpFaces();
	for (int i = 0; i < head->numLpFaces * 2; i++) {
		if (i < backupSector->numLpFaces * 2)
			newLpFaces[i] = oldLpFaces[i];
		else
			newLpFaces[i] = 0;
	}

	SceneFace* oldHpFaces = backupSector->GetHpFaces(), *newHpFaces = head->GetHpFaces();
	for (int i = 0; i < head->numHpFaces; i++) {
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
		memset((void*) ((uintptr) head + newSize), 0, curSize - newSize);

	// Done!
	return head;
}

void SetupCollisionLinks() {
	// Reset collision cache
	for (int i = 0; i < sceneData->numSectors; i++)
		collisionCache.sectorCaches[i].numTriangles = 0;
	collisionCache.numUnlinkedTriangles = 0;

	// NEW: Rebuild collision cache
	if ((collData/* || s1CollData uncommented until surface types clarified */) && sceneData && sceneData->numSectors < 0xFF) {
		CollTri* triangles = (collData ? collData->triangleData : s1CollData->triangleData);
		int numTriangles = (collData ? collData->numTriangles : s1CollData->numTriangles);
		uint16* triangleTypes = (collData ? collData->surfaceType : (uint16*)nullptr); // lol? where is the Spyro 1 surface type
		const int maxSeparationDistance = 4;
		bool* cachedTriangles = (bool*) malloc(collData->numTriangles * sizeof (bool)); // For each triangle, value is true if it has now been cached
		memset(cachedTriangles, 0, collData->numTriangles * sizeof (bool));

		// This operation is slow, so we'll deliberately compartmentalise the collision triangles into smaller--wait. Doesn't Spyro already do this?
		// Todo: Abuse this Spyro feature
		for (int s = 0; s < sceneData->numSectors; s++) {
			SceneSectorHeader* sector = sceneData->sectors[s];
			CollSectorCache* collSector = &collisionCache.sectorCaches[s];
			SceneFace* faces = sceneData->sectors[s]->GetHpFaces();
			uint32* verts = sceneData->sectors[s]->GetHpVertices();
			int sectorX = sector->xyPos >> 16, sectorY = (sector->xyPos & 0xFFFF), sectorZ = ((sector->zPos >> 14) & 0xFFFF) >> 2;
			bool flatSector = (sector->centreRadiusAndFlags >> 12 & 1);
			IntVector absoluteVerts[256];
			int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX, maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;

			// Get absolute vertex positions and sector boundaries
			for (int i = 0; i < sector->numHpVertices; i++) {
				uint32 curVertex = verts[i];
				int curX = sectorX + ((curVertex >> 19 & 0x1FFC) >> 2), curY = sectorY + ((curVertex >> 8 & 0x1FFC) >> 2),
					curZ = sectorZ + ((curVertex << 3 & 0x1FFC) >> 2);

				if (game == SPYRO1)
					curZ = sectorZ + ((curVertex << 3 & 0x1FFC) >> 3);

				if (flatSector)
					curZ >>= 3;

				absoluteVerts[i].x = curX;
				absoluteVerts[i].y = curY;
				absoluteVerts[i].z = curZ;

				if (curX < minX) minX = curX;
				if (curY < minY) minY = curY;
				if (curZ < minZ) minZ = curZ;
				if (curX > maxX) maxX = curX;
				if (curY > maxY) maxY = curY;
				if (curZ > maxZ) maxZ = curZ;
			}

			// Stretch out the sector boundaries a little so we can eliminate triangles without an additional subtraction
			minX -= 256; minY -= 256; minZ -= 256; maxX += 256; maxY += 256; maxZ += 256;

			// Search through triangles to see which ones match
			for (CollTri* tri = triangles; tri < &triangles[numTriangles]; ++tri) {
				// Eliminate triangles that are outside the sector boundary
				int16 p1X = bitsu(tri->xCoords, 0, 14), p2X = bitss(tri->xCoords, 14, 9) + p1X, p3X = bitss(tri->xCoords, 23, 9) + p1X;
				if (p1X < minX || p1X > maxX)
					continue;
				int16 p1Y = bitsu(tri->yCoords, 0, 14), p2Y = bitss(tri->yCoords, 14, 9) + p1Y, p3Y = bitss(tri->yCoords, 23, 9) + p1Y;
				if (p1Y < minY || p1Y > maxY)
					continue;
				int16 p1Z = bitsu(tri->zCoords, 0, 14), p2Z = bitsu(tri->zCoords, 16, 8) + p1Z, p3Z = bitsu(tri->zCoords, 24, 8) + p1Z;
				if (p1Z < minZ || p1Z > maxZ)
					continue;

				// Check if the vertices match with any of this sector's faces
				int matches[4];
				for (int f = 0; f < sector->numHpFaces; ++f) {
					int numMatches = 0;

					for (int side = (faces[f].verts[0] == faces[f].verts[1] ? 1 : 0); side < 4; side++) {
						IntVector* vert = &absoluteVerts[faces[f].verts[side]];
						if (vert->x >= p1X - maxSeparationDistance && vert->x <= p1X + maxSeparationDistance &&
							vert->y >= p1Y - maxSeparationDistance && vert->y <= p1Y + maxSeparationDistance &&
							vert->z >= p1Z - maxSeparationDistance && vert->z <= p1Z + maxSeparationDistance) {
							matches[0] = side;
							++numMatches;
						}

						if (vert->x >= p2X - maxSeparationDistance && vert->x <= p2X + maxSeparationDistance &&
							vert->y >= p2Y - maxSeparationDistance && vert->y <= p2Y + maxSeparationDistance &&
							vert->z >= p2Z - maxSeparationDistance && vert->z <= p2Z + maxSeparationDistance) {
							matches[1] = side;
							++numMatches;
						}

						if (vert->x >= p3X - maxSeparationDistance && vert->x <= p3X + maxSeparationDistance &&
							vert->y >= p3Y - maxSeparationDistance && vert->y <= p3Y + maxSeparationDistance &&
							vert->z >= p3Z - maxSeparationDistance && vert->z <= p3Z + maxSeparationDistance) {
							matches[2] = side;
							++numMatches;
						}
					}

					if (numMatches >= 3) {
						// Match found! Throw it into the cache
						collSector->triangles[collSector->numTriangles].faceIndex = f;
						collSector->triangles[collSector->numTriangles].triangleIndex = tri - triangles;
						collSector->triangles[collSector->numTriangles].trianglePoints = *tri;
						collSector->triangles[collSector->numTriangles].triangleType = triangleTypes[tri - triangles];
						collSector->triangles[collSector->numTriangles].vert1Index = matches[0];
						collSector->triangles[collSector->numTriangles].vert2Index = matches[1];
						collSector->triangles[collSector->numTriangles].vert3Index = matches[2];
						collSector->numTriangles++;

						cachedTriangles[tri - triangles] = true;
					}
				}
			}
		}

		/* Great! Now register triangles that haven't been linked to a sector in order of most important to least important
			For now, we'll assume the least important triangles to be those highest up on the map. These are the ones we'll steal if the user creates terrain
			Don't write a complex sorting algorithm for now. Instead, just split them into 8 segments */
		// Get the height range of the map first
		int maxZ = -32767, minZ = 32767;
		for (CollTri* tri = triangles, *e = &triangles[numTriangles]; tri < e; tri++) {
			int z = bitsu(tri->zCoords, 0, 14);

			if (z < minZ)
				minZ = z;
			if (z > maxZ)
				maxZ = z;
		}

		// Throw the triangles into the unlinked cache, lowest to highest
		for (int currentRange = 0; currentRange < 8; currentRange++) {
			int currentMinZ = minZ + (maxZ - minZ) * currentRange / 8, currentMaxZ = minZ + (maxZ - minZ) * (currentRange + 1) / 8;
			for (CollTri* tri = triangles, *e = &triangles[numTriangles]; tri < e; tri++) {
				if (cachedTriangles[tri - triangles])
					continue; // a sector owns this triangle

				if (bitsu(tri->zCoords, 0, 14) >= currentMinZ && bitsu(tri->zCoords, 0, 14) < currentMaxZ) {
					CollTriCache* cacheTri = &collisionCache.unlinkedCaches[collisionCache.numUnlinkedTriangles++];
						
					cacheTri->triangleIndex = tri - triangles;
					cacheTri->faceIndex = -1;
					cacheTri->triangleType = triangleTypes[tri - triangles];
						
					cachedTriangles[tri - triangles] = true;
				}
			}
		}

		free(cachedTriangles);
	}
}

void RebuildCollisionTriangles() {
	if ((!collData && !s1CollData) || !sceneData)
		return;

	CollTri* triangles = NULL;
	uint16* surfaces = NULL;
	int oldNumTriangles = 0;

	if (collData) {
		triangles = collData->triangleData;
		surfaces = collData->blocks;
		oldNumTriangles = collData->numTriangles;
	} else if (s1CollData) {
		triangles = s1CollData->triangleData;
		surfaces = s1CollData->blocks;
		oldNumTriangles = s1CollData->numTriangles;
	}

	// Perform massive triangle build
	int numTriangles = 0;

	for (int s = 0; s < sceneData->numSectors; s++) {
		GenMesh* genSector = NULL;
		BuildGenSector(s);

		if (genSectors[s])
			genSector = genSectors[s]->GetMesh();
		if (!genSector)
			continue;

		const GenMeshVert* genVerts = genSector->GetVerts();
		const GenMeshFace* genFaces = genSector->GetFaces();
		for (int i = 0, numFaces = genSector->GetNumFaces(); i < numFaces && numTriangles < collData->numTriangles; i++) {
			for (int t = 0; t < genFaces[i].numSides - 2; t++) {
				const GenVec3* vert1 = &genVerts[genFaces[i].sides[t*2].vert].pos, *vert2 = &genVerts[genFaces[i].sides[t*2+1].vert].pos,
							  *vert3 = &genVerts[genFaces[i].sides[(t*2+2) & 3].vert].pos;

				// Relocate the vertices
				triangles[numTriangles].SetPoints(numTriangles,
					(int)(vert1->x / WORLDSCALE), (int)(vert1->y / WORLDSCALE), (int)(vert1->z / WORLDSCALE), 
					(int)(vert2->x / WORLDSCALE), (int)(vert2->y / WORLDSCALE), (int)(vert2->z / WORLDSCALE), 
					(int)(vert3->x / WORLDSCALE), (int)(vert3->y / WORLDSCALE), (int)(vert3->z / WORLDSCALE));

				surfaces[numTriangles] = 0;
				numTriangles++;
			}
		}
	}
	
	if (collData)
		collData->numTriangles = numTriangles;
	else if (s1CollData)
		s1CollData->numTriangles = numTriangles;

	// Relink collision data
	SetupCollisionLinks();

	RebuildCollisionTree();
}

struct Point16 {
	int16 x, y, z;
};

struct TriBounds {
	union {
		struct {
			Point16 p1, p2, p3;
		};
		Point16 points[3];
	};
	uint8 minXBlock, maxXBlock, minYBlock, maxYBlock, minZBlock, maxZBlock;
};

void RebuildCollisionTree() {
	if (!collData && !s1CollData)
		return;

	uint32* tri;
	int numTris;
	uint32 blockAddr, blockTreeAddr, triangleDataAddr;
	
	if (collData) {
		 tri = (uint32*) collData->triangleData;
		 numTris = collData->numTriangles;
		 blockAddr = collData->blocks.address;
		 blockTreeAddr = collData->blockTree.address;
		 triangleDataAddr = collData->triangleData.address;
	} else {
		tri = (uint32*) s1CollData->triangleData;
		numTris = s1CollData->numTriangles;
		blockAddr = s1CollData->blocks.address;
		blockTreeAddr = s1CollData->blockTree.address;
		triangleDataAddr = s1CollData->triangleData.address;
	}

	// phase 1: find minimum and maximum collision vert positions and count the size of each block	 
	int xBlockSize[256] = {0}, yBlockSize[256] = {0}, zBlockSize[256] = {0};
	TriBounds* bounds = (TriBounds*) malloc(numTris * sizeof (TriBounds));
	TriBounds* curBounds = bounds;

	for (uint32 *start = &tri[0], *end = &tri[numTris * 3]; tri < end; tri += 3) {
		int16 p1X = tri[0] & 0x3FFF, p2X = ((tri[0] >> 14) & 0x1FF), p3X = ((tri[0] >> 23) & 0x1FF);
		int16 p1Y = tri[1] & 0x3FFF, p2Y = ((tri[1] >> 14) & 0x1FF), p3Y = ((tri[1] >> 23) & 0x1FF);
		int16 p1Z = tri[2] & 0x3FFF, p2Z = ((tri[2] >> 16) & 0x0FF), p3Z = ((tri[2] >> 24) & 0x0FF);

		if (p2X & 0x100) p2X -= 0x0200; if (p3X & 0x100) p3X -= 0x0200;
		if (p2Y & 0x100) p2Y -= 0x0200; if (p3Y & 0x100) p3Y -= 0x0200;
		p2X += p1X; p3X += p1X; p2Y += p1Y; p3Y += p1Y; p2Z += p1Z; p3Z += p1Z;

		curBounds->p1.x = p1X; curBounds->p1.y = p1Y; curBounds->p1.z = p1Z;
		curBounds->p2.x = p2X; curBounds->p2.y = p2Y; curBounds->p2.z = p2Z;
		curBounds->p3.x = p3X; curBounds->p3.y = p3Y; curBounds->p3.z = p3Z;

		p1X >>= 8; p1Y >>= 8; p1Z >>= 8; p2X >>= 8; p2Y >>= 8; p2Z >>= 8;
		p3X >>= 8; p3Y >>= 8; p3Z >>= 8;
		// get boundaries
		curBounds->minXBlock = p1X;
		if (p2X < curBounds->minXBlock) curBounds->minXBlock = p2X;
		if (p3X < curBounds->minXBlock) curBounds->minXBlock = p3X;
		curBounds->minYBlock = p1Y;
		if (p2Y < curBounds->minYBlock) curBounds->minYBlock = p2Y;
		if (p3Y < curBounds->minYBlock) curBounds->minYBlock = p3Y;
		curBounds->minZBlock = p1Z;
		
		curBounds->maxXBlock = p1X;
		if (p2X > curBounds->maxXBlock) curBounds->maxXBlock = p2X;
		if (p3X > curBounds->maxXBlock) curBounds->maxXBlock = p3X;

		curBounds->maxYBlock = p1Y;
		if (p2Y > curBounds->maxYBlock) curBounds->maxYBlock = p2Y;
		if (p3Y > curBounds->maxYBlock) curBounds->maxYBlock = p3Y;

		curBounds->maxZBlock = p2Z;
		if (p3Z > curBounds->maxZBlock)
			curBounds->maxZBlock = p3Z;

		// count blocks
		for (int i = curBounds->minXBlock; i <= curBounds->maxXBlock; i++)
			xBlockSize[i]++;
		for (int i = curBounds->minYBlock; i <= curBounds->maxYBlock; i++)
			yBlockSize[i]++;
		for (int i = curBounds->minZBlock; i <= curBounds->maxZBlock; i++)
			zBlockSize[i]++;

		// next!
		curBounds++;
	}

	// phase 2: Create X, Y and Z sectors for the triangles
	uint16 *xBlocks[256], *yBlocks[256], *zBlocks[256];
	int minXBlock = 256, minYBlock = 256, minZBlock = 256, maxXBlock = 0, maxYBlock = 0, maxZBlock = 0;
	
	for (int i = 0; i < 256; i++) {
		if (xBlockSize[i]) {
			xBlocks[i] = (uint16*) malloc(xBlockSize[i] * sizeof (uint16));
			maxXBlock = i;
			if (i < minXBlock)
				minXBlock = i;
		} else
			xBlocks[i] = NULL;
		if (yBlockSize[i]) {
			yBlocks[i] = (uint16*) malloc(yBlockSize[i] * sizeof (uint16));
			maxYBlock = i;
			if (i < minYBlock)
				minYBlock = i;
		} else
			yBlocks[i] = NULL;
		if (zBlockSize[i]) {
			zBlocks[i] = (uint16*) malloc(zBlockSize[i] * sizeof (uint16));
			maxZBlock = i;
			if (i < minZBlock)
				minZBlock = i;
		} else
			zBlocks[i] = NULL;
	}

	// phase 3: Fill the sectors with triangles
	int newXBlockSize[256] = {0}, newYBlockSize[256] = {0}, newZBlockSize[256] = {0};
	for (int i = 0; i < numTris; i++) {
		for (int x = bounds[i].minXBlock; x <= bounds[i].maxXBlock; x++)
			xBlocks[x][newXBlockSize[x]++] = i;

		for (int y = bounds[i].minYBlock; y <= bounds[i].maxYBlock; y++)
			yBlocks[y][newYBlockSize[y]++] = i;

		for (int z = bounds[i].minZBlock; z <= bounds[i].maxZBlock; z++)
			zBlocks[z][newZBlockSize[z]++] = i;
	}

	// phase 4: create collision blocks and pointers to them
	uint16* blocks = (uint16*) malloc(1024 * 1024 * 2);
	uint16* curBlock = blocks;
	uint16* zList = (uint16*) malloc(0x1000), *yList = (uint16*) malloc(0x2000), *xList = (uint16*) malloc(0x20000);
	uint16 *curZ = zList, *curY = yList, *curX = xList;

	int numZSectors = maxZBlock - minZBlock - 1, numYSectors = maxYBlock - minYBlock - 1, numXSectors = maxXBlock - minXBlock - 1;
	int maxZSection = 0;
	uint16* zNum = curZ;

	for (int z = 0; z <= maxZBlock; z++) {
		int maxZYSection = -1;
		uint16* ySectorPtr = &curZ[1+z];
		uint16* yNum = curY;

		if (z < minZBlock)
			goto ContZ;

		for (int y = 0; y <= maxYBlock; y++) {
			int maxZYXSection = -1;
			uint16* xSectorPtr = &curY[1+y];
			uint16* xNum = curX;

			if (y < minYBlock)
				goto ContY;

			for (int x = 0; x <= maxXBlock; x++) {
				uint16* blockPtr = &curX[1+x];
				int numTrisHere = 0;

				if (x < minXBlock)
					goto ContX;

				for (int i = 0; i < xBlockSize[x]; i++) {
					TriBounds* tri = &bounds[xBlocks[x][i]];

					if (tri->minYBlock > y || tri->maxYBlock < y || tri->minZBlock > z || tri->maxZBlock < z)
						continue;

					int blX1 = x << 8, blX2 = (x + 1) << 8, blY1 = y << 8, blY2 = (y + 1) << 8, blZ1 = z << 8, blZ2 = (z + 1) << 8;
					bool pointInBlock = false;

					if ((tri->p1.x >= blX1 && tri->p1.x < blX2 && tri->p1.y >= blY1 && tri->p1.y < blY2 && tri->p1.z >= blZ1 && tri->p1.z < blZ2) || 
						(tri->p2.x >= blX1 && tri->p2.x < blX2 && tri->p2.y >= blY1 && tri->p2.y < blY2 && tri->p2.z >= blZ1 && tri->p2.z < blZ2) || 
						(tri->p3.x >= blX1 && tri->p3.x < blX2 && tri->p3.y >= blY1 && tri->p3.y < blY2 && tri->p3.z >= blZ1 && tri->p3.z < blZ2))
						pointInBlock = true;

					if (!pointInBlock) {
						for (int i = 0; i < 3; i++) {
							Point16* cur = &tri->points[i], *next = &tri->points[(i + 1) % 3];
							int testX, testY, testZ;
							
							if (next->x != cur->x) {
								// Check X plane crosses
								if ((cur->x >= blX1 && next->x <= blX1) || (cur->x <= blX1 && next->x >= blX1)) {
									testY = cur->y + (next->y - cur->y) * (blX1 - cur->x) / (next->x - cur->x);
									testZ = cur->z + (next->z - cur->z) * (blX1 - cur->x) / (next->x - cur->x);

									if (testY >= blY1 && testY < blY2 && testZ >= blZ1 && testZ < blZ2)
										goto FoundPoint;
								}
								
								if ((cur->x >= blX2 && next->x <= blX2) || (cur->x <= blX2 && next->x >= blX2)) {
									testY = cur->y + (next->y - cur->y) * (blX2 - cur->x) / (next->x - cur->x);
									testZ = cur->z + (next->z - cur->z) * (blX2 - cur->x) / (next->x - cur->x);

									if (testY >= blY1 && testY < blY2 && testZ >= blZ1 && testZ < blZ2)
										goto FoundPoint;
								}
							}

							if (next->y != cur->y) {
								// Check Y plane crosses
								if ((cur->y >= blY1 && next->y <= blY1) || (cur->y <= blY1 && next->y >= blY1)) {
									testX = cur->x + (next->x - cur->x) * (blY1 - cur->y) / (next->y - cur->y);
									testZ = cur->z + (next->z - cur->z) * (blY1 - cur->y) / (next->y - cur->y);

									if (testX >= blX1 && testX < blX2 && testZ >= blZ1 && testZ < blZ2)
										goto FoundPoint;
								}
								
								if ((cur->y >= blY2 && next->y <= blY2) || (cur->y <= blY2 && next->y >= blY2)) {
									testX = cur->x + (next->x - cur->x) * (blY2 - cur->y) / (next->y - cur->y);
									testZ = cur->z + (next->z - cur->z) * (blY2 - cur->y) / (next->y - cur->y);

									if (testX >= blX1 && testX < blX2 && testZ >= blZ1 && testZ < blZ2)
										goto FoundPoint;
								}
							}

							continue;
							FoundPoint:
							pointInBlock = true;
							break;
						}
					}

					if (!pointInBlock)
						continue;

					// FINALLY FOUND SOMETHING
					if (!numTrisHere)
						curBlock[numTrisHere++] = xBlocks[x][i] | 0x8000;
					else
						curBlock[numTrisHere++] = xBlocks[x][i];
				}

				ContX:
				if (numTrisHere) {
					*blockPtr = curBlock - blocks; // * 2?
					curBlock += numTrisHere;
					maxZYXSection = x;
				} else
					*blockPtr = 0xFFFF;
			}

			ContY:
			if (maxZYXSection != -1) {
				*xSectorPtr = (curX - xList) * 2;
				*xNum = maxZYXSection + 1;
				curX += maxZYXSection + 2;
				maxZYSection = y;
			} else
				*xSectorPtr = 0xFFFF;
		}

		ContZ:
		if (maxZYSection != -1) {
			*ySectorPtr = (curY - yList) * 2;
			*yNum = maxZYSection + 1;
			curY += maxZYSection + 2;
			maxZSection = z;
		} else
			*ySectorPtr = 0xFFFF; // no relevant Y sector list for this Z sector
	}

	// trim Z list
	*zNum = maxZSection + 1;
	curZ += maxZSection + 2;

	// Add list lengths to all pointers (so that the lists will be stacked atop each other)
	int zLen = curZ - zList, yLen = curY - yList, xLen = curX - xList;
	for (uint16* zAdd = zList; zAdd < curZ; ) {
		int len = *zAdd;

		for (int i = 0; i < len; i++) {
			if (zAdd[1+i] != 0xFFFF)
				zAdd[1+i] += zLen * 2;
		}

		zAdd += len + 1;
	}

	for (uint16* yAdd = yList; yAdd < curY; ) {
		int len = *yAdd;

		for (int i = 0; i < len; i++) {
			if (yAdd[1+i] != 0xFFFF)
				yAdd[1+i] += (zLen + yLen) * 2;
		}

		yAdd += len + 1;
	}

	// Copy to memory, yolo.
	uint16* actualBlockTree = (uint16*) ((genuintptr)memory + (blockTreeAddr & 0x003FFFFF));
	int gameTreeSize = blockAddr - blockTreeAddr;
	int curTreeSize = 0;
	int copyAmt;

	// copy Z list
	copyAmt = zLen * 2;
	if (copyAmt > gameTreeSize - curTreeSize)
		copyAmt = gameTreeSize - curTreeSize;
	memcpy(actualBlockTree, zList, copyAmt);
	curTreeSize += copyAmt;

	// copy Y list
	copyAmt = yLen * 2;
	if (copyAmt > gameTreeSize - curTreeSize)
		copyAmt = gameTreeSize - curTreeSize;
	memcpy(&actualBlockTree[zLen], yList, copyAmt);
	curTreeSize += copyAmt;

	// copy X list
	copyAmt = xLen * 2;
	if (copyAmt > gameTreeSize - curTreeSize)
		copyAmt = gameTreeSize - curTreeSize;
	memcpy(&actualBlockTree[zLen + yLen], xList, copyAmt);
	curTreeSize += copyAmt;

	// Copy blocks
	uint16* actualBlockList = (uint16*) ((genuintptr)memory + (blockAddr & 0x003FFFFF));

	copyAmt = (curBlock - blocks) * 2;

	if (copyAmt > (triangleDataAddr & 0x003FFFFF) - (blockAddr & 0x003FFFFF))
		copyAmt = (triangleDataAddr & 0x003FFFFF) - (blockAddr & 0x003FFFFF);

	memcpy(actualBlockList, blocks, copyAmt);

	// Set status..es to inform stuff
	char collStatus[256];

	sprintf(collStatus, "colblcks %04X/%04X tree %04X/%04X", (curBlock - blocks) * 2, (triangleDataAddr & 0x003FFFFF) - (blockAddr & 0x003FFFFF), 
		(xLen + yLen + zLen) * 2, gameTreeSize);

	SetControlString(static_genSceneStatus, collStatus);

	// Cleanup
	free(blocks);
	free(zList);
	free(yList);
	free(xList);
	free(bounds);

	for (int i = 0; i < 256; i++) {
		if (xBlocks[i])
			free(xBlocks[i]);
		if (yBlocks[i])
			free(yBlocks[i]);
		if (zBlocks[i])
			free(zBlocks[i]);
	}
}