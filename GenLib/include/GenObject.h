#pragma once
#include "GenType.h"
#include "GenState.h"
#include "GenError.h"

#define MAXNUMMODELMODS 10

class GenObject; class GenScene; class GenMesh; class GenMod; class GenModel; class GenInst;

/* WARNING: AS MORE STATE TYPES ARE CREATED, THE BIT-BASED SYSTEM FOR VALIDATION NEEDS UPDATING */

// Genesis object types --NOTE Make sure to update genObjTypeDefs AND Scene::objects!!--
enum GenObjType : genu16 {
	GENOBJ_SCENE = 0, // global scene data
	GENOBJ_MODEL = 1, // model data (mod IDs, etc)
	GENOBJ_MESH = 2, // mesh element data (vertex, edge, face, etc)
	GENOBJ_INST = 3, // inst data (properties, mesh connection, etc)
	GENOBJ_MOD = 4, // mod data (mesh data (contains mesh object substates))
	GENOBJ_MAT = 5, // material data (source file, etc)
	GENOBJ_ANIM = 6, // animation (animation tracks, etc)
	GENOBJ_NUMTYPES,

	GENOBJ_UNKNOWN = 0xFFFF,
};

struct GenObjTypeDef {
	template <class T>
	GenObjTypeDef(T, const char* namie, GenStateType sStart, GenStateType sEnd) : name(namie), classSize(sizeof (T)), stateStart(sStart), stateEnd(sEnd), 
		createFunc((void(GenObject::*)())&T::Create), destroyFunc((void(GenObject::*)())&T::Destroy), 
		setStateFunc((int(GenObject::*)(const GenStateData*))&T::SetState), getStateFunc((int(GenObject::*)(GenState*) const)&T::GetState) {};

	const char* const name; // obj type name
	genu32 classSize;

	GenStateType stateStart; // Range of GenStateTypes for this object type, or -1 if N/A (e.g. GENSCENE_START)
	GenStateType stateEnd; // (e.g. GENSCENE_END)

	void (GenObject::*createFunc)();
	void (GenObject::*destroyFunc)();

	int (GenObject::*setStateFunc)(const GenStateData* stateDataIn);
	int (GenObject::*getStateFunc)(GenState* stateInOut) const;
};

class GenObject {
	public:
		// creation/destruction
		void Create(); // dummy create: does nothing--please don't call. yeah it's dumb
		void Create(genid id, GenObjType type, GenScene* parentScene);
		void Destroy();

		// base getters
		inline genid GetId() const;
		inline GenObjType GetObjType() const;
		
		// state stuff
		int SetState(const GenStateData* state); // returns number of states successfully applied
		int GetState(GenState* state) const; // returns number of states successfully retrieved

		int DefSetState(const GenStateData* state); // used in SetState functions to handle stuff like GENCOMMON_MULTIPLE
		int DefGetState(GenState* state) const; // GetState equivalent ^

		// validation stuff
		inline bool IsValid() const; // returns whether all states are valid
		inline bool IsStateValid(GenStateType stateType) const; // returns whether the given state is valid

		inline void ValidateState(GenStateType stateType); // marks a state as valid. should typically be used only with states particular to the GenObject type
		inline void InvalidateState(GenStateType stateType); // marks a state as invalid. ditto ^

		inline void ValidateAll(); // validates all states
		inline void InvalidateAll(); // invalidates all states

		// user stuff
		inline void SetExtra(void* extraPtr); // sets an application-defined pointer. useful for associating genobjects with application objects. does NOT affect validation
		inline void* GetExtra() const; // retrieves application-defined pointer

		// static functions
		inline static const char* GetTypeName(GenObjType type);

		static const GenObjTypeDef typeDefs[GENOBJ_NUMTYPES];

	protected:
		genid id; // main object GenID
		GenObjType objType; // of GenObjType
		GenScene* scene; // parent scene
		void* extra; // application-defined pointer

		genvalidflags validStates[(GENSTATE_NUMTYPES + 31) / 32]; // bit flags indicating state validity (validStates[stateType / 32] & (1 << (stateType & 31)))
};

// GenScene
class GenScene : public GenObject {
	public:
		// Create/Destroy: these must be called when using GenScenes
		// GenScene is the only object that you'll call Create/Destroy on. The rest of the objects you create should be via GenScene::CreateObject
		// GenScene is self-contained; when destroyed, all its child objects will be cleaned up automagically
		void Create();
		void Destroy();

		void Update();

		void SetId(genid id);

		int SetState(const GenStateData* state);
		int GetState(GenState* state) const;

		// CreateObject: Creates an object of the specified type. Use an ID of 0 to generate a random ID.
		// Returns the created object. Fails if an object with the ID already exists, regardless of its type.
		GenObject* CreateObject(genid id, GenObjType type);
		bool DestroyObject(genid id);

		// GetObject: various functions for obtaining objects by ID or index. Indexed objects are valid between 0-GetNumObjects().
		inline GenObject* GetObjectByIndex(genu32 index);
		inline genu32 GetNumObjects();
		inline genu32 GetNumObjects(GenObjType typeFilter);

		GenObject* GetObjectById(genid id);
		const GenObject* GetObjectById(genid id) const;
		inline GenMesh* GetMeshById(genid id);
		inline GenMod* GetModById(genid id);
		inline GenModel* GetModelById(genid id);
		inline GenInst* GetInstById(genid id);

		// Changed objects
		// Functions provided to iterate through objects that have changed since their last validation
		// When finished updating you should validate all the objects involved to confirm they've been successfully updated or checked
		inline GenObject* GetNextChangedObject(GenObject* object); // finds the next invalidated GenObject following "object", or if object==nullptr, retrieves the first one
		inline genu32 GetNumChangedObjects() const; // returns current total number of changed objects

		inline void ValidateAllObjects(); // validates all states of all objects, useful on scene loads and initialisations

	private:
		GenList<GenObject> objects;
};

// GenModel
class GenModel : public GenObject {
	public:
		bool AddModifier(genid modId);

		GenMesh* GetMesh();
		genid GetMeshId()  const;

		inline genid GetModifier(int index) const;
		inline int GetNumModifiers() const;

		int SetState(const GenStateData* state);
		int GetState(GenState* state) const;
	private:
		genid modifiers[MAXNUMMODELMODS];
		genu32 numModifiers;
};

// GenInst
class GenInst : public GenObject {
	public:
		void Create();

		inline void SetModel(genid modelId);
		inline genid GetModel() const;

		inline void SetName(const genwchar* name);
		inline void SetName(const genchar* name);
		inline const genwchar* GetName() const;

		inline const GenTransform* GetTransform() const;

		inline void SetMaterial(genid materialId);
		inline genid GetMaterial() const;
		
		inline void SetMesh(genid meshId);
		inline genid GetMesh() const;

		int SetState(const GenStateData* state);
		int GetState(GenState* state) const;

	private:
		genid model;
		genid material;
		genid mesh;
		genwchar name[256];

		GenTransform transform;
};

// GenMod --remember to update modInitTable!--
enum GenModifierType : genu16 {
	MOD_UNDEFINED = 0, // This monster exists due to the magic of implicit creation

	// CREATION MODS
	MOD_BASECUBE,
	MOD_BASESPHERE,
	MOD_BASECYLINDER,
	MOD_BASEPLANE,

	// MESH EDIT MODS
	MOD_EDITMESH,
	MOD_UVEDIT,
	MOD_VERTPAINT,

	// EFFECT MODS
	MOD_MIRROR,

	// UNORDERED MODS // TODO add compatibility so we can reorder
	MOD_BASEBONE,
	MOD_SKIN,

	NUMMODTYPES,
	MOD_INVALID = 0xFFFF // This monster exists due to the magic of errors and error return values
};

class GenMod : public GenObject {
	public:
		void Create();
		void Destroy();

		int SetState(const GenStateData* state);
		int GetState(GenState* state) const;

		inline void SetModType(GenModifierType type);
		inline GenModifierType GetModType() const;

		GenMesh* GetMesh();
		genid GetMeshId() const;

	private:
		genid mesh;
		GenModifierType modType;
};

class GenMaterial : public GenObject {
	public:
		void Create();
};

class GenAnim : public GenObject {
	public:
		void Create();
};

// GenMesh stuff starts here
enum GenMeshElementType { // remember to update GenMesh::elementSizes if changed!
	GENMET_VERT = 0,
	GENMET_EDGE,
	GENMET_FACE,
	GENMET_UV,
	GENMET_COLOUR,
	GENMET_NUMTYPES
};

enum GenMeshElementLockType {
	GENMETLOCK_UNLOCKED = 0,
	GENMETLOCK_RW = 1,
	GENMETLOCK_CONST = 2
};

struct GenMeshVert {
	GenVec3 pos;
};

struct GenMeshEdge {
	union {
		struct {genu32 vert1, vert2;};
		genu32 verts[2];
	};
};

struct GenMeshFaceSide {
	genu32 vert;
	genu32 edge;
	genu32 uv;
	genu32 colour;
};

struct GenMeshFace {
	GenMeshFaceSide sides[16]; // for now, random hack-cap of 16 sides per face...what could go wrong... :>
	genu32 numSides;
};

class GenMesh : public GenObject {
	public:
		void Create();
		void Destroy();

		bool SetNum(GenMeshElementType type, genu32 num);
		inline genu32 GetNum(GenMeshElementType type) const;

		int SetState(const GenStateData* state);
		int GetState(GenState* state) const;

		// CopyElements: Copies all elements from another mesh to this one. Returns true only if all elements can be copied. Fails if any elements are locked.
		bool CopyElements(const GenMesh* source);

		// Element reading (basic): receive the array of elements directly.
		// WARNING: if SetNum is called on the elements, or an internal action changes the number, its base address will be invalidated!
		inline const GenMeshVert* GetVerts() const {return verts;}
		inline const GenMeshEdge* GetEdges() const {return edges;}
		inline const GenMeshFace* GetFaces() const {return faces;}
		inline const GenVec2* GetUvs() const {return uvs;}
		inline const genu32* GetColours() const {return colours;}

		inline genu32 GetNumVerts() {return numElements[GENMET_VERT];}
		inline genu32 GetNumEdges() {return numElements[GENMET_EDGE];}
		inline genu32 GetNumFaces() {return numElements[GENMET_FACE];}
		inline genu32 GetNumUvs() {return numElements[GENMET_UV];}
		inline genu32 GetNumColours() {return numElements[GENMET_COLOUR];}

		// Element locking: lock elements for direct reading and/or writing
		// Use the Const functions if you plan solely on reading as this will prevent any unnecessary validation.
		// The only difference between the Const and the Get functions are safety: the base address is guaranteed not to change during a lock
		inline void* LockElements(GenMeshElementType type);
		inline const void* LockElementsConst(GenMeshElementType type);
		inline void UnlockElements(GenMeshElementType type, void* pointer);

		inline GenMeshVert* LockVerts() {return (GenMeshVert*) LockElements(GENMET_VERT);}
		inline GenMeshEdge* LockEdges() {return (GenMeshEdge*) LockElements(GENMET_EDGE);}
		inline GenMeshFace* LockFaces() {return (GenMeshFace*) LockElements(GENMET_FACE);}
		inline GenVec2* LockUvs() {return (GenVec2*) LockElements(GENMET_UV);}
		inline genu32* LockColours() {return (genu32*) LockElements(GENMET_COLOUR);}

		inline const GenMeshVert* LockVertsConst() {return (const GenMeshVert*) LockElementsConst(GENMET_VERT);}
		inline const GenMeshEdge* LockEdgesConst() {return (const GenMeshEdge*) LockElementsConst(GENMET_EDGE);}
		inline const GenMeshFace* LockFacesConst() {return (const GenMeshFace*) LockElementsConst(GENMET_FACE);}
		inline const GenVec2* LockUvsConst() {return (const GenVec2*) LockElements(GENMET_UV);}
		inline const genu32* LockColoursConst() {return (const genu32*) LockElementsConst(GENMET_COLOUR);}

		inline void UnlockVerts(GenMeshVert* ptr) {UnlockElements(GENMET_VERT, ptr);}
		inline void UnlockEdges(GenMeshEdge* ptr) {UnlockElements(GENMET_EDGE, ptr);}
		inline void UnlockFaces(GenMeshFace* ptr) {UnlockElements(GENMET_FACE, ptr);}
		inline void UnlockUvs(GenVec2* ptr) {UnlockElements(GENMET_UV, ptr);}
		inline void UnlockColours(genu32* ptr) {UnlockElements(GENMET_COLOUR, ptr);}

		// Element validation
		inline bool IsElementValid(GenMeshElementType type) const;

		inline void ValidateElements(GenMeshElementType type);
		inline void InvalidateElements(GenMeshElementType type);

		static const int elementSizes[GENMET_NUMTYPES];

	private:
		union {
			// elements can be accessed via the elements base list array (e.g. elements[GENMET_VERT]), or the explicitly named equivalent (e.g. verts)
			struct {
				// note: change this if GenMeshElementType enum is modified
				GenMeshVert* verts;
				GenMeshEdge* edges;
				GenMeshFace* faces;
				GenVec2* uvs;
				genu32* colours;
			};
			void* elements[GENMET_NUMTYPES];
		};
		genu32 numElements[GENMET_NUMTYPES];
		genu8 elementsLocked[GENMET_NUMTYPES];
};

inline genid GenObject::GetId() const {
	return id;
}

inline GenObjType GenObject::GetObjType() const {
	return (GenObjType)objType;
}

inline const char* GenObject::GetTypeName(GenObjType type) {
	if (type < GENOBJ_NUMTYPES && type >= 0)
		return GenObject::typeDefs[type].name;
	else
		return "GENOBJ_UNKNOWN";
}

inline void GenObject::ValidateState(GenStateType stateType) {
	validStates[stateType / 32] |= (1 << (stateType & 31));
}

inline void GenObject::InvalidateState(GenStateType stateType) {
	validStates[stateType / 32] &= ~(1 << (stateType & 31));
}

inline void GenObject::ValidateAll() {
	for (genu32 i = 0; i < sizeof (validStates)/sizeof (validStates[0]); i++)
		validStates[i] = 0xFFFFFFFF;
}

inline void GenObject::InvalidateAll() {
	for (genu32 i = 0; i < sizeof (validStates)/sizeof (validStates[0]); i++)
		validStates[i] = 0;
}

inline void GenObject::SetExtra(void* extraPtr) {
	extra = extraPtr;
}

inline void* GenObject::GetExtra() const {
	return extra;
}

inline bool GenObject::IsValid() const {
	for (genu32 i = 0; i < sizeof (validStates)/sizeof (validStates[0]); i++) {
		if (validStates[i] != 0xFFFFFFFF)
			return false;
	}
	return true;
}

inline bool GenObject::IsStateValid(GenStateType stateType) const {
	return (validStates[stateType / 32] & (1 << (stateType & 31))) != 0;
}

inline GenObject* GenScene::GetObjectByIndex(genu32 index) {
	if (index >= objects.GetNum())
		return nullptr;

	return objects[index];
}

inline genu32 GenScene::GetNumObjects() {
	return objects.GetNum();
}

inline genu32 GenScene::GetNumObjects(GenObjType typeFilter) {
	genu32 num = 0;

	for (genu32 i = 0, e = objects.GetNum(); i < e; i++) {
		if (objects[i]->GetObjType() == typeFilter)
			num++;
	}

	return num;
}

inline GenMesh* GenScene::GetMeshById(genid id) {
	if (GenObject* obj = GetObjectById(id)) {
		if (obj->GetObjType() == GENOBJ_MESH)
			return (GenMesh*) obj;
	}
	return nullptr;
}

inline GenMod* GenScene::GetModById(genid id) {
	if (GenObject* obj = GetObjectById(id)) {
		if (obj->GetObjType() == GENOBJ_MOD)
			return (GenMod*) obj;
	}
	return nullptr;
}

inline GenModel* GenScene::GetModelById(genid id) {
	if (GenObject* obj = GetObjectById(id)) {
		if (obj->GetObjType() == GENOBJ_MODEL)
			return (GenModel*) obj;
	}
	return nullptr;
}

inline GenInst* GenScene::GetInstById(genid id) {
	if (GenObject* obj = GetObjectById(id)) {
		if (obj->GetObjType() == GENOBJ_INST)
			return (GenInst*) obj;
	}
	return nullptr;
}

inline GenObject* GenScene::GetNextChangedObject(GenObject* object) {
	genbool8 foundFirst = (object == nullptr ? true : false);
	for (genu32 i = 0, e = objects.GetNum(); i < e; i++) {
		if (objects[i] == object) {
			foundFirst = true;
			continue;
		} else if (foundFirst) {
			if (!objects[i]->IsValid())
				return objects[i];
		}
	}

	return nullptr;
}

inline genu32 GenScene::GetNumChangedObjects() const {
	genu32 num = 0;
	for (genu32 i = 0, e = objects.GetNum(); i < e; i++) {
		if (!objects[i]->IsValid())
			num++;
	}

	return num;
}

inline void GenScene::ValidateAllObjects() {
	for (genu32 i = 0, e = objects.GetNum(); i < e; i++)
		objects[i]->ValidateAll();
}

int GenModel::GetNumModifiers() const {
	return numModifiers;
}

genid GenModel::GetModifier(int index) const {
	return modifiers[index];
}

inline void GenMod::SetModType(GenModifierType type) {
	if (modType != type) {
		modType = type;
		InvalidateState(GENMOD_TYPE);
	}
}

inline GenModifierType GenMod::GetModType() const {
	return (GenModifierType) modType;
}

inline void GenInst::Create() {
	transform.Reset();
}

inline void GenInst::SetModel(genid modelId) {
	if (this->model != modelId) {
		this->model = modelId;
		InvalidateState(GENINST_MODELID);
	}
}

inline genid GenInst::GetModel() const {
	return model;
}

inline void GenInst::SetName(const genwchar* newName) {
	int len = 0;
	for (len = 0; newName[len] && len < (sizeof (name) / sizeof (name[0])) - 1; len++)
		name[len] = newName[len];
	name[len] = '\0';

	InvalidateState(GENINST_NAME);
}

inline void GenInst::SetName(const genchar* newName) {
	int len = 0;
	for (len = 0; newName[len] && len < (sizeof (name) / sizeof (name[0])) - 1; len++)
		name[len] = newName[len];
	name[len] = '\0';

	InvalidateState(GENINST_NAME);
}

inline const genwchar* GenInst::GetName() const {
	return name;
}

inline const GenTransform* GenInst::GetTransform() const {
	return &transform;
}

inline void GenInst::SetMaterial(genid materialId) {
	material = materialId;
}

inline genid GenInst::GetMaterial() const {
	return material;
}

inline void GenInst::SetMesh(genid meshId) {
	mesh = meshId;
}

inline void* GenMesh::LockElements(GenMeshElementType type) {
	if (elementsLocked[type]) {
		GenError::CallError(GenErr("Elements already locked", GENERR_LOCKEDRESOURCE));
		return nullptr;
	}

	elementsLocked[type] = GENMETLOCK_RW;
	return elements[type];
}

inline const void* GenMesh::LockElementsConst(GenMeshElementType type) {
	if (elementsLocked[type]) {
		GenError::CallError(GenErr("Elements already locked", GENERR_LOCKEDRESOURCE));
		return nullptr;
	}

	elementsLocked[type] = GENMETLOCK_CONST;
	return elements[type];
}

inline void GenMesh::UnlockElements(GenMeshElementType type, void* pointer) {
	if (elementsLocked[type] && elements[type] == pointer) {
		// todo: invalidation
		elementsLocked[type] = GENMETLOCK_UNLOCKED;
	} else if (!elementsLocked[type])
		GenError::CallError(GenWarn("Elements unlocked when already unlocked", GENERR_UNLOCKEDRESOURCE));
	else if (elements[type] != pointer)
		GenError::CallError(GenErr("Invalid unlock pointer", GENERR_INVALIDARG));
}

inline bool GenMesh::IsElementValid(GenMeshElementType type) const {
	switch (type) {
		case GENMET_VERT: return IsStateValid(GENMESH_VERTS);
		case GENMET_EDGE: return IsStateValid(GENMESH_EDGEVERTINDEX);
		case GENMET_FACE: return IsStateValid(GENMESH_FACEVERTINDEX) || IsStateValid(GENMESH_FACEEDGEINDEX) || IsStateValid(GENMESH_FACECLRINDEX) ||
								 IsStateValid(GENMESH_FACEUVINDEX);
		case GENMET_UV: return IsStateValid(GENMESH_UVS);
		case GENMET_COLOUR: return IsStateValid(GENMESH_COLOURS);
	}
	GenError::CallError(GenWarn("Invalid mesh element type provided", GENERR_INVALIDARG));
	return true;
}

inline void GenMesh::ValidateElements(GenMeshElementType type) {
	switch (type) {
		case GENMET_VERT: ValidateState(GENMESH_VERTS); break;
		case GENMET_EDGE: ValidateState(GENMESH_EDGEVERTINDEX); break;
		case GENMET_FACE:
			ValidateState(GENMESH_FACEVERTINDEX);
			ValidateState(GENMESH_FACEEDGEINDEX);
			ValidateState(GENMESH_FACEUVINDEX);
			ValidateState(GENMESH_FACECLRINDEX);
			break;
		case GENMET_UV: ValidateState(GENMESH_UVS); break;
		case GENMET_COLOUR: ValidateState(GENMESH_COLOURS); break;
	}
}

inline void GenMesh::InvalidateElements(GenMeshElementType type) {
	switch (type) {
		case GENMET_VERT: InvalidateState(GENMESH_VERTS); break;
		case GENMET_EDGE: InvalidateState(GENMESH_EDGEVERTINDEX); break;
		case GENMET_FACE:
			InvalidateState(GENMESH_FACEVERTINDEX);
			InvalidateState(GENMESH_FACEEDGEINDEX);
			InvalidateState(GENMESH_FACEUVINDEX);
			InvalidateState(GENMESH_FACECLRINDEX);
			break;
		case GENMET_UV: InvalidateState(GENMESH_UVS); break;
		case GENMET_COLOUR: InvalidateState(GENMESH_COLOURS); break;
	}
}

inline genu32 GenMesh::GetNum(GenMeshElementType type) const {
	return numElements[type];
}