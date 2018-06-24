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

	bool animated = (mobyModels[mobyModelId].address & 0x80000000);

	GenValueSetContainer<0x25000> modelData;
	mod->SetProperty("spyroBaseAddr", GenValueSet((gens32)mobyModels[mobyModelId].address));

	if (mobyModelId == 0) {
		SpyroModelHeader* header = ((SpyroModelHeader*)mobyModels[mobyModelId]);
		int size = header->iDontNeedThis[12];
		if (size >= 0 && size < 0x25000) {
			modelData.numValues = size;
			modelData.type = GENTYPE_RAW;
			memcpy(modelData.byte, mobyModels[mobyModelId], size);

			// Set any out-of-range addresses to nullptr
			for (int i = 0, e = header->numAnims; i < e; ++i) {
				if ((header->anims[i].address & 0x003FFFFF) < (mobyModels[mobyModelId].address & 0x003FFFFF)
						|| (header->anims[i].address & 0x003FFFFF) >= (mobyModels[mobyModelId].address & 0x003FFFFF) + size) {

					((SpyroModelHeader*)modelData.byte)->anims[i].address = 0;
				}
			}

			mod->SetProperty("spyroModel", modelData);
		}

		mod->SetProperty("spyroModelType", GenValueSet("spyro"));
	} else if (animated) {
		int size = (mobyModels[mobyModelId]->unkSomeOtherPlace & 0x003FFFFF) - (mobyModels[mobyModelId].address & 0x003FFFFF);
		if (size >= 0 && size < 128000) {
			modelData.numValues = size;
			modelData.type = GENTYPE_RAW;
			memcpy(modelData.byte, mobyModels[mobyModelId], size);
			mod->SetProperty("spyroModel", modelData);
		}

		mod->SetProperty("spyroModelType", GenValueSet("animated"));
	} else {
		int size = (((SimpleModelHeader*)mobyModels[mobyModelId])->unkExtra & 0x003FFFFF) - (mobyModels[mobyModelId].address & 0x003FFFFF);
		if (size >= 0 && size < 128000) {
			modelData.numValues = size;
			modelData.type = GENTYPE_RAW;
			memcpy(modelData.byte, mobyModels[mobyModelId], size);
			mod->SetProperty("spyroModel", modelData);
		}

		mod->SetProperty("spyroModelType", GenValueSet("simple"));
	}

	// Send the whole tex map I guess T_T
	GenValueSetContainer<sizeof (objTexMap)> texMap;

	texMap.type = GENTYPE_RAW;
	texMap.numValues = sizeof (objTexMap) + objTexMap.numTextures * sizeof (objTexMap.textures[0]) - sizeof (objTexMap.textures);
	memcpy(texMap.byte, &objTexMap, texMap.numValues);

	mod->SetProperty("spyroTexMap", texMap);
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