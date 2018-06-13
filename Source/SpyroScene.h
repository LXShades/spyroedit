#pragma once
#include "SpyroData.h"

class Scene {
public:
	Scene() : genScene(nullptr), spyroScene(0) {}

public:

	// Called when the level loads. Typically converts spyro scene to gen scene
	void OnLevelEntry();

	// Converts sectors from and to Genesis
	void ConvertSpyroToGen(int sectorIndex);
	void ConvertGenToSpyro(int sectorIndex);

	class GenMesh* GetGenSector(int sectorIndex);

	// Resizes a Spyro scene sector, potentially shuffling sectors in order to fit it in
	struct SceneSectorHeader* ResizeSector(int sectorId, const struct SceneSectorHeader& newHeader);

	// Restores a sector to its originally loaded state
	void RestoreSector(int sectorIndex);

	// Generation functions
	// Generate a crater
	void GenerateCrater(int centreX, int centreY, int centreZ);

	// Uploads scene to Genesis
	void UploadToGen();

	// Save/load scene (WIP)
	void Save(const char* filename);
	void Load(const char* filename);

	void SetGenScene(class GenScene* scene) {genScene = scene;}
	void SetSpyroScene(const SpyroPointer<struct SpyroSceneHeader>& newPointer) {spyroScene = newPointer;}

	// Resets the scene to its originally loaded state
	void Reset();
	
	// Gets the original scene backup since level load
	inline int GetOriginalSceneSize() const;

	// Cleans up the scene cache, etc
	void Cleanup();

public:
	SpyroPointer<struct SpyroSceneHeader> spyroScene;

private:
	class GenScene* genScene;

	struct SpyroSceneCache {
		SpyroSceneCache() : scene(nullptr) {};
		~SpyroSceneCache() {Cleanup();}

		void CopySnapshot(const SpyroSceneHeader* spyroScene);
		void PasteSnapshot(SpyroSceneHeader* spyroScene);
		void Cleanup();
		
		// Base address of the scene on backup (required to determine scene data stuff)
		uint32 baseAddress;

		// Scene block (contains the entire scene)
		SpyroSceneHeader* scene;
	};

	SpyroSceneCache originalScene;
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

	inline int GetSize() const {
		return (7 + numLpVertices + numLpColours + numLpFaces * 2 + numHpVertices + numHpColours * 2 + numHpFaces * 4) * 4;
	}

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

	inline void GetHpVertexAbsolutePositions(IntVector positions[256]) {
		uint32* verts = GetHpVertices();
		int sectorX = xyPos >> 16, sectorY = (xyPos & 0xFFFF), sectorZ = ((zPos >> 14) & 0xFFFF) >> 2;
		bool isSectorFlat = (centreRadiusAndFlags >> 12 & 1);

		for (int i = 0; i < numHpVertices; ++i) {
			uint32 curVertex = verts[i];
			positions[i].x = sectorX + ((curVertex >> 19 & 0x1FFC) >> 2);
			positions[i].y = sectorY + ((curVertex >> 8 & 0x1FFC) >> 2),
			positions[i].z = sectorZ + ((curVertex << 3 & 0x1FFC) >> 2);

			if (game == SPYRO1)
				positions[i].z = sectorZ + ((curVertex << 3 & 0x1FFC) >> 3);

			if (isSectorFlat)
				positions[i].z >>= 3;
		}
	}
};

struct SpyroSceneHeader {
	uint32 size;
	uint32 iForget;
	uint32 numSectors;

	SpyroPointer<SceneSectorHeader> sectors[1];

	// Gets the total size of the data in this scene in bytes. Only works on valid scenes in actual PS1 memory (due to SpyroPointers)
	int GetSize() {
		int count = 12 + numSectors * 4;
		for (int i = 0; i < numSectors; ++i) {
			count += sectors[i]->GetSize();
		}
		return count;
	}
};

struct SceneFace {
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
};

inline int Scene::GetOriginalSceneSize() const {
	return originalScene.scene->size;
}

extern Scene scene;