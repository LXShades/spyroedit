#pragma once

// Colour helpers
#define MAKECOLOR16(r, g, b) ((((r) * 32 / 256) & 31) | (((g) * 32 / 256 & 31) << 5) | (((b) * 32 / 256 & 31) << 10) | 0x8000)
#define MAKECOLOR32(r, g, b) ((((r) & 0xFF)) | (((g) & 0xFF) << 8) | (((b) & 0xFF) << 16))
#define GETR16(clr) (((clr) & 31) * 264 / 32)
#define GETG16(clr) (((clr) >> 5 & 31) * 264 / 32)
#define GETB16(clr) (((clr) >> 10 & 31) * 264 / 32)
#define GETR32(clr) ((clr) & 0xFF)
#define GETG32(clr) (((clr) >> 8) & 0xFF)
#define GETB32(clr) (((clr) >> 16) & 0xFF)

// Bit helpers
// get bits unsigned
#define bitsu(num, bitstart, numbits) ((num) >> (bitstart) & ~(0xFFFFFFFF << (numbits)))
// get bits signed
#define bitss(num, bitstart, numbits) (int) ((((num) >> (bitstart) & ~(0xFFFFFFFF << (numbits))) | (0 - (((num) >> ((bitstart)+(numbits)-1) & 1)) << (numbits))))
// build bits (use |)
#define bitsout(num, bitstart, numbits) (((num) & ~(0xFFFFFFFF << (numbits))) << (bitstart))

// Typedefs
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long int uint64;

typedef uint8 bool8;

#ifndef _M_X64
typedef unsigned int uintptr;
#else
typedef unsigned long long int uintptr;
#endif