/*AMDG*/
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "app.h"
#include "util.h"

App* app = new App();

bool App::init(HWND hwnd)
{
	pRenderer = std::make_unique<Dx12Renderer>(Dx12Renderer());
	initDevice(pRenderer.get(), hwnd);

	ID3D12Device* pDevice = pRenderer->pDevice;

	struct Vertex
	{
		glm::vec3 p;
		glm::vec2 tc;
	};

	// create vertex buffer
	Vertex vb[] =
	{
		{ { 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { 1.0f, -1.0f, 0.0f },{ 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f },{ 0.0f, 0.0f } }
	};

	const UINT vbSize = sizeof(vb);

	D3D12_RESOURCE_DESC desc = {};
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Height = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Width = vbSize;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	HRESULT hr = pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
				 &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&vertexBuffer));

	if (FAILED(hr)) { ErrorMsg("VB creation failed"); }

	transitionResource(pRenderer.get(), vertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	uploadBuffer(pRenderer.get(), vertexBuffer, vb, vbSize, 0);

	if (FAILED(hr)) { assert(!"vb upload failed"); }

	transitionResource(pRenderer.get(), vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vbSize;
	vertexBufferView.StrideInBytes = 28;
	// create shaders

	// create pipeline states

	return true;
}

void App::drawFrame()
{
	// draw triangle

	// flip bufers
}