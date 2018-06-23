#include "sdTypes.h"

enum DbgObjType {
	DBGOBJ_POINT = 0,
	DBGOBJ_LINE = 1,
	DBGOBJ_CIRCLE = 2, 
};

struct DebugObject {
	int8 type;
	bool8 is3D;
	Vec3 p1, p2;
	color32 colour;
	float32 radius;
	bool8 drawInfo;
};

struct DebugString {
	bool8 active;
	char string[256];
	int32 x, y;
};

enum ShaderMode {
	SMODE_DEFAULT = 0,
	SMODE_USER = 1,
};