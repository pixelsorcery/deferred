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

    /*
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

    if (FAILED(hr)) { ErrorMsg("vb upload failed"); }

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
    */

    // load model
    loadModel(boxModel, "..\\models\\BoxTextured.gltf");

    // create textures (TODO: add this to renderer and generalize this to create RTs and DBs)
    for (size_t i = 0; i < boxModel.images.size(); i++)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Height = boxModel.images[i].height;
        resourceDesc.Width = boxModel.images[i].width;
        resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.DepthOrArraySize = 1;

        D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COPY_DEST;

        ID3D12Resource* textureResource = nullptr;
        HRESULT hr = pDevice->CreateCommittedResource(&heapProps, flags, &resourceDesc, resourceState, nullptr, __uuidof(ID3D12Resource), (void**) &textureResource);

        if (FAILED(hr))
        {
            ErrorMsg("Failed to create texture for model.");
        }

        boxTextures.push_back(textureResource);
    }

    // modelview matrix and descriptor heap with texture
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
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    params[1] = {};
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &srvRange;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_STATIC_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampDesc.MaxAnisotropy = 1;
    sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
    sampDesc.ShaderRegister = 0;
    sampDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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
    for (int i = 0; i < boxModel.meshes.size(); i++)
    {
        for (int j = 0; j < boxModel.meshes[i].primitives.size(); j++)
        {
            auto it = boxModel.meshes[i].primitives[j].attributes.begin();
            semanticNames.resize(boxModel.meshes[i].primitives[j].attributes.size());
            for (int k = 0; k < boxModel.meshes[i].primitives[j].attributes.size(); k++)
            {
                tinygltf::Accessor accessor = boxModel.accessors[it->second];
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
                tinygltf::BufferView gltfBufView = boxModel.bufferViews[accessor.bufferView];
                ID3D12Resource* pBuf = nullptr;
                pBuf = createBuffer(pRenderer.get(), D3D12_HEAP_TYPE_DEFAULT, gltfBufView.byteLength, D3D12_RESOURCE_STATE_COPY_DEST);
                boxBuffers.push_back(pBuf);

                // upload buffer
                uploadBuffer(pRenderer.get(),
                    boxBuffers.back(),
                    &boxModel.buffers[gltfBufView.buffer].data.at(gltfBufView.byteOffset + accessor.byteOffset),
                    (UINT)gltfBufView.byteLength,
                    0);         

                transitionResource(pRenderer.get(), boxBuffers.back(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

                D3D12_VERTEX_BUFFER_VIEW bufView = {};
                bufView.BufferLocation = boxBuffers.back()->GetGPUVirtualAddress();
                bufView.SizeInBytes = (UINT)gltfBufView.byteLength;
                bufView.StrideInBytes = (UINT)gltfBufView.byteStride;
            }

            // load index buffer
            UINT accessorIdx = boxModel.meshes[i].primitives[j].indices;
            UINT bufViewIdx = boxModel.accessors[accessorIdx].bufferView;
            tinygltf::BufferView gltfIdxBufView = boxModel.bufferViews[bufViewIdx];

            pIndexBuffer = createBuffer(pRenderer.get(), D3D12_HEAP_TYPE_DEFAULT, gltfIdxBufView.byteLength, D3D12_RESOURCE_STATE_COPY_DEST);

            uploadBuffer(pRenderer.get(),
                pIndexBuffer,
                &boxModel.buffers[gltfIdxBufView.buffer].data[gltfIdxBufView.byteOffset + boxModel.accessors[accessorIdx].byteOffset],
                (UINT)gltfIdxBufView.byteLength,
                0);

            transitionResource(pRenderer.get(), pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        }
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC boxPsoDesc = {};
    boxPsoDesc.pRootSignature = boxRootSignature;
    boxPsoDesc.VS = bytecodeFromBlob(boxVs);
    boxPsoDesc.PS = bytecodeFromBlob(boxPs);
    boxPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    boxPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
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
    dbgModel(boxModel);
#endif 

    if (S_OK != hr)
    {
        ErrorMsg("Triangle pipeline creation failed.");
        return false;
    }

    // create a descriptor table for cbv and srv
    D3D12_ROOT_DESCRIPTOR_TABLE cbvDescriptorTable;
    cbvDescriptorTable.NumDescriptorRanges = cbvRange.NumDescriptors;
    cbvDescriptorTable.pDescriptorRanges = &cbvRange;

    D3D12_ROOT_DESCRIPTOR_TABLE srvDescriptorTable;
    srvDescriptorTable.NumDescriptorRanges = srvRange.NumDescriptors;
    srvDescriptorTable.pDescriptorRanges = &srvRange;

    for (int i = 0; i < renderer::swapChainBufferCount; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 2;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pRenderer->mainDescriptorHeap[i]));
        if (FAILED(hr))
        {
            ErrorMsg("Descriptor heap creation failed.");
            return false;
        }
    }

    for (int i = 0; i < renderer::swapChainBufferCount; ++i)
    {
        D3D12_RESOURCE_DESC heapDesc = {};
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        hr = pDevice->CreateCommittedResource(&heapProps,
            D3D12_HEAP_FLAG_NONE,
            )


    return true;
}

void App2::drawFrame()
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
    pCmdList->IASetVertexBuffers(0, static_cast<UINT>(boxBufferViews.size()), &boxBufferViews[0]);

    pCmdList->IASetIndexBuffer(&indexBufView);

    // set root sig
    pCmdList->SetGraphicsRootSignature(boxRootSignature);

    // Set pipeline
    pCmdList->SetPipelineState(boxPipeline);

    // update model view projection matrix
    glm::mat4 model = {};

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));


    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      (float)renderer::width / (float)renderer::height,
                                      0.1f,
                                      100.0f);

    glm::mat4 mvp = proj * view * model;


    //// draw triangle
    //pCmdList->DrawInstanced(3, 1, 0, 0);

    // draw box


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