#pragma once
#include "sdTypes.h"
#include "sdRenderResources.h"
#include "sdRenderPrivateTypes.h"

#ifndef _WINDOWS_
//#include <Windows.h> // Needed to define interfaces, for whatever reason
#endif

#define DECLARE_NAMED_ITEMS(...)

#define MAXDEBUGSTRINGS 50
#define MAXDEBUGOBJECTS 50

#define MAXSCRATCHVERTS 6 // Verts in the scratch vertex buffer
#define MAXRENDERTARGETS 8

// Minor Predeclarations
class Window; class Matrix;
struct cbObjectDef; struct cbViewDef; // Default shader cbuffer types
__interface ID3D11Device; __interface ID3D11DeviceContext; __interface IDXGISwapChain; __interface ID3D11RenderTargetView; __interface IDXGIFactory; __interface ID3D11InputLayout;
__interface ID3D11SamplerState; __interface IWICImagingFactory; __interface ID3D11Buffer;

// Main Renderer class
class Renderer {
public:
	// Initialisation and shutdown functions (used primarily for D3D setup)
	bool Init(bool d3dDebug = false);
	void Shutdown();

	// Render cycle functions
	void BeginScene(bool8 clear = true, color32 clearClr = MAKECOLOR(25, 127, 255));
	void EndScene();

	void RenderScene(bool8 wait);

	// Draw state functions
	// SetCamera: Sets main general perspective and projection (i.e. the camera) based on the given vectors. Affects all standard non-2D draw functions
	void SetCamera(const Vec3& position, const Vec3& direction, float rotation);
	// SetCameraByMatrix: Sets main general perspective and projection based on a matrix
	void SetCameraByMatrix(const Matrix* transform);

	// SetPerspectiveMode: Sets perspective mode to 2D (PMODE_2D) or 3D (PMODE_3D). Affects all standard non-2D draw functions, much like SetCamera
	void SetPerspectiveMode(PerspectiveMode pMode);
	
	// Transform states
	const Matrix* GetCameraTransform();

	void SetObjectTransform(const Matrix* transform); // Use nullptr for identity
	const Matrix* GetObjectTransform() const;

	// Shader states (custom shaders) (use GetDefaultShader to use a default shader)
	void SetShader(const Shader* shader);
	void SetShaders(const ShaderSet* shaderSet);

	// Resource states
	void SetTextures(ShaderType shaderType, int startSlot, int numTextures, const Texture*const* textureList);

	// Draw target states
	// SetRenderTargets: Sets render targets and returns whether successful. Not necessary by default draws. Targets must all be the same dimensions. However, data format may vary.
	// By default, the viewport used in default draws occupies slot 0. This function overrides that, unless renderTargetList[0] is set to nullptr, in which case the viewport is used.
	bool SetRenderTargets(RenderTarget** renderTargetList, int numRenderTargets);

	// SetStreamOutputs: For compatible shaders (geometry shaders with stream output), this sets the target vertex buffers with stream outputs.
	// Buffers must have the VBF_STREAMOUTPUT flag
	bool SetStreamOutputs(VertBuffer** bufferList, int numBuffers);

	// Viewports
	void SetActiveViewport(Viewport* viewport);
	void SetActiveViewports(Viewport** viewportArray, int numViewports);
	Viewport* GetActiveViewport();

	// CBuffers
	void BindCBuffer(const CBufferRaw* buf, ShaderType shaderType, int slot);
	void BindCBuffers(const CBufferRaw*const* cBuffers, ShaderType shaderType, int startSlot, int count);

	// Draw functions
	// Raw draws
	// DrawCustomDirect: Draws unbuffered vertices using current custom shaders
	void DrawCustomDirect(const void* vertices, int numVertices, int sizePerVertex, PrimitiveType primType);
	// DrawCustomBuffer: Draws buffered vertices (entire buffer)
	void DrawCustomBuffer(const VertBuffer* verts, int sizePerVertex, PrimitiveType primType);
	// DrawCustomBufferRange: Draws a range of buffered vertices
	void DrawCustomBufferRange(const VertBuffer* verts, int startVert, int numVertsToDraw, int sizePerVertex, PrimitiveType primType);

	// Default/helper draws. Uses default shaders, selected according to the given VertType.
	// DrawDefaultDirect: Draws unbuffered vertices using default shaders
	void DrawDefaultDirect(const void* vertices, int numVertices, VertType vertType, PrimitiveType primType);
	// DrawDefaultBuffer: Draws buffered vertices (entire buffer)
	void DrawDefaultBuffer(const VertBuffer* buffer, VertType vertType, PrimitiveType primType);
	// DrawDefaultBufferRange: Draws a range of buffered vertices
	void DrawDefaultBufferRange(const VertBuffer* buffer, int startVertIndex, int numVertsToDraw, VertType vertType, PrimitiveType primType);

	// Clear draw functions--clear the whole viewport
	void DrawClear(color32 clearColour, float clearDepth, int clearFlags = 3); // of ClearFlag

	// Shortcut draw functions. These use DrawDefaultDirect. These functions use a naming format shared with the vertices: C=Coloured, T=Textured
	void DrawLineListC(const CVertex* vertices, int numVertices);
	void DrawTriangleListC(const CVertex* vertices, int numVertices);
	void DrawTriangleListCT(const CTVertex* vertices, int numVertices);
	void DrawTriangleListCN(const CNVertex* vertices, int numVertices);

	// 2D Draw functions. These use pixel coordinates relative to the current viewport
	void DrawRectangleC(int x1, int y1, int x2, int y2, color32 color);
	void DrawRectangleT(int x1, int y1, int x2, int y2, const Texture* tex);
	void DrawRectangleCT(int x1, int y1, int x2, int y2, color32 color, const Texture* tex);
	void DrawRectangleTUV(int x1, int y1, int x2, int y2, const Texture* tex, float u1, float v1, float u2, float v2);
	void DrawRectangleCTUV(int x1, int y1, int x2, int y2, color32 color, const Texture* tex, float u1, float v1, float u2, float v2);

	void DrawText(int x, int y, int width, int height, Font* font, const tchar* string, color32 color);
	void DrawTextTransformed(int x, int y, int width, int height, Font* font, const tchar* string, color32 color, Matrix2* transform);

	// 2D debug draws
	void DrawDebug(int lineX, int lineY, const char* string, ...); // StringID should be a fixed random number for each different use of DrawDebug
	void DrawMatrixInfo(int lineX, int lineY, const char* matName, const Matrix& mat);
	void DrawMatrix(const Matrix& mat, const Vec3& offset, bool drawInfo = true);
	void DrawDebugLine2D(const Vec2& start, const Vec2& end, color32 colour = 0xFFFFFFFF, bool drawInfo = true);
	void DrawDebugPoint2D(const Vec2& position, color32 colour = 0xFFFFFFFF, bool drawInfo = true);
	void DrawDebugCircle2D(const Vec2& position, float radius, color32 colour = 0xFFFFFFFF, bool drawInfo = true);

	// 3D debug draws
	void DrawDebugVec3D(const Vec3& origin, const Vec3& vector, color32 colour = 0xFFFFFFFF, bool drawInfo = true);
	void DrawDebugLine3D(const Vec3& start, const Vec3& end, color32 colour = 0xFFFFFFFF, bool drawInfo = true);
	void DrawDebugPoint3D(const Vec3& position, color32 colour = 0xFFFFFFFF, bool drawInfo = true);
	void DrawDebugCircle3D(const Vec3& position, float radius, color32 colour = 0xFFFFFFFF, bool drawInfo = true);

	// Abandoned neglected functions (no emphasis on 's') (it's a lonely world)
	bool SetShaderOutputs(VertBuffer** outputs, int numOutputs);

	// D3D retrieval - used by resources
	inline ID3D11Device* GetD3DDevice();
	inline ID3D11DeviceContext* GetD3DDeviceContext();
	inline IDXGIFactory* GetDXGIFactory();
	inline IWICImagingFactory* GetWicFactory();

	// Debug information
	int GetNumResources() const;
	int GetNumResources(RendererResourceType type) const;

	// Default type information
	const VertexFormat* GetDefaultVertexFormat(VertType vertType);
	const Shader* GetDefaultShader(ShaderType shaderType, VertType vertType);

	// Conversion functions
	static uint32 SizeofVertType(VertType vertType);

private:
	friend class RendererResource; // Hey buddy!

	// Resource registration
	void RegisterResource(RendererResource* resource);
	void ReregisterResource(RendererResource* resourceOld, RendererResource* resourceNew);
	void UnregisterResource(RendererResource* resource);
	
	// Perspective/shader mode
	void SetDrawMode(PerspectiveMode pMode = PMODE_3D, ShaderMode sMode = SMODE_DEFAULT, VertType vertType = VT_C);
	void UpdatePerspective();

	// Render target
	void UpdateRenderTargets();

	// Temp buffers
	ID3D11Buffer* CreateTempVertBuffer(const void* initData, int vertexSize, int numVertices);

	// Misc
	void DrawPrimitives(int startVert, int numVertices, PrimitiveType primType);
	void D3DDrawDefaultBufferRange(ID3D11Buffer* buffer, int startVert, int numVertsToDraw, VertType vertType, PrimitiveType primType);

	// Debug draws
	DebugObject* AddDebugObject(int8 type, color32 colour, bool is3D, bool drawInfo);
	int DrawDebugObject(CVertex* verts, int curVert, const DebugObject* obj); // Returns the number of vertices drawn

	// VARIABLES----------------
	bool8 init;

	// Main API declarations
	ID3D11Device* d3dDev;
	ID3D11DeviceContext* d3dDraw;

	IDXGIFactory* dxgiMain;

	IWICImagingFactory* wicMain;

	// State variables
	Viewport* activeViewport; // Current target viewport
	RenderTarget* activeRenderTargets[MAXRENDERTARGETS]; // Current render targets, overlaps with viewport most of the time
	uint32 numActiveRenderTargets;

	int8 perspectiveMode; // Perspective mode as selected by the user. Differs from perspectiveMode in the sense that perspectiveMode changes with every 2D draw.
	int8 activePerspectiveMode, activeShaderMode; // Current modes, sometimes toggled by draw functions
	int8 activeVertType;
	bool8 activeDefaultVertexBuffers; // Whether or not the default vertex buffers are currently bound

	const Shader* currentCustomShaders[NUMSHADERTYPES];
	const VertexFormat* currentCustomVertFormat;

	Matrix matViewProj; // Default view/proj in 3D mode

	void* activeDxShaders[NUMSHADERTYPES];

	// Default shader cbuffers
	CBufferRaw *cbView, *cbObject, *cbLights;

	// Draw shortcut variables
	CTVertex ctRect2D[6];
	VertBuffer* scratchVertsC;
	VertBuffer* scratchVertsCT;

	// Misc variables
	DebugString debugStrings[MAXDEBUGSTRINGS];
	DebugObject debugObjects[MAXDEBUGOBJECTS];
	Font* debugFont;

	int32 numDebugObjects;

	// Resources
	Shader* defaultShaders[NUMDEFSHADERS]; // contains pointers to the reserved default shaders
	ShaderSet* defaultShaderSets[NUMVERTTYPES]; // contains shader sets for each default vertex type (VT_*)
	VertexFormat* defaultVertexFormats[NUMVERTTYPES]; // contains pointers to the vertex formats of the reserved default shaders
	uint32 defaultVertexStrides[NUMVERTTYPES]; // contains the sizes of vertices by their type

	ID3D11SamplerState* testSampler;

	// List of resources linked to this renderer
	Array<RendererResource*> resources;
};

enum LightType {
	LT_AMBIENT = 1,
	LT_DIRECTIONAL = 2
};

enum ClearFlag {
	CLEAR_COLOUR = 1,
	CLEAR_DEPTH = 2,
	CLEAR_BOTH = 3
};


inline ID3D11Device* Renderer::GetD3DDevice() {
	return d3dDev;
}

inline ID3D11DeviceContext* Renderer::GetD3DDeviceContext() {
	return d3dDraw;
}

inline IDXGIFactory* Renderer::GetDXGIFactory() {
	return dxgiMain;
}

inline IWICImagingFactory* Renderer::GetWicFactory() {
	return wicMain;
}

inline int Renderer::GetNumResources() const {
	return resources.GetNum();
}

inline int Renderer::GetNumResources(RendererResourceType type) const {
	int count = 0;

	for (int i = 0, e = resources.GetNum(); i < e; i++) {
		if (resources[i]->GetType() == type)
			count++;
	}

	return count;
}