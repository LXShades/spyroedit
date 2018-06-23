#pragma once
#define _MATHINLINE

#include "sdMath.h"

inline void Vec3::Set(float _x, float _y, float _z) {
	x = _x; y = _y; z = _z;
}

inline Vec3& Vec3::Zero() {
#ifdef X86
	*(uint32*)&x = 0;
	*(uint32*)&y = 0;
	*(uint32*)&z = 0;
#else
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
#endif
	return *this;
}

inline bool Vec3::IsZero() const {
	return (x == 0.0f && y == 0.0f && z == 0.0f);
}

inline bool Vec3::operator==(const Vec3& check) const {
	return (x == check.x && y == check.y && z == check.z);
}

inline bool Vec3::operator!=(const Vec3& check) const {
	return (x != check.x || y != check.y || z != check.z);
}

inline Vec3& Vec3::operator+(const Vec3& in) const {
	tempVecs[curTempVec].x = x + in.x;
	tempVecs[curTempVec].y = y + in.y;
	tempVecs[curTempVec].z = z + in.z;
	return tempVecs[curTempVec++];
}

inline Vec3& Vec3::operator-(const Vec3& in) const {
	tempVecs[curTempVec].x = x - in.x;
	tempVecs[curTempVec].y = y - in.y;
	tempVecs[curTempVec].z = z - in.z;
	return tempVecs[curTempVec++];
}

inline Vec3& Vec3::operator*(const Vec3& in) const {
	tempVecs[curTempVec].x = x * in.x;
	tempVecs[curTempVec].y = y * in.y;
	tempVecs[curTempVec].z = z * in.z;
	return tempVecs[curTempVec++];
}

inline Vec3& Vec3::operator/(const Vec3& in) const {
	tempVecs[curTempVec].x = x / in.x;
	tempVecs[curTempVec].y = y / in.y;
	tempVecs[curTempVec].z = z / in.z;
	return tempVecs[curTempVec++];
}

inline Vec3& Vec3::operator*(float in) const {
	tempVecs[curTempVec].x = x * in;
	tempVecs[curTempVec].y = y * in;
	tempVecs[curTempVec].z = z * in;
	return tempVecs[curTempVec++];
}

inline Vec3& Vec3::operator/(float in) const {
	tempVecs[curTempVec].x = x / in;
	tempVecs[curTempVec].y = y / in;
	tempVecs[curTempVec].z = z / in;
	return tempVecs[curTempVec++];
}

inline void Vec3::operator+=(const Vec3& in) {
	x += in.x;
	y += in.y;
	z += in.z;
}

inline void Vec3::operator-=(const Vec3& in) {
	x -= in.x;
	y -= in.y;
	z -= in.z;
}

inline void Vec3::operator*=(const Vec3& in) {
	x *= in.x;
	y *= in.y;
	z *= in.z;
}

inline void Vec3::operator/=(const Vec3& in) {
	x /= in.x;
	y /= in.y;
	z /= in.z;
}

inline void Vec3::operator*=(float in) {
	x *= in;
	y *= in;
	z *= in;
}

inline void Vec3::operator/=(float in) {
	x /= in;
	y /= in;
	z /= in;
}

inline Vec3& Vec3::operator*(const Matrix& in) const {
	float oldX, oldY, oldZ;
	COPYFLOAT(oldX, x); COPYFLOAT(oldY, y); COPYFLOAT(oldZ, z);

	tempVecs[curTempVec].x = oldX * in.m11 + oldY * in.m21 + oldZ * in.m31 + in.m41;
	tempVecs[curTempVec].y = oldX * in.m12 + oldY * in.m22 + oldZ * in.m32 + in.m42;
	tempVecs[curTempVec].z = oldX * in.m13 + oldY * in.m23 + oldZ * in.m33 + in.m43;

	return tempVecs[curTempVec++];
}

inline void Vec3::operator*=(const Matrix& in) {
	float oldX, oldY, oldZ;
	COPYFLOAT(oldX, x); COPYFLOAT(oldY, y); COPYFLOAT(oldZ, z);

	x = oldX * in.m11 + oldY * in.m21 + oldZ * in.m31 + in.m41;
	y = oldX * in.m12 + oldY * in.m22 + oldZ * in.m32 + in.m42;
	z = oldX * in.m13 + oldY * in.m23 + oldZ * in.m33 + in.m43;
}


inline Vec3& Vec3::Normalise() {
	float totalSquared = x * x + y * y + z * z;
	float root;

	GETSQRT(root, totalSquared);

	if (root == 0.0f) {
		// HACKNC
		x = 1.0f;
		return *this;
	}

#ifdef X86
	__asm {
		fld1
		mov eax,dword ptr[this]

		fdiv dword ptr[root]

		fld dword ptr[eax]Vec3.x
		fmul st,st(1)
		fstp dword ptr[eax]Vec3.x
		
		fld dword ptr[eax]Vec3.y
		fmul st,st(1)
		fstp dword ptr[eax]Vec3.y

		fld dword ptr[eax]Vec3.z
		fmulp st(1),st
		fstp dword ptr[eax]Vec3.z
	};
	/*__asm {
		mov eax,dword ptr[this]

		fld dword ptr[root]

		fld dword ptr[eax]Vec3.x
		fdiv st,st(1)
		fstp dword ptr[eax]Vec3.x
		
		fld dword ptr[eax]Vec3.y
		fdiv st,st(1)
		fstp dword ptr[eax]Vec3.y

		fld dword ptr[eax]Vec3.z
		fdiv st,st(1)
		fstp dword ptr[eax]Vec3.z
		fstp st
	};*/
#else
	x /= root;
	y /= root;
	z /= root;
#endif
	return *this;
}

inline Vec3& Vec3::PerspectiveMultiply(const Matrix* in) {
	float oldX, oldY, oldZ;
	COPYFLOAT(oldX, x); COPYFLOAT(oldY, y); COPYFLOAT(oldZ, z);

	x = oldX * in->m11 + oldY * in->m21 + oldZ * in->m31 + in->m41;
	y = oldX * in->m12 + oldY * in->m22 + oldZ * in->m32 + in->m42;
	z = oldX * in->m13 + oldY * in->m23 + oldZ * in->m33 + in->m43;
	
	float w = oldX * in->m14 + oldY * in->m24 + oldZ * in->m34 + in->m44;
	x /= w; y /= w;
	return *this;
}

inline float Vec3::Length() const {
#ifdef X86
	float totalSquared = x * x + y * y + z * z;
	float _length;

	GETSQRT(_length, totalSquared);

	return _length;
#else
	return sqrt(x * x + y * y + z * z);
#endif
}

inline Vec3& Vec3::Scale(float factor) {
	x *= factor;
	y *= factor;
	z *= factor;
	return *this;
}

inline Vec3& Vec3::ScaleTo(float newLength) {
	float totalSquared = x * x + y * y + z * z;
	float root;

	GETSQRT(root, totalSquared);

#ifdef X86
	__asm {
		fld dword ptr[newLength]
		mov eax,dword ptr[this]

		fdiv dword ptr[root]

		fld dword ptr[eax]Vec3.x
		fmul st,st(1)
		fstp dword ptr[eax]Vec3.x
		
		fld dword ptr[eax]Vec3.y
		fmul st,st(1)
		fstp dword ptr[eax]Vec3.y

		fld dword ptr[eax]Vec3.z
		fmulp st(1),st
		fstp dword ptr[eax]Vec3.z
	};
#else
	float mul = scale / root;
	x *= mul;
	y *= mul;
	z *= mul;
#endif
	return *this;
}

inline float Vec3::Dot(const Vec3& vecA, const Vec3& vecB) {
	return vecA.x * vecB.x + vecA.y * vecB.y + vecA.z * vecB.z;
}

inline float Vec3::Distance(const Vec3& vecA, const Vec3& vecB) {
	// TODO: Optimise
	float register xDiff = vecA.x - vecB.x, yDiff = vecA.y - vecB.y, zDiff = vecA.z - vecB.z;
	float sq = xDiff * xDiff + yDiff * yDiff + zDiff * zDiff;

#ifdef X86
	float root;
	GETSQRT(root, sq);

	return root;
#else
	return sqrt(sq);
#endif
}

inline const Vec3& Vec3::Cross(const Vec3& vecA, const Vec3& vecB) {
	Vec3* vecOut = &tempVecs[curTempVec++];
	vecOut->x = vecA.y * vecB.z - vecA.z * vecB.y;
	vecOut->y = vecA.z * vecB.x - vecA.x * vecB.z;
	vecOut->z = vecA.x * vecB.y - vecA.y * vecB.x;

	return *vecOut;
}

inline void Vec3::Cross(Vec3* vecOut, const Vec3& vecA, const Vec3& vecB) {
	vecOut->x = vecA.y * vecB.z - vecA.z * vecB.y;
	vecOut->y = vecA.z * vecB.x - vecA.x * vecB.z;
	vecOut->z = vecA.x * vecB.y - vecA.y * vecB.x;
}

inline const Vec3& Vec3::Normalise(const Vec3& vecIn) {
	float totalSquared = vecIn.x * vecIn.x + vecIn.y * vecIn.y + vecIn.z * vecIn.z;
	float root;
	Vec3* vecOut = &tempVecs[curTempVec++];

	GETSQRT(root, totalSquared);

	if (root == 0.0f) {
		// HACKNC
		vecOut->x = 1.0f; vecOut->y = 0.0f; vecOut->z = 0.0f;
		return *vecOut;
	}

#ifdef X86
	__asm {
		fld1
		mov eax,dword ptr[vecOut]
		mov ebx,dword ptr[vecIn]

		fdiv dword ptr[root]

		fld dword ptr[ebx]Vec3.x
		fmul st,st(1)
		fstp dword ptr[eax]Vec3.x

		fld dword ptr[ebx]Vec3.y
		fmul st,st(1)
		fstp dword ptr[eax]Vec3.y

		fld dword ptr[ebx]Vec3.z
		fmulp st(1),st
		fstp dword ptr[eax]Vec3.z
	};
	return *vecOut;
#else
	vecOut->x /= root;
	vecOut->y /= root;
	vecOut->z /= root;
#endif
}

inline bool Vec3::GetPlaneCrossPoint(Vec3* pointOut, const Vec3& lineStart, const Vec3& line, const Vec3& planeOrigin, const Vec3& planeNormal) {
	float dot2 = GetPlaneCrossMultiplier(lineStart, line, planeOrigin, planeNormal);

	if (dot2 < 0.0f || dot2 >= 1.0f)
		return false;

	pointOut->x = lineStart.x + line.x * dot2;
	pointOut->y = lineStart.y + line.y * dot2;
	pointOut->z = lineStart.z + line.z * dot2;
	return true;
}

inline bool Vec3::GetPlaneCrossPointInfinite(Vec3* pointOut, const Vec3& lineStart, const Vec3& line, const Vec3& planeOrigin, const Vec3& planeNormal) {
	float dot2 = GetPlaneCrossMultiplier(lineStart, line, planeOrigin, planeNormal);

	pointOut->x = lineStart.x + line.x * dot2;
	pointOut->y = lineStart.y + line.y * dot2;
	pointOut->z = lineStart.z + line.z * dot2;
	return true;
}

inline bool Vec3::GetPlaneCrossPointInfiniteAhead(Vec3* pointOut, const Vec3& lineStart, const Vec3& line, const Vec3& planeOrigin, const Vec3& planeNormal) {
	float dot2 = GetPlaneCrossMultiplier(lineStart, line, planeOrigin, planeNormal);

	if (dot2 < 0.0f)
		return false;

	pointOut->x = lineStart.x + line.x * dot2;
	pointOut->y = lineStart.y + line.y * dot2;
	pointOut->z = lineStart.z + line.z * dot2;
	return true;
}

inline float Vec3::GetPlaneCrossMultiplier(const Vec3& lineStart, const Vec3& line, const Vec3& planeOrigin, const Vec3& planeNormal) {
	float dot1 = ((planeOrigin.x - lineStart.x) * planeNormal.x + (planeOrigin.y - lineStart.y) * planeNormal.y + (planeOrigin.z - lineStart.z) * planeNormal.z) / 
		Vec3::Dot(planeNormal, planeNormal);
	Vec3 crossPoint = {planeNormal.x * dot1, planeNormal.y * dot1, planeNormal.z * dot1};
	float dot2 = (crossPoint.x * crossPoint.x + crossPoint.y * crossPoint.y + crossPoint.z * crossPoint.z) / (line.x * crossPoint.x + line.y * crossPoint.y + line.z * crossPoint.z);

	return dot2;
}

inline float Vec3::GetTriZAtXY(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& pXY) {
	Vec3 ortho(-(p2.y - p1.y), p2.x - p1.x, 0.0f);
	float factor = Vec3::Dot(pXY - p1, p2 - p1) / Vec3::Dot(p2 - p1, p2 - p1);
	float factor2 = Vec3::Dot(pXY - p1, ortho) / Vec3::Dot(p3 - p1, ortho);
	float height = p1.z + (p2.z - p1.z) * factor;
	height += (p3.z - height) * factor2;

	return height;
}

inline void Vec2::Set(float x, float y) {
	this->x = x;
	this->y = y;
}

inline Vec2& Vec2::operator+(const Vec2& in) const {
	tempVecs2[curTempVec2].x = x + in.x;
	tempVecs2[curTempVec2].y = y + in.y;
	return tempVecs2[curTempVec2++];
}

inline Vec2& Vec2::operator-(const Vec2& in) const {
	tempVecs2[curTempVec2].x = x - in.x;
	tempVecs2[curTempVec2].y = y - in.y;
	return tempVecs2[curTempVec2++];
}

inline Vec2& Vec2::operator*(float in) const {
	tempVecs2[curTempVec2].x = x * in;
	tempVecs2[curTempVec2].y = y * in;
	return tempVecs2[curTempVec2++];
}

inline Vec2& Vec2::operator/(float in) const {
	tempVecs2[curTempVec2].x = x / in;
	tempVecs2[curTempVec2].y = y / in;
	return tempVecs2[curTempVec2++];
}

inline void Vec2::operator+=(const Vec2& in) {
	x += in.x;
	y += in.y;
}

inline void Vec2::operator-=(const Vec2& in) {
	x -= in.x;
	y -= in.y;
}
inline void Vec2::operator*=(float in) {
	x *= in;
	y *= in;
}

inline void Vec2::operator/=(float in) {
	x /= in;
	y /= in;
}

inline void Vec2::operator*=(const Matrix2& in) {
	float oX = x, oY = y;
	x = oX * in.m11 + oY * in.m21 + in.m31;
	y = oX * in.m12 + oY * in.m22 + in.m32;
}

inline float Vec2::Length() const {
#ifdef X86
	float totalSquared = x * x + y * y;
	float _length;

	GETSQRT(_length, totalSquared);

	return _length;
#else
	return sqrt(x * x + y * y);
#endif
}

inline void Vec2::Normalise() {
	float totalSquared = x * x + y * y;
	float root;

	GETSQRT(root, totalSquared);

	if (root == 0.0f) {
		// HACKNC
		x = 1.0f;
		return;
	}

#ifdef X86
	__asm {
		fld1
		mov eax,dword ptr[this]

		fdiv dword ptr[root]

		fld dword ptr[eax]Vec2.x
		fmul st,st(1)
		fstp dword ptr[eax]Vec2.x
		
		fld dword ptr[eax]Vec2.y
		fmulp st(1),st
		fstp dword ptr[eax]Vec2.y
	};
#else
	x /= root;
	y /= root;
#endif
}

inline void Vec2::Zero() {
	x = 0.0f;
	y = 0.0f;
}

inline float Vec2::Dot(const Vec2& vecA, const Vec2& vecB) {
	return vecA.x * vecB.x + vecA.y * vecB.y;
}

/*
Matrix layout:
11 12 13 14
21 22 23 24
31 32 33 34
41 42 43 44
*/

inline Matrix& Matrix::operator*=(const Matrix& matIn) {
	matTemp = *this; // Todo: faster copy function

	m11 = matTemp.m11 * matIn.m11 + matTemp.m12 * matIn.m21 + matTemp.m13 * matIn.m31 + matTemp.m14 * matIn.m41;
	m21 = matTemp.m21 * matIn.m11 + matTemp.m22 * matIn.m21 + matTemp.m23 * matIn.m31 + matTemp.m24 * matIn.m41;
	m31 = matTemp.m31 * matIn.m11 + matTemp.m32 * matIn.m21 + matTemp.m33 * matIn.m31 + matTemp.m34 * matIn.m41;
	m41 = matTemp.m41 * matIn.m11 + matTemp.m42 * matIn.m21 + matTemp.m43 * matIn.m31 + matTemp.m44 * matIn.m41;

	m12 = matTemp.m11 * matIn.m12 + matTemp.m12 * matIn.m22 + matTemp.m13 * matIn.m32 + matTemp.m14 * matIn.m42;
	m22 = matTemp.m21 * matIn.m12 + matTemp.m22 * matIn.m22 + matTemp.m23 * matIn.m32 + matTemp.m24 * matIn.m42;
	m32 = matTemp.m31 * matIn.m12 + matTemp.m32 * matIn.m22 + matTemp.m33 * matIn.m32 + matTemp.m34 * matIn.m42;
	m42 = matTemp.m41 * matIn.m12 + matTemp.m42 * matIn.m22 + matTemp.m43 * matIn.m32 + matTemp.m44 * matIn.m42;

	m13 = matTemp.m11 * matIn.m13 + matTemp.m12 * matIn.m23 + matTemp.m13 * matIn.m33 + matTemp.m14 * matIn.m43;
	m23 = matTemp.m21 * matIn.m13 + matTemp.m22 * matIn.m23 + matTemp.m23 * matIn.m33 + matTemp.m24 * matIn.m43;
	m33 = matTemp.m31 * matIn.m13 + matTemp.m32 * matIn.m23 + matTemp.m33 * matIn.m33 + matTemp.m34 * matIn.m43;
	m43 = matTemp.m41 * matIn.m13 + matTemp.m42 * matIn.m23 + matTemp.m43 * matIn.m33 + matTemp.m44 * matIn.m43;

	m14 = matTemp.m11 * matIn.m14 + matTemp.m12 * matIn.m24 + matTemp.m13 * matIn.m34 + matTemp.m14 * matIn.m44;
	m24 = matTemp.m21 * matIn.m14 + matTemp.m22 * matIn.m24 + matTemp.m23 * matIn.m34 + matTemp.m24 * matIn.m44;
	m34 = matTemp.m31 * matIn.m14 + matTemp.m32 * matIn.m24 + matTemp.m33 * matIn.m34 + matTemp.m34 * matIn.m44;
	m44 = matTemp.m41 * matIn.m14 + matTemp.m42 * matIn.m24 + matTemp.m43 * matIn.m34 + matTemp.m44 * matIn.m44;

	return *this;
}

inline void Matrix::Translate(float x, float y, float z) {
	m11 += m14 * x;
	m21 += m24 * x;
	m31 += m34 * x;
	m41 += m44 * x;

	m12 += m14 * y;
	m22 += m24 * y;
	m32 += m34 * y;
	m42 += m44 * y;

	m13 += m14 * z;
	m23 += m24 * z;
	m33 += m34 * z;
	m43 += m44 * z;
}

inline void Matrix::RotateX(float rotX) {
	matTemp = *this;

	float s, c;
	GETSINCOS(s, c, rotX);

	m12 = matTemp.m12 * c - matTemp.m13 * s;
	m22 = matTemp.m22 * c - matTemp.m23 * s;
	m32 = matTemp.m32 * c - matTemp.m33 * s;
	m42 = matTemp.m42 * c - matTemp.m43 * s;

	m13 = matTemp.m12 * s + matTemp.m13 * c;
	m23 = matTemp.m22 * s + matTemp.m23 * c;
	m33 = matTemp.m32 * s + matTemp.m33 * c;
	m43 = matTemp.m42 * s + matTemp.m43 * c;
}

inline void Matrix::RotateY(float rotY) {
	matTemp = *this;

	float s, c;
	GETSINCOS(s, c, rotY);

	m11 = matTemp.m11 * c + matTemp.m13 * s;
	m21 = matTemp.m21 * c + matTemp.m23 * s;
	m31 = matTemp.m31 * c + matTemp.m33 * s;
	m41 = matTemp.m41 * c + matTemp.m43 * s;

	m13 = matTemp.m13 * c - matTemp.m11 * s;
	m23 = matTemp.m23 * c - matTemp.m21 * s;
	m33 = matTemp.m33 * c - matTemp.m31 * s;
	m43 = matTemp.m43 * c - matTemp.m41 * s;
}

inline void Matrix::RotateZ(float rotZ) {
	matTemp = *this;

	float s, c;
	GETSINCOS(s, c, rotZ);

	m11 = matTemp.m11 * c - matTemp.m12 * s;
	m21 = matTemp.m21 * c - matTemp.m22 * s;
	m31 = matTemp.m31 * c - matTemp.m32 * s;
	m41 = matTemp.m41 * c - matTemp.m42 * s;

	m12 = matTemp.m11 * s + matTemp.m12 * c;
	m22 = matTemp.m21 * s + matTemp.m22 * c;
	m32 = matTemp.m31 * s + matTemp.m32 * c;
	m42 = matTemp.m41 * s + matTemp.m42 * c;
}

inline void Matrix::RotateXYZ(float rotX, float rotY, float rotZ) {
	Matrix matX = matIdentity, matY = matIdentity, matZ = matIdentity;

	float s, c, sY, cY, sZ, cZ;

	GETSINCOS(s, c, rotX); /// extremely dumb reason this isn't sX and cX: cX conflicts with the ecx register. Yes, really
	GETSINCOS(sY, cY, rotY);
	GETSINCOS(sZ, cZ, rotZ);

	// X rotation
	matTemp = *this;

	m12 = matTemp.m12 * c - matTemp.m13 * s;
	m22 = matTemp.m22 * c - matTemp.m23 * s;
	m32 = matTemp.m32 * c - matTemp.m33 * s;
	m42 = matTemp.m42 * c - matTemp.m43 * s;

	m13 = matTemp.m12 * s + matTemp.m13 * c;
	m23 = matTemp.m22 * s + matTemp.m23 * c;
	m33 = matTemp.m32 * s + matTemp.m33 * c;
	m43 = matTemp.m42 * s + matTemp.m43 * c;

	// Y rotation
	matTemp = *this;

	m11 = matTemp.m11 * cY + matTemp.m13 * sY;
	m21 = matTemp.m21 * cY + matTemp.m23 * sY;
	m31 = matTemp.m31 * cY + matTemp.m33 * sY;
	m41 = matTemp.m41 * cY + matTemp.m43 * sY;

	m13 = matTemp.m13 * cY - matTemp.m11 * sY;
	m23 = matTemp.m23 * cY - matTemp.m21 * sY;
	m33 = matTemp.m33 * cY - matTemp.m31 * sY;
	m43 = matTemp.m43 * cY - matTemp.m41 * sY;

	// Z rotation
	matTemp = *this;

	m11 = matTemp.m11 * cZ - matTemp.m12 * sZ;
	m21 = matTemp.m21 * cZ - matTemp.m22 * sZ;
	m31 = matTemp.m31 * cZ - matTemp.m32 * sZ;
	m41 = matTemp.m41 * cZ - matTemp.m42 * sZ;

	m12 = matTemp.m11 * sZ + matTemp.m12 * cZ;
	m22 = matTemp.m21 * sZ + matTemp.m22 * cZ;
	m32 = matTemp.m31 * sZ + matTemp.m32 * cZ;
	m42 = matTemp.m41 * sZ + matTemp.m42 * cZ;
}

inline void Matrix::Scale(float x, float y, float z) {
	// it's almost TOOO EASY!!!
	m11 *= x;
	m21 *= x;
	m31 *= x;
	m41 *= x;

	m12 *= y;
	m22 *= y;
	m32 *= y;
	m42 *= y;

	m13 *= z;
	m23 *= z;
	m33 *= z;
	m43 *= z;
}

inline void Matrix::MakeTranslation(const Vec3& translation) {
	MakeTranslation(translation.x, translation.y, translation.z);
}

inline void Matrix::MakeIdentity() {
	m11 = 1.0f; m12 = 0.0f; m13 = 0.0f; m14 = 0.0f;
	m21 = 0.0f; m22 = 1.0f; m23 = 0.0f; m24 = 0.0f;
	m31 = 0.0f; m32 = 0.0f; m33 = 1.0f; m34 = 0.0f;
	m41 = 0.0f; m42 = 0.0f; m43 = 0.0f; m44 = 1.0f;
}

inline void Matrix::MakeTranslation(float x, float y, float z) {
	// Memory transfer will probably be slow, so assign identity matrix values manually (my new thought process)
	m11 = 1.0f; m12 = 0.0f; m13 = 0.0f; m14 = 0.0f;
	m21 = 0.0f; m22 = 1.0f; m23 = 0.0f; m24 = 0.0f;
	m31 = 0.0f; m32 = 0.0f; m33 = 1.0f; m34 = 0.0f;
	m41 = x;    m42 = y;    m43 = z;    m44 = 1.0f;
}

inline void Matrix::MakeScale(float xScale, float yScale, float zScale) {
	m11 = xScale; m12 = 0.0f; m13 = 0.0f; m14 = 0.0f;
	m21 = 0.0f; m22 = yScale; m23 = 0.0f; m24 = 0.0f;
	m31 = 0.0f; m32 = 0.0f; m33 = zScale = 0.0f;
	m41 = 0.0f; m42 = 0.0f; m43 = 0.0f; m44 = 1.0f;
}

#define ERRORMARGIN 0.001f
inline void Matrix::MakeAxisAngle(const Vec3& axis, float angle) {
	float axisSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
	float s = (float)sin(angle), c = (float)cos(angle), iC = 1.0f - c;
	float x, y, z;

	if (axisSq > (1.0f + ERRORMARGIN) || axisSq < (1.0f - ERRORMARGIN)) {
		float sq = (float) sqrt(axisSq);
		x = axis.x / sq; y = axis.y / sq; z = axis.z / sq;
	} else {
		x = axis.x; y = axis.y; z = axis.z;
	}

	m11 = iC * x * x + c; m12 = iC * x * y - z * s; m13 = iC * x * z + y * s; m14 = 0.0f;
	m21 = iC * x * y + z * s; m22 = iC * y * y + c; m23 = iC * y * z - x * s; m24 = 0.0f;
	m31 = iC * x * z - y * s; m32 = iC * y * z + x * s; m33 = iC * z * z + c; m34 = 0.0f;
	m41 = 0.0f; m42 = 0.0f; m43 = 0.0f; m44 = 1.0f;
}

inline void Matrix::MakePerspectiveRH(float fovY, float whRatio, float zNear, float zFar) {
	float t = (float) tan(fovY * 0.5f);
	m11 = 1.0f / t / whRatio;			  m12 = 0.0f;	  m13 = 0.0f;						   m14 = 0.0f;
	m21 = 0.0f;							  m22 = 1.0f / t; m23 = 0.0f;						   m24 = 0.0f;
	m31 = 0.0f;							  m32 = 0.0f;	  m33 = zFar / (zNear - zFar);		   m34 = -1.0f;
	m41 = 0.0f;							  m42 = 0.0f;	  m43 = zNear * zFar / (zNear - zFar); m44 = 0.0f;
}

/* Matrix2 layout
m11 m12 m13
m21 m22 m23
m31 m32 m33 */
inline Matrix2& Matrix2::operator*=(const Matrix2& matIn) {
	Matrix2 old;
	old = *this; // Todo: faster copy function

	m11 = old.m11 * matIn.m11 + old.m12 * matIn.m21 + old.m13 * matIn.m31;
	m21 = old.m21 * matIn.m11 + old.m22 * matIn.m21 + old.m23 * matIn.m31;
	m31 = old.m31 * matIn.m11 + old.m32 * matIn.m21 + old.m33 * matIn.m31;

	m12 = old.m11 * matIn.m12 + old.m12 * matIn.m22 + old.m13 * matIn.m32;
	m22 = old.m21 * matIn.m12 + old.m22 * matIn.m22 + old.m23 * matIn.m32;
	m32 = old.m31 * matIn.m12 + old.m32 * matIn.m22 + old.m33 * matIn.m32;

	m13 = old.m11 * matIn.m13 + old.m12 * matIn.m23 + old.m13 * matIn.m33;
	m23 = old.m21 * matIn.m13 + old.m22 * matIn.m23 + old.m23 * matIn.m33;
	m33 = old.m31 * matIn.m13 + old.m32 * matIn.m23 + old.m33 * matIn.m33;

	return *this;
}

inline void Matrix2::MakeTranslation(float x, float y) {
	m11 = 1.0f; m12 = 0.0f; m13 = 0.0f;
	m21 = 0.0f; m22 = 1.0f; m23 = 0.0f;
	m31 = x;    m32 = y;    m33 = 1.0f;
}

inline void Matrix2::MakeTranslation(const Vec2& vec) {
	m11 = 1.0f;  m12 = 0.0f;  m13 = 0.0f;
	m21 = 0.0f;  m22 = 1.0f;  m23 = 0.0f;
	m31 = vec.x; m32 = vec.y; m33 = 1.0f;
}

inline void Matrix2::MakeRotation(float angle, float xOrigin, float yOrigin) {
	// First matrix
	float s = (float) sin(angle), c = (float) cos(angle);

	m11 = c;
	m21 = s;
	m31 = -xOrigin * c + -yOrigin * s + xOrigin;

	m12 = -s;
	m22 = c;
	m32 = -xOrigin * -s + -yOrigin * c + yOrigin;
	
	m13 = 0.0f;
	m23 = 0.0f;
	m33 = 1.0f;
}

inline void Matrix2::MakeScale(float xScale, float yScale, float xOrigin, float yOrigin) {
	m11 = xScale;
	m21 = 0.0f;
	m31 = -xOrigin * xScale + xOrigin;

	m12 = 0.0f;
	m22 = yScale;
	m32 = -yOrigin * yScale + yOrigin;

	m13 = 0.0f;
	m23 = 0.0f;
	m33 = 1.0f;
}

// ---------- QUATERNION FUNCTIONS ----------
inline void Quat4::MakeAxisAngle(const Vec3* axis, float angle) {
	float s, c;
#ifdef X86
	__asm {
		fld dword ptr[angle]

		fsincos

		fstp dword ptr[s]
		fstp dword ptr[c]
	};
#else
	s = sin(angle * 0.5f);
#endif
	x = axis->x * s;
	y = axis->y * s;
	z = axis->z * s;
#ifdef X86
	w = c;
#else
	w = cos(angle * 0.5f);
#endif
}

// ---------- LINE FUNCTIONS ----------
inline void Line3::Move(const Vec3* translation) {
	p1.x += translation->x;
	p1.y += translation->y;
	p1.z += translation->z;
	p2.x += translation->x;
	p2.y += translation->y;
	p2.z += translation->z;
}

inline float PointDistance2(float x1, float y1, float x2, float y2) {
	return (float) sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

inline float RoundUp(float value, float interval) {
	float fDiv = value / interval;
	float iDiv = (float) ((int) fDiv);
	float remainder = fDiv - iDiv;

	float result = iDiv * interval;

	if (remainder >= 0.5f)
		return result + interval;
	else
		return result;
}

inline double RoundUp(double value, double interval) {
	double fDiv = value / interval;
	double iDiv = (double) ((int) fDiv);
	double remainder = fDiv - iDiv;

	double result = iDiv * interval;

	if (remainder >= 0.5)
		return result + interval;
	else
		return result;
}