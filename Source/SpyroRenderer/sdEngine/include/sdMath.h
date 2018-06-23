// Math.cpp/Math.h/MathInline.inl
// Basic math/vector/matrix types and functions
// NOTE: Pointers returned by math functions such as Vec3::Normalise(const Vec3*) are temporary. They'll last a while until they're replaced with another temporary.
// Currently there are 256 temporaries for vectors, which is more than enough for combination operations.
#pragma once
#include "sdTypes.h"
#include <math.h>

#define MAKECOLOR(r, g, b) (0xFF000000 | ((b) << 16) | ((g) << 8) | (r))
#define MAKECOLORA(a, r, g, b) (((a) << 24) | ((b) << 16) | ((g) << 8) | (r))
#define GETCOLORR(clr) ((clr) & 0xFF)
#define GETCOLORG(clr) ((clr) >> 8 & 0xFF)
#define GETCOLORB(clr) ((clr) >> 16 & 0xFF)
#define GETCOLORA(clr) ((clr) >> 24 & 0xFF)

/*
TODO: Figure out how to make it inline on only in Release mode.
Current problem is that including an inline file with non-inlined code will cause multiple definitions!
#ifndef DEBUG
#define MATHINLINE __forceinline
#else
#define MATHINLINE 
#endif*/

#define SSE
#define X86

#ifdef X86
#define GETSINCOS(ss, cc, angle) do {__asm {__asm fld dword ptr[angle] __asm fsincos __asm fstp dword ptr[cc] __asm fstp dword ptr[ss]}} while(0)
#define GETSQRT(dest, src) do {__asm {__asm fld dword ptr[src] __asm fsqrt __asm fstp dword ptr[dest]}} while(0)
#else
#define GETSINCOS(s, c, angle) do {s = sin(angle); c = cos(angle);} while(0)
#define GETSQRT(dest, src) do {dest = sqrt(src);} while(0)
#endif

#ifdef SSE
#include <emmintrin.h>
#endif

#ifndef COPYFLOAT
// COPYFLOAT: Uses pointers to directly transfer floats (avoiding any possible conversion stuff)
#define COPYFLOAT(dest, src) (dest = *(float*) ((uint32*) &src))
#endif

#define PI       3.14159265f
#define DOUBLEPI 6.28318531f
#define HALFPI   1.57079633f
#define RAD  0.01745329f // Multiply by this to go degrees -> radians
#define DEG 57.29577951f // Multiply by this to go radians -> degrees

class Matrix; class Matrix2; // Predeclaration
struct PlaneCrossInfo; // Plane cross info

// Functions
namespace sdMath {
	template <typename VarType>

	// Clamp functions
	// clamp: clamps value between min and max values
	inline VarType& clamp(VarType& value, VarType& min, VarType& max) {
		return ((value < min) ? min : ((value > max) ? max : value));
	}
	inline float clamp(float value, float min, float max) {return clamp<float>(value, min, max);}
	inline int clamp(int value, int min, int max) {return clamp<int>(value, min, max);}
	inline unsigned int clamp(unsigned int value, unsigned int min, unsigned int max) {return clamp<unsigned int>(value, min, max);}
	
	// clampmax: returns value if value <= max, max otherwise
	template <typename VarType>
	inline VarType& clampmax(VarType& value, VarType& max) {
		return ((value > max) ? max : value);
	}
	inline float clampmax(float value, float max) {return clampmax<float>(value, max);}
	inline int clampmax(int value, int max) {return clampmax<int>(value, max);}
	inline unsigned int clampmax(unsigned int value, unsigned int max) {return clampmax<unsigned int>(value, max);}
	
	// clampmin: returns value if value >= min, min otherwise
	template <typename VarType>
	inline VarType& clampmin(VarType& value, VarType& min) {
		return ((value < min) ? min : value);
	}
	inline float clampmin(float value, float min) {return clampmin<float>(value, min);}
	inline int clampmin(int value, int min) {return clampmin<int>(value, min);}
	inline unsigned int clampmin(unsigned int value, unsigned int min) {return clampmin<unsigned int>(value, min);}

	// Wrap functions
	// wrap: circularly wraps value between min and max values
	template <typename VarType>
	inline VarType wrap(VarType value, VarType min, VarType max) {
		if (value < min)
			value += (max - min) * ((int)(-(value - min) / (max - min)) + 1);
		
		return min + value - (VarType)((int)(value / (max - min))) * (max - min);
	}
};

// Vectors and matrices
class Vec2 {
	public:
		inline Vec2() = default;
		inline Vec2(float _x, float _y) : x(_x), y(_y) {};

		inline void Set(float x, float y);

		inline Vec2& operator+(const Vec2& in) const;
		inline Vec2& operator-(const Vec2& in) const;
		inline Vec2& operator*(float in) const;
		inline Vec2& operator/(float in) const;

		inline void operator+=(const Vec2& in);
		inline void operator-=(const Vec2& in);
		inline void operator*=(float in);
		inline void operator/=(float in);
		
		inline void operator*=(const Matrix2& in);

		inline float Length() const;

		inline void Normalise();
		inline void Zero();

		static inline float Dot(const Vec2& vecA, const Vec2& vecB);

		float32 x, y;
};

class Vec3 {
	public:
		inline Vec3() = default;
		inline Vec3(const Vec3& v) : x(v.x), y(v.y), z(v.z) {};
		inline Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};

		inline Vec3& operator+(const Vec3& in) const;
		inline Vec3& operator-(const Vec3& in) const;
		inline Vec3& operator*(const Vec3& in) const;
		inline Vec3& operator/(const Vec3& in) const;
		inline Vec3& operator*(float in) const;
		inline Vec3& operator/(float in) const;

		inline void operator+=(const Vec3& in);
		inline void operator-=(const Vec3& in);
		inline void operator*=(const Vec3& in);
		inline void operator/=(const Vec3& in);
		inline void operator*=(float in);
		inline void operator/=(float in); // note: when precision isn't important, use * (1.0f / divisor) for a generally faster calculation than /.

		inline Vec3& operator*(const Matrix& in) const;
		inline void operator*=(const Matrix& in);

		inline bool operator==(const Vec3& compare) const;
		inline bool operator!=(const Vec3& compare) const;

		// Assign
		inline void Set(float x, float y, float z);
		inline Vec3& Zero();

		// Check
		inline bool IsZero() const;

		// Length and scale
		inline float Length() const;

		inline Vec3& Scale(float factor);
		inline Vec3& ScaleTo(float length);

		inline Vec3& Normalise();
		static inline const Vec3& Normalise(const Vec3& vecIn); // Temporary normalise (static)

		inline Vec3& PerspectiveMultiply(const Matrix* in);

		// Get scalars
		static inline float Dot(const Vec3& vecA, const Vec3& vecB);
		static inline float Distance(const Vec3& vecA, const Vec3& vecB);

		// Cross product
		static inline void Cross(Vec3* vecOut, const Vec3& vecA, const Vec3& vecB);
		static inline const Vec3& Cross(const Vec3& vecA, const Vec3& vecB); // Temporary cross (static)

		// Plane cross test
		static inline bool GetPlaneCrossPoint(Vec3* pointOut, const Vec3& lineStart, const Vec3& lineDirection, const Vec3& planeOrigin, const Vec3& planeNormal);
		static inline bool GetPlaneCrossPointInfinite(Vec3* pointOut, const Vec3& lineStart, const Vec3& lineDirection, const Vec3& planeOrigin, const Vec3& planeNormal);
		static inline bool GetPlaneCrossPointInfiniteAhead(Vec3* pointOut, const Vec3& lineStart, const Vec3& line, const Vec3& planeOrigin, const Vec3& planeNormal);

		static inline float GetPlaneCrossMultiplier(const Vec3& lineStart, const Vec3& lineDirection, const Vec3& planeOrigin, const Vec3& planeNormal);

		// Line closest points
		static void GetLineClosestPoints(Vec3* p1Out, Vec3* p2Out, const Vec3& line1Start, const Vec3& line2Start, const Vec3& line1Vec, const Vec3& line2Vec);
		static void GetLineClosestPointsInfinite(Vec3* p1Out, Vec3* p2Out, const Vec3& line1Start, const Vec3& line2Start, const Vec3& line1Dir, const Vec3& line2Dir);

		// Point distance from line
		static float GetPointDistanceFromLineInfinite(const Vec3& point, const Vec3& lineOrigin, const Vec3& lineDirection);

		// Triangle test
		// GetTri*At**: Finds the interpolated value on an axis based on where that axis crosses with a triangle. Does not verify whether the point is within the tri!
		static inline float GetTriZAtXY(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& pXY); // Finds the Z value at an XY point on a triangle

		// Euler angles
		// When used as Euler angles, Vec3 components are represented in radians and the default order of rotation is X, Y, Z
		void GetEulerAxes(Vec3* xOut, Vec3* yOut, Vec3* zOut) const;
		void SetEulerByAxes(Vec3& xAxisIn, Vec3& yAxisIn, Vec3& zAxisIn); // warning: you must ensure the axes are normalised yourself

		// add rotation around a given angle on a global axis
		void RotateEulerGlobalX(float rotX);
		void RotateEulerGlobalY(float rotY);
		void RotateEulerGlobalZ(float rotZ);

		// VARS/DATA
		union {
			struct {float32 x, y, z;}; // Direct Vec3 vars
			struct {Vec2 xy; float32 __nullptr1;}; // xy split
			struct {float32 __nullptr2; Vec2 yz;}; // yz split
		};
};

class Vec4 {
	public:
		union {
			struct {float32 x, y, z, w;};
			struct {Vec3 xyz; float32 __nullptr1;};
			struct {Vec2 xy; float32 __nullptr2[2];};
		};
};

class Matrix {
	public:
		union {
			struct {
				float32 m11, m12, m13, m14;
				float32 m21, m22, m23, m24;
				float32 m31, m32, m33, m34;
				float32 m41, m42, m43, m44;
			};

			float32 m[4][4];
		};

		inline Matrix& operator*=(const Matrix& in);
		
		inline void Translate(float x, float y, float z);
		
		inline void RotateXYZ(float xRot, float yRot, float zRot);
		inline void RotateX(float xRot);
		inline void RotateY(float yRot);
		inline void RotateZ(float zRot);

		inline void Scale(float x, float y, float z);

		inline void MakeIdentity();
		inline void MakeTranslation(float x, float y, float z);
		inline void MakeTranslation(const Vec3& translation);
		inline void MakeScale(float xScale, float yScale, float zScale);
		inline void MakeAxisAngle(const Vec3& axis, float angle);

		void MakeViewProjRH(const Vec3& position, const Vec3& fwd, const Vec3& up, float fovY, float whRatio, float zNear, float zFar);
		void MakeViewOrthoRH(const Vec3& position, const Vec3& fwd, const Vec3& up, float xUnits, float yUnits, float zNear, float zFar);
		inline void MakePerspectiveRH(float fovY, float whRatio, float zNear, float zFar);
};

class Matrix2 {
	public:
		union {
			struct {
				float32 m11, m12, m13;
				float32 m21, m22, m23;
				float32 m31, m32, m33;
			};

			float32 m[3][3];
		};

		inline Matrix2& operator*=(const Matrix2& in);
		
		inline void MakeTranslation(float x, float y);
		inline void MakeTranslation(const Vec2& translation);
		inline void MakeRotation(float angle, float xOrigin = 0.0f, float yOrigin = 0.0f);
		inline void MakeScale(float xScale, float yScale, float xOrigin = 0.0f, float yOrigin = 0.0f);
};

class Quat4 {
	public:
		float32 x, y, z, w;

		void MakeAxisAngle(const Vec3* axis, float angle);
};

// Geometry
class Line3 {
	public:
		Vec3 p1, p2;

		inline void Move(const Vec3* translation);
};

class BBox3 {
	public:
		Vec3 min, max;

		inline bool CheckPointInside(const Vec3* point);
		inline bool CheckLineIntersects(const Line3* line);
};

inline float PointDistance2(float x1, float y1, float x2, float y2);

inline float RoundUp(float value, float interval); // RoundUp: Rounds 'value' up to the nearest 'interval'
inline double RoundUp(double value, double interval); // RoundUp: Rounds 'value' up to the nearest 'interval'

float RandFloat();

extern Vec3 tempVecs[256]; // Temporary vectors used for multiple operator use on a single line
extern Vec2 tempVecs2[256]; // Temporary vectors used for multiple operator use on a single line
extern uint8 curTempVec;   // Alternative idea: use a raw char data pointer, inc
extern uint8 curTempVec2;   // Alternative idea: use a raw char data pointer, inc

extern Matrix matTemp; // Temporary matrix used by math functions

extern const Vec3 vecZero;
extern const Vec3 vecOne;

extern const Vec3 vecX;
extern const Vec3 vecY;
extern const Vec3 vecZ;

extern const Matrix matIdentity;
extern const Matrix2 matIdentity2;

#ifndef _MATHINLINE
#include "sdMathInline.inl"
#endif