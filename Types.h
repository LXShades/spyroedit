#pragma once

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