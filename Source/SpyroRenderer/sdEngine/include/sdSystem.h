#pragma once
#include "sdError.h"
#include "sdTypes.h"

#define SDREPORTDEBUGERRORS
#define SDOVERLOAD_NEW 1

#ifdef _DEBUG
#define SDDEBUGMEMORY // Use custom memory pool that tracks memory used
#define SDPROTECTEDMEMORY // Use reserved memory that will immediately throw an exception when the upper boundary is overrun (for more precise debugging)
#define SDTRACKFREES // Tracks 
#endif
#ifndef RESTRICT
#define RESTRICT __restrict
#endif

#define ALLOC(output, size, pool) (&##output ? (*((void**) &##output) = Sys::Alloc(size, pool)) : Sys::Alloc(size, pool))
#define REALLOC(output, input, size, pool) (&##output ? *((void**) &##output) = Sys::Realloc((void*) (input), size, pool) : Sys::Realloc((void*) (input), size, pool))
#define FREE(input) Sys::Free(input);

struct AllocData; struct AllocDebugDetails;  struct SysTimeLog; enum MemPool; struct VirtualBlock;
class VoidPtr;

#if SDOVERLOAD_NEW==1
void* operator new (std::size_t size);
#endif

// MemPool: Section/pool of dynamic memory used for allocations. You can define your own pools from 0+!
enum MemPool : int8 {
	POOL_UNDEFINED = -1, 
	POOL_SCRATCH = -2, // Temporary memory
	POOL_SYSTEM = -3, // Standard system memory objects e.g. lists, strings
	POOL_GUI = -4, // GUI stuff
	POOL_TEXTURES = -5, // Textures
	POOL_SHADERS = -6, // Shaders
	POOL_D3DBUFFERS = -7, // D3D Buffers
	POOL_SOUNDS = -8, // Sound streams & related stuff
	POOL_SDM = -9, // SDM file stuff
	POOL_COLLISION = -10, // Collision system data
	POOL_NETWORK = -11, // Networking buffers etc
};

// Sys: Main functions relating to the core system (allocation, time, debugging, printing etc) (might be split into classes at a later stage)
class Sys {
public:
	// ===== Memory allocation functions =====
	/* Alloc: Allocates memory in the specified pool (for debugging) and stores in output.
	Output can be nullptr if preferred; the pointer to the newly allocated memory will always be returned as a void*.
	An allocation of 0 returns nullptr. */
	static sdRawPtr Alloc(int size, MemPool pool = POOL_UNDEFINED);

	// Realloc: Reallocates memory.
	static sdRawPtr Realloc(void* input, int size, MemPool pool = POOL_UNDEFINED);

	// Free: Frees memory. nullptr frees are safe.
	static void Free(void* input);

	// ===== Memory information functions =====
	// GetNumActiveAllocs: Returns number of allocations
	static uint32 GetNumActiveAllocs();

	// GetTotalAllocSize: Returns total size of allocated memory
	static uint64 GetTotalAllocSize();

	// GetPoolSize: Returns the size in bytes of the memory allocated in the specified memory pool(s)
	static uint64 GetPoolSize(MemPool pool);

	// GetAllocs: Returns a direct pointer to the raw list of allocations. Not very safe; use for debugging only!
	static const AllocData* GetAllocs();
	static uint32 GetNumAllocs();

#ifdef SDDEBUGMEMORY
	// ===== Memory debugging functions =====
	// GetAllocDetails: Returns detailed information about an allocation
	static bool GetAllocDetails(const AllocData* allocIn, AllocDebugDetails* detailsOut, unsigned int maxStackSteps = Sys::maxNumStackSteps);

	// PrintAllocDetails: Prints detailed information of an allocation
	static void PrintAllocDetails(const AllocData* alloc);

	// PrintAllocHistory: Prints alloc history of up to maxNumItems or ALLOCHISTORYSIZE
	static void PrintAllocHistory(int maxNumItems);
#endif

	// ===== Output functions =====
	// Print: Prints a debug message and, if desired, logs it (currently unimplemented)
	// Buffered prints can be used for instances where you expect to print multiple lines in a short time. Use PrintFlush to flush them to the console
	static void Print(const char*, ...);
	static void Print(const wchar*, ...);
	static void PrintBuffered(const char*, ...);
	static void PrintStack(int maxNumSteps);
	static void PrintFlush();

	// ===== Error functions =====
	// Error: Used by the engine to report an error. Currently only the last error stays logged
	static void Error(const char* formatString, ...);
	// Error (type 2): Used by the engine to report an error. Checks the condition first; if the condition is TRUE an error is assumed. If the error occurs, returns true.
	static bool Error(bool condition, const char* formatString, ...);
	/* Error (type 3): Used by the engine to report an error. Checks the condition first; if the condition is TRUE an error is assumed. If the error occurs, returns true.
	Includes error code and function name. */
	static bool Error(bool condition, const char* functionName, int errorCode, const char* errorCodeString, const char* formatString, ...);
	/* Warning: Used to report warnings, which are errors that are nonfatal to the current functions but may cause problems later if unaddressed.
	Generally used only for cases where the error management is unimplemented. */ 
	static void Warning(const char* formatString, ...);

	// Assert: Use SDASSERT
#ifdef _DEBUG
	static void Assert(const char* erMsg);
#endif

	// ===== Error functions =====
	// GetLastError: Returns the last logged error as a string.
	static const char* GetLastError();

	// GetLastErrorCode: Returns the last logged error code as an int.
	static int GetLastErrorCode();

	// GetLastErrorCode: Returns the last logged error code as a string.
	static const char* GetLastErrorCodeString();

	// GetLastWarning: Returns the last logged error as a string.
	static const char* GetLastWarning();

	// GlobalError: Crashes the program with an error message
	static void GlobalError(const wchar* string, ...);

	// ===== Time functions =====
	// GetTime: Gets the system time in milliseconds
	static uint32 GetTime();

	// GetTimeUs: Gets the system time in microseconds
	static uint64 GetTimeUs();

	// Sleep: Waits for the given time period in milliseconds
	static void Sleep(int time);

	// ===== Time log functions =====
	// TimeLog: Starts a time log with the tag 'tag'. Stops the previous time log unless parallel. If parallel, the time log runs until it is explicitly stopped
	static void TimeLog(const char* tag, bool parallel = false);

	// StopTimeLog: Ends the active nonparallel time log. If tag is non-nullptr, only the tagged specific log is stopped, which may or may not be parallel
	static void StopTimeLog(const char* tag = nullptr);

	// StopAllTimeLogs: Ends all time logs including parallel ones
	static void StopAllTimeLogs();

	// CheckpointTimeLogs: Resets the checkpoint on all time logs. Usually used per-frame. All accumulated log times since the last checkpoint are reset
	static void CheckpointTimeLogs();

	// GetTimeLog: Returns time log info either by tag or ID, or nullptr if unavailable. All tags previously logged via LogTime are retrievable
	static const SysTimeLog* GetTimeLog(const char* tag);
	static const SysTimeLog* GetTimeLog(int id);

	// GetNumTimeLogs: Returns numder of time logs recorded
	static int GetNumTimeLogs();

public:
	// System-wide const parameters
	static const uint32 maxNumStackSteps = 5; // Maximum number of stack steps for memory allocation tracking

#ifdef SDTRACKFREES
	static const uint32 maxFreeTrackers = 20; // TODO: implement free trackers
#endif

private:
#ifdef SDDEBUGMEMORY
	static __declspec(noinline) void RegisterAdditionalAllocInfo(AllocData* alloc, const char* varName, const char* file, int line, const void* ra, int action);
#endif

	// Internal allocation/reallocatin/free functions
	static inline void* PerformAlloc(int size);
	static inline void* PerformRealloc(void* ptr, int oldSize, int newSize);
	static inline void PerformFree(void* ptr, int size);

	static inline VirtualBlock* FindVirtualBlock(void* dataAddress);
};

struct AllocData {
	uint16 dbgAllocId;
	AllocData* next; // pointer to next free alloc block
	AllocData* prev; // pointer to the previous free alloc block

	void* data; // Pointer to the data
	uint32 dataSize; // Size of the data

	MemPool memPool; // MemPool ID

#ifdef SDDEBUGMEMORY
	void* returnAddresses[Sys::maxNumStackSteps]; // Address following the calling instruction
	int16 numReturnAddresses;

	int8 lastAction; // 0 = alloc, 1 = realloc, 2 = free
#endif
};

#ifdef SDDEBUGMEMORY
struct AllocDebugDetails {
	AllocDebugDetails() : functionName{{0}}, fileName{{0}}, line{{0}}, stackDepth(0) {};

	char functionName[Sys::maxNumStackSteps][64]; // name of the function where the allocation took place
	char fileName[Sys::maxNumStackSteps][32]; // name of the code file where the allocation took place
	int32 line[Sys::maxNumStackSteps]; // line number of the allocation
	int8 stackDepth; // depth of the stack
};
#endif

#define SDTIMELOGNAMELENGTH 50
struct SysTimeLog {
	char tag[SDTIMELOGNAMELENGTH];
	uint32 tagHash; // quickfind

	bool8 active; // whether this time log is (still) counting
	uint64 startTime; // time this log started
	uint64 timeRecorded, totalTimeRecorded;

	uint32 numHits;
};

#if SDOVERLOAD_NEW == 1
// Pooled new
void* operator new(unsigned size, MemPool pool);
#endif
