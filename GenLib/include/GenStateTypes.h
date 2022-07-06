// Gen state types
// This file uses the X-macro technique to both register new enums and their names
// Two versions of the X macro are provided, one which lets you mark the start and end of a state type

/* Common abbreviations:
	Model: Model/Mat: Material/Mod: Modifier/Inst: Instance/Vert: Vertex
*/

// ----- Generic/Common states -----
X3(GENCOMMON_MULTIPLE, GENCOMMON_START, GENCOMMON_END) // Multiple states/pack of states

// ----- Generic/Event states -----
X2(GENEVENT_CREATE, GENEVENT_START)
X(GENEVENT_DESTROY)
X2(GENEVENT_COPY, GENEVENT_END) // Element/genid (theoretical, not yet used) // 3

// ----- Scene states -----
// lol ok
X3(GENSCENE_PROPS, GENSCENE_START, GENSCENE_END)

// ----- Model states -----
X2(GENMODEL_MODIDS, GENMODEL_START) // Element/genid: list of linked modifiers by ID
X2(GENMODEL_MESHID, GENMODEL_END) // Element/genid
	
// ----- Mesh states -----
X2(GENMESH_VERTPROP, GENMESH_START)
X(GENMESH_EDGEPROP)
X(GENMESH_FACEPROP)

X(GENMESH_VERTS) // Element/GenVec3
X(GENMESH_COLOURS) // Element/u32
X(GENMESH_UVS) // Element/GenVec2/u16 (tofloat = u16 / 65535.0f, range 0-1)
X(GENMESH_EDGEVERTINDEX) // Element/u32/u16/u8
X(GENMESH_FACEVERTINDEX) // Element/s32/s16/s8
X(GENMESH_FACEEDGEINDEX) // Element/s32/s16/s8
X(GENMESH_FACECLRINDEX) // Element/s32/s16/s8
X(GENMESH_FACEUVINDEX) // Element/s32/s16/s8
X2(GENMESH_FACEPROPINDEX, GENMESH_END) // Element/s32/s16/s8

// ----- Instance states -----
X2(GENINST_NAME, GENINST_START) // Element/wstring
X(GENINST_PARENTID) // Element/genid
X(GENINST_MODELID) // Element/genid
X(GENINST_MATIDS) // Element/genid
X(GENINST_ANIMID) // Element/genid
X(GENINST_MESHID) // Element/genid
X2(GENINST_TRANSFORM, GENINST_END) // Element/GenTransform

// ----- Modifier states -----
X2(GENMOD_TYPE, GENMOD_START) // Element/u32 (GenModType)
X(GENMOD_PROPS) // Element/GenProp
X(GENMOD_MESHID) // Element/genid
X2(GENMOD_MESHDATA, GENMOD_END) // State/GENSTATE_MESH

// ----- Material states -----
X3(GENMAT_SOURCEFILE, GENMAT_START, GENMAT_END)

// ----- Anim states -----
X3(GENANIM_GRAPHDATA, GENANIM_START, GENANIM_END) // State/GENSTATE_GRAPH (genstate_graph not yet defined)

// ----- Anim graph states -----
X2(GENGRAPH_TARGET, GENGRAPH_START) // Element/genid
X(GENGRAPH_TIMERANGE) // Element/u32 (ms) (2 elements: 0=start 1=end)
X(GENGRAPH_KEYFRAMETIMES) // Element/u32 (ms)
X2(GENGRAPH_KEYFRAMEVALUES, GENGRAPH_END) // Element/undefined (various)

// The end!
X(GENSTATE_END)