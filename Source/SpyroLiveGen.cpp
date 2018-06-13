#include "Window.h"
#include "SpyroData.h"
#include "Main.h"
#include "SpyroTextures.h"
#include "GenLive.h"
#include "GenObject.h"
#include "SpyroGenesis.h"
#include "SpyroScene.h"

#include <cstdio>
#include <cwchar> // for instance name

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

bool genSceneCreated = false;

extern HWND edit_genIp;

void SendLiveGenMobyInstances();
void SetupCollisionLinks(int minSectorIndex = 0, int maxSectorIndex = -1);
void ResetCollisionLinks();
void RebuildCollisionTree();
void UpdateFlaggedMemory();

void BuildGenMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrame);
void BuildSpyroMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrame);

DWORD lastMobyUpdateTime = 0;

bool isColltreeValid = true; // set to false to rebuild colltris at the end of the frame

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
	// Disconnect old session
	if (live) {
		DestroyLiveGen(live);
		live = NULL;
	}

	// Connect new session
	live = CreateLiveGen();

	if (!live)
		return false;

	char ipAddress[20] = {0};

	SendMessageA(edit_genIp, WM_GETTEXT, 20, (LPARAM) ipAddress);
	ipAddress[19] = 0;

	if (!live->Connect(LgAddress(ipAddress, 6253))) {
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

	if (!isColltreeValid) {
		RebuildCollisionTree();
		isColltreeValid = true;
	}

	if (!live)
		return;

	live->Update();

	// Receive updates from Genesis and send them to genScene
	GenState liveStatePack, liveStatePack2;
	while (live->GetState(&liveStatePack, NULL, LGALLNODES))
		genScene.SetState(&liveStatePack.GetSubstate());

	// Update and validate objects that have been changed since the receive
	// They may or may not be invalidated later on, but by the end of this function they must be validated so that they're not re-sent to Genesis unless desired
	// That isn't actually programmed to happen anyway but uhhh it's on the list? Maybe!?!!?!? WHAT AM I DOING
	for (GenObject* obj = genScene.GetNextChangedObject(NULL); obj != NULL; obj = genScene.GetNextChangedObject(obj)) {
		genid objId = obj->GetId();
		GenObjType objType = obj->GetObjType();

		GenMesh* testMesh = nullptr; GenModel* testModel = nullptr; GenMod* testMod = nullptr;
		if (objType == GENOBJ_MESH) {
			testMesh = (GenMesh*) obj;
		} else if (objType == GENOBJ_MODEL) {
			testModel = (GenModel*) obj;
		} else if (objType == GENOBJ_MOD) {
			testMod = (GenMod*) obj;
		}

		// Update scene sector if applicable
		// Check if model has changed (e.g. mods changed, consequently mesh has changed)
		if (objId >= GENID_SCENESECTORMODEL && objId <= GENID_SCENESECTORMODEL + 256 && objType == GENOBJ_MODEL) {
			// in case the mods have changed
			scene.ConvertGenToSpyro(objId - GENID_SCENESECTORMODEL);
		}

		// Check if a model's mesh has changed
		if (objType == GENOBJ_MESH && scene.spyroScene) {
			for (int i = 0; i < scene.spyroScene->numSectors; i++) {
				if (GenModel* model = (GenModel*) genScene.GetObjectById(GENID_SCENESECTORMODEL + i)) {
					if (objId == model->GetMeshId())
						scene.ConvertGenToSpyro(i);
				}
			}
		}

		// Check if a mod's mesh has been set (and that mesh is the relevant mesh for a scene sector model)
		if (objType == GENOBJ_MOD && scene.spyroScene) {
			genid meshId = ((GenMod*)obj)->GetMeshId();

			if (meshId) {
				for (int i = 0; i < scene.spyroScene->numSectors; ++i) {
					if (GenModel* model = (GenModel*) genScene.GetObjectById(GENID_SCENESECTORMODEL + i)) {
						if (objId == model->GetMeshId())
							scene.ConvertGenToSpyro(i);
					}
				}
			}
		}

		// Update moby if applicable
		if (objId >= GENID_MOBYINSTANCES && objId <= GENID_MOBYINSTANCES + 300 && objType == GENOBJ_INST) {
			int mobyId = objId - GENID_MOBYINSTANCES;
			float zSubtract = MOBYSPHERESIZE;
			
			if (mobys[mobyId].type < 768 && genScene.GetModelById(GENID_MOBYMODELS + mobys[mobyId].type))
				zSubtract = 0.0f;

			const GenTransform* trans = ((GenInst*)obj)->GetTransform();
			mobys[mobyId].angle.z = (int) (trans->localRotation.z / 6.28f * 255.0f);

			mobys[mobyId].SetPosition((int) (trans->localTranslation.x / MOBYPOSITIONSCALE), 
									  (int) (trans->localTranslation.y / MOBYPOSITIONSCALE), 
									  (int) ((trans->localTranslation.z - zSubtract) / MOBYPOSITIONSCALE));
		}

		// Move Spyro
		if (objId == GENID_SPYROINSTANCE) {
			const GenTransform* trans = ((GenInst*)obj)->GetTransform();
			spyro->x = (int) (trans->localTranslation.x / MOBYPOSITIONSCALE);
			spyro->y = (int) (trans->localTranslation.y / MOBYPOSITIONSCALE);
			spyro->z = (int) ((trans->localTranslation.z - MOBYSPHERESIZE) / MOBYPOSITIONSCALE);
		}

		obj->ValidateAll();
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

char rawDataHackyStacky[sizeof (GenValueSet) + 256 * 4 * sizeof (gens32)];

void LiveGenOnLevelEntry() {
	if (!genSceneCreated) {
		genScene.Create();
		genScene.SetId(0xDEAFD00D);

		// Create Gen scene sectors and pointers and stuffs
		for (int i = 0; i < 256; i++) {
			GenModel* model = (GenModel*) genScene.CreateObject(GENID_SCENESECTORMODEL + i, GENOBJ_MODEL);
			GenMod* mod = (GenMod*) genScene.CreateObject(GENID_SCENESECTORMOD + i, GENOBJ_MOD);
			GenInst* inst = (GenInst*) genScene.CreateObject(GENID_SCENESECTORINSTANCE + i, GENOBJ_INST);
			
			model->AddModifier(mod->GetId());
			mod->SetModType(MOD_EDITMESH);
			inst->SetModel(GENID_SCENESECTORMODEL + i);
			inst->SetMaterial(GENID_SCENETEXTURE);

			char nameStr[128];
			sprintf(nameStr, "Sector %i", i);
			inst->SetName(nameStr);
		}

		scene.SetGenScene(&genScene);

		genSceneCreated = true;
	}

	// Create the links between collision data and scenery data
	ResetCollisionLinks();
	SetupCollisionLinks();

	// Reset memory pool
	memset(usedMem, 0, sizeof (usedMem));
}

void SendLiveGenScene() {
	if (!live)
		return;
	
	GenValueSet& data = *((GenValueSet*)rawDataHackyStacky); // use a fixed-size stack portion for all our data today~

	// Send scene texture
	data.type = GENTYPE_CSTRING;
	GetLevelFilename(data.s8, SEF_TEXTURES);
	data.numValues = strlen(data.s8) + 1;
	
	if (FILE* texExists = fopen(data.s8, "rb"))
		fclose(texExists); // no need to save texture file; it already exists
	else
		SaveTexturesAsSingle(); // save the textures

	live->SendStateDirect(GENID_SCENETEXTURE, GENMAT_SOURCEFILE, data);

	// Send scene geometry
	scene.UploadToGen();
}

void ResetLiveGenScene() {
	scene.Reset();
}

void SendLiveGenCollision() {
	if (!live || !spyroCollision.IsValid())
		return;

	uint32* triangles = (uint32*) spyroCollision.triangles;
	int numTriangles = spyroCollision.numTriangles;

	// Create the object models if they don't already exist
	GenModel* model = (GenModel*) genScene.CreateOrGetObject(GENID_COLLISIONDATAMODEL, GENOBJ_MODEL);
	GenMod* mod = (GenMod*) genScene.CreateOrGetObject(GENID_COLLISIONDATAMOD, GENOBJ_MOD);
	GenInst* instance = (GenInst*) genScene.CreateOrGetObject(GENID_COLLISIONDATAINSTANCE, GENOBJ_INST);

	// Set IDs
	mod->SetModType(MOD_EDITMESH);
	instance->SetModel(model->GetId());
	model->AddModifier(mod->GetId());
	
	// Convert Spyro collision model to Gen model
	GenMesh* mesh = mod->GetMesh();
	mesh->SetNum(GENMET_VERT, numTriangles * 3);
	mesh->SetNum(GENMET_FACE, numTriangles);

	GenMeshVert* verts = mesh->LockVerts();
	GenMeshFace* faces = mesh->LockFaces();

	for (int part = 0, e = numTriangles * 3; part < e; part += 3) {
		int16 offX = triangles[part+0] & 0x3FFF, offY = triangles[part+1] & 0x3FFF, offZ = triangles[part+2] & 0x3FFF;
		int16 p1X = offX, p2X = bitss(triangles[part+0], 14, 9) + offX, p3X = bitss(triangles[part+0], 23, 9) + offX;
		int16 p1Y = offY, p2Y = bitss(triangles[part+1], 14, 9) + offY, p3Y = bitss(triangles[part+1], 23, 9) + offY;
		int16 p1Z = offZ, p2Z = bitsu(triangles[part+2], 16, 8) + offZ, p3Z = bitsu(triangles[part+2], 24, 8) + offZ;

		verts[part+0].pos.x = (float) p1X * WORLDSCALE; verts[part+0].pos.y = (float) p1Y * WORLDSCALE; verts[part+0].pos.z = (float) p1Z * WORLDSCALE;
		verts[part+1].pos.x = (float) p2X * WORLDSCALE; verts[part+1].pos.y = (float) p2Y * WORLDSCALE; verts[part+1].pos.z = (float) p2Z * WORLDSCALE;
		verts[part+2].pos.x = (float) p3X * WORLDSCALE; verts[part+2].pos.y = (float) p3Y * WORLDSCALE; verts[part+2].pos.z = (float) p3Z * WORLDSCALE;

		GenMeshFace* face = &faces[part / 3];
		face->numSides = 3;
		face->sides[0].vert = part;
		face->sides[1].vert = part + 1;
		face->sides[2].vert = part + 2;
	}

	mesh->UnlockVerts(verts);
	mesh->UnlockFaces(faces);

	// Send all the objects over LiveGen
	live->SendState(model->GetState(GENCOMMON_MULTIPLE).GetSubstate());
	live->SendState(mod->GetState(GENCOMMON_MULTIPLE));
	live->SendState(instance->GetState(GENCOMMON_MULTIPLE));
	live->SendState(mesh->GetState(GENCOMMON_MULTIPLE));
}

int lastMobyX[1000], lastMobyY[1000], lastMobyZ[1000];

void SendLiveGenMobyInstances() {
	if (!live)
		return;

	SaveObjectTextures();

	// Send objects!
	if (mobys && numMobys) {
		GenValueSet& data = *((GenValueSet*)rawDataHackyStacky);
		
		// Create the generic moby model if it doesn't exist
		if (!genScene.GetObjectById(GENID_GENERICMOBYMODEL)) {
			GenModel* genericMobyModel = (GenModel*)genScene.CreateObject(GENID_GENERICMOBYMODEL, GENOBJ_MODEL);
			GenMod* genericMobyMod = (GenMod*)genScene.CreateObject(GENID_GENERICMOBYMOD, GENOBJ_MOD);

			if (genericMobyModel && genericMobyMod) {
				genericMobyMod->SetModType(MOD_BASESPHERE);
				genericMobyMod->SetProperty("radius", GenValueSet((float)MOBYSPHERESIZE));
				genericMobyModel->AddModifier(genericMobyMod->GetId());

				live->SendState(genericMobyMod->GetState(GENCOMMON_MULTIPLE));
				live->SendState(genericMobyModel->GetState(GENCOMMON_MULTIPLE));
			}
		}

		// Send every living moby in the level
		for (int i = 0; (uintptr) &mobys[i + 1] < (uintptr) memory + 0x00200000; i++) {
			if (mobys[i].state == -1)
				break; // last moby
			else if (mobys[i].state < 0)
				continue; // dead moby
			//if (mobys[i].x == lastMobyX[i] && mobys[i].y == lastMobyY[i] && mobys[i].z == lastMobyZ[i])
			//	continue; // moby hasn't changed

			GenInst* mobyInst = (GenInst*)genScene.CreateOrGetObject(GENID_MOBYINSTANCES + i, GENOBJ_INST);
			float mobyX = mobys[i].x * MOBYPOSITIONSCALE, mobyY = mobys[i].y * MOBYPOSITIONSCALE, mobyZ = mobys[i].z * MOBYPOSITIONSCALE;

			if (mobys[i].type < 768 && genScene.GetModelById(GENID_MOBYMODELS + mobys[i].type)) {
				mobyInst->SetModel(GENID_MOBYMODELS + mobys[i].type);
			} else {
				mobyInst->SetModel(GENID_GENERICMOBYMODEL);
				mobyZ += MOBYSPHERESIZE;
			}

			char instanceName[250];
			sprintf(instanceName, "Moby %i (%08X)", i, (uintptr) &mobys[i] - (uintptr) memory);
			mobyInst->SetTransform(GenTransform(GenVec3(mobyX, mobyY, mobyZ), GenVec3(0.0f, 0.0f, mobys[i].angle.z / 255.0f * 6.28f)));
			mobyInst->SetMaterial(GENID_MOBYTEXTURES);
			mobyInst->SetName(instanceName);

			live->SendState(mobyInst->GetState(GENCOMMON_MULTIPLE));

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
	GenValueSet* elems = modelState.Lock(GENTYPE_ID, 1);
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

	elems->numValues = numSides;
	faceState.Unlock();

	GenState instState(GENID_SPYROINSTANCE, GENINST_MODELID);
	elems = instState.Lock(GENTYPE_ID, 1);
	elems->id[0] = GENID_SPYROMODEL;
	instState.Unlock();

	GenState modMeshState(GENID_SPYROMODELMOD, GENMOD_MESHDATA);

	modMeshState.AddSubstate(vertState);
	modMeshState.AddSubstate(faceState);

	// Send all states
	live->SendState(modState);
	live->SendState(modMeshState);
	live->SendState(modelState);
	live->SendState(instState);
}

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
	char filename[MAX_PATH];

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

	mod->SetModType(MOD_EDITMESH);

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

void BuildSpyroMobyModel(uint32 mobyModelId, uint32 animId, uint32 animFrameIndex) {
	GenMesh* genMoby = nullptr;
	if (GenModel* genMobyModel = genScene.GetModelById(GENID_MOBYMODELS + mobyModelId)) {
		genMoby = genMobyModel->GetMesh();
	}

	if (!genMoby || !(mobyModels[mobyModelId].address & 0x80000000))
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
	const float scale = (float) (MOBYPOSITIONSCALE) * ((float) (anim->vertScale + 1));

	// Set colours
	genu32* genColours = genMoby->LockColours();
	genu32 genNumColours = genMoby->GetNumColours();

	for (int i = 0; i < anim->numColours && i < genNumColours; i++)
		anim->colours[i] = genColours[i] & 0x00FFFFFF;

	genMoby->UnlockColours(genColours);
}

void MakeIceWall(int _x, int _y, int _z) {
	if (!scene.spyroScene)
		return;

	int sX = _x >> 4, sY = _y >> 4, sZ = _z >> 4;
	float x = (float) (_x >> 4) * WORLDSCALE, y = (float) (_y >> 4) * WORLDSCALE, z = (float) (_z >> 4) * WORLDSCALE;
	uint64 maxX = 0, maxY = 0, maxZ = 0;
	uint32 closestId = 0;
	uint64 closestDist = 0xFFFFFFFFFFFFFFFFll;

	for (int i = 0; i < scene.spyroScene->numSectors; i++) {
		SceneSectorHeader* sector = scene.spyroScene->sectors[i];
		int sectorX = sector->xyPos >> 16, sectorY = (sector->xyPos & 0xFFFF), sectorZ = ((sector->zPos >> 14) & 0xFFFF) >> 2;
		uint64 dist = (sectorX - sX) * (sectorX - sX) + (sectorY - sY) * (sectorY - sY) + (sectorZ - sZ) * (sectorZ - sZ);

		if (dist < closestDist && sectorX < sX && sectorY < sY && sectorZ < sZ) {
			closestId = i;
			closestDist = dist;
		}
	}

	// Ice wall, coming right up
	SceneSectorHeader* sector = scene.spyroScene->sectors[closestId];
	GenModel* genSectorModel = genScene.GetModelById(GENID_SCENESECTORINSTANCE + closestId);
	GenMesh* genSector = nullptr;

	if (genSectorModel) {
		genSector = genSectorModel->GetMesh();
	}

	if (!genSector) {
		return;
	}

	int startVert = genSector->GetNumVerts(), startFace = genSector->GetNumFaces();
	
	if (startVert + 4 < 256 && startFace + 1 < 256) {
		scene.ConvertSpyroToGen(closestId);
		
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

		scene.ConvertGenToSpyro(closestId);

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

const int maxSeparationDistance = 6;

void SetupCollisionLinks(int minSectorIndex, int maxSectorIndex) {
	if (!scene.spyroScene) {
		return;
	}

	if (maxSectorIndex == -1) {
		maxSectorIndex = scene.spyroScene->numSectors - 1;
	}

	// Rebuild collision cache
	if (spyroCollision.IsValid() && scene.spyroScene) {
		CollTri* triangles = spyroCollision.triangles;
		int numTriangles = spyroCollision.numTriangles;
		uint16* triangleTypes = spyroCollision.surfaceType;
		bool* cachedTriangles = (bool*) malloc(numTriangles * sizeof (bool)); // For each triangle, value is true if it has now been cached
		memset(cachedTriangles, 0, numTriangles * sizeof (bool));

		// This operation is slow, so we'll deliberately compartmentalise the collision triangles into smaller--wait. Doesn't Spyro already do this?
		// Todo: Abuse this Spyro feature
		for (int s = minSectorIndex; s <= maxSectorIndex; s++) {
			SceneSectorHeader* sector = scene.spyroScene->sectors[s];
			CollSectorCache* collSector = &collisionCache.sectorCaches[s];
			SceneFace* faces = scene.spyroScene->sectors[s]->GetHpFaces();
			IntVector absoluteVerts[256];
			int minX = INT_MAX, minY = INT_MAX, minZ = INT_MAX, maxX = INT_MIN, maxY = INT_MIN, maxZ = INT_MIN;

			sector->GetHpVertexAbsolutePositions(absoluteVerts);

			// Get sector boundaries
			for (int i = 0; i < sector->numHpVertices; i++) {
				if (absoluteVerts[i].x < minX) minX = absoluteVerts[i].x;
				if (absoluteVerts[i].y < minY) minY = absoluteVerts[i].y;
				if (absoluteVerts[i].z < minZ) minZ = absoluteVerts[i].z;
				if (absoluteVerts[i].x > maxX) maxX = absoluteVerts[i].x;
				if (absoluteVerts[i].y > maxY) maxY = absoluteVerts[i].y;
				if (absoluteVerts[i].z > maxZ) maxZ = absoluteVerts[i].z;
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
						collSector->triangles[collSector->numTriangles].vert1Index = matches[0];
						collSector->triangles[collSector->numTriangles].vert2Index = matches[1];
						collSector->triangles[collSector->numTriangles].vert3Index = matches[2];
						collSector->numTriangles++;

						cachedTriangles[tri - triangles] = true;
						break;
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
						
					cachedTriangles[tri - triangles] = true;
				}
			}
		}

		free(cachedTriangles);
	}
}

void UpdateCollisionLinks(int sectorIndex) {
	if (!spyroCollision.triangles || !scene.spyroScene) {
		return;
	}

	// Get vertex absolute positions
	SceneSectorHeader* sector = scene.spyroScene->sectors[sectorIndex];
	IntVector verts[256];
	SceneFace* faces = sector->GetHpFaces();

	sector->GetHpVertexAbsolutePositions(verts);

	// See which triangles still match with polygons, and remove those that don't
	bool* isTriangleMatched = (bool*)malloc(collisionCache.sectorCaches[sectorIndex].numTriangles * sizeof (bool));
	memset(isTriangleMatched, 0, collisionCache.sectorCaches[sectorIndex].numTriangles * sizeof (bool));

	for (int i = 0; i < collisionCache.sectorCaches[sectorIndex].numTriangles; ++i) {
		CollTriCache* triCache = &collisionCache.sectorCaches[sectorIndex].triangles[i];
		CollTri* tri = &spyroCollision.triangles[triCache->triangleIndex];

		int16 p1X = bitsu(tri->xCoords, 0, 14), p2X = bitss(tri->xCoords, 14, 9) + p1X, p3X = bitss(tri->xCoords, 23, 9) + p1X;
		int16 p1Y = bitsu(tri->yCoords, 0, 14), p2Y = bitss(tri->yCoords, 14, 9) + p1Y, p3Y = bitss(tri->yCoords, 23, 9) + p1Y;
		int16 p1Z = bitsu(tri->zCoords, 0, 14), p2Z = bitsu(tri->zCoords, 16, 8) + p1Z, p3Z = bitsu(tri->zCoords, 24, 8) + p1Z;

		// First check if the triangle is in the same position as before
		if (triCache->faceIndex < sector->numHpFaces) {
			SceneFace* face = &sector->GetHpFaces()[triCache->faceIndex];

			if (verts[face->verts[triCache->vert1Index]].x == p1X && verts[face->verts[triCache->vert1Index]].y == p1Y && verts[face->verts[triCache->vert1Index]].z == p1Z && 
					verts[face->verts[triCache->vert2Index]].x == p2X && verts[face->verts[triCache->vert2Index]].y == p2Y && verts[face->verts[triCache->vert2Index]].z == p2Z && 
					verts[face->verts[triCache->vert3Index]].x == p3X && verts[face->verts[triCache->vert3Index]].y == p3Y && verts[face->verts[triCache->vert3Index]].z == p3Z) {
				isTriangleMatched[i] = true;
				continue;
			}
		}

		// If that didn't work, try to match it up with a new one
		int matches[4];
		for (int f = 0; f < sector->numHpFaces; ++f) {
			int numMatches = 0;

			for (int side = (faces[f].verts[0] == faces[f].verts[1] ? 1 : 0); side < 4; side++) {
				IntVector* vert = &verts[faces[f].verts[side]];
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

			if (numMatches == 3) {
				// Found!
				triCache->faceIndex = f;
				triCache->vert1Index = matches[0];
				triCache->vert2Index = matches[1];
				triCache->vert3Index = matches[2];
				isTriangleMatched[i] = true;
				break;
			}
		}

		// If that didn't work
		if (!isTriangleMatched[i]) {
			// Into the unlinked triangles you go!
			collisionCache.unlinkedCaches[collisionCache.numUnlinkedTriangles++] = *triCache;

			// Delete this one from the sector cache list
			--collisionCache.sectorCaches[sectorIndex].numTriangles;
			for (int j = i; j < collisionCache.sectorCaches[sectorIndex].numTriangles; ++j) {
				collisionCache.sectorCaches[sectorIndex].triangles[j] = collisionCache.sectorCaches[sectorIndex].triangles[j + 1];
			}

			--i;

			// Throw the triangle into nowhere
			tri->xCoords = 0;
			tri->yCoords = 0;
			// not touching Z because of those weird flags
			continue;
		}
	}

	free(isTriangleMatched);
}

void ResetCollisionLinks() {
	// Reset collision caches
	for (int i = 0; i < 256; i++)
		collisionCache.sectorCaches[i].numTriangles = 0;
	collisionCache.numUnlinkedTriangles = 0;
}

void RebuildCollisionTriangles() {
	if (!spyroCollision.IsValid() || !scene.spyroScene)
		return;

	CollTri* triangles = spyroCollision.triangles;
	uint16* surfaces = spyroCollision.blocks; // ??
	int oldNumTriangles = spyroCollision.numTriangles;

	// Perform massive triangle build
	int numTriangles = 0;

	for (int s = 0; s < scene.spyroScene->numSectors; s++) {
		GenMesh* genSector = scene.GetGenSector(s);
		scene.ConvertSpyroToGen(s);

		if (!genSector)
			continue;

		const GenMeshVert* genVerts = genSector->GetVerts();
		const GenMeshFace* genFaces = genSector->GetFaces();
		for (int i = 0, numFaces = genSector->GetNumFaces(); i < numFaces && numTriangles < spyroCollision.numTriangles; i++) {
			for (int t = 0; t < genFaces[i].numSides - 2; t++) {
				const GenVec3* vert1 = &genVerts[genFaces[i].sides[t*2].vert].pos, *vert2 = &genVerts[genFaces[i].sides[t*2+1].vert].pos,
							  *vert3 = &genVerts[genFaces[i].sides[(t*2+2) & 3].vert].pos;

				// Relocate the vertices
				triangles[numTriangles].SetPoints(
					(int)(vert1->x / WORLDSCALE), (int)(vert1->y / WORLDSCALE), (int)(vert1->z / WORLDSCALE), 
					(int)(vert2->x / WORLDSCALE), (int)(vert2->y / WORLDSCALE), (int)(vert2->z / WORLDSCALE), 
					(int)(vert3->x / WORLDSCALE), (int)(vert3->y / WORLDSCALE), (int)(vert3->z / WORLDSCALE));

				surfaces[numTriangles] = 0;
				numTriangles++;
			}
		}
	}
	
	spyroCollision.numTriangles = numTriangles;

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
	if (!spyroCollision.IsValid())
		return;

	uint32* tri = (uint32*)spyroCollision.triangles;
	int numTris = spyroCollision.numTriangles;
	uint32 blockAddr = spyroCollision.blocks.address, blockTreeAddr = spyroCollision.blockTree.address, triangleDataAddr = spyroCollision.triangles.address;
	
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