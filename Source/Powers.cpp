#include "SpyroData.h"
#include "Online.h"
#include "Powers.h"

#include <cmath>
#include <Windows.h>

void MakeCrater(int craterX, int craterY, int craterZ);

uint32 powers = 0;

void UpdatePowers() {
	if (!mobys || !spyro)
		return;

	static int shockwaveDist = 0;
	static int headbashingPlayer = -1;
	static int tkObject = -1;
	static int tkLockDist = 0;
	uint32* uintmem = (uint32*) memory;

	for (int j = 0; j < numPlayers; j++) {
		Spyro* curSpyro;
		uint32 curPowers;
			
		if (j == localPlayerId) { // Local player
			curSpyro = spyro;
			curPowers = powers;
		} else { // Another player
			curSpyro = &players[j].spyro;
			curPowers = players[j].powers;
		}

		if (!curPowers)
			continue; // No powers to use!

		if ((curPowers & (PWR_SUPERBASH|PWR_ULTRABASH|PWR_HEADBASHPOCALYPSE))) {
			if (headbashingPlayer != -1) {
				if (shockwaveDist > 15000)
					headbashingPlayer = -1; // Finished!

				shockwaveDist += 1000;
			}

			int numHeadbashFrames = 1;
			if (mobyModels && mobyModels[0].address && mobyModels[0]->anims[46].address)
				numHeadbashFrames = mobyModels[0]->anims[46]->numFrames;

			if (curSpyro->anim.nextAnim == 46 && 
				curSpyro->anim.nextFrame >= 12 * numHeadbashFrames / 22 && curSpyro->anim.nextFrame <= 14 * numHeadbashFrames / 22 && headbashingPlayer == -1) {
				headbashingPlayer = j;
				shockwaveDist = 1000;
				
				if (curPowers & PWR_HEADBASHPOCALYPSE)
					MakeCrater(curSpyro->x, curSpyro->y, curSpyro->z);
			}
		}

		// Iterate all game objects (mobys)
		for (int i = 0; i < 500; i++) {
			if (mobys[i].state == -1)
				break;

			if (mobys[i].state < 0)
				continue;

#define RECALCDIST dist = Distance(mobys[i].x, mobys[i].y, mobys[i].z, curSpyro->x, curSpyro->y, curSpyro->z)
			bool killable = (mobys[i].type != 206); // Question mark jars can freeze Spyro if destroyed from a distance
			int dist = Distance(mobys[i].x, mobys[i].y, mobys[i].z, curSpyro->x, curSpyro->y, curSpyro->z);
			float angle = atan2((float) (mobys[i].y - curSpyro->y), (float) (mobys[i].x - curSpyro->x));
			uint8 ang1 = (uint8) ToAngle(angle);
			uint8 ang2 = (uint8) (curSpyro->angle.z + curSpyro->headAngle.z) & 0xFF;
			int16 angDifference = 0;
			if (ang1 > ang2)      angDifference = ang1 - ang2;
			else if (ang2 > ang1) angDifference = ang2 - ang1;
			if (angDifference > 128)
				angDifference = 256 - angDifference;

			if ((curPowers & PWR_ATTRACTION) && dist < 10000 && dist > 1000) {
				float attractAmt = dist < 400 ? (float) dist : 400.0f;

				if (dist - attractAmt < 1000)
					attractAmt = dist - 1000;

				float dir = atan2((float) -(mobys[i].y - curSpyro->y), (float) -(mobys[i].x - curSpyro->x));
				mobys[i].x += cos(dir) * attractAmt;
				mobys[i].y += sin(dir) * attractAmt;
				mobys[i].z -= sin(atan2((float) (mobys[i].z - curSpyro->z), dist)) * attractAmt;

				RECALCDIST;
			}

			if ((curPowers & PWR_REPELSTARE) && dist < 10000 && angDifference < 64) {
				float repelForce = (8000.0f - (float) dist) / 10.0f * (1.0f - angDifference / 64.0f);

				mobys[i].x += cos(angle) * repelForce;
				mobys[i].y += sin(angle) * repelForce;
					
				RECALCDIST;
			}

			if ((curPowers & PWR_ATTRACTSTARE) && dist < 10000 && angDifference < 5/* && mobys[i].type == 1*/) { // GEM HACK
				float attractForce = dist < 400 ? (float) dist : 400.0f;

				if (dist - attractForce < 600)
					attractForce = dist - 600;

				mobys[i].x -= cos(angle) * attractForce;
				mobys[i].y -= sin(angle) * attractForce;
				mobys[i].z -= sin(atan2((float) (mobys[i].z - curSpyro->z), dist)) * attractForce;
					
				RECALCDIST;
			}
				
			if ((curPowers & PWR_GEMATTRACT) && mobys[i].type == 1 && dist < 100000) {
				float attractAmt = (100000.0f - (float) dist) / 10.0f;
				//float attractAmt = dist < 400 ? (float) dist : 400.0f;

				if (attractAmt > 400.0f) attractAmt = 400.0f;

				if (attractAmt > dist)
					attractAmt = dist;

				float dir = atan2((float) -(mobys[i].y - curSpyro->y), (float) -(mobys[i].x - curSpyro->x));
				mobys[i].x += cos(dir) * attractAmt;
				mobys[i].y += sin(dir) * attractAmt;
				mobys[i].z -= sin(atan2((float) (mobys[i].z - curSpyro->z), dist)) * attractAmt;
					
				RECALCDIST;
			}

			if (((curPowers & PWR_SUPERBASH) || (curPowers & PWR_ULTRABASH)) && headbashingPlayer == j && killable) {
				if (dist < shockwaveDist || (curPowers & PWR_ULTRABASH))
					mobys[i].attackFlags = 0x00950000;
			}

			if ((curPowers & PWR_BUTTERFLYBREATH) && mobys[i].attackFlags == 0x00010000 && mobys[i].type != 16 && 
				((!(uintmem[0x0006F9F8 / 4] < 300 && mobys[i].type == 527)) || game != SPYRO3)) { // SHEILA HACK
				// Butterfly ID: 16
				Animation null = {0, 0, 0, 0};
				uint32* uintmem = (uint32*) memory;

				mobys[i].extraData = 0x80000000;
				mobys[i].collisionData = 0;
				mobys[i].anim = null;
				mobys[i].anim.nextFrame = 1;
				mobys[i].type = 16;
				mobys[i].state = 0;
				//mobys[i]._unknown[0] = 0;
				//mobys[i]._unknown[1] = 0;
				mobys[i].animSpeed = 0x30;
				mobys[i].animRun = 0xFF;

				uintmem[0x00000000/4] = mobys[i].x;
				uintmem[0x00000004/4] = mobys[i].y;
				uintmem[0x00000008/4] = mobys[i].z + 10000;
				uintmem[0x0000000c/4] = 0x05dc0001;
				uintmem[0x00000010/4] = 0x1cc80000;
				uintmem[0x00000014/4] = 0x00000000;
			}

			// DEATH STARE, deserves more code; doesn't need it though!
			if ((curPowers & PWR_DEATHSTARE) && dist < 10000 && killable && angDifference <= 5)
				mobys[i].attackFlags = 0x00950000; // SuperFlame: 00950000 Frozen Altars Laser: 10000000

			if ((curPowers & PWR_DEATHFIELD) && dist < 10000 && killable)
				mobys[i].attackFlags = 0x00950000;

			if ((curPowers & PWR_REPULSION) && mobys[i].type != 120 && dist <= 8000) { // Don't repel Sparx
				float repelForce = (8000.0f - (float) dist) / 10.0f;

				mobys[i].x += cos(angle) * repelForce;
				mobys[i].y += sin(angle) * repelForce;
					
				RECALCDIST;
			}

			if ((curPowers & PWR_FORCEFIELD) && mobys[i].type != 120 && dist < 2250) {
				mobys[i].x += cos(angle) * (2250 - dist);
				mobys[i].y += sin(angle) * (2250 - dist);
					
				RECALCDIST;
			}

			if ((curPowers & PWR_TORNADO) && dist < 10000 && mobys[i].type != 120) {
				mobys[i].x += cos(angle - 3.141592f / 2.0f) * 600;
				mobys[i].y += sin(angle - 3.141592f / 2.0f) * 600;

				// Don't throw them out of orbit, now!
				int newDist = Distance(curSpyro->x, curSpyro->y, curSpyro->z, mobys[i].x, mobys[i].y, mobys[i].z);
				mobys[i].x = (mobys[i].x - curSpyro->x) * dist / newDist + curSpyro->x;
				mobys[i].y = (mobys[i].y - curSpyro->y) * dist / newDist + curSpyro->y;
					
				//RECALCDIST; No need; we're locking the distance
			}

			if ((curPowers & PWR_TELEKINESIS) && dist < 15000 && angDifference <= 5 && uintmem[0x00071340/4] == 7 && // Note: Only currently compatible with PAL (camera mode code)
				mobys[i].type != 1002 && game == SPYRO3 && gameRegion == PAL) {

				if (tkObject == -1) {
					tkObject = i;
					tkLockDist = dist;
				} else {
					if (dist < Distance(curSpyro->x, curSpyro->y, curSpyro->z, mobys[tkObject].x, mobys[tkObject].y, mobys[tkObject].z)) {
						tkObject = i;
						tkLockDist = dist;
					}
				}
			}
		} // for loop
	
		if (tkObject != -1) {
			if (((curPowers & PWR_TELEKINESIS) && uintmem[0x00071340/4] != 7) || mobys[tkObject].state < 0) // Note: Only currently compatible with PAL (camera mode code)
				tkObject = -1; // Reset TK object
			else {
				// Move object
				int dist = Distance(mobys[tkObject].x, mobys[tkObject].y, mobys[tkObject].z, curSpyro->x, curSpyro->y, curSpyro->z);
				float angleRad = ToRad((uint8) curSpyro->headAngle.z + (uint8) curSpyro->angle.z);
				float verAngleRad = ToRad((uint8) curSpyro->headAngle.y + (uint8) curSpyro->angle.y);
				mobys[tkObject].x = curSpyro->x + cos(angleRad) * tkLockDist * cos(verAngleRad);
				mobys[tkObject].y = curSpyro->y + sin(angleRad) * tkLockDist * cos(verAngleRad);
				mobys[tkObject].z = curSpyro->z + tkLockDist * sin(verAngleRad);
			}
		}
	}

	// Single-player powers
	int mobyCount = 0;
	static int barrelRollTime = 0, barrelRollDirection = 0;
	static int backflipTime = 0;
	static int upsideDownTime = 0;
	bool sonicSpinningLastFrame = false;

	if ((powers & PWR_LOOKATSTUFF) && game == SPYRO3 && gameRegion == PAL) {
		if (mobys && spyro) {
			int closestMoby = -1;
			int closestMobyDist = 5000;
			for (int i = 0; ; i++) {
				if (mobys[i].state == -1)
					break;
				mobyCount++;

				if (mobys[i].state >= 0 && mobys[i].type != 120 && mobys[i].type < 1000) {
					int dist = Distance(spyro->x, spyro->y, spyro->z, mobys[i].x, mobys[i].y, mobys[i].z);
					int head = (int) ((atan2(mobys[i].y - spyro->y, mobys[i].x - spyro->x)) * 8192.0f / 3.14f) - (spyroExt->zAngle * 0x4000 / 0x1000);

					while (head >= 8192) head -= 16384;
					while (head <= -8192) head += 16384;

					if (head > -5000 && head < 5000 && dist < closestMobyDist) {
						closestMoby = i;
						closestMobyDist = dist;
					}
				}
			}

			if (closestMoby != -1) {
				SpyroExtended3* spyroExt = (SpyroExtended3*) spyro;
				spyroExt->headZAngle = (int) ((atan2(mobys[closestMoby].y - spyro->y, mobys[closestMoby].x - spyro->x)) * 8192.0f / 3.14f) - 
					(spyroExt->zAngle * 0x4000 / 0x1000);
				spyroExt->headYAngle = (int) ((atan2(mobys[closestMoby].z - spyro->z + 500, Distance(mobys[closestMoby].x, mobys[closestMoby].y, 0, spyro->x, spyro->y, 0)) * 
					8192.0f / 3.14f));

				while (spyroExt->headZAngle >= 8192) spyroExt->headZAngle -= 16384;
				while (spyroExt->headZAngle <= -8192) spyroExt->headZAngle += 16384;
			}
		}
	}

	if (powers & PWR_BARRELROLLS) {
		if ((joker & BUT_X) && (joker & (BUT_LEFT | BUT_RIGHT)) && !barrelRollTime) {
			if (joker & BUT_RIGHT) barrelRollDirection = 0;
			if (joker & BUT_LEFT) barrelRollDirection = 1;

			barrelRollTime = 1;
		}

		if (spyro->anim.nextAnim != 0x11 && spyro->anim.nextAnim != 0x21)
			barrelRollTime = 0;

		if (barrelRollTime) {
			if (barrelRollDirection == 0) spyroExt->xAngle = (barrelRollTime * 0x1000 / 40) & 0x0FFF;
			else if (barrelRollDirection == 1) spyroExt->xAngle = -(barrelRollTime * 0x1000 / 40) & 0x0FFF;
			spyro->z += (int) (sin((float) barrelRollTime / 40.0f * 6.28f) * 75.0f);
			spyro->angle.x = spyroExt->xAngle * 255 / 0x1000;

			barrelRollTime++;
			if (barrelRollTime >= 40)
				barrelRollTime = 0;
		}
	}

	if (powers & PWR_SANICROLLS) {
		if (spyro->anim.nextAnim == 0x16 || spyro->anim.nextAnim == 0x06) {
			spyroExt->yAngle -= 400;
			sonicSpinningLastFrame = true;
		} else if (sonicSpinningLastFrame) {
			sonicSpinningLastFrame = false;
			spyroExt->yAngle = 0;
		}
	}

	if ((powers & PWR_GIRAFFE) && gameState == GAMESTATE_INLEVEL && mobyModels && mobyModels[0].address) {
		SpyroModelHeader* spyroModel = (SpyroModelHeader*) mobyModels[0];

		if (spyroModel->anims[spyro->anim.nextAnim].address) {
			for (int i = 0; i < spyroModel->anims[spyro->anim.nextAnim]->numFrames; i++) {
				SpyroFrameInfo* frame = &spyroModel->anims[spyro->anim.nextAnim]->frames[i];

				frame->headPos = (frame->headPos & ~0x00000FFF) | 0x480;
			}
		}
	}

	if ((powers & PWR_LUCIO) && gameState == GAMESTATE_INLEVEL && mobyModels && mobyModels[0].address) {
		SpyroModelHeader* spyroModel = (SpyroModelHeader*) mobyModels[0];
		static float boop = 0.0f;
		static DWORD startTime = GetTickCount();

		boop = ((GetTickCount() - startTime) % 1000) / 1000.0f * 6.28f;

		if (spyroModel->anims[spyro->anim.nextAnim].address) {
			for (int i = 0; i < spyroModel->anims[spyro->anim.nextAnim]->numFrames; i++) {
				SpyroFrameInfo* frame = &spyroModel->anims[spyro->anim.nextAnim]->frames[i];

				frame->headPos = bitsout((int) (sin(boop) * 100.0f) + 120, 21, 11) | 
								 bitsout((int) (cos(boop) * 100.0f), 10, 11) | 
								 bitsout((int) (-(1.0f + cos(boop*2.0f)) * 25.0f) + 8, 0, 10);
			}
		}
	}
}