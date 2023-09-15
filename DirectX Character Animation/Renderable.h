#pragma once

#include <d3d11_1.h>
#include <directxmath.h>
#include <wrl/client.h>
#include <fstream>
#include <vector>
#include "DDSTextureLoader.h"

using namespace DirectX;
using namespace std;

using Microsoft::WRL::ComPtr;

struct LightsConstantBuffer
{
	XMFLOAT4 vLightDir[2];
	XMFLOAT4 vLightColor[2];
	XMFLOAT4 vOutputColor;
};

struct TransformsConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
};

std::vector<uint8_t> load_binary_blob(const char* path)
{
	std::vector<uint8_t> blob;

	std::fstream file{ path, std::ios_base::in | std::ios_base::binary };

	if (file.is_open())
	{
		file.seekg(0, std::ios_base::end);
		blob.resize(file.tellg());
		file.seekg(0, std::ios_base::beg);

		file.read((char*)blob.data(), blob.size());

		file.close();
	}

	return std::move(blob);
}

class Renderable
{
public:
	XMMATRIX world = DirectX::XMMatrixIdentity();

	//Vertex data objects
	ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	int vertexCount = 0;
	UINT vertexSize = 0;
	ComPtr<ID3D11Buffer> indexBuffer = nullptr;
	int indexCount = 0;
	D3D11_PRIMITIVE_TOPOLOGY primitiveTopology =
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Shader obejcts
	ComPtr<ID3D11InputLayout> inputLayout = nullptr;
	ComPtr<ID3D11VertexShader> vertexShader = nullptr;
	ComPtr<ID3D11PixelShader> pixelShader = nullptr;
	ComPtr<ID3D11Buffer> constantBufferVS = nullptr;
	ComPtr<ID3D11Buffer> constantBufferPS = nullptr;

	// Shader Resources (Texture)
	ComPtr<ID3D11ShaderResourceView> resourceView = nullptr;
	ComPtr<ID3D11SamplerState> samplerState = nullptr;

	void setPosition(XMVECTOR posIn)
	{
		world.r[3] = posIn;
	}

	void setPosition(float _x, float _y, float _z)
	{
		world.r[3] = { _x,_y,_z, 1.0f };
	}

	void setRotation(XMMATRIX rotIn)
	{
		// set rotation
		XMMATRIX tmpWorld = rotIn;

		// get position
		tmpWorld.r[3] = world.r[3];

		// set Transform
		world = tmpWorld;
	}

	HRESULT CreateBuffers(ID3D11Device* device, vector<int>& indices,
		float* vertices, int vSize, int vCount)
	{
		HRESULT hr = S_OK;

		hr = CreateIndexBuffer(device, indices);
		if (FAILED(hr))
			return hr;

		hr = CreateVertexBuffer(device, vertices, vSize, vCount);
		return hr;
	}

	HRESULT CreateIndexBuffer(ID3D11Device* device, vector<int>& indices)
	{
		HRESULT hr = S_OK;

		indexCount = (int)indices.size();
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(int) * indexCount;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = indices.data();
		hr = device->CreateBuffer(&bd, &InitData,
			indexBuffer.ReleaseAndGetAddressOf());
		return hr;
	}

	HRESULT CreateVertexBuffer(ID3D11Device* device, float* vertices, int size, int count)
	{
		HRESULT hr = S_OK;

		vertexCount = count;
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		vertexSize = size;
		bd.ByteWidth = vertexSize * vertexCount;;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = vertices;
		hr = device->CreateBuffer(&bd, &InitData,
			vertexBuffer.ReleaseAndGetAddressOf());
		return hr;
	}

	HRESULT CreateVertexShaderAndInputLayoutFromFile(
		ID3D11Device* device, const char* filename,
		D3D11_INPUT_ELEMENT_DESC layout[], UINT numElements)
	{
		HRESULT hr = S_OK;

		auto vs_blob = load_binary_blob(filename);

		// Create the vertex shader
		hr = device->CreateVertexShader(vs_blob.data(), vs_blob.size(), nullptr,
			vertexShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			return hr;
		}

		// Create the input layout
		hr = device->CreateInputLayout(layout, numElements, vs_blob.data(),
			vs_blob.size(),
			inputLayout.ReleaseAndGetAddressOf());
		return hr;
	}

	HRESULT CreatePixelShaderFromFile(ID3D11Device* device, const char* filename)
	{
		HRESULT hr = S_OK;

		auto vs_blob = load_binary_blob(filename);

		// Create the pixel shader
		hr = device->CreatePixelShader(vs_blob.data(), vs_blob.size(), nullptr,
			pixelShader.ReleaseAndGetAddressOf());
		return hr;
	}
	HRESULT CreateConstantBufferVS(ID3D11Device* device, UINT size)
	{
		return CreateConstantBuffer(device, size, &constantBufferVS);
	}
	HRESULT CreateConstantBufferPS(ID3D11Device* device, UINT size)
	{
		return CreateConstantBuffer(device, size, &constantBufferPS);
	}

	HRESULT CreateConstantBuffer(ID3D11Device* device, UINT size, ID3D11Buffer **constantBuffer)
	{
		HRESULT hr = S_OK;

		// Create the constant buffer
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = size;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		hr = device->CreateBuffer(&bd, nullptr, constantBuffer);
		return hr;
	}

	HRESULT CreateTextureFromFile(ID3D11Device* device, std::string filename)
	{
		HRESULT hr = S_OK;

		// string magic to convert std::string to work with the texture loader
		std::wstring widestr = std::wstring(filename.begin(), filename.end());
		const wchar_t* widecstr = widestr.c_str();
		hr = CreateDDSTextureFromFile(device, widecstr, nullptr,
			resourceView.ReleaseAndGetAddressOf());
		return hr;
	}

	HRESULT CreateDefaultSampler(ID3D11Device* device)
	{
		HRESULT hr = S_OK;

		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = device->CreateSamplerState(&sampDesc,
			samplerState.ReleaseAndGetAddressOf());
		return hr;
	}

	void Bind(ID3D11DeviceContext* context)
	{
		// Set shaders
		if (constantBufferVS)
			context->VSSetConstantBuffers(0, 1, constantBufferVS.GetAddressOf());
		if (constantBufferPS)
			context->PSSetConstantBuffers(0, 1, constantBufferPS.GetAddressOf());
		if (inputLayout)
			context->IASetInputLayout(inputLayout.Get());
		if (vertexShader)
			context->VSSetShader(vertexShader.Get(), nullptr, 0);
		if (pixelShader)
			context->PSSetShader(pixelShader.Get(), nullptr, 0);

		// Set texture and sampler.
		if (resourceView.Get())
			context->PSSetShaderResources(0, 1, resourceView.GetAddressOf());
		if (samplerState.Get())
			context->PSSetSamplers(0, 1, samplerState.GetAddressOf());

		// Set vertex buffer
		UINT stride = vertexSize;
		UINT offset = 0;
		if (vertexBuffer)
			context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride,
				&offset);
		// Set index buffer
		if (indexBuffer)
			context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		// Set primitive topology
		context->IASetPrimitiveTopology(primitiveTopology);
	}

	void Draw(ID3D11DeviceContext* context)
	{
		if (indexBuffer)
			context->DrawIndexed(indexCount, 0, 0);
		else if (vertexBuffer)
			context->Draw(vertexCount, 0);
	}

	void DrawIndexed(ID3D11DeviceContext* context)
	{
		if (indexBuffer && vertexBuffer)
			context->DrawIndexed(indexCount, 0, 0);
	}
};