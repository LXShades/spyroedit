#pragma once
#include "Types.h"

enum PowerFlag {
	PWR_ATTRACTION = 0x01,
	PWR_REPULSION = 0x02,
	PWR_GEMATTRACT = 0x04,
	PWR_FORCEFIELD = 0x08,
	PWR_SUPERBASH = 0x10,
	PWR_ULTRABASH = 0x20,
	PWR_HEADBASHPOCALYPSE = 0x40,
	PWR_BUTTERFLYBREATH = 0x80,
	PWR_DEATHSTARE = 0x0100,
	PWR_DEATHFIELD = 0x0200,
	PWR_REPELSTARE = 0x0400,
	PWR_ATTRACTSTARE = 0x0800,
	PWR_EXORCIST = 0x1000,
	PWR_TORNADO = 0x2000,
	PWR_TELEKINESIS = 0x4000,
	PWR_TRAIL = 0x8000,
	PWR_LOOKATSTUFF = 0x00010000,
	PWR_BARRELROLLS = 0x00020000,
	PWR_SANICROLLS  = 0x00040000,
	PWR_GIRAFFE     = 0x00080000,
	PWR_LUCIO       = 0x00100000
};

void UpdatePowers();

extern uint32 powers;
extern float headbash_range;
extern float headbash_depth;
extern float headbash_rimHeight;
extern float headbash_rimWidth;
extern float headbash_crushFactor;
extern float headbash_crushFactorOuter;
