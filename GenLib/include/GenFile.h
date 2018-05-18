#pragma once
#include "GenState.h"

class File; struct GenFileBlock; struct GenFileBlockHeader;

class GenFile {
	public:
		void Create();
		void Destroy();

		bool LoadFromFile(const genwchar* filename, genu32 validFlagsIn, genu32* validFlagsOut);
		bool SaveToFile(const genwchar* filename);
		bool SaveToFileASCII(const genwchar* filename);

		GenState* AddState(const GenState* stateIn = nullptr /* optional source-copy state */);
		GenState* GetState(genu32 id);
		genu32 GetNumStates() const;

		void* AddBlock(const char* blockTag, int blockSize);
		void* ResizeBlock(const char* blockTag, int blockSize);
		void* GetBlock(const char* blockTag);
		genu32 GetBlockSize(const char* blockTag) const;
		int GetBlockId(const char* blockTag) const; // -1 if not found
		bool RemoveBlock(const char* blockTag);
		void ClearBlocks();

	private:
		void WriteBlock(File* file, const char* blockTag, genu32 blockSize);
		bool ReadBlock(File* file, GenFileBlockHeader* blockOut);
		void WriteASCIIState(File* file, const GenState* state, int indentationLevel);

		GenState* states;
		genu32 numStates;

		GenFileBlock** blocks;
		genu32 numBlocks;
};

enum GenFileValidFlags {
	GENVF_VERSION = 1,
	GENVF_READABLE = 2, // whether the file was read
	GENVF_BLOCKS = 4, // whether the block format is valid
	GENVF_STATES = 8, // whether the states are valid
	GENVF_TYPES = 16, // whether the file contains a valid internal or external type database
	GENVF_ALLSTATESKNOWN = 32, // whether the state types in the file are all known by this version
	GENVF_ALLSTATESMATCH = 64, // whether the state types in the file all match the IDs of this version of GenLib (i.e. no translation required)

	GENVF_SALVAGABLE = GENVF_STATES,
	GENVF_ALLSAFE = 1 | 2 | 4 | 8 | 16 | 32 | 64,
};