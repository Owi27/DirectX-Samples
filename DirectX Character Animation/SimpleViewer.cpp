//--------------------------------------------------------------------------------------
// File: Tutorial06.cpp
//
// This application demonstrates simple lighting in the vertex shader
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729723.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include "resource.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "MeshUtils.h"
#include "LineUtils.h"
#include "Renderable.h"
#include "LoaderUtils.h"
#include "debug_renderer.h"
#include "math_types.h"


using namespace DirectX;
using namespace std;
using namespace end;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
bool RENDER_STYLE_WIREFRAME = false;
bool RENDER_STYLE_TEXTURED = true;
bool RENDER_STYLE_TRANSPARENCY = false;
bool RASTER_FILL_CULL_NONE = false;
bool DEPTH_WRITE_ENABLED = true;
bool DEBUG_VIEW_ENABLED = true;
bool SKYBOX_ENABLED = false;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11Device1* g_pd3dDevice1 = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
ID3D11DeviceContext1* g_pImmediateContext1 = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
IDXGISwapChain1* g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D* g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11PixelShader* g_pPixelShaderSolid = nullptr;
XMMATRIX                g_World;
XMMATRIX                g_View;
XMMATRIX                g_Projection;
ID3D11RasterizerState* rasterStateDefault;
ID3D11RasterizerState* rasterStateWireframe;
ID3D11RasterizerState* rasterStateFillNoCull;
ID3D11BlendState* transparencyState;
ID3D11DepthStencilState* pDSState;
ID3D11DepthStencilState* pDSStateNoWrite;
ID3D11DepthStencilState* pDSStateNoTest;

TransformsConstantBuffer modelViewProjection;
LightsConstantBuffer lightsAndColor;

ID3D11Buffer* vertex_buffer;

vector<Renderable> renderables;
// Main mesh
Renderable skinnedRenderable;
anim_clip_t anim_clip;

struct alignas(16) joint_deltas_t
{
	XMMATRIX m[67];
};
joint_deltas_t joint_deltas;
ID3D11Buffer* joint_deltas_CB = nullptr;

// Grid mesh
Renderable gridRenderable;

// Main mesh
Renderable skyboxRenderable;

// Used for overriding the main mesh texture
// with a generated white pixel texture to
// simulate no texturing
ComPtr<ID3D11ShaderResourceView> texSRV;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
HRESULT InitContent();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Update();
void Render();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// enable console
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	if (FAILED(InitContent()))
	{
		CleanupDevice();
		return 0;
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update();
			Render();
		}
		//Sleep(1);
	}

	CleanupDevice();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 1024, 768 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Simple Viewer",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

void InitDebugTexture()
{
	static const uint32_t s_pixel = 0xffffffff;

	D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	ComPtr<ID3D11Texture2D> tex;
	HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &initData, tex.GetAddressOf());

	if (SUCCEEDED(hr))
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		hr = g_pd3dDevice->CreateShaderResourceView(tex.Get(),
			&SRVDesc, texSRV.GetAddressOf());
	}
	assert(!FAILED(hr));
}

void InitRasterizerStates()
{
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc, sizeof(rasterDesc));

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	HRESULT hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc, &rasterStateWireframe);
	assert(!FAILED(hr));

	ZeroMemory(&rasterDesc, sizeof(rasterDesc));

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc, &rasterStateDefault);
	assert(!FAILED(hr));

	ZeroMemory(&rasterDesc, sizeof(rasterDesc));

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	hr = g_pd3dDevice->CreateRasterizerState(&rasterDesc, &rasterStateFillNoCull);
	assert(!FAILED(hr));
}

void InitDepthStates()
{
	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;// D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSStateNoWrite);
	// g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSState);
	// g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);

	// Depth test parameters
	dsDesc.DepthEnable = false;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSStateNoTest);
}

HRESULT InitSkybox()
{
	HRESULT hr = S_OK;

	//////////////////////////////////////////
	//Create mesh render components
	//////////////////////////////////////////
	{
		// Generate the geometry
		SimpleMesh<SimpleVertex> mesh;
		MeshUtils::makeCubePNT(mesh);

		// Create the vertex buffers from the generated SimpleMesh
		hr = skyboxRenderable.CreateBuffers(
			g_pd3dDevice,
			mesh.indicesList,
			(float*)mesh.vertexList.data(),
			sizeof(SimpleVertex),
			mesh.vertexList.size());

		// Load the Texture
		std::string filename = "sunsetcube1024.dds";
		hr = skyboxRenderable.CreateTextureFromFile(g_pd3dDevice, filename);
		if (FAILED(hr))
			return hr;

		// Create the sampler state
		hr = skyboxRenderable.CreateDefaultSampler(g_pd3dDevice);

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		// Create the shaders
		hr = skyboxRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Skybox_VS.cso", layout, ARRAYSIZE(layout));
		hr = skyboxRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Skybox_PS.cso");

		// Create the shader constant buffer
		hr = skyboxRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
	}
	return hr;
}

void InitBlendState()
{
	//Define the Blending Equation
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));

	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	ZeroMemory(&rtbd, sizeof(rtbd));

	rtbd.BlendEnable = true;
	int ALPHA_MODE = 0;
	switch (ALPHA_MODE)
	{
	case 0: // object alpha
	{
		rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.AlphaToCoverageEnable = false;
		break;
	}
	case 1: // pixel alpha
	{
		rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.AlphaToCoverageEnable = true;
		break;
	}
	case 2: // additive (emissive)
	{
		rtbd.SrcBlend = D3D11_BLEND_ONE;
		rtbd.DestBlend = D3D11_BLEND_ONE;
		blendDesc.AlphaToCoverageEnable = false;
		break;
	}
	}
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	blendDesc.RenderTarget[0] = rtbd;
	g_pd3dDevice->CreateBlendState(&blendDesc, &transparencyState);
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	// Create swap chain
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		// DirectX 11.1 or later
		hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
		if (SUCCEEDED(hr))
		{
			(void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;

		hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
		}

		dxgiFactory2->Release();
	}
	else
	{
		// DirectX 11.0 systems
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = g_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
	}

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 1000.0f);

	return hr;
}

void create_debug_line_buffer()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = (UINT)(sizeof(colored_vertex) * debug_renderer::get_line_vert_capacity());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	HRESULT hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &vertex_buffer);
}

HRESULT InitContent()
{
	InitDebugTexture();
	InitRasterizerStates();
	InitDepthStates();
	InitSkybox();
	InitBlendState();
	InitFBX();
	create_debug_line_buffer();

	//modelViewProjection = new ConstantBufferTransforms();

	HRESULT hr = S_OK;

	//////////////////////////////////////////
	//Create mesh render components
	//////////////////////////////////////////
	if (false) {
		Renderable meshRenderable;

		// Generate the geometry
		SimpleMesh<SimpleVertex> mesh;

		// filename for texture file
		std::string filename;

		// Load it!
		//LoadFBX(".//Assets//cube.fbx", mesh, filename);
		scale = 0.02f;
		//		LoadFBX(".//Assets//Chest1-1.fbx", mesh, filename);
		scale = 0.01f;
		//LoadFBX(".//Assets//raft_tris.fbx", mesh, filename);
		//filename = "Texture_Raft.dds";
		//LoadFBX(".//Assets//duck_tris.fbx", mesh, filename);
		LoadFBX(".//Assets//rock01.fbx", mesh, filename);

		//LoadFBXAnimation(".//Assets//Run.fbx", mesh, filename, anim_clip);

		// Create the vertex buffers from the generated SimpleMesh
		hr = meshRenderable.CreateBuffers(
			g_pd3dDevice,
			mesh.indicesList,
			(float*)mesh.vertexList.data(),
			sizeof(SimpleVertex),
			mesh.vertexList.size());

		// Load the Texture when texture filename is valid
		if (filename != "")
		{
			hr = meshRenderable.CreateTextureFromFile(g_pd3dDevice, ".//Assets//" + filename);
			if (FAILED(hr))
				return hr;

			// Create the sampler state
			hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		}

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		// Create the shaders
		hr = meshRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Tutorial06_VS.cso", layout, ARRAYSIZE(layout));
		hr = meshRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Tutorial06_PS.cso");

		// Create the shader constant buffer
		hr = meshRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		hr = meshRenderable.CreateConstantBufferPS(g_pd3dDevice, sizeof(LightsConstantBuffer));
		meshRenderable.setPosition(-3.0f, 0.0f, 0.0f);
		renderables.push_back(meshRenderable);
		meshRenderable.setPosition(3.0f, 0.0f, 0.0f);
		//meshRenderable.setRotation(XMMatrixRotationY(3.14159265359f));
		renderables.push_back(meshRenderable);

		//meshRenderable.setPosition(0.0f, 0.0f, 1.0f);
		//meshRenderable.setRotation(XMMatrixRotationY(3.14159265359f));
		//renderables.push_back(meshRenderable);
	}

	// Create grid render components
	{
		// Generate the geometry
		DebugLines lines;
		LineUtils::MakeGrid(lines);

		// Create the vertex buffers from the lines vector
		hr = gridRenderable.CreateVertexBuffer(g_pd3dDevice, (float*)lines.vertexList.data(), sizeof(ColorVertex), (int)lines.vertexList.size());

		gridRenderable.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC lineLayoutDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		// Create the shaders
		hr = gridRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Debug_VS.cso", lineLayoutDesc, ARRAYSIZE(lineLayoutDesc));
		hr = gridRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Debug_PS.cso");

		// Create the shader constant buffer
		hr = gridRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
	}

	// load and create the pixel shader for the light markers
	auto ps_blob = load_binary_blob("PSSolid.cso");
	hr = g_pd3dDevice->CreatePixelShader(ps_blob.data(), ps_blob.size(), nullptr, &g_pPixelShaderSolid);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->RSSetState(rasterStateDefault);

	//skinnedRenderable;
	{
		Renderable meshRenderable;

		// Generate the geometry
		SimpleMesh<SkinnedVertex> mesh;

		// filename for texture file
		std::string filename;

		// Load it!
		scale = 1.00f; // must be 1.0f
		LoadFBXAnimation(".//Assets//Run.fbx", mesh, filename, anim_clip);

		// Create the vertex buffers from the generated SimpleMesh
		hr = meshRenderable.CreateBuffers(
			g_pd3dDevice,
			mesh.indicesList,
			(float*)mesh.vertexList.data(),
			sizeof(SkinnedVertex),
			mesh.vertexList.size());

		// Load the Texture when texture filename is valid
		if (filename != "")
		{
			hr = meshRenderable.CreateTextureFromFile(g_pd3dDevice, ".//Assets//" + filename);
			if (FAILED(hr))
				return hr;

			// Create the sampler state
			hr = meshRenderable.CreateDefaultSampler(g_pd3dDevice);
		}

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		// Create the shaders
		hr = meshRenderable.CreateVertexShaderAndInputLayoutFromFile(g_pd3dDevice, "Skinned_VS.cso", layout, ARRAYSIZE(layout));
		hr = meshRenderable.CreatePixelShaderFromFile(g_pd3dDevice, "Tutorial06_PS.cso");

		// Create the shader constant buffer
		hr = meshRenderable.CreateConstantBufferVS(g_pd3dDevice, sizeof(TransformsConstantBuffer));
		hr = meshRenderable.CreateConstantBufferPS(g_pd3dDevice, sizeof(LightsConstantBuffer));

		D3D11_BUFFER_DESC mvp_bd;
		ZeroMemory(&mvp_bd, sizeof(mvp_bd));

		mvp_bd.Usage = D3D11_USAGE_DEFAULT;
		mvp_bd.ByteWidth = sizeof(joint_deltas_t);
		mvp_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		mvp_bd.CPUAccessFlags = 0;

		hr = g_pd3dDevice->CreateBuffer(&mvp_bd, NULL, &joint_deltas_CB);

		assert(!FAILED(hr));

		meshRenderable.setPosition(0.0f, 0.0f, 0.0f);
		skinnedRenderable = meshRenderable;
	}

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (rasterStateDefault) rasterStateDefault->Release();
	if (rasterStateWireframe) rasterStateWireframe->Release();
	if (rasterStateFillNoCull) rasterStateFillNoCull->Release();
	if (g_pPixelShaderSolid) g_pPixelShaderSolid->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain1) g_pSwapChain1->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext1) g_pImmediateContext1->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice1) g_pd3dDevice1->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
	if (pDSState) pDSState->Release();
	if (pDSStateNoWrite) pDSStateNoWrite->Release();
	if (transparencyState) transparencyState->Release();

	if (pDSStateNoTest) pDSStateNoTest->Release();
	if (vertex_buffer) vertex_buffer->Release();
	if (joint_deltas_CB) joint_deltas_CB->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case '1':
			cout << "Keypressed - 1" << endl;
			// toggle wireframe state
			RENDER_STYLE_WIREFRAME = !RENDER_STYLE_WIREFRAME;
			cout << "RENDER_STYLE_WIREFRAME: " << RENDER_STYLE_WIREFRAME << endl;
			break;
		case '2':
			cout << "Keypressed - 2" << endl;
			// toggle textured state
			RENDER_STYLE_TEXTURED = !RENDER_STYLE_TEXTURED;
			cout << "RENDER_STYLE_TEXTURED: " << RENDER_STYLE_TEXTURED << endl;
			break;
		case '3':
			cout << "Keypressed - 3" << endl;
			// toggle alpha blending state
			RENDER_STYLE_TRANSPARENCY = !RENDER_STYLE_TRANSPARENCY;
			cout << "RENDER_STYLE_TRANSPARENCY: " << RENDER_STYLE_TRANSPARENCY << endl;
			break;
		case '4':
			cout << "Keypressed - 4" << endl;
			// toggle cull front/none state
			RASTER_FILL_CULL_NONE = !RASTER_FILL_CULL_NONE;
			cout << "RASTER_FILL_CULL_NONE: " << RASTER_FILL_CULL_NONE << endl;
			break;
		case '5':
			cout << "Keypressed - 5" << endl;
			// toggle cull front/none state
			DEPTH_WRITE_ENABLED = !DEPTH_WRITE_ENABLED;
			cout << "DEPTH_WRITE_ENABLED: " << DEPTH_WRITE_ENABLED << endl;
			break;
		case '0':
			cout << "Keypressed - 0" << endl;
			// toggle skybox
			SKYBOX_ENABLED = !SKYBOX_ENABLED;
			cout << "SKYBOX_ENABLED: " << SKYBOX_ENABLED << endl;
			break;
		case VK_TAB:
			cout << "Keypressed - [TAB]" << endl;
			// toggle debug view
			DEBUG_VIEW_ENABLED = !DEBUG_VIEW_ENABLED;
			cout << "DEBUG_VIEW_ENABLED: " << DEBUG_VIEW_ENABLED << endl;
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

XMFLOAT4 vLightDirs[2] =
{
	XMFLOAT4(-0.577f, 0.577f, -0.577f, 1.0f),
	XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f),
};
XMFLOAT4 vLightColors[2] =
{
	XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f),
	XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
};

float last_time = 0.0f;
float delta_time = 0.0f;
void compute_global_delta_time(float time)
{
	delta_time = time - last_time;
	last_time = time;
}

void interpolate_transform(const float4x4& xform1, const float4x4& xform2, const float tween_ratio, float4x4& lerp_transform)
{
	XMVECTOR q1 = XMQuaternionRotationMatrix((const XMMATRIX&)xform1);
	XMVECTOR q2 = XMQuaternionRotationMatrix((const XMMATRIX&)xform2);
	XMVECTOR qt = XMQuaternionSlerp(q1, q2, tween_ratio);

	lerp_transform = (float4x4&)XMMatrixRotationQuaternion(qt);
	lerp_transform[3].xyz = xform1[3].xyz + (xform2[3].xyz - xform1[3].xyz) * tween_ratio;
}

void debug_render_skeleton(const joint_set_t& pose_joints, const float joint_scale)
{
	for (int j = 1; j < pose_joints.size(); ++j)
	{
		int parent = pose_joints[j].parent;

		debug_renderer::add_line(
			pose_joints[j].transform[3].xyz * joint_scale,
			pose_joints[parent].transform[3].xyz * joint_scale,
			{ 1.0f, 1.0f, 1.0f });
	}

	// draw joint markers
	float marker_scale = 0.275;
	for (int j = 0; j < pose_joints.size(); ++j)
	{
		float4x4 xform = pose_joints[j].transform;
		xform[0].xyz *= marker_scale;
		xform[1].xyz *= marker_scale;
		xform[2].xyz *= marker_scale;
		xform[3].xyz *= joint_scale;

		debug_renderer::add_transform(xform);
	}
}

void animate_character()
{
	/*
	float joint_scale = 0.75f;
	debug_render_skeleton(anim_clip.keyframes[0].joints, joint_scale);
	
	static float anim_timer = anim_clip.keyframes[1].time;

	anim_timer += delta_time * 0.5f;

	if (anim_timer > anim_clip.duration)
	{
		anim_timer -= anim_clip.duration;
		for (size_t i = 0; i < anim_clip.keyframes.size(); i++)
		{
			anim_timer = anim_clip.keyframes[i].time;
			debug_render_skeleton(anim_clip.keyframes[i].joints, joint_scale);
		}
	}*/
	//joint_set_t pose_joints;

	//// step thourgh each keyframe
	//static float anim_timer = anim_clip.keyframes[1].time;

	//// advance animation timer by delta time
	//anim_timer += delta_time * 0.05f;

	//// if passed duration then reset
	//if (anim_timer > anim_clip.duration)
	//{
	//	anim_timer -= anim_clip.duration;
	//}

	//// make sure pose container is ready to be filled
	//const int joint_count = (int)(anim_clip.keyframes.front().joints.size());
	//pose_joints.resize(joint_count);

	//// get or interpolate the pose
	//int prev = (int)(anim_clip.keyframes.size() - 1);
	//int next = 1;

	//for (; next < anim_clip.keyframes.size(); prev = next++)
	//{
	//	if (anim_timer < anim_clip.keyframes[next].time)
	//		break;
	//}

	// keyframe_t& prev_frame = anim_clip.keyframes[prev];
	// keyframe_t& next_frame = anim_clip.keyframes[next];

	//float tween_ratio = (anim_timer - prev_frame.time) / (next_frame.time - prev_frame.time);

	//for (int j = 0; j < joint_count; ++j)
	//{
	//	const float4x4& xform1 = prev_frame.joints[j].transform;
	//	const float4x4& xform2 = next_frame.joints[j].transform;

	//	interpolate_transform(xform1, xform2, tween_ratio, pose_joints[j].transform);
	//}

	//float joint_scale = 0.75f;
	//// needed for debug renderer lines
	//{
	//	for (int j = 0; j < joint_count; ++j)
	//	{
	//		pose_joints[j].parent = anim_clip.keyframes.front().joints[j].parent;
	//	}

	//	// generate the debug line geometry for the skeleton
	//	debug_render_skeleton(pose_joints, joint_scale);
	//}

	//// prepare the joint transforms amd copy for the skinning shader constant buffer
	//for (int j = 0; j < pose_joints.size(); j++)
	//{
	//	XMMATRIX inv_bind = XMMatrixInverse(nullptr, (XMMATRIX&)anim_clip.keyframes.front().joints[j].transform);
	//	XMMATRIX tween_joint = (XMMATRIX&)pose_joints[j].transform;

	//	XMMATRIX joint_delta = XMMatrixMultiply(inv_bind, tween_joint);
	//	joint_delta *= joint_scale;
	//	joint_deltas.m[j] = XMMatrixTranspose(joint_delta);
	//}

	joint_set_t pose_joints;

	// step thourgh each keyframe
	static float anim_timer = anim_clip.keyframes[1].time;

	// advance animation timer by delta time
	anim_timer += delta_time *0.5f;

	// if passed duration then reset
	if (anim_timer > anim_clip.duration)
	{
		anim_timer -= anim_clip.duration;
	}

	const int joint_count = (int)(anim_clip.keyframes.front().joints.size());
	pose_joints.resize(joint_count);

	int prev = (int)(anim_clip.keyframes.size() - 1);
	int next = 1;

	for (; next < anim_clip.keyframes.size(); prev = next++)
	{
		if (anim_timer < anim_clip.keyframes[next].time)
			break;
	}

	float joint_scale = 0.75f;
	debug_render_skeleton(anim_clip.keyframes[prev].joints, joint_scale);

}

//--------------------------------------------------------------------------------------
// Update
//--------------------------------------------------------------------------------------
void Update()
{
	// Update our time
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart) / 1000.0f;
	}
	compute_global_delta_time(t);

	animate_character();

	// Initialize the view matrix
	// Stationary camera
	XMVECTOR Eye = XMVectorSet(4.5f, 3.5f, 4.5f, 0.0f);
	// Orbit camera
	//XMVECTOR Eye = XMVectorSet(cos(t / 2) * -6.5, 3.5f, sin(t / 2) * -6.5, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 2.5f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// Setup our lighting parameters
	XMStoreFloat4(&vLightDirs[0], { 0.577f, 0.577f, 0.577f, 1.0f });
	XMStoreFloat4(&vLightDirs[1], { 0.577f, 0.2577f, -0.577f, 1.0f });

	XMStoreFloat4(&vLightColors[0], { 0.85f, 0.85f, 0.85f, 1.0f });
	XMStoreFloat4(&vLightColors[1], { 1.0f, 0.75f, 0.25f, 1.0f });

	// Rotate the second light around the origin
	XMMATRIX mRotate = XMMatrixRotationY(-1.0f * t);
	XMVECTOR vLightDir = XMLoadFloat4(&vLightDirs[1]);
	// rotates the second light
	vLightDir = XMVector3Transform(vLightDir, mRotate);
	XMStoreFloat4(&vLightDirs[1], vLightDir);
}

void renderGrid()
{
	// set up default render state
	// no blending
	g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);
	// solid, no wireframe
	g_pImmediateContext->RSSetState(rasterStateDefault);
	// write z
	g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);

	TransformsConstantBuffer cbDebug;
	cbDebug.mWorld = XMMatrixIdentity();
	cbDebug.mView = XMMatrixTranspose(g_View);
	cbDebug.mProjection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(gridRenderable.constantBufferVS.Get(), 0, nullptr, &cbDebug, 0, 0);

	gridRenderable.Bind(g_pImmediateContext);
	gridRenderable.Draw(g_pImmediateContext);
}

// Mesh render routine that supports toggling texturing
// and toggling overlay wireframe
void renderMesh(Renderable meshRenderable)
{
	// copy transform to constant buffer
	modelViewProjection.mWorld = XMMatrixTranspose(meshRenderable.world);

	// send the constant buffers to the GPU
	g_pImmediateContext->UpdateSubresource(meshRenderable.constantBufferVS.Get(), 0, nullptr, &modelViewProjection, 0, 0);
	g_pImmediateContext->UpdateSubresource(meshRenderable.constantBufferPS.Get(), 0, nullptr, &lightsAndColor, 0, 0);

	// set all of the render states
	meshRenderable.Bind(g_pImmediateContext);

	// set the solid raster state
	if (RASTER_FILL_CULL_NONE)
		g_pImmediateContext->RSSetState(rasterStateFillNoCull);
	else
		g_pImmediateContext->RSSetState(rasterStateDefault);

	// if not textured, set the current texture to the generate white texture
	if (!RENDER_STYLE_TEXTURED)
	{
		g_pImmediateContext->PSSetShaderResources(0, 1, texSRV.GetAddressOf());
	}
	if (RENDER_STYLE_TRANSPARENCY)
	{
		g_pImmediateContext->OMSetBlendState(transparencyState, 0, 0xffffffff);
	}
	else
	{
		g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);
	}

	// Bind and Draw the vertices
	meshRenderable.Draw(g_pImmediateContext);

	// redraw the whole mesh in wireframe mode
	if (RENDER_STYLE_WIREFRAME)
	{
		g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);

		// Set the color of the wireframe
		//cb1.vOutputColor = { 0.9f, 0.9f, 0.9f, 1.0f };
		g_pImmediateContext->UpdateSubresource(meshRenderable.constantBufferVS.Get(), 0, nullptr, &modelViewProjection, 0, 0);

		// set the solid pixel shader
		g_pImmediateContext->PSSetShader(g_pPixelShaderSolid, nullptr, 0);

		// set the wireframe raster state
		g_pImmediateContext->RSSetState(rasterStateWireframe);

		// Draw the mesh
		meshRenderable.Draw(g_pImmediateContext);
	}
}

void renderSkyBox()
{
	TransformsConstantBuffer cbDebug;
	cbDebug.mWorld = XMMatrixIdentity();
	// zero out the camera postion so the skybox renders 
	// AT the camera postion
	XMMATRIX tmpMat = g_View;
	tmpMat.r[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
	cbDebug.mView = XMMatrixTranspose(tmpMat);
	cbDebug.mProjection = XMMatrixTranspose(g_Projection);

	g_pImmediateContext->UpdateSubresource(skyboxRenderable.constantBufferVS.Get(), 0, nullptr, &cbDebug, 0, 0);
	g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);
	g_pImmediateContext->RSSetState(rasterStateFillNoCull);

	skyboxRenderable.Bind(g_pImmediateContext);
	skyboxRenderable.Draw(g_pImmediateContext);
	//g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);
}
//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
	//
	// Clear the back buffers
	//
	const float color[4] = { 0.2f, 0.2f, 0.2f, 1 };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, color);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//
	// Render the grid
	//
	if (DEBUG_VIEW_ENABLED)
		renderGrid();

	//
	// Update matrix variables and lighting variables
	//
	modelViewProjection.mView = XMMatrixTranspose(g_View);
	modelViewProjection.mProjection = XMMatrixTranspose(g_Projection);

	lightsAndColor.vLightDir[0] = vLightDirs[0];
	lightsAndColor.vLightDir[1] = vLightDirs[1];
	lightsAndColor.vLightColor[0] = vLightColors[0];
	lightsAndColor.vLightColor[1] = vLightColors[1];
	lightsAndColor.vOutputColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// set up some global states
	g_pImmediateContext->RSSetState(rasterStateDefault);
	g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);
	g_pImmediateContext->OMSetBlendState(nullptr, 0, 0xffffffff);
	if (DEPTH_WRITE_ENABLED)
		g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);
	else
		g_pImmediateContext->OMSetDepthStencilState(pDSStateNoWrite, 1);

	// Render all of the renderables in the scene
	for (auto r : renderables)
	{
		renderMesh(r);
		if (DEBUG_VIEW_ENABLED)
			end::debug_renderer::add_transform((end::float4x4&)r.world);
	}

	// skinned mesh
	g_pImmediateContext->UpdateSubresource(joint_deltas_CB, 0, NULL, &joint_deltas, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &joint_deltas_CB);
	//renderMesh(skinnedRenderable);
	if (DEBUG_VIEW_ENABLED)
		end::debug_renderer::add_transform((end::float4x4&)skinnedRenderable.world);

	// Draw Skybox
	if (SKYBOX_ENABLED)
		renderSkyBox();

	// Render debug transform markers using gridRenderable objects
	// and some overrides
	if (DEBUG_VIEW_ENABLED)
	{
		// Render each light
		for (int m = 1; m < 2; m++)
		{
			XMMATRIX mLight = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&vLightDirs[m]));
			XMMATRIX mLightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
			mLight = mLightScale * mLight;
			debug_renderer::add_transform((end::float4x4&)mLight);
		}
		////////////////////////////////////////
		//Render Debug lines
		////////////////////////////////////////

		// borrow the pipeline states from the grid renderable
		gridRenderable.Bind(g_pImmediateContext);

		// update the vertex data buffer
		g_pImmediateContext->UpdateSubresource(vertex_buffer, 0, NULL, debug_renderer::get_line_verts(), 0, 0);

		// Force visibility of the debug lines
		g_pImmediateContext->OMSetDepthStencilState(pDSStateNoTest, 1);

		UINT stride = sizeof(colored_vertex);
		UINT offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);

		// makes the debug lines thicker
		g_pImmediateContext->RSSetState(rasterStateWireframe);

		if (UINT vert_count = (UINT)debug_renderer::get_line_vert_count())
			g_pImmediateContext->Draw(vert_count, 0);

	}
	// reset the debug lines container for the next frame
	debug_renderer::clear_lines();

	//
	// Present our back buffer to our front buffer
	//
	g_pSwapChain->Present(0, 0);
}