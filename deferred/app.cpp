/*AMDG*/
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "app.h"
#include "util.h"
#include "shaders.h"

App* app = new App();

App::App() : pRenderer(new Dx12Renderer()), vertexBufferView() {};

bool App::init(HWND hwnd)
{
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

	transitionResource(pRenderer.get(), vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vbSize;
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	// create shaders
	triangleVs = compileShaderFromFile("triangleVS.hlsl", "vs_5_1", "main");
	trianglePs = compileShaderFromFile("trianglePs.hlsl", "ps_5_1", "main");

	// create empty root signature
	D3D12_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.NumParameters = 0;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootDesc.NumStaticSamplers = 0;
	rootDesc.pStaticSamplers = 0;
	rootDesc.pParameters = 0;

	CComPtr<ID3DBlob> serializedRS;
	hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &serializedRS, nullptr);
	if (S_OK != hr)
	{
		ErrorMsg("Triangle root signature creation failed.");
		return false;
	}

	hr = pDevice->CreateRootSignature(0, serializedRS->GetBufferPointer(), serializedRS->GetBufferSize(), IID_PPV_ARGS(&triRootSignature));
	if (S_OK != hr)
	{
		ErrorMsg("Triangle root sig creation failed.");
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// create pipeline
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	setDefaultPipelineState(pRenderer.get(), &psoDesc);
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = triRootSignature;
	psoDesc.VS = bytecodeFromBlob(triangleVs);
	psoDesc.PS = bytecodeFromBlob(trianglePs);
	psoDesc.DepthStencilState = {};
	psoDesc.RTVFormats[0] = pRenderer->colorFormat;

	hr = pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&triPipeline));

	if (S_OK != hr)
	{
		ErrorMsg("Triangle pipeline creation failed.");
		return false;
	}

	return true;
}

void App::drawFrame()
{
	// clear
	HRESULT hr = S_OK;

	ID3D12GraphicsCommandList* pCmdList = pRenderer->pGfxCmdList;

	// set rt
	pCmdList->OMSetRenderTargets(1, &pRenderer->backbufDescHandle[pRenderer->backbufCurrent], false, 0);

	transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// clear rt
	float clearCol[4] = { 0.4f, 0.4f, 0.6f, 1.0f };
	pCmdList->ClearRenderTargetView(pRenderer->backbufDescHandle[pRenderer->backbufCurrent], clearCol, 0, nullptr);
	pCmdList->RSSetViewports(1, &pRenderer->defaultViewport);
	pCmdList->RSSetScissorRects(1, &pRenderer->defaultScissor);

	// set prim topology
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// set root sig
	pCmdList->SetGraphicsRootSignature(triRootSignature);

	// Set pipeline
	pCmdList->SetPipelineState(triPipeline);

	// draw triangle
	pCmdList->DrawInstanced(3, 1, 0, 0);

	// exec cmd buffer
	transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	submitCmdBuffer(pRenderer.get());

	// present
	present(pRenderer.get(), vsyncOff);
}
