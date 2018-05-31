#pragma once
#include "GenContainer.h"
#include "GenValueSet.h"
#include "GenType.h"

#define GENSTATEVERSION 1

enum GenObjType : genu16;
struct GenStateTypeTranslator; struct GenStateFilter;
class GenState;

typedef const genchar*(genstateidnamefunc)(genid id); // user-defined function to convert an ID to an ANSI debug name; used for debug printing

// State subtypes (or 'state types' per object type)
// - GENCOMMON_MULTIPLE is a recurring subtype that is used to set or retrieve multiple states from an object.
//   On retrieval, all available states for the object are embedded into it. On set, any available embedded states will be set.
//   GENCOMMON_MULTIPLE states needn't contain all possible states for the object. However, on retrieval, the default behaviour is to retrieve everything available.
// - Event states generally shouldn't be saved or loaded, however they can be synced over LiveGen
// - As a general warning, event states shouldn't be added to GEN*_MULTIPLE states of the object's own self.
//   This is because an object could destroy itself, copy itself, etc while iteratively applying the states.
#define X(def) def,
#define X2(def, startOrEnd) def, startOrEnd = def,
#define X3(def, start, end) def, start = def, end = def,
enum GenStateType : gens16 {
#include "GenStateTypes.h"

	GENSTATE_UNKNOWN = -1,
};

enum GenStateTypeNums {
	GENCOMMON_NUMTYPES = GENCOMMON_END - GENCOMMON_START + 1,
	GENSCENE_NUMTYPES = GENSCENE_END - GENSCENE_START + 1,
	GENMESH_NUMTYPES = GENMESH_END - GENMESH_START + 1,
	GENMODEL_NUMTYPES = GENMODEL_END - GENMODEL_START + 1,
	GENMOD_NUMTYPES = GENMOD_END - GENMOD_START + 1,
	GENMAT_NUMTYPES = GENMAT_END - GENMAT_START + 1,
	GENANIM_NUMTYPES = GENANIM_END - GENANIM_START + 1,
	GENSTATE_NUMTYPES = GENSTATE_END,
};

#pragma pack(push,1)
struct GenStateInfo {
	genid id; // ID of the affected object, scene, etc.
	GenStateType type; // Type of state

	genu32 dataSize;
};

// POD state data for Genesis states. Use GenState to manage states dynamically.
class GenSubstate {
	public:
		GenStateInfo info;
		GenValueSet data;
	
	public:
		// State retrieval functions
		// Retrieves an embedded state as a copy. Fails if no state is available at the index
		bool GetState(GenState* stateOut, int stateIndex) const;

		// Returns a direct pointer to a substate
		const GenSubstate* GetSubstate(int stateIndex) const;
		GenSubstate* GetSubstate(int stateIndex);

		// Returns the number of embedded substates
		int GetNumSubstates() const; // Returns total number of embedded states, or 0 if this is not a valid embedded state type.

	public:
		// Data translation
		// Converts GenStateTypes and/or GenTypes to the specification provided by GenStateTypeTranslator. Used for backwards and forwards-compatibility.
		bool TranslateTypes(const GenStateTypeTranslator* translator);

		// Counts the number of states that would be filtered from this substate, returning the number
		int CountFilteredStates(const GenStateFilter* filter) const;

		// Removes filtered states and returns the state's total size difference
		int RemoveFilteredStates(const GenStateFilter* filter);
		
	public:
		// Miscellaneous
		// Estimates the GenObjType based on the types of substate or substates contained
		GenObjType GetObjType() const;

		static inline int GetSize(GenType elementType, genu32 numElements);
};
#pragma pack(pop)

// GenState: State data container for Genesis objects. This is used to save, load, transfer all kinds of object information and elements.
// Data is contained as either GenValueSet or embedded GenStates. GenStates can be obtained as writable GenState copies, or read directly from the GenStateData.
// When using data as GenValueSet, use the Lock function to both read the data and reallocate enough space for it at the same time.
// When using data as embedded states, use AddState, GetState, and GetNumStates. GetNumStates will return 0 if the data isn't state data.
// GenState has constructors and destructors but also provides equivalent Create and Destroy functions in case it's dynamically allocated.
// Destroy is safe to call multiple times after initialisation
// Note: As of 9/01/17, state data size will never be smaller than sizeof (GenStateInfo) + GenValueSet::GetSize(0, GENTYPE_RAW), in other words guaranteeing
//   that the type and number fields of the GenValueSet struct will be available
class GenState {
	public:
		// Creation/destruction
		GenState();
		GenState(const GenState& srcCopy);
		
		// Construct with preset target
		GenState(genid id, GenStateType type);

		// Construct with elements
		GenState(genid id, GenStateType type, const GenValueSet& initElements); // init with a copy of elements
		~GenState();

	public:
		// Manual creation/destruction
		void Create(genid id, GenStateType type);
		void Create(genid id, GenStateType type, const GenValueSet& initElements);
		void Create(const GenState& srcCopy);
		void Create();
		void Destroy();

	public:
		// Element functions
		GenValueSet* Lock(); // Locks and returns element data without reallocating anything
		GenValueSet* Lock(GenType dataType, genu32 numElements); // Locks data with a size to contain elements of type and num.
		void Unlock();

		bool SetElements(const GenValueSet& values); // Sets the elements directly from a source valueset. Fails if locked.
		const GenValueSet& GetElements() const; // Returns a const pointer to elements without locking. For ease of use. Ensure the class isn't locked during this time.

		bool ClearElements(); // Clears state elements (not state info). Must be unlocked.

	public:
		// Substate functions. To add substates, the GenState must unlocked and be of the type 'GENTYPE_GENSTATE'.
		bool AddSubstate(const GenState& stateIn); // Copies and embeds 'stateIn' to this state
		int  AddSubstates(const GenState*const* sourceStates, int numStates); // Embeds states via a list of GenState pointers. Faster than adding states separately.
		
		const GenSubstate& GetSubstate() const {return *substate;} // Retrieves the root substate
		bool GetSubstate(GenState* stateOut, int stateIndex) const; // Retrives an embedded state as a copy. stateOut must be initialised. Fails if no state is available
		const GenSubstate* GetSubstate(int embeddedStateIndex) const; // Retrieves an embedded state directly as a pointer. Returns nullptr if unavailable
		
		int GetNumSubstates() const; // Returns total number of embedded states, or 0 if this is not a valid embedded state type

		// State transferral
		// MoveFrom: Moves state data and info from another state object. The source state is zeroed out after this as two states cannot share the same data!
		bool MoveFrom(GenState* source);

		// Data conversion
		bool TranslateTypes(const GenStateTypeTranslator* translator); // Converts state type IDs

		int FilterStates(const GenStateFilter* filter); // Removes states flagged false in the GenStateFilter, returning the number of states removed. 0 means no change

	public:
		// Getters/setters
		inline genid GetId() const {return substate->info.id;}
		inline void SetId(genid id) {substate->info.id = id;}

		inline GenStateType GetType() const {return (GenStateType) substate->info.type;}
		inline void SetType(GenStateType type) {substate->info.type = type;}
		
		inline const GenStateInfo& GetInfo() const {return substate->info;}
		inline void SetInfo(const GenStateInfo& infoIn) {substate->info.id = infoIn.id; substate->info.type = infoIn.type;} // Warning: ignores dataSize. Use Lock to change that.
		inline void SetInfo(genid id, GenStateType type) {substate->info.id = id; substate->info.type = type;}

		inline genu32 GetSize() const {return substate->info.dataSize;} // returns total size of embedded GenValueSet OR embedded states
		inline GenType GetElementType() const {return (GenType) substate->data.type;}
		inline genu32 GetNumElements() const {return substate->data.numValues;}

		GenObjType GetObjType() const; // Discovers this state's object type based on its state type. If GENCOMMON_MULTIPLE, this also searches embedded states
		
		// PrintInfo: detailed information about the state for debugging
		void PrintInfo(bool printChildren = true, int indentNum = 0, genstateidnamefunc idNameFunc = 0, bool firstPrint = true) const;

		// Static functions in case you don't have a GenState in hand
		static GenObjType GetObjType(GenStateType type); // Converts state type to its associated obj type. If you can, use the nonstatic version.
		static inline const char* GetTypeName(GenStateType type);

	private:
		GenSubstate* substate;
		GenSubstate emptySubstate; // Hack to avoid having leftover global data before the program quits (Yeah it's annoying!)

		genbool8 locked;

		void SetSubstateSize(genu32 size);
};

struct GenStateTypeTranslator {
	// stateTypes and elementTypes are indexes that contain type IDs from an external source/format
	// e.g. if the local GENTYPE_U32 is 5 and the external one is 2, elementTypes[GENTYPE_U32] = 2
	gens16 stateTypes[GENSTATE_NUMTYPES];
	gens16 elementTypes[GENTYPE_NUMTYPES];

	inline void ResetToLocal() {
		for (int i = 0; i < GENSTATE_NUMTYPES; i++)
			stateTypes[i] = i;
		for (int i = 0; i < GENTYPE_NUMTYPES; i++)
			elementTypes[i] = i;
	};

	inline void ResetToUnknown() {
		for (int i = 0; i < GENSTATE_NUMTYPES; i++)
			stateTypes[i] = -1;
		for (int i = 0; i < GENTYPE_NUMTYPES; i++)
			elementTypes[i] = -1;
	};

	inline GenType ConvertType(GenType type) {
		for (int i = 0; i < GENTYPE_NUMTYPES; i++) {
			if (elementTypes[i] == type)
				return (GenType) i;
		}
		return GENTYPE_UNKNOWN;
	}

	inline GenStateType ConvertStateType(GenStateType type) {
		for (int i = 0; i < GENSTATE_NUMTYPES; i++) {
			if (stateTypes[i] == type)
				return (GenStateType) i;
		}
		return GENSTATE_UNKNOWN;
	}
};

struct GenStateChangeLog {
	genu32 flags[(GENSTATE_NUMTYPES + 31) / 32];

	inline bool GetChanged(GenStateType type) {
		return (flags[(int) type / 32] & (1 << (type & 31))) != 0;
	}

	inline void SetChanged(GenStateType type, bool changed) {
		flags[(int) type / 32] = changed ? (flags[(int) type / 32] | (1 << (type & 31))) : (flags[(int) type / 32] & ~(1 << (type & 31)));
	}
};

// GenStateFilter: Declares what is desired and what isn't desired for a GenState
// Used to decide which states to retrieve from objects, or which states to send/receive over LiveGen. Optimises bandwidth.
struct GenStateFilter {
	GenStateFilter() : enabledStates() {
		enabledStates[GENCOMMON_MULTIPLE / 32] |= GENCOMMON_MULTIPLE & 31; // GENCOMMON_MULTIPLE should always be allowed
	};

	genu32 enabledStates[(GENSTATE_NUMTYPES + 31) / 32];

	inline bool IsEnabled(GenStateType index) const {
		if (index < GENSTATE_NUMTYPES)
			return (enabledStates[index / 32] & (1 << (index & 31))) != 0;
		return false;
	}

	void EnableStates(int numStates, GenStateType firstState, ...);
	void DisableStates(int numStates, GenStateType firstState, ...);

	inline void EnableAll() {
		for (int i = 0; i < sizeof (enabledStates)/4; i++)
			enabledStates[i] = 0xFFFFFFFF;
	}

	inline void DisableAll() {
		for (int i = 0; i < sizeof (enabledStates)/4; i++)
			enabledStates[i] = 0;
		enabledStates[GENCOMMON_MULTIPLE / 32] |= GENCOMMON_MULTIPLE & 31;
	}
};

#undef X
#undef X2
#undef X3
#define X(def) #def,
#define X2(def, startOrEnd) #def,
#define X3(def, start, end) #def,
static const char* genStateTypeNames[] = {
#include "GenStateTypes.h"
};
static const int genStateMaxTypeNameLength = 32;

inline const char* GenState::GetTypeName(GenStateType type) {
	if ((genu32) type < sizeof (genStateTypeNames) / sizeof (genStateTypeNames[0]))
		return genStateTypeNames[(int) type];
	else
		return "<genstate_unknown>";
}

inline int GenSubstate::GetSize(GenType type, genu32 numElements) {
	return sizeof (GenStateInfo) + GenValueSet::GetSize(type, numElements);
}