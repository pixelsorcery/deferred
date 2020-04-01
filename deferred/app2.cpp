/*AMDG*/
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "app2.h"
#include "util.h"
#include "shaders.h"
#include "gltfHelper.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

using namespace std;

App2* app2 = new App2();

App2::App2() : pRenderer(new Dx12Renderer()) {};

#if defined(_DEBUG)
#include <iostream>

void dbgModel(tinygltf::Model& model) {
    for (auto& mesh : model.meshes) {
        std::cout << "mesh : " << mesh.name << std::endl;
        for (auto& primitive : mesh.primitives) {
            const tinygltf::Accessor& indexAccessor =
                model.accessors[primitive.indices];

            std::cout << "indexaccessor: count " << indexAccessor.count << ", type "
                << indexAccessor.componentType << std::endl;

            tinygltf::Material& mat = model.materials[primitive.material];
            for (auto& mats : mat.values) {
                std::cout << "mat : " << mats.first.c_str() << std::endl;
            }

            for (auto& image : model.images) {
                std::cout << "image name : " << image.uri << std::endl;
                std::cout << "  size : " << image.image.size() << std::endl;
                std::cout << "  w/h : " << image.width << "/" << image.height
                    << std::endl;
            }

            std::cout << "indices : " << primitive.indices << std::endl;
            std::cout << "mode     : "
                << "(" << primitive.mode << ")" << std::endl;

            for (auto& attrib : primitive.attributes) {
                std::cout << "attribute : " << attrib.first.c_str() << std::endl;
            }
        }
    }
}
#endif

bool App2::init(HWND hwnd)
{
    initDevice(pRenderer.get(), hwnd);

    ID3D12Device* pDevice = pRenderer->pDevice;

    // load model
    loadModel(pRenderer.get(), Model, "..\\models\\BoxTextured.gltf");

    // modelview matrix and texture
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;

    D3D12_DESCRIPTOR_RANGE cbvRange = {};
    cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvRange.NumDescriptors = 1;

    D3D12_ROOT_PARAMETER params[2];
    params[0] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[0].DescriptorTable.NumDescriptorRanges = 1;
    params[0].DescriptorTable.pDescriptorRanges = &cbvRange;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    params[1] = {};
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &srvRange;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.MaxAnisotropy = 0;
    sampDesc.ShaderRegister = 0;
    sampDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 2;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pParameters = params;
    rootSigDesc.pStaticSamplers = &sampDesc;

    // Serialize and create root signature
    CComPtr<ID3DBlob> serializedRootSig;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, NULL);

    if (S_OK != hr)
    {
        ErrorMsg("D3D12SerializeRootSignature() failed.");
        return false;
    }

    hr = pDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&boxRootSignature));

    if (S_OK != hr)
    {
        ErrorMsg("CreateRootSignature() failed.");
        return false;
    }

    // create shaders
    boxVs = compileShaderFromFile("boxTexturedVS.hlsl", "vs_5_1", "main");
    boxPs = compileShaderFromFile("boxTexturedPs.hlsl", "ps_5_1", "main");

    // Create pipelines for every mesh and primitive
    UINT numInputElements = 0;
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[D3D12_STANDARD_VERTEX_ELEMENT_COUNT];
    vector<string> semanticNames;
    tinygltf::Model* pModel = &Model.TinyGltfModel;
    for (int i = 0; i < pModel->meshes.size(); i++)
    {
        for (int j = 0; j < pModel->meshes[i].primitives.size(); j++)
        {
            auto it = pModel->meshes[i].primitives[j].attributes.begin();
            semanticNames.resize(pModel->meshes[i].primitives[j].attributes.size());
            for (int k = 0; k < pModel->meshes[i].primitives[j].attributes.size(); k++)
            {
                tinygltf::Accessor accessor = pModel->accessors[it->second];
                int index = 0;
                processSemantics(it->first, semanticNames[k], index);

                inputElementDescs[k] = {};
                inputElementDescs[k].SemanticName = semanticNames[k].c_str();
                inputElementDescs[k].SemanticIndex = index;
                inputElementDescs[k].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                inputElementDescs[k].InputSlot = k;
                inputElementDescs[k].Format = GetFormat(accessor.type, accessor.componentType);
                inputElementDescs[k].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
                inputElementDescs[k].InstanceDataStepRate = 0;
                it++;
                numInputElements++;

                // create buffer
                tinygltf::BufferView gltfBufView = pModel->bufferViews[accessor.bufferView];
                ID3D12Resource* pBuf = nullptr;
                pBuf = createBuffer(pRenderer.get(), D3D12_HEAP_TYPE_DEFAULT, gltfBufView.byteLength, D3D12_RESOURCE_STATE_COPY_DEST);
                boxBuffers.push_back(pBuf);

                // upload buffer
                uploadBuffer(pRenderer.get(),
                    boxBuffers.back(),
                    &pModel->buffers[gltfBufView.buffer].data.at(gltfBufView.byteOffset + accessor.byteOffset),
                    (UINT)gltfBufView.byteLength,
                    0);         

                transitionResource(pRenderer.get(), boxBuffers.back(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

                D3D12_VERTEX_BUFFER_VIEW bufView = {};
                bufView.BufferLocation = boxBuffers.back()->GetGPUVirtualAddress();
                bufView.SizeInBytes = (UINT)gltfBufView.byteLength;
                bufView.StrideInBytes = (UINT)gltfBufView.byteStride;

                boxBufferViews.push_back(bufView);
            }

            // load index buffer
            UINT accessorIdx = pModel->meshes[i].primitives[j].indices;
            UINT bufViewIdx = pModel->accessors[accessorIdx].bufferView;
            tinygltf::BufferView gltfIdxBufView = pModel->bufferViews[bufViewIdx];

            pIndexBuffer = createBuffer(pRenderer.get(), D3D12_HEAP_TYPE_DEFAULT, gltfIdxBufView.byteLength, D3D12_RESOURCE_STATE_COPY_DEST);

            uploadBuffer(pRenderer.get(),
                pIndexBuffer,
                &pModel->buffers[gltfIdxBufView.buffer].data[gltfIdxBufView.byteOffset + pModel->accessors[accessorIdx].byteOffset],
                (UINT)gltfIdxBufView.byteLength,
                0);

            transitionResource(pRenderer.get(), pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            UINT stride = GetFormatSize(pModel->accessors[accessorIdx].componentType);
            indexBufView = {};
            indexBufView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
            indexBufView.Format = (gltfIdxBufView.byteStride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
            indexBufView.SizeInBytes = static_cast<UINT>(gltfIdxBufView.byteLength);

            indexBufSize = static_cast<UINT>(pModel->accessors[accessorIdx].count);
        }
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC boxPsoDesc = {};
    boxPsoDesc.pRootSignature = boxRootSignature;
    boxPsoDesc.VS = bytecodeFromBlob(boxVs);
    boxPsoDesc.PS = bytecodeFromBlob(boxPs);
    boxPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    boxPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    boxPsoDesc.RasterizerState.DepthClipEnable = TRUE;
    boxPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    boxPsoDesc.SampleMask = UINT_MAX;
    boxPsoDesc.DepthStencilState = {};
    boxPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    boxPsoDesc.NumRenderTargets = 1;
    boxPsoDesc.RTVFormats[0] = pRenderer->colorFormat;
    boxPsoDesc.SampleDesc.Count = 1;
    boxPsoDesc.InputLayout.pInputElementDescs = inputElementDescs;
    boxPsoDesc.InputLayout.NumElements = numInputElements;

    hr = pDevice->CreateGraphicsPipelineState(&boxPsoDesc, IID_PPV_ARGS(&boxPipeline));

#if defined(_DEBUG)
    dbgModel(*pModel);
#endif 

    if (S_OK != hr)
    {
        ErrorMsg("Triangle pipeline creation failed.");
        return false;
    }

    // create a descriptor table for cbv and srv
    D3D12_DESCRIPTOR_RANGE descRanges[2] = { cbvRange, srvRange };
    Model.DescriptorTable.NumDescriptorRanges = cbvRange.NumDescriptors + srvRange.NumDescriptors;
    Model.DescriptorTable.pDescriptorRanges = descRanges;

    for (int i = 0; i < renderer::swapChainBufferCount; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 2;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pRenderer->mainDescriptorHeaps[i]));
        if (FAILED(hr))
        {
            ErrorMsg("Descriptor heap creation failed.");
            return false;
        }

        pRenderer->mainDescriptorHeaps[i]->SetName(L"Main Descriptor Heap");
    }

    for (int i = 0; i < renderer::swapChainBufferCount; ++i)
    {
        ID3D12Resource* cbvSrvUploadHeap;
        ID3D12Resource* cbvSrvHeap;

        cbvSrvUploadHeap = createBuffer(pRenderer.get(), D3D12_HEAP_TYPE_UPLOAD, 512, D3D12_RESOURCE_STATE_GENERIC_READ);
        cbvSrvHeap = createBuffer(pRenderer.get(), D3D12_HEAP_TYPE_DEFAULT, 512, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        pRenderer.get()->cbvSrvUavUploadHeaps[i].Attach(cbvSrvUploadHeap);
        pRenderer.get()->cbvSrvUavHeaps[i].Attach(cbvSrvHeap);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = cbvSrvHeap->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (sizeof(16 * 4) + 255) & ~255;    // CB size is required to be 256-byte aligned.
        pDevice->CreateConstantBufferView(&cbvDesc, pRenderer->mainDescriptorHeaps[i]->GetCPUDescriptorHandleForHeapStart());
    }

    return true;
}

void App2::drawFrame()
{
    // clear
    HRESULT hr = S_OK;

    ID3D12Device* pDevice = pRenderer->pDevice;
    ID3D12GraphicsCommandList* pCmdList = pRenderer->pGfxCmdList;

    // set rt
    pCmdList->OMSetRenderTargets(1, &pRenderer->backbufDescHandle[pRenderer->backbufCurrent], false, 0);

    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // clear rt
    float clearCol[4] = { 0.4f, 0.4f, 0.6f, 1.0f };
    pCmdList->ClearRenderTargetView(pRenderer->backbufDescHandle[pRenderer->backbufCurrent], clearCol, 0, nullptr);
    pCmdList->RSSetViewports(1, &pRenderer->defaultViewport);
    pCmdList->RSSetScissorRects(1, &pRenderer->defaultScissor);

    // set root sig
    pCmdList->SetGraphicsRootSignature(boxRootSignature);

    // set pipeline
    pCmdList->SetPipelineState(boxPipeline);

    D3D12_CPU_DESCRIPTOR_HANDLE dest = pRenderer->mainDescriptorHeaps[pRenderer->backbufCurrent]->GetCPUDescriptorHandleForHeapStart();
    dest.ptr += pRenderer->cbvSrvUavDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE src = Model.TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart = pRenderer->mainDescriptorHeaps[pRenderer->backbufCurrent]->GetGPUDescriptorHandleForHeapStart();
    srvTableStart.ptr += pRenderer->cbvSrvUavDescriptorSize;
    // copy descriptor to heap
    pDevice->CopyDescriptorsSimple(1, dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // update model view projection matrix
    glm::mat4 model = glm::mat4(1.0);

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));


    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      (float)renderer::width / (float)renderer::height,
                                      0.1f,
                                      100.0f);

    glm::mat4 mvp = proj * view * model;

    D3D12_RANGE range = {};
    UINT* pData;

    pRenderer->cbvSrvUavUploadHeaps[pRenderer->backbufCurrent]->Map(0, &range, reinterpret_cast<void**>(&pData));
    memcpy(pData, &mvp, sizeof(mvp));
    transitionResource(pRenderer.get(), pRenderer->cbvSrvUavHeaps[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    pCmdList->CopyResource(pRenderer->cbvSrvUavHeaps[pRenderer->backbufCurrent], pRenderer->cbvSrvUavUploadHeaps[pRenderer->backbufCurrent]);
    transitionResource(pRenderer.get(), pRenderer->cbvSrvUavHeaps[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    // set prim topology
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmdList->IASetVertexBuffers(0, static_cast<UINT>(boxBuffers.size()), &boxBufferViews[0]);
    pCmdList->IASetIndexBuffer(&indexBufView);

    pCmdList->SetDescriptorHeaps(1, &pRenderer->mainDescriptorHeaps[pRenderer->backbufCurrent].p);
    pCmdList->SetGraphicsRootDescriptorTable(0, pRenderer->mainDescriptorHeaps[pRenderer->backbufCurrent]->GetGPUDescriptorHandleForHeapStart());
    pCmdList->SetGraphicsRootDescriptorTable(1, srvTableStart);

    // draw box
    pCmdList->DrawIndexedInstanced(indexBufSize, 1, 0, 0, 0);

    // exec cmd buffer
    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    submitCmdBuffer(pRenderer.get());

    // present
    present(pRenderer.get(), vsyncOff);
}

App2::~App2()
{
    for (int i = 0; i < boxBuffers.size(); i++)
    {
        if (boxBuffers[i] != nullptr)
        {
            boxBuffers[i]->Release();
        }
    }
}