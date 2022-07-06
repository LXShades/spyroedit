#pragma once
#include "sdSystem.h" // remove when sdError gets its own class
#include "sdError.h"
#include "sdMath.h"
#include "sdWindow.h"

#include <dxgiformat.h> // todo (probably)

/* Default vertex struct names are prepended with any combination of the following letters in alphabetical order. They represent:
	C: Coloured (contains colour data)
	T: Textured (contains texture coordinates)
	N: Normals (contains normals for lighting)

	All vertex types have position data as a vector/xyz union
*/

/* Note: Load functions upload data to the GPU. Unload functions are also sometimes provided, but are not always necessary.
As a general rule, it's always safe to call Load multiple times without calling Unload. */

class Renderer;
class Viewport; class Texture; class Shader;
struct VertexFormatElement;
__interface ID3D11Texture2D; __interface ID3D11ShaderResourceView;  __interface ID3D11Buffer;
__interface IDXGISwapChain; __interface ID3D11RenderTargetView; __interface ID3D11DepthStencilView; __interface ID3D11DepthStencilState; __interface ID3D11InputLayout;
__interface ID3D11GeometryShader; __interface ID3D11VertexShader; __interface ID3D11PixelShader;
__interface ID3D11Device; __interface ID3D11DeviceContext; __interface IDXGIFactory; __interface IWICImagingFactory;

enum RendererResourceType : int8 {
	RRSRC_TEXTURE = 1,
	RRSRC_VIEWPORT,
	RRSRC_FONT,
	RRSRC_RENDERTARGET,
	RRSRC_SHADER,
	RRSRC_VERTEXFORMAT,
	RRSRC_SHADERSET,
	RRSRC_CBUFFER,
	RRSRC_VERTBUFFER,
};

enum ShaderType : int8 {
	ST_VERTEX,
	ST_GEOMETRY,
	ST_PIXEL,
	NUMSHADERTYPES,
};

// Resource classes
class RendererResource {
public:
	// Abstract constructor: Construct empty resource with no links to a renderer
	RendererResource() : renderer(nullptr) {};

	// Standard constructor: Constructs resource and links to a renderer
	RendererResource(Renderer* parent) : renderer(nullptr) {
		RegisterRenderer(parent);
	};

	// Move constructor: Informs renderer of a move
	// Finish later?
	RendererResource(RendererResource&& moveFrom) noexcept {
		renderer = moveFrom.renderer;
		moveFrom.renderer = nullptr;

		if (renderer) {
			ReregisterRenderer(&moveFrom);
		}
	}

	// RegisterRenderer: Registers the parent renderer of this resource
	void RegisterRenderer(Renderer* parent);

	// UnregisterRenderer: Unregisters the parent renderer if applicable
	void UnregisterRenderer();

	// ReregisterRenderer: Informs the renderer of a move
	void ReregisterRenderer(RendererResource* source);

	// Destroy: Destroys this resource.
	// This function must be available for all resources. It should be callable multiple times without consequence. It should also call UnregisterRenderer().
	virtual void Destroy() = 0;

	// GetType: Returns the type of resource
	virtual RendererResourceType GetType() const = 0;

protected:
	// Device retrieval functions for resources
	Renderer* GetRenderer() {
		return renderer;
	}

	ID3D11Device* GetD3DDevice();
	ID3D11DeviceContext* GetD3DDeviceContext();
	IDXGIFactory* GetDXGIFactory();
	IWICImagingFactory* GetWicFactory();
private:
	Renderer* renderer;
};

class Texture : RendererResource {
	friend class Renderer; friend class Viewport;

public:
	Texture() : RendererResource(nullptr), locked(false), loaded(false), bitmap(nullptr), texture(nullptr), resView(nullptr), width(0), height(0) {};

	Texture(Renderer* parent) : RendererResource(parent), locked(false), loaded(false), bitmap(nullptr), texture(nullptr), resView(nullptr), width(0), height(0) {};
	Texture(Renderer* parent, const wchar* filename) : RendererResource(parent), locked(false), loaded(false), bitmap(nullptr), texture(nullptr), resView(nullptr), width(0), height(0) {
		Create(parent, filename);
	}
	Texture(Renderer* parent, int width, int height, const void* initBitmap) : Texture() {
		Create(parent, width, height, initBitmap);
	}

	~Texture() {
		Destroy();
	}

	// Creation/destruction
	bool Create(Renderer* parent, int width, int height, const void* initBitmap);
	bool Create(Renderer* parent, const wchar* filename);
	void Destroy();

	// Get dimensions
	int GetWidth() const;
	int GetHeight() const;

	// Set dimensions (fails if bits are locked)
	bool SetDimensions(int width, int height);

	// Access raw bits
	bool LockBits(void** bitsOut);
	bool UnlockBits(bool load = true);

	// Load
	bool LoadImageFromFile(const wchar* filename);

	// GPU upload/download
	bool Load();
	bool LoadArea(int x1, int y1, int x2, int y2);
	void Unload();

	RendererResourceType GetType() const {
		return RRSRC_TEXTURE;
	}

private:
	int16 width, height;

	bool8 locked, loaded; // see what i did there lawl
	int32 loadedWidth, loadedHeight;

	void* bitmap;
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* resView;
};

class Viewport : RendererResource {
	friend Renderer;
	public:
		Viewport() : Viewport(nullptr) {};
		Viewport(Renderer* parent) : RendererResource(parent), swapChain(nullptr), renderTargetView(nullptr), depthStencilView(nullptr), depthStencilState(nullptr), depthStencil(nullptr), readableDepthStencil(nullptr), textSurface(parent),
					depthStencilSize(0), vpHwnd(0), window(nullptr) {};
		Viewport(Renderer* parent, const Window* window, int x, int y, int width, int height) : Viewport(parent) {
			Create(parent, window, x, y, width, height);
		};

		~Viewport() {
			Destroy();
		}

		// Creation/destruction
		bool Create(Renderer* parent, const Window* window, int x, int y, int width, int height);
		void Destroy();

		// Resize: Resizes the internal viewport (not the external viewable area). NOTE: This recreates the viewport meaning the image on it will be destroyed.
		bool Resize(int width, int height);

		// SetViewArea: Resizes the external viewport
		bool ResizeView(int x, int y, int width, int height);

		// Get dimensions
		int GetWidth() const;
		int GetHeight() const;
		int GetViewWidth() const;
		int GetViewHeight() const;

		// GetDepthStencilData: Returns current depth stencil data. Accepts varied dimensions and corrects them if necessary. dataPitchOut returns the number of elements
		// (e.g. uint32) per row.
		bool GetDepthStencilData(uint32* dataOut, uint32* dataPitchOut, uint32* widthOut, uint32* heightOut);
		int GetDepthStencilSize();

		RendererResourceType GetType() const {
			return RRSRC_VIEWPORT;
		}

	private:
		int32 x, y;
		int32 width, height; // Dimensions in pixels
		int32 viewWidth, viewHeight; // Dimensions of visible area in pixels
		float32 widthConvert, heightConvert; // Conversion variables. For pixel-to-screen* location conversion, use e.g. pixel * convert - 1.0f (*depends on view size)

		uint32 depthStencilSize;

		const Window* window; // Window this viewport is attached to
		HWND vpHwnd; // HWND of the actual viewport

		// D3D11 resources
		IDXGISwapChain* swapChain;
		ID3D11RenderTargetView* renderTargetView;
		ID3D11DepthStencilView* depthStencilView;
		ID3D11DepthStencilState* depthStencilState;
		ID3D11Texture2D* depthStencil;
		ID3D11Texture2D* readableDepthStencil; // Optional readable copy of the depth stencil; may be nullptr
		Texture textSurface; // Surface for text draws

		// Individual component init functions
		bool InitInternals();
		void DeinitInternals();
		bool _InitSwapChain();
		bool _InitRenderTarget();
		bool _InitTextSurface();
		bool _InitDepthStencil();
};

class RenderTarget : RendererResource {
	friend Renderer;
	public:
		RenderTarget(Renderer* parent) : RendererResource(parent), renderTarget(nullptr), depthStencil(nullptr),readableRenderTarget(nullptr), readableDepthStencil(nullptr), renderTargetView(nullptr), 
										depthStencilView(nullptr), depthStencilState(nullptr) {};

		~RenderTarget() {
			Destroy();
		}

		bool Create(Renderer* parent, int width, int height, DXGI_FORMAT targetFormat, DXGI_FORMAT depthFormat);
		void Destroy();

		bool Resize(int width, int height);

		void Clear(bool clearTarget, bool clearDepthStencil, color32 clearTargetClr, float32 clearDepthValue, uint8 clearStencilValue);

		RendererResourceType GetType() const {
			return RRSRC_RENDERTARGET;
		}

	private:
		int32 width, height; // Dimensions in pixels
		uint32 targetFormat, depthFormat;

		// D3D11 resources
		ID3D11Texture2D* renderTarget;
		ID3D11Texture2D* depthStencil;
		ID3D11Texture2D* readableRenderTarget; // Optional readable copy of the render target; may be nullptr
		ID3D11Texture2D* readableDepthStencil; // Optional readable copy of the depth stencil; may be nullptr

		ID3D11RenderTargetView* renderTargetView;
		ID3D11DepthStencilView* depthStencilView;
		ID3D11DepthStencilState* depthStencilState;

		// Individual component init functions
		bool _InitRenderTarget();
		bool _InitDepthStencil();
};

class Font : RendererResource {
	friend Renderer;
	public:
		Font(Renderer* parent) : RendererResource(parent), winFont(nullptr) {};
		Font(Renderer* parent, const wchar* fontName, int fontSize) : RendererResource(parent) {
			Create(parent, fontName, fontSize);
		};

		~Font() {
			Destroy();
		}

		bool Create(Renderer* parent, const wchar* fontName, int fontSize);
		void Destroy();

		bool GetTextDimensions(int32* widthOut, int32* heightOut, const tchar* string); // widthOut and/or heightOut may be nullptr.

		RendererResourceType GetType() const {
			return RRSRC_FONT;
		}

	private:
		HFONT winFont;
};

class VertexFormat : RendererResource {
	friend Renderer;
	public:
		VertexFormat(Renderer* parent) : RendererResource(parent) {};
		VertexFormat(Renderer* parent, const VertexFormatElement* vfe, int numElements, const Shader* shader) : RendererResource(parent) {
			Create(parent, vfe, numElements, shader);
		}

		~VertexFormat() {
			Destroy();
		}

		// Creation/destruction
		bool Create(Renderer* parent, const VertexFormatElement* vfe, int numElements, const Shader* shader);
		void Destroy();

		RendererResourceType GetType() const {
			return RRSRC_VERTEXFORMAT;
		}
	private:
		uint8 numElements;
		uint16 vertSize;

		ID3D11InputLayout* inputLayout;
};

class Shader : RendererResource {
	friend VertexFormat; friend Renderer;
	public:
		Shader(Renderer* parent) : RendererResource(parent) {};
		Shader(Renderer* parent, const wchar* resourceName, int resourceType, ShaderType shaderType) : RendererResource(parent) {
			CreateFromResource(parent, resourceName, resourceType, shaderType);
		};

		~Shader() {
			Destroy();
		}

		bool CreateFromFile(Renderer* parent, ShaderType shaderType, const wchar* filename);
		bool CreateFromData(Renderer* parent, ShaderType shaderType, const void* shaderData, int shaderSize);
		bool CreateFromShaderInterface(Renderer* parent, ShaderType shaderType, const void* shaderPointer);
		bool CreateFromResource(Renderer* parent, const wchar* resourceName, int resourceType, ShaderType shaderType);
		void Destroy();

		RendererResourceType GetType() const {
			return RRSRC_SHADER;
		}

	private:
		void* data;
		int32 dataSize;

		ShaderType shaderType;
	
		union {
			ID3D11VertexShader* vertShader;
			ID3D11GeometryShader* geoShader;
			ID3D11PixelShader* pixelShader;
		};
};

class ShaderSet : RendererResource {
	friend Renderer;
	// ShaderSet: A set of pointers to shaders (owned by the parent Renderer) which the renderer can switch using SetShaders.
	// There isn't yet any performance advantage to using these, just the the convenience of having specific combinations of shaders for the right moments.
	// Any shaders that are nullptr are simply not changed during draws. This may lead to undefined effects.
	/* Using a ShaderSet is a convenient alternative, and nothing more. The Renderer is not 'locked' to a ShaderSet; that is, if one of the shaders is changed during a draw,
	   the change occurs as normal and other shader pointers are unchanged (and possibly used in the next call).
	   Similarly, if a shader in a ShaderSet is replaced after Renderer::SetShaders() has been called, this change does not take effect unless SetShaders is called again. */
	public:
		ShaderSet(Renderer* parent) : RendererResource(parent), shaders{}, vertFormat(nullptr) {};
		ShaderSet(Renderer* parent, const Shader* vertShader, const Shader* geoShader, const Shader* pixelShader, const VertexFormat* vertFormat) : RendererResource(parent) {
			Create(parent, vertShader, geoShader, pixelShader, vertFormat);
		};

		~ShaderSet() {
			Destroy();
		}

		void Create(Renderer* parent, const Shader* vertShader, const Shader* geoShader, const Shader* pixelShader, const VertexFormat* vertFormat);
		void Destroy();

		void SetShader(const Shader* shader, const ShaderType shaderType);

		void SetVertexFormat(const VertexFormat* vertFormat);

		RendererResourceType GetType() const {
			return RRSRC_SHADERSET;
		}

	private:
		const Shader* shaders[NUMSHADERTYPES];
		const VertexFormat* vertFormat;
};

template <typename StructType>
class CBuffer : public CBufferRaw {
	friend Renderer;
public:
	// Construct detault-initialised CBuffer
	CBuffer(Renderer* parent) : CBufferRaw(parent) {
		Create(parent, nullptr, sizeof (StructType));
	};

	// Construct copy-initialised CBuffer
	CBuffer(Renderer* parent, const StructType& initialData) : CBufferRaw(parent) {
		Create(parent, &initialData, sizeof (StructType));
	};

	// Modify CBuffer
	inline StructType* Lock() {
		return (StructType*) CBufferRaw::Lock();
	}
};

class CBufferRaw : RendererResource {
	friend Renderer; // grant Renderer direct access (for now)
	public:
		CBufferRaw(Renderer* parent) : RendererResource(parent), data(nullptr), size(0), physicalSize(0), locked(false), buffer(nullptr) {};
		CBufferRaw(Renderer* parent, const void* initialData, int dataSize) : RendererResource(parent) {
			Create(parent, initialData, dataSize);
		};

		~CBufferRaw() {
			Destroy();
		}

		bool Create(Renderer* parent, const void* initialData, int dataSize);
		void Destroy();

		bool Load();

		void* Lock();
		bool Unlock(bool load = true);

		RendererResourceType GetType() const {
			return RRSRC_CBUFFER;
		}

	private:
		void* data;

		int32 size;
		int32 physicalSize; // multiple of 16

		bool8 locked;

		ID3D11Buffer* buffer;
};

class VertBuffer : RendererResource {
	friend Renderer;
	public:
		VertBuffer(Renderer* parent) : RendererResource(parent), bufferFlags(0), mappable(false), data(nullptr), size(0), vertSize(0), buffer(nullptr), locked(false) {};
		VertBuffer(Renderer* parent, const void* initData, int numVerts, int sizePerVertex, int vertBufferFlags = 0) : RendererResource(parent) {
			Create(parent, initData, numVerts, sizePerVertex, vertBufferFlags);
		};

		~VertBuffer() {
			Destroy();
		}

		bool Create(Renderer* parent, const void* initData, int numVerts, int sizePerVertex, int vertBufferFlags = 0);
		void Destroy();

		bool Load();

		void* Lock();
		bool Unlock(bool load = true);

		int32 GetSize() const;

		bool CopyData(VertBuffer* source);

		RendererResourceType GetType() const {
			return RRSRC_VERTBUFFER;
		}

	private:
		uint16 bufferFlags;
		bool8 mappable;

		void* data; // Buffer data (local version)
		
		int32 size; // Size of buffer
		int32 vertSize; // Size of each vertex in buffer

		ID3D11Buffer* buffer; // Buffer data (D3D version)
		bool8 locked;
};

// Default vertex types
struct CVertex { // Colour vertex
	union {
		Vec3 pos;
		struct {float32 x, y, z;};
	};

	color32 clr;
};

struct CTVertex { // Coloured textured vertex
	union {
		Vec3 pos;
		struct {float32 x, y, z;};
	};

	color32 clr;
	float32 u, v;
};

struct CNVertex { // Coloured normal'd vertex
	union {
		Vec3 pos;
		struct {float32 x, y, z;};
	};

	color32 clr;
	Vec3 nml;
};

struct CTNVertex { // Coloured textured normal'd vertex
	union {
		Vec3 pos;
		struct {float32 x, y, z;};
	};

	color32 clr;
	float32 u, v;
	Vec3 nml;
};

// Vertex format element
struct VertexFormatElement {
	const char* semanticName;
	uint8 semanticIndex;
	uint16 byteOffset;
	uint8 dxgiFormat;
};

// Public enums
enum VertType {
	VT_UNK = 0,
	VT_C = 1,
	VT_T = 2,
	VT_CT = 3,
	VT_CN = 4,
	VT_CTN = 5,
	NUMVERTTYPES
};

enum VertBufferFlags {
	VBF_STANDARD = 0x00,
	VBF_STREAMOUTPUT = 0x01,

	VBF_READABLE = 0x100,
	VBF_WRITABLE = 0x200,
};

enum PrimitiveType { // NOTE: IF THIS IS CHANGED BE SURE TO UPDATE const ptToTopology.
	PT_POINTLIST = 1,
	PT_LINESTRIP = 2,
	PT_LINELIST = 3,
	PT_TRIANGLESTRIP = 4,
	PT_TRIANGLELIST = 5,
};

enum RenderResourceError {
	SDRE_D3DBUFFERFAILED = 1,
	SDRE_ALREADYLOCKED,
	SDRE_D3DMAPFAILED, 
	SDRE_NOACCESS,
};

// Private enums
enum DefShader {
	DEFSHADER_CVERTEX = 0,
	DEFSHADER_CTVERTEX = 1,
	DEFSHADER_CNVERTEX = 2,
	DEFSHADER_CTNVERTEX = 3,

	DEFSHADER_CPIXEL = 4,
	DEFSHADER_CTPIXEL = 5,
	NUMDEFSHADERS = 6
};

enum PerspectiveMode {
	PMODE_UNDEFINED = 0,
	PMODE_USER = 1,
	PMODE_2D = 2,
	PMODE_3D = 3
};
