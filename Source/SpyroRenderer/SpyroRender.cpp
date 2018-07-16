#ifdef SPYRORENDER
#include <windows.h>
#include <cmath>
#include <mutex>

#include "SpyroRender.h"
#include "../SpyroData.h"
#include "../SpyroScene.h"
#include "../SpyroTextures.h"
#include "../Vram.h"

#include "sdWindow.h"
#include "sdRender.h"
#include "sdSystem.h"

#define MOBYPOSSCALE 0.000625f

namespace SpyroRender {
struct DrawFace {
	uint32 verts[4];
	uint32 colours[4];
	uint32 numSides;
	float32 u[4], v[4];
};

struct Model {
	Model() : numFaces(0), numVerts(0), numColours(0) {};

	DrawFace faces[700];
	uint32 numFaces;

	Vec3 verts[256];
	uint32 numVerts;

	uint32 colours[256];
	uint32 numColours;
};

struct LerpMoby {
	Model model;
	Vec3 position;
	Vec3 rotation;
	bool8 visible;
};

struct LerpSpyro {
	Model model;
	Vec3 position, rotation;
};

struct LerpCamera {
	Vec3 position;
	float32 hAngle;
	float32 vAngle;
};

struct LerpEffect {
	SpyroEffect effect;
};

// note: scene doesn't actually lerp...
struct LerpScene {
	Model sectors[256];
	uint32 numSectors;
};

// note: sky doesn't actually lerp, just keeping things consistent here...
struct LerpSky {
	Model sectors[256];
};

struct GameState {
	static const uint16 maxNumEffects = 100;
	static const uint16 maxNumMobys = 500;

	LerpMoby mobys[maxNumMobys];
	uint16 numMobys;

	LerpSpyro spyro;

	LerpCamera camera;

	LerpEffect effects[maxNumEffects];
	uint16 numEffects;

	uint32 time; // time (in ms) that this gamestate occurred
};

LerpScene lerpScene;
LerpSky skyModel;

const int numEffectsModels = 3;
Model effectsModels[numEffectsModels];

GameState* gameStates;
GameState* curGameState;
GameState* nextGameState;
GameState* writeGameState = 0;
const int numGameStates = 10;
float gameStateFactor; // blend factor between current and last game state
const int gameStateDelay = 90; // visible gamestate delay in ms. numGameStates must be large enough to support the number of gamestates required to buffer this much time.

const uint32 mobyVertAdjustTable[127] = {
	0xFE9FD3FA, 0xFF5FD3FA, 0x001FD3FA, 0x00DFD3FA, 0x019FD3FA, 0xFE9FEBFA, 0xFF5FEBFA, 0x001FEBFA, 0x00DFEBFA, 0x019FEBFA, 0xFE8003FA, 0xFF4003FA, 0x000003FA, 0x00C003FA,
	0x018003FA, 0xFE801BFA, 0xFF401BFA, 0x00001BFA, 0x00C01BFA, 0x01801BFA, 0xFE8033FA, 0xFF4033FA, 0x000033FA, 0x00C033FA, 0x018033FA, 0xFE9FD3FD, 0xFF5FD3FD, 0x001FD3FD,
	0x00DFD3FD, 0x019FD3FD, 0xFE9FEBFD, 0xFF5FEBFD, 0x001FEBFD, 0x00DFEBFD, 0x019FEBFD, 0xFE8003FD, 0xFF4003FD, 0x000003FD, 0x00C003FD, 0x018003FD, 0xFE801BFD, 0xFF401BFD,
	0x00001BFD, 0x00C01BFD, 0x01801BFD, 0xFE8033FD, 0xFF4033FD, 0x000033FD, 0x00C033FD, 0x018033FD, 0xFE9FD000, 0xFF5FD000, 0x001FD000, 0x00DFD000, 0x019FD000, 0xFE9FE800,
	0xFF5FE800, 0x001FE800, 0x00DFE800, 0x019FE800, 0xFE800000, 0xFF400000, 0x00000000, 0x00C00000, 0x01800000, 0xFE801800, 0xFF401800, 0x00001800, 0x00C01800, 0x01801800,
	0xFE803000, 0xFF403000, 0x00003000, 0x00C03000, 0x01803000, 0xFE9FD003, 0xFF5FD003, 0x001FD003, 0x00DFD003, 0x019FD003, 0xFE9FE803, 0xFF5FE803, 0x001FE803, 0x00DFE803,
	0x019FE803, 0xFE800003, 0xFF400003, 0x00000003, 0x00C00003, 0x01800003, 0xFE801803, 0xFF401803, 0x00001803, 0x00C01803, 0x01801803, 0xFE803003, 0xFF403003, 0x00003003,
	0x00C03003, 0x01803003, 0xFE9FD006, 0xFF5FD006, 0x001FD006, 0x00DFD006, 0x019FD006, 0xFE9FE806, 0xFF5FE806, 0x001FE806, 0x00DFE806, 0x019FE806, 0xFE800006, 0xFF400006,
	0x00000006, 0x00C00006, 0x01800006, 0xFE801806, 0xFF401806, 0x00001806, 0x00C01806, 0x01801806, 0xFE803006, 0xFF403006, 0x00003006, 0x00C03006, 0x01803006, 0x7E7F7F80,
	0x7C7C7D7D // Note: the last two values may be invalid
};

const uint32 gemColours[] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000040, 0x002000FF, 0x00504040, 0x00A08080, 
	0x00005066, 0x00005066, 0x00005066, 0x00005066, 
	0x00480020, 0x00FF0080, 0x0040C000, 0x0080FF00, 
	0x00005066, 0x0000C0FF, 0x00004050, 0x0000A0C0, 
	0x00240034, 0x008000C0, 0x00004050, 0x0000A0C0, 
	0x00003848, 0x002060FF, 0x00004050, 0x0000A0C0, 
	0x00E0E0E0, 0x00101010
};

Window wndMain;
Renderer draw;
Viewport* vpMain;

uint32 wndMainW = 512, wndMainH = 512;
uint32 rndrW = 512, rndrH = 512;

Texture* sceneTex;
Texture* whiteTex;
Texture* objTex;

VertBuffer* modelBuffers[64];
int curModelBuffer = 0;

HANDLE renderThread;
DWORD WINAPI RerenderThread(LPVOID null);

bool updatingData = false;
bool updatingRender = false;

bool isRenderOpen = false;

void UpdateScene(); void UpdateMobys(); void UpdateSpyro(); void UpdateSky(); void UpdateEffects();
void DrawScene(); void DrawMobys(); void DrawSpyro(); void DrawSky(); void DrawEffects();
void DrawModel(const Model* model, const Matrix* transform);
void OnLevelEntry();
inline float LerpAngle(float val1, float val2, float factor);
void Alert(const char* string);
void Close();

void ConvertAnimatedVertices(Model* modelOut, uint32* verts, uint16* blocks, uint8* adjusts, uint32 startOutputVertexIndex, uint32 numOutputVertices, bool doStartOnBlock, float scaleFactor, bool isSpyro);
void ConvertFaces(Model* modelOut, uint32* faces, int uniqueColourIndex, bool isAnimated, bool isSpyro);

bool OnWndMainEvent(CtrlEventParams* params) {
	if (params->wndMsg == WM_SIZE) {
		wndMainW = LOWORD(params->wndLparam);
		wndMainH = HIWORD(params->wndLparam);
	}

	if (params->tag == EVT_CLOSE) {
		Close();
		return false;
	}

	return true;
}

void Open() {
	if (isRenderOpen) {
		return;
	}

	wndMain.Create(L"Spyro Rerendered", wndMainW, wndMainH);
	wndMain.SetEventHandler(OnWndMainEvent);
	
	if (!draw.Init()) {
		Alert("Draw init failed");
		const char* err = Sys::GetLastError();
		Alert(err);
	}

	if (!(vpMain = new Viewport(&draw, &wndMain, 0, 0, wndMainW, wndMainH)))
		Alert("Viewport init failed");

	if (!(sceneTex = new Texture(&draw, 1024, 1024, nullptr)))
		Alert("Scene texture init failed");
	if (!(objTex = new Texture(&draw, 512, 512, nullptr)))
		Alert("Object texture init failed");

	uint32 white[8] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
	whiteTex = new Texture(&draw, 2, 2, white);

	for (int i = 0; i < 64; i++) {
		if (!(modelBuffers[i] = new VertBuffer(&draw, NULL, 700 * 2 * 3, sizeof (CTVertex), VBF_WRITABLE)))
			Alert("Model buffer init failed");
	}

	if (!gameStates) {
		gameStates = (GameState*) malloc(sizeof (GameState) * numGameStates);
		memset(gameStates, 0, sizeof (GameState) * numGameStates);
		// Note: gameStates should be freed on Close, but for some reason if that is done the game can't realloc it
	}

	draw.SetActiveViewport(vpMain);

	isRenderOpen = true;
	if (!(renderThread = CreateThread(NULL, 0, RerenderThread, NULL, 0, NULL)))
		Alert("Render thread init failed");

	OnLevelEntry();
}

std::mutex gameIsUpdatingMutex;

void Close() {
	if (!isRenderOpen) {
		return;
	}

	isRenderOpen = false;
	gameIsUpdatingMutex.lock(); // Wait for the render thread to shutdown
	gameIsUpdatingMutex.unlock();

	wndMain.Destroy();
	draw.Shutdown();
}

void OnLevelEntry() {
	if (!isRenderOpen) {
		return;
	}

	UpdateTextures();
	UpdateSky();
}

void OnUpdateLace() {
	if (!isRenderOpen) {
		return;
	}

	static int interval = 0;
	interval++;
	
	if (interval & 1)
		return;
	if (!memory)
		return;

	while (!gameIsUpdatingMutex.try_lock());
	
	// Windows stuff
	MSG message;
	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	wndMain.Update();
	ShowCursor(true);
	
	// Update renderer size
	if (rndrW != wndMainW || rndrH != wndMainH) {
		vpMain->Resize(wndMainW, wndMainH);
		vpMain->ResizeView(0, 0, wndMainW, wndMainH);
		draw.SetActiveViewport(vpMain);

		rndrW = wndMainW; rndrH = wndMainH;
	}

	// Build a snapshot of the current game state
	if (!writeGameState)
		writeGameState = gameStates;
	else {
		writeGameState++;

		if (writeGameState >= &gameStates[numGameStates])
			writeGameState = gameStates;
	}
	
	writeGameState->time = Sys::GetTime();

	// Update visuals for this gamestate
	if (scene.spyroScene)
		UpdateScene();

	if (mobys && mobyModels)
		UpdateMobys();

	if (spyro && mobyModels)
		UpdateSpyro();

	if (spyroSky)
		UpdateSky();

	if(spyroEffects)
		UpdateEffects();

	//if ((interval & 63) == 0)
	//	UpdateTextures();

	if (spyroCamera) {
		float camAng = bitss(spyroCamera->hAngle1, 0, 12) * PI / (float)0x800;
		float camVAng = bitss(spyroCamera->vAngle1, 0, 12) * PI / (float)0x800;
		writeGameState->camera.position = Vec3(spyroCamera->x1 * MOBYPOSSCALE, spyroCamera->y1 * MOBYPOSSCALE, spyroCamera->z1 * MOBYPOSSCALE);
		writeGameState->camera.hAngle = camAng + 90.0f * RAD;
		writeGameState->camera.vAngle = -camVAng;
	}

	gameIsUpdatingMutex.unlock();
}

void OnWinOpen() {
	wndMain.SetHidden(false);
}

void OnWinClose() {
	wndMain.SetHidden(true);
}

DWORD WINAPI RerenderThread(LPVOID null) {
	while (isRenderOpen) {
		while (!gameIsUpdatingMutex.try_lock());

		updatingRender = true;

		// Determine the current game state and interpolation
		uint32 renderTime = Sys::GetTime() - gameStateDelay;

		curGameState = nextGameState = NULL;
		GameState* latestGameState = &gameStates[0];

		if (numGameStates > 1) {
			for (int i = 0; i < numGameStates; i+=2) {
				if (gameStates[i].time <= renderTime && gameStates[(i + 2) % numGameStates].time >= renderTime) {
					curGameState = &gameStates[i];
					nextGameState = &gameStates[(i + 2) % numGameStates];
					gameStateFactor = (float) (renderTime - curGameState->time) / (float) (nextGameState->time - curGameState->time);
				}
			}
		}

		if (!curGameState || !nextGameState) {
			updatingRender = false;
			gameIsUpdatingMutex.unlock();
			Sleep(5);
			continue;
		}

		// Render the game!
		if (vpMain && spyro) {
			draw.BeginScene();
			float camAng = LerpAngle(curGameState->camera.hAngle, nextGameState->camera.hAngle, gameStateFactor);
			float camVAng = LerpAngle(curGameState->camera.vAngle, nextGameState->camera.vAngle, gameStateFactor);

			Matrix cameraMatrix;
			cameraMatrix.MakeViewProjRH(curGameState->camera.position * (1.0f - gameStateFactor) + nextGameState->camera.position * gameStateFactor, 
				Vec3(sin(camAng) * cos(camVAng), -cos(camAng) * cos(camVAng), sin(camVAng)), vecZ, 72.0f, (float)wndMainW / (float)wndMainH, 0.01f, 100.0f);
			draw.SetCameraByMatrix(&cameraMatrix);

			if (skyBackColour) {
				draw.DrawClear(*skyBackColour, 1.0f);
			}

			draw.SetObjectTransform(NULL);
			DrawSky();
			DrawScene();
			DrawMobys();
			DrawEffects();
			DrawSpyro();

			draw.SetObjectTransform(NULL);
			draw.DrawDebug(0, 0, "Cur game state %i", curGameState - gameStates);

			draw.EndScene();

			draw.RenderScene(false);
		}

		gameIsUpdatingMutex.unlock();
	}

	return 1;
}

void UpdateScene() {
	if (!scene.spyroScene || scene.spyroScene->numSectors > 256)
		return;

	SpyroSceneHeader* sceneData = scene.spyroScene;
	draw.SetTextures(ST_PIXEL, 0, 1, &sceneTex);
	const float texFullWidth = sceneTex->GetWidth(), texFullHeight = sceneTex->GetHeight();
	float uPerTex = 1.0f / 8.0f, vPerTex = uPerTex * texFullWidth / texFullHeight, uNudge = 1.0f / texFullWidth, vNudge = 1.0f / texFullHeight;

	for (int s = 0; s < sceneData->numSectors; s++) {
		SceneSectorHeader* sector = sceneData->sectors[s];
		Model& drawModel = lerpScene.sectors[s];
		Vec3* verts = drawModel.verts;
		uint32* colours = drawModel.colours;

		uint32* spyroVerts = sector->GetHpVertices();
		uint32* spyroColours = sector->GetHpColours();

		int x = sector->xyPos >> 16, y = (sector->xyPos & 0xFFFF), z = ((sector->zPos >> 14) & 0xFFFF) >> 2;
		float fX = (float) x * WORLDSCALE, fY = (float) y * WORLDSCALE, fZ = (float) z * WORLDSCALE;
		bool flatSector = (sector->centreRadiusAndFlags >> 12 & 1);

		// Get sector vertices
		for (int v = 0; v < sector->numHpVertices; v++) {
			uint32 curVertex = spyroVerts[v];
			int curX = (curVertex >> 19 & 0x1FFC) >> 2, curY = (curVertex >> 8 & 0x1FFC) >> 2, curZ = (curVertex << 3 & 0x1FFC) >> 2;

			if (game == SPYRO1) {
				curX = (curVertex >> 19 & 0x1FFC) >> 2;
				curY = (curVertex >> 8 & 0x1FFC) >> 2;
				curZ = (curVertex << 3 & 0x1FFC) >> 3;
			}

			if (flatSector)
				curZ >>= 3;

			verts[v].x = (float) curX * WORLDSCALE + fX;
			verts[v].y = (float) curY * WORLDSCALE + fY;
			verts[v].z = (float) curZ * WORLDSCALE + fZ;
		}

		// Get colours
		for (int c = 0; c < sector->numHpColours; c++)
			colours[c] = spyroColours[c] | 0xFF000000;

		// Count total triangle count
		int numTriangles = 0;
		SceneFace* faces = sector->GetHpFaces();

		for (int f = 0; f < sector->numHpFaces; f++) {
			if (faces[f].verts[0] == faces[f].verts[1])
				numTriangles += 1;
			else
				numTriangles += 2;
		}

		if (!numTriangles) {
			drawModel.numFaces = 0;
			continue;
		}

		// Make face
		CTVertex buf[4096];
		int curVert = 0;

		for (int f = 0; f < sector->numHpFaces; f++) {
			DrawFace* drawFace = &drawModel.faces[f];
			int texId = faces[f].GetTexture();
			float texU1 = (uPerTex * texId) - (int)(uPerTex * texId) + uNudge, texU2 = texU1 + uPerTex - uNudge - uNudge;
			float texV1 = (vPerTex * (texId / 8)) + vNudge, texV2 = texV1 + vPerTex - vNudge - vNudge;

			if (!faces[f].GetFlip()) {
				drawFace->verts[0] = faces[f].verts[3];
				drawFace->verts[1] = faces[f].verts[2];
				drawFace->verts[2] = faces[f].verts[1];
				drawFace->colours[0] = faces[f].colours[3];
				drawFace->colours[1] = faces[f].colours[2];
				drawFace->colours[2] = faces[f].colours[1];
				drawFace->u[0] = texU1; drawFace->v[0] = texV1;
				drawFace->u[1] = texU2; drawFace->v[1] = texV1;
				drawFace->u[2] = texU2; drawFace->v[2] = texV2;

				if (faces[f].verts[1] != faces[f].verts[0]) {
					drawFace->verts[3] = faces[f].verts[0];
					drawFace->colours[3] = faces[f].colours[0];
					drawFace->u[3] = texU1; drawFace->v[3] = texV2;
					drawFace->numSides = 4;
				} else
					drawFace->numSides = 3;
			} else {
				int add = 0;
				if (faces[f].verts[1] != faces[f].verts[0]) {
					drawFace->verts[0] = faces[f].verts[0];
					drawFace->colours[0] = faces[f].colours[0];
					drawFace->u[add+0] = texU1; drawFace->v[add+0] = texV2;
					drawFace->numSides = 4;
					add = 1;
				} else
					drawFace->numSides = 3;
				
				drawFace->verts[add+0] = faces[f].verts[1];
				drawFace->verts[add+1] = faces[f].verts[2];
				drawFace->verts[add+2] = faces[f].verts[3];
				drawFace->colours[add+0] = faces[f].colours[1];
				drawFace->colours[add+1] = faces[f].colours[2];
				drawFace->colours[add+2] = faces[f].colours[3];
				drawFace->u[add+0] = texU2; drawFace->v[add+0] = texV2;
				drawFace->u[add+1] = texU2; drawFace->v[add+1] = texV1;
				drawFace->u[add+2] = texU1; drawFace->v[add+2] = texV1;

				texV1 = texV2;
				texV2 = texV1 - (64.0f / 1024.0f);
			}
		}

		lerpScene.sectors[s].numVerts = sector->numHpVertices;
		lerpScene.sectors[s].numFaces = sector->numHpFaces;
		lerpScene.sectors[s].numColours = sector->numHpColours;
	}

	lerpScene.numSectors = sceneData->numSectors;
}

bool UpdateMoby(int mobyId, float x, float y, float z);
void UpdateMobys() {
	if (!mobys)
		return;

	writeGameState->numMobys = 0;
	for (int i = 0; i < sizeof (writeGameState->mobys) / sizeof (writeGameState->mobys[0]); i++) {
		if (mobys[i].state == -1) {
			writeGameState->numMobys = i;
			break;
		}

		if (mobys[i].state >= 0) {
			if (UpdateMoby(i, mobys[i].x * MOBYPOSSCALE, mobys[i].y * MOBYPOSSCALE, mobys[i].z * MOBYPOSSCALE)) {
				writeGameState->mobys[i].visible = true;
			} else {
				writeGameState->mobys[i].visible = false;
			}
		} else
			writeGameState->mobys[i].visible = false;
	}
}

int32 sra(int32 val, uint8 amt) {
	return (val & 0x80000000) ? (val >> amt) | ~(0xFFFFFFFF >> amt) : (uint32)val >> amt;
}

bool UpdateMoby(int mobyId, float x, float y, float z) {
	int mobyModelId = mobys[mobyId].type;
	if (mobyModelId >= 768 || mobyModelId == 0)
		return false;
	if (!IsModelValid(mobyModelId, mobys[mobyId].anim.prevAnim))
		return false;
	
	// NEW CONDITIONS:
	// If a face is three-sided and its colours & 0x10, it'll appear as a large screen-facing polygon. & 7 is its depth? and & 8 is unknown
	// This can be affected by textures with the colours' &80000000 flag
	// word0 >> 24 becomes the origin vertex, word0 >> 16(?) becomes the colour for the entire thing. The other two must be equal values which don't matter
	uint32 modelAddress = mobyModels[mobyModelId].address & 0x003FFFFF;
	bool animated = (mobyModels[mobyModelId].address & 0x80000000);

	float scale = MOBYPOSSCALE;
	uint32* verts, *faces, *colours;
	int numVerts, numColours;

	if (animated) {
		ModelHeader* model = mobyModels[mobyModelId];
		ModelAnimHeader* anim = model->anims[mobys[mobyId].anim.nextAnim];

		if (mobys[mobyId].anim.nextAnim > model->numAnims || !model->anims[mobys[mobyId].anim.nextAnim].IsValid()) {
			return false;
		}

		verts = anim->verts; faces = anim->faces; colours = anim->colours;
		numVerts = anim->numVerts; numColours = anim->numColours;

		scale = (float) (MOBYPOSSCALE) * ((float) (1 << anim->vertScale));

		if (mobys[mobyId].scaleDown) {
			scale *= 32.0f / (float)mobys[mobyId].scaleDown;
		}
	} else {
		SimpleModelHeader* model = (SimpleModelHeader*) (mobyModels[mobyModelId]);
		SimpleModelStateHeader* state = model->states[mobys[mobyId].anim.prevAnim];

		if (mobys[mobyId].anim.prevAnim >= model->numStates || !model->states[mobys[mobyId].anim.prevAnim].IsValid())
			return false;

		verts = state->data32; faces = &state->data32[(state->hpFaceOff - 16) / 4]; colours = &state->data32[(state->hpColourOff - 16) / 4];
		numVerts = state->numHpVerts; numColours = state->numHpColours;
		scale = MOBYPOSSCALE * 4.0f;
	}

	if (numVerts >= 256 || numColours >= 256 || faces[0] / 8 >= 900)
		return false;

	// Begin model conversion
	Model drawModel;

	// Send frame vertices
	if (animated) {
		float animProgress = (float)(mobys[mobyId].animProgress & 0x0F) / (float)(mobys[mobyId].animProgress >> 4);

		if (!(mobys[mobyId].animProgress >> 4))
			animProgress = 0.0f;

		for (int i = 0; i < numVerts; i++)
			drawModel.verts[i].Zero();

		for (int frame = 0; frame < 2; frame++) {
			ModelHeader* model = mobyModels[mobyModelId];
			ModelAnimHeader* animHeader = model->anims[frame == 0 ? mobys[mobyId].anim.prevAnim : mobys[mobyId].anim.nextAnim];
			ModelAnimFrameInfo* frameInfo = (frame == 0 ? &animHeader->frames[mobys[mobyId].anim.prevFrame] : &animHeader->frames[mobys[mobyId].anim.nextFrame]);

			verts = animHeader->verts;
			uint16* blocks = &animHeader->data[frameInfo->blockOffset];
			uint8* adjusts = (uint8*)&blocks[frameInfo->numBlocks + ((frameInfo->startFlags & 1) << 8)];

			ConvertAnimatedVertices(&drawModel, verts, blocks, adjusts, 0, animHeader->numVerts, (frameInfo->startFlags & 2) != 0, (frame == 0 ? scale * (1.0f - animProgress) : scale * animProgress), false);
		}
	} else {
		for (int i = 0; i < numVerts; i++)
			drawModel.verts[i].Set((float) -bitss(verts[i], 22, 10) * scale, (float) -bitss(verts[i], 12, 10) * scale, (float) -bitss(verts[i], 0, 12) * scale / 2.0f);
	}

	// Set colours
	int uniqueColour = numColours;
	uint32* drawColours = drawModel.colours;

	for (int i = 0; i < numColours; i++)
		drawColours[i] = colours[i] | 0xFF000000;
	if (mobys[mobyId].colour <= 5) {
		drawColours[uniqueColour] = gemColours[mobys[mobyId].colour * 16] | 0xFF000000;
	} else {
		drawColours[uniqueColour] = mobys[mobyId].colour | 0xFF000000;
	}

	// Set model faces
	// Colour flags: &1 - isColouredByMoby
	ConvertFaces(&drawModel, faces, uniqueColour, animated, false);

	// Update the model
	drawModel.numColours = numColours + 1;
	drawModel.numVerts = numVerts;

	writeGameState->mobys[mobyId].model = drawModel;
	writeGameState->mobys[mobyId].rotation.x = mobys[mobyId].angle.x * DOUBLEPI / 255.0f;
	writeGameState->mobys[mobyId].rotation.y = -mobys[mobyId].angle.y * DOUBLEPI / 255.0f;
	writeGameState->mobys[mobyId].rotation.z = mobys[mobyId].angle.z * DOUBLEPI / 255.0f;
	writeGameState->mobys[mobyId].position.x = mobys[mobyId].x * MOBYPOSSCALE;
	writeGameState->mobys[mobyId].position.y = mobys[mobyId].y * MOBYPOSSCALE;
	writeGameState->mobys[mobyId].position.z = mobys[mobyId].z * MOBYPOSSCALE;
	return true;
}

void UpdateSpyro() {
	if (!spyro || !IsModelValid(0, spyro->anim.nextAnim))
		return;

	SpyroModelHeader* model = (SpyroModelHeader*)mobyModels[0];
	SpyroAnimHeader* anim = model->anims[spyro->anim.nextAnim];
	SpyroFrameInfo* frame = anim ? &anim->frames[spyro->anim.nextFrame] : nullptr;

	if (!anim || !frame || anim->numVerts >= 256 || anim->numColours >= 256 || !anim->faces)
		return;

	uint32* faces = anim->faces, *colours = anim->colours;
	int numVerts = anim->numVerts, numColours = anim->numColours, maxNumFaces = faces[0] / 8;
	float scale = MOBYPOSSCALE;

	// Get frame vertices
	Vec3* drawVerts = writeGameState->spyro.model.verts;
	
	for (int i = 0; i < numVerts; i++)
		drawVerts[i].Zero();

	float animProgress = (float)(spyro->animprogress & 0x0F) / (float)(spyro->animprogress >> 4 & 0x0F);
	if (!(spyro->animprogress >> 4 & 0x0F))
		animProgress = 1.0f;

	for (int frameIndex = 0; frameIndex < 2; frameIndex++) {
		int curX = 0, curY = 0, curZ = 0;
		int blockX = 0, blockY = 0, blockZ = 0;
		int curBlock = 0;
		bool nextBlock = true, blockVert = false;
		int adjustVert = 0;
			
		anim = model->anims[frameIndex == 0 ? spyro->anim.prevAnim : spyro->anim.nextAnim];
		frame = &anim->frames[frameIndex == 0 ? spyro->anim.prevFrame : spyro->anim.nextFrame];

		uint32* verts = anim->verts;
		uint16* blocks = &anim->data[frame->blockOffset / 2];
		uint8* adjusts = &((uint8*)anim->data)[frame->blockOffset + ((frame->word1 >> 10) & 0x3FF)];
		int headX = bitss(frame->headPos, 21, 11), headY = bitss(frame->headPos, 10, 11), headZ = bitss(frame->headPos, 0, 10) * 2;
		int headVert = anim->headVertStart;
		int headAdjust = ((uintptr)((uint16*)anim->data) + (frame->word1 >> 20) + frame->blockOffset) - (uintptr)adjusts;
		int headBlock = (frame->word1 & 0x3FF) / 2;
		
		float frameScale = scale * (frameIndex == 0 ? 1.0f - animProgress : animProgress);
		ConvertAnimatedVertices(&writeGameState->spyro.model, verts, blocks, adjusts, 0, headVert, (frame->unk1 & 1), frameScale, true);
		ConvertAnimatedVertices(&writeGameState->spyro.model, &verts[headVert], &blocks[headBlock], &adjusts[headAdjust], headVert, numVerts - headVert, (frame->unk1 & 2), frameScale, true);

		// Add head position
		Vec3 headOffset = {headX * frameScale, headY * frameScale, headZ * frameScale};
		for (int v = headVert; v < numVerts; ++v) {
			writeGameState->spyro.model.verts[v] += headOffset;
		}
	}

	// Set colours
	uint32* drawColours = writeGameState->spyro.model.colours;

	for (int i = 0; i < numColours; i++)
		drawColours[i] = colours[i] | 0xFF000000;

	// Set model faces
	ConvertFaces(&writeGameState->spyro.model, faces, numColours, true, true);

	// Update the model
	writeGameState->spyro.model.numVerts = numVerts;
	writeGameState->spyro.model.numColours = numColours;

	// Update info
	writeGameState->spyro.position.Set(spyro->x * MOBYPOSSCALE, spyro->y * MOBYPOSSCALE, spyro->z * MOBYPOSSCALE);
	writeGameState->spyro.rotation.Set(spyro->angle.x * DOUBLEPI / 255.0f, -spyro->angle.y * DOUBLEPI / 255.0f, spyro->angle.z * DOUBLEPI / 255.0f);
}

void UpdateSky() {
	for (int i = 0; i < 256; ++i) {
		skyModel.sectors[i].numFaces = 0;
		skyModel.sectors[i].numVerts = 0;
	}

	if (!spyroSky || spyroSky->numSectors > 300) {
		return;
	}

	for (int i = 0; i < spyroSky->numSectors && i < *skyNumSectors; ++i) {
		SkySectorHeader* sector = spyroSky->sectors[i];
		Model* model = &skyModel.sectors[i];

		uint32* verts = &sector->data32[0], *colours = &sector->data32[sector->numVertices], *faceStarters = &sector->data32[sector->numVertices + sector->numColours];
		uint16* faceBlocks = &sector->data16[((sector->numColours + sector->numVertices) * 4 + sector->faceStarterSize) / 2], *endFaceBlock = &faceBlocks[sector->faceBlockSize / 2];
		int numVerts = sector->numVertices;
		
		if (!numVerts) {
			continue;
		}

		// Convert vertices
		model->numVerts = sector->numVertices;
		for (int vert = 0; vert < sector->numVertices; ++vert) {
			model->verts[vert].Set((sector->x + bitss(verts[vert], 21, 11)) * 0.001f, 
								   (-sector->y + (int)bitss(verts[vert], 10, 11)) * 0.001f, 
								   (-sector->z + (int)bitsu(verts[vert], 0, 10)) * 0.001f);
		}

		// Convert colours
		model->numColours = sector->numColours;
		for (int colour = 0; colour < sector->numColours; ++colour) {
			model->colours[colour] = 0xFF000000 | colours[colour];
		}

		// Convert faces
		DrawFace* curModelFace = &model->faces[0];
		uint16* curFaceBlock = faceBlocks;
		uint32* curFaceStarter = faceStarters;
		const float blankUv = 8.0f / (float) 512.0f;

		while (curFaceBlock < endFaceBlock) {
			uint32 starter = *curFaceStarter++;
			uint16 starterBlock = *curFaceBlock++;
			uint8 positions[12], colours[12];
			uint8 numExtraTriangles;
			int firstVert = 1, secondVert = 0;

			numExtraTriangles = bitsu(starter, 0, 3);
			colours[0] = bitsu(starter, 10, 7);
			colours[1] = bitsu(starter, 3, 7);
			colours[2] = bitsu(starter, 17, 7);
			positions[0] = bitsu(starterBlock, 0, 8) % numVerts;
			positions[1] = bitsu(starter, 24, 8) % numVerts;
			positions[2] = bitsu(starterBlock, 8, 8) % numVerts;

			curModelFace->numSides = 3;
			for (int side = 0; side < 3; ++side) {
				curModelFace->verts[side] = positions[side];
				curModelFace->colours[side] = colours[side];
				curModelFace->u[side] = curModelFace->v[side] = blankUv;
			}
			++curModelFace;

			for (int tri = 0; tri < numExtraTriangles; ++tri) {
				colours[3 + tri] = bitsu(*curFaceBlock, 8, 7);
				positions[3 + tri] = bitsu(*curFaceBlock, 0, 8);

				curModelFace->numSides = 3;
				for (int side = 0; side < 3; ++side) {
					curModelFace->u[side] = curModelFace->v[side] = blankUv;
				}

				curModelFace->verts[0] = positions[firstVert] % numVerts;
				curModelFace->verts[1] = positions[secondVert] % numVerts;
				curModelFace->verts[2] = positions[tri + 3] % numVerts;
				curModelFace->colours[0] = colours[firstVert];
				curModelFace->colours[1] = colours[secondVert];
				curModelFace->colours[2] = colours[tri + 3];

				if (bitsu(*curFaceBlock, 15, 1)) {
					secondVert = tri + 2;
					if (tri == 0) {
						secondVert = 1;
					}
				}
				firstVert = tri + 3;

				++curFaceBlock;
				++curModelFace;
			}
		}
		model->numFaces = curModelFace - model->faces;
	}
}

void UpdateEffects() {
	writeGameState->numEffects = 0;

	if (spyroEffects) {
		for (int effect = 0; effect < 512 && writeGameState->numEffects < GameState::maxNumEffects; ++effect) {
			if (spyroEffects[effect].renderType == SpyroEffectRenderType::StateInactive) {
				continue;
			} else if (spyroEffects[effect].renderType == SpyroEffectRenderType::StateFinal) {
				break;
			} else {
				writeGameState->effects[writeGameState->numEffects++].effect = spyroEffects[effect];
			}
		}
	}
}

void DrawScene() {
	draw.SetTextures(ST_PIXEL, 0, 1, &sceneTex);
	for (int i = 0; i < lerpScene.numSectors; i++) {
		DrawModel(&lerpScene.sectors[i], NULL);
	}
}

void DrawMobys() {
	draw.SetTextures(ST_PIXEL, 0, 1, &objTex);

	for (int i = 0; i < curGameState->numMobys && i < nextGameState->numMobys; i++) {
		if (!curGameState->mobys[i].visible || !nextGameState->mobys[i].visible)
			continue;

		Model tempModel = curGameState->mobys[i].model;
		Matrix transform = matIdentity;
		float factor = gameStateFactor;
	
		if (curGameState->mobys[i].visible && !nextGameState->mobys[i].visible)
			factor = 0.0f;
		else if (!curGameState->mobys[i].visible && nextGameState->mobys[i].visible)
			factor = 1.0f;

		if (curGameState->mobys[i].visible && nextGameState->mobys[i].visible) {
			for (int j = 0; j < tempModel.numVerts; j++) {
				tempModel.verts[j] = curGameState->mobys[i].model.verts[j] * (1.0f - gameStateFactor) + nextGameState->mobys[i].model.verts[j] * gameStateFactor;
			}
		}

		Vec3 pos = curGameState->mobys[i].position * (1.0f - gameStateFactor) + nextGameState->mobys[i].position * gameStateFactor;
		Vec3 rot(LerpAngle(curGameState->mobys[i].rotation.x, nextGameState->mobys[i].rotation.x, gameStateFactor), 
			LerpAngle(curGameState->mobys[i].rotation.y, nextGameState->mobys[i].rotation.y, gameStateFactor), 
			LerpAngle(curGameState->mobys[i].rotation.z, nextGameState->mobys[i].rotation.z, gameStateFactor));
		transform.RotateXYZ(rot.x, rot.y, rot.z);
		transform.Translate(pos.x, pos.y, pos.z);
	
		DrawModel(&tempModel, &transform);
	}
}



// Converts animated vertices from Spyro format to absolute format and adds them to modelOut->verts.
// Note that this RELATIVELY ADDS positions to the vertices! This is so that you can call the function multiple times to blend animations.
void ConvertAnimatedVertices(Model* modelOut, uint32* verts, uint16* blocks, uint8* adjusts, uint32 startOutputVertexIndex, uint32 numOutputVertices, bool doStartOnBlock, float scaleFactor, bool isSpyro) {
	int curX = 0, curY = 0, curZ = 0;
	int blockX = 0, blockY = 0, blockZ = 0;
	int curBlock = 0;
	bool nextBlock = true, blockVert = false;
	int adjustVert = 0;

	if (doStartOnBlock) {
		curX = 0; curY = 0; curZ = 0;
		nextBlock = true;
	} else {
		curX = bitss(verts[0], 21, 11);
		curY = bitss(verts[0], 10, 11);
		curZ = bitss(verts[0], 0, 10);
		curX = curY = curZ = 0;
		nextBlock = false;
	}

	for (int i = 0; i < numOutputVertices; i++) {
		bool usedBlock = false;
		int x = bitss(verts[i], 21, 11), y = bitss(verts[i], 10, 11), z = bitss(verts[i], 0, 10);

		if (isSpyro) {
			z *= 2;
		}

		if (nextBlock) {
			blockVert = (blocks[curBlock] & 1) != 0;

			nextBlock = blockVert;
			if (blocks[curBlock] & 0x2) {
				// Long block
				uint32 pos = blocks[curBlock] | (blocks[curBlock + 1] << 16);

				if (isSpyro) {
					curX = bitss(pos, 21, 11);
					curY = bitss(pos, 11, 11);
					curZ = bitss(pos, 1, 11);
				} else {
					curX =  bitss(pos, 21, 11);
					curY = -bitss(pos, 11, 11);
					curZ = -bitss(pos, 1, 11);
				}

				x = 0; y = 0; z = 0;
				curBlock += 2;
			} else {
				// Short block
				if (isSpyro) {
					curX += bitss(blocks[curBlock], 9, 7);
					curY -= bitss(blocks[curBlock], 4, 7);
					curZ -= bitss(blocks[curBlock], 1, 5) * 4;
				} else {
					curX += bitss(blocks[curBlock], 10, 6) * 2;
					curY += bitss(blocks[curBlock], 5, 6) * 2;
					curZ += bitss(blocks[curBlock], 1, 5) * 2;
				}

				curBlock += 1;
			}

			usedBlock = true;
		}

		if (!usedBlock && !blockVert) {
			int32 adjusted = mobyVertAdjustTable[(adjusts[adjustVert] >> 1)] + verts[i];

			if (adjusts[adjustVert] & 1)
				nextBlock = true;

			x = bitss(adjusted, 21, 11);
			y = bitss(adjusted, 10, 11);
			z = bitss(adjusted, 0, 10);
			
			if (isSpyro) {
				z *= 2;
			}

			adjustVert++;
		}

		if (isSpyro) {
			curX += x; curY -= y; curZ -= z; // x = fwd/back y = left/right z = up/down
			modelOut->verts[startOutputVertexIndex + i] += Vec3((float)curX * scaleFactor, (float)-curY * scaleFactor, (float)-curZ * scaleFactor);
		} else {
			curX += x; curY += y; curZ += z;
			modelOut->verts[startOutputVertexIndex + i] += Vec3((float)curX * scaleFactor, (float)curY * scaleFactor, (float)curZ * 2.0f * scaleFactor);
		}
	}

	modelOut->numVerts = startOutputVertexIndex + numOutputVertices;
}

void ConvertFaces(Model* modelOut, uint32* faces, int uniqueColourIndex, bool isAnimated, bool isSpyro) {
	int numSides = 0, numUvs = 0, numUvIndices = 0, index = 1, maxIndex = faces[0] / 4 + 1;
	int curFace = 0;

	while (index < maxIndex) {
		DrawFace* drawFace = &modelOut->faces[curFace++];
		uint32 face = faces[index];
		uint32 clrs = faces[index + 1];
		int numFaceSides = 0;

		if (isAnimated)
			numFaceSides = ((face & 0xFF) != ((face >> 8) & 0xFF) ? 4 : 3);
		else
			numFaceSides = ((face & 0x7F) != (face >> 7 & 0x7F) ? 4 : 3);

		if (numFaceSides == 4) {
			if (isAnimated) {
				drawFace->verts[0] = face >> 8 & 0xFF;
				drawFace->verts[1] = face & 0xFF;
				drawFace->verts[2] = face >> 16 & 0xFF;
				drawFace->verts[3] = face >> 24 & 0xFF;
			} else {
				drawFace->verts[0] = face >> 7 & 0x7F; // btLeft
				drawFace->verts[1] = face & 0x7F; // tpLeft
				drawFace->verts[2] = face >> 14 & 0x7F; // topRight
				drawFace->verts[3] = face >> 21 & 0x7F; // bottomRight
			}

			drawFace->colours[0] = clrs >> 10 & 0x7F;
			drawFace->colours[1] = clrs >> 3 & 0x7F;
			drawFace->colours[2] = clrs >> 17 & 0x7F;
			drawFace->colours[3] = clrs >> 24 & 0x7F;
		} else {
			if (isAnimated) {
				drawFace->verts[0] = face >> 24 & 0xFF;
				drawFace->verts[1] = face >> 16 & 0xFF;
				drawFace->verts[2] = face >> 8 & 0xFF;
			} else {
				drawFace->verts[0] = face >> 21 & 0x7F;
				drawFace->verts[1] = face >> 14 & 0x7F;
				drawFace->verts[2] = face >> 7 & 0x7F;
			}

			drawFace->colours[0] = clrs >> 24 & 0x7F;
			drawFace->colours[1] = clrs >> 17 & 0x7F;
			drawFace->colours[2] = clrs >> 10 & 0x7F;
		}

		if ((clrs & 1) && 0) {
			// Just use this colour for now (todo shinyness, etc)
			drawFace->colours[0] = drawFace->colours[1] = drawFace->colours[2] = drawFace->colours[3] = uniqueColourIndex;
		}

		for (int i = 0; i < numFaceSides; i++) {
			drawFace->colours[i] %= uniqueColourIndex;
		}

		drawFace->numSides = numFaceSides;

		if ((clrs & 0x80000000) || isSpyro) {
			uint32 word0 = faces[index], word1 = faces[index + 1], word2 = faces[index + 2], word3 = faces[index + 3], word4 = faces[index + 4];
			int p1U = bitsu(word4, 16, 8), p1V = bitsu(word4, 24, 8), p2U = bitsu(word4, 0, 8), p2V = bitsu(word4, 8, 8), 
				p3U = bitsu(word3, 0, 8),  p3V = bitsu(word3, 8, 8),  p4U = bitsu(word2, 0, 8), p4V = bitsu(word2, 8, 8);
			int imageBlockX = bitsu(word3, 16, 4) * 0x40, imageBlockY = bitsu(word3, 20, 1) * 0x100;
			int p1X = p1U + (imageBlockX * 4), p1Y = p1V + imageBlockY, p2X = p2U + (imageBlockX * 4), p2Y = p2V + imageBlockY, p3X = p3U + (imageBlockX * 4),
				p3Y = p3V + imageBlockY, p4X = p4U + (imageBlockX * 4), p4Y = p4V + imageBlockY;
			bool gotTextures = false;

			for (int i = 0; i < objTexMap.numTextures; i++) {
				if (objTexMap.textures[i].minX <= p1X && objTexMap.textures[i].minY <= p1Y && objTexMap.textures[i].maxX >= p1X && objTexMap.textures[i].maxY >= p1Y) {
					ObjTex* tex = &objTexMap.textures[i];
					const int w = objTexMap.width, h = objTexMap.height;

					if (numFaceSides == 4) {
						int pX[4] = {p2X, p1X, p3X, p4X}, pY[4] = {p2Y, p1Y, p3Y, p4Y};
						for (int j = 0; j < 4; j++) {
							drawFace->u[j] = (float) (tex->mapX + pX[j] - objTexMap.textures[i].minX) / 512.0f;
							drawFace->v[j] = (float) (tex->mapY + pY[j] - objTexMap.textures[i].minY) / (float)objTexMap.height;
						}
					} else {
						int pX[4] = {p2X, p3X, p4X}, pY[4] = {p2Y, p3Y, p4Y};
						for (int j = 0; j < 3; j++) {
							drawFace->u[j] = (float) (tex->mapX + pX[2 - j] - objTexMap.textures[i].minX) / 512.0f;
							drawFace->v[j] = (float) (tex->mapY + pY[2 - j] - objTexMap.textures[i].minY) / (float)objTexMap.height;
						}
					}

					gotTextures = true;
					break;
				}
			}

			if (!gotTextures) {
				for (int i = 0; i < numFaceSides; i++) {
					drawFace->u[i] = 8.0f / (float) 512.0f;
					drawFace->v[i] = 8.0f / (float) 512.0f;
				}
			}

			index += 5; // textured face, continue
		} else {
			for (int i = 0; i < numFaceSides; i++) {
				drawFace->u[i] = 8.0f / (float) 512.0f;
				drawFace->v[i] = 8.0f / (float) 512.0f;
			}

			index += 2; // no textures, continue
		}
	}

	modelOut->numFaces = curFace;
}

void DrawSpyro() {
	draw.SetTextures(ST_PIXEL, 0, 1, &objTex);

	Model tempModel = nextGameState->spyro.model;

	for (int j = 0; j < tempModel.numVerts; j++) {
		tempModel.verts[j] = curGameState->spyro.model.verts[j] * (1.0f - gameStateFactor) + nextGameState->spyro.model.verts[j] * gameStateFactor;
	}

	Vec3 pos = curGameState->spyro.position * (1.0f - gameStateFactor) + nextGameState->spyro.position * gameStateFactor;
	Vec3 rot(LerpAngle(curGameState->spyro.rotation.x, nextGameState->spyro.rotation.x, gameStateFactor), 
		LerpAngle(curGameState->spyro.rotation.y, nextGameState->spyro.rotation.y, gameStateFactor), 
		LerpAngle(curGameState->spyro.rotation.z, nextGameState->spyro.rotation.z, gameStateFactor));
	Matrix transform = matIdentity;
	transform.RotateXYZ(rot.x, rot.y, rot.z);
	transform.Translate(pos.x, pos.y, pos.z);

	DrawModel(&tempModel, &transform);
}

void DrawSky() {
	draw.SetTextures(ST_PIXEL, 0, 1, &objTex);

	for (int i = 0; i < 256; ++i) {
		Matrix transform;

		Vec3 pos = curGameState->camera.position * (1.0f - gameStateFactor) + nextGameState->camera.position * gameStateFactor;
		transform.MakeScale(98.0f, 98.0f, 98.0f);
		transform.Translate(pos.x, pos.y, pos.z);
		DrawModel(&skyModel.sectors[i], &transform);
	}
}

void DrawEffects() {
	float camAng = LerpAngle(curGameState->camera.hAngle, nextGameState->camera.hAngle, gameStateFactor);
	float camVAng = LerpAngle(curGameState->camera.vAngle, nextGameState->camera.vAngle, gameStateFactor);
	Vec3 camPos = nextGameState->camera.position * gameStateFactor + curGameState->camera.position * (1.0f - gameStateFactor);
	Vec3 camFwd = Vec3(sin(camAng) * cos(camVAng), -cos(camAng) * cos(camVAng), sin(camVAng)), 
		camRight = Vec3(sin(camAng + HALFPI) * cos(camVAng), -cos(camAng + HALFPI) * cos(camVAng), sin(camVAng)), 
		camUp = vecZ;

	for (int i = 0; i < numEffectsModels; ++i) {
		effectsModels[i].numColours = effectsModels[i].numFaces = effectsModels[i].numVerts = 0;
	}

	int curVert = 0, curFace = 0, curColour = 0;
	Model* model = &effectsModels[0];
	for (int curEffect = 0; curEffect < curGameState->numEffects; ++curEffect) {
		SpyroEffect* effect = &curGameState->effects[curEffect].effect;
		Vec3 position = Vec3(effect->x * WORLDSCALE / 4.0f, effect->y * WORLDSCALE / 4.0f, effect->z * WORLDSCALE / 4.0f);

		// Check if we need to advance to the next model (if possible)
		if (curVert + 4 > 256 || curFace + 4 > 256) {
			++model;
			if (model == &effectsModels[numEffectsModels])
				goto Final;

			curVert = 0;
			curFace = 0;
		}
		
		// Render the effect
		auto addPatch = [&](const Vec3& pos1, const Vec3& pos2, const Vec3& pos3, const Vec3& pos4, 
			int clr1, int clr2, int clr3, int clr4) {
			model->verts[curVert+0] = pos1;
			model->verts[curVert+1] = pos2;
			model->verts[curVert+2] = pos3;
			model->verts[curVert+3] = pos4;
			for (int i = 0; i < 4; ++i) {
				model->faces[curFace].verts[i] = curVert + i;
				model->faces[curFace].u[i] = model->faces[curFace].v[i] = 8.0f / 512.0f;
			}
			model->faces[curFace].colours[0] = clr1;
			model->faces[curFace].colours[1] = clr2;
			model->faces[curFace].colours[2] = clr3;
			model->faces[curFace].colours[3] = clr4;
			model->faces[curFace].numSides = 4;
			curFace += 1;
			curVert += 4;
		};
		auto convertColour = [](uint32 colour) {
			return 0xFF000000 | (colour >> 16 & 0xFF) | (colour & 0xFF00) | (colour << 16 & 0xFF0000);
		};

		switch (effect->renderType) {
			case SpyroEffectRenderType::StateInactive:
				continue;
			case SpyroEffectRenderType::StateFinal:
				goto Final;
			case SpyroEffectRenderType::Spark: {
				const float size = 0.005f * Vec3::Dot(camFwd, position - camPos);
				Vec3 up = camUp * size, right = camRight * size;
				addPatch(position + up - right, position + up + right, position - up + right, position - up - right, curColour, curColour, curColour, curColour);
				model->colours[curColour++] = effect->spark.colour | 0xFF000000;
				break;
			}
			case SpyroEffectRenderType::String: {
				const float size = 0.005f * Vec3::Dot(camFwd, position - camPos);
				Vec3 position2 = Vec3(effect->string.x2 * WORLDSCALE / 4.0f, effect->string.y2 * WORLDSCALE / 4.0f, effect->string.z2 * WORLDSCALE / 4.0f);
				Vec3 edgeVector = Vec3::Cross(camFwd, position2 - position);
				edgeVector.ScaleTo(size);

				addPatch(position + edgeVector, position - edgeVector, position2 - edgeVector, position2 + edgeVector, curColour, curColour, curColour + 1, curColour + 1);
				model->colours[curColour++] = effect->string.colour1 | 0xFF000000;
				model->colours[curColour++] = effect->string.colour2 | 0xFF000000;
				break;
			}
		}

		model->numVerts = curVert;
		model->numFaces = curFace;
		model->numColours = curColour;
	}

Final:;
	draw.SetTextures(ST_PIXEL, 0, 1, &objTex);

	for (int i = 0; i < numEffectsModels; ++i) {
		DrawModel(&effectsModels[i], &matIdentity);
	}
}

void DrawModel(const Model* model, const Matrix* transform) {
	if (!model->numVerts || !model->numFaces) {
		return;
	}

	int curVert = 0;

	CTVertex* vertBuf = (CTVertex*) modelBuffers[curModelBuffer]->Lock();
	const Vec3* verts = model->verts;
	const uint32* colours = model->colours;
	for (const DrawFace* face = model->faces; face < &model->faces[model->numFaces]; face++) {
		for (int tri = 0; tri < face->numSides - 2; tri++) {
			vertBuf[curVert].pos = verts[face->verts[0]];
			vertBuf[curVert].clr = colours[face->colours[0]];
			vertBuf[curVert].u = face->u[0];
			vertBuf[curVert].v = face->v[0];
			vertBuf[curVert+1].pos = verts[face->verts[tri+1]];
			vertBuf[curVert+1].clr = colours[face->colours[tri+1]];
			vertBuf[curVert+1].u = face->u[tri+1];
			vertBuf[curVert+1].v = face->v[tri+1];
			vertBuf[curVert+2].pos = verts[face->verts[tri+2]];
			vertBuf[curVert+2].clr = colours[face->colours[tri+2]];
			vertBuf[curVert+2].u = face->u[tri+2];
			vertBuf[curVert+2].v = face->v[tri+2];
			curVert += 3;
		}
	}

	Texture* tex = {NULL};
	draw.SetObjectTransform(transform);

	modelBuffers[curModelBuffer]->Unlock();
	draw.DrawDefaultBufferRange(modelBuffers[curModelBuffer], 0, curVert, VT_CT, PT_TRIANGLELIST);

	curModelBuffer = (curModelBuffer + 1) % (sizeof (modelBuffers) / sizeof (modelBuffers[0]));
}

void UpdateTextures(bool doForceReplaceTextures) {
	if (!textures && !hqTextures)
		return;
	
	// Load the texture
	char filename[MAX_PATH];
	wchar filenameW[MAX_PATH];
	GetLevelFilename(filename, SEF_RENDERTEXTURES, true);
	mbstowcs(filenameW, filename, MAX_PATH);

	if (doForceReplaceTextures || GetFileAttributes(filename) == INVALID_FILE_ATTRIBUTES) {
		// Texture file doesn't exist? Create it
		SaveTexturesAsSingle(filename);
	}

	if (sceneTex->LoadImageFromFile(filenameW)) {
		// Convert blacks to transparent
		uint32* bits;
		
		if (sceneTex->LockBits((void**)&bits)) {
			for (uint32* bit = bits, *endBit = &bits[sceneTex->GetWidth() * sceneTex->GetHeight()]; bit < endBit; ++bit) {
				if (*bit == 0xFF000000) {
					*bit = 0x00000000;
				}
			}

			sceneTex->UnlockBits();
		}
	} else {
		// Use white texture
		uint32* bits;
		sceneTex->SetDimensions(1, 1);
		if (sceneTex->LockBits((void**)&bits)) {
			bits[0] = 0xFFFFFFFF;
			sceneTex->UnlockBits();
		}
	}

	// Build obj texture
	UpdateObjTexMap();

	GetLevelFilename(filename, SEF_RENDEROBJTEXTURES, true);
	mbstowcs(filenameW, filename, MAX_PATH);

	if (doForceReplaceTextures || GetFileAttributes(filename) == INVALID_FILE_ATTRIBUTES) {
		// Texture file doesn't exist? Create it
		SaveObjectTextures(filename);
	}

	if (objTex->LoadImageFromFile(filenameW)) {
		// Copypasta...
		uint32* bits;

		if (objTex->LockBits((void**)&bits)) {
			for (uint32* bit = bits, *endBit = &bits[objTex->GetWidth() * objTex->GetHeight()]; bit < endBit; ++bit) {
				if (*bit == 0xFF000000) {
					*bit = 0x00000000;
				}
			}

			objTex->UnlockBits();
		}
	} else {
		// Use white texture
		uint32* bits;
		objTex->SetDimensions(1, 1);
		if (objTex->LockBits((void**)&bits)) {
			bits[0] = 0xFFFFFFFF;
			objTex->UnlockBits();
		}
	}
}

inline float LerpAngle(float val1, float val2, float factor) {
	float diff = val2 - val1;

	if (val2 - val1 >= PI) {
		diff -= PI * 2.0f;
	} else if (val2 - val1 <= -PI)
		diff += PI * 2.0f;

	return val1 + diff * factor;
}

void Alert(const char* string) {
	MessageBoxA(wndMain.GetHwnd(), string, "Alert", MB_OK);
}
}
#endif