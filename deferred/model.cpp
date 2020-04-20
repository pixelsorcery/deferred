#include <vector>
#include <string>
#include "model.h"
#include "gltfHelper.h"
#include "util.h"
#include "dynArray.h"
#include "shaders.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

using namespace std;

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

bool loadModel(Dx12Renderer* pRenderer, GltfModel& model, const char* filename)
{
    ID3D12Device* pDevice = pRenderer->pDevice;
    HRESULT hr = S_OK;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model.TinyGltfModel, &err, &warn, filename);

    if (res == false) return false;

    DynArray<D3D12_RESOURCE_DESC> arr;

    tinygltf::Model* pModel = &model.TinyGltfModel;

    // create textures
    for (size_t i = 0; i < pModel->images.size(); i++)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Height = pModel->images[i].height;
        resourceDesc.Width = pModel->images[i].width;
        resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// GetFormat(pModel->images[i].component, pModel->images[i].pixel_type); //todo verify this
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.DepthOrArraySize = 1;

        D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COPY_DEST;

        CComPtr<ID3D12Resource> textureResource = nullptr;
        hr = pDevice->CreateCommittedResource(&heapProps, flags, &resourceDesc, resourceState, nullptr, __uuidof(ID3D12Resource), (void**)&textureResource);

        if (FAILED(hr))
        {
            ErrorMsg("Failed to create texture for model.");
        }

        uploadTexture(pRenderer,
            textureResource,
            &pModel->images[i].image[i],
            pModel->images[i].width,
            pModel->images[i].height,
            pModel->images[i].component,
            resourceDesc.Format);

        Texture tex = {};
        tex.pRes = textureResource;
        tex.desc = resourceDesc;
        model.Textures.push_back(tex);

        arr.push(resourceDesc);
    }

    // Create cpu descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = arr.size;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags;

    hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&model.TextureDescriptorHeap));
    if (FAILED(hr))
    {
        ErrorMsg("Descriptor heap creation for model failed.");
        return false;
    }

    model.TextureDescriptorHeap->SetName(L"Texture Descriptor Heap for Model");

    uint heapIncrementSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   
    // Create srvs
    for (uint i = 0; i < arr.size; i++)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = arr[i].Format;
        srvDesc.Texture2D.MipLevels = arr[i].MipLevels;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        D3D12_CPU_DESCRIPTOR_HANDLE descHandle = model.TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        pDevice->CreateShaderResourceView(model.Textures[i].pRes, &srvDesc, descHandle);

        descHandle.ptr += pRenderer->cbvSrvUavDescriptorSize;
    }

    // Create root signature for model

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
    params[0].DescriptorTable.NumDescriptorRanges = 1; // just mvp for now
    params[0].DescriptorTable.pDescriptorRanges = &cbvRange;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    params[1] = {};
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = arr.size;
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
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, NULL);

    if (S_OK != hr)
    {
        ErrorMsg("D3D12SerializeRootSignature() failed.");
        return false;
    }

    hr = pDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&model.pRootSignature));

    if (S_OK != hr)
    {
        ErrorMsg("CreateRootSignature() failed.");
        return false;
    }

    // create shaders // todo add option for shaders
    model.pModelVs = compileShaderFromFile("boxTexturedVS.hlsl", "vs_5_1", "main");
    model.pModelPs = compileShaderFromFile("boxTexturedPs.hlsl", "ps_5_1", "main");

    // Create pipelines for every mesh and primitive
    UINT numInputElements = 0;
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[D3D12_STANDARD_VERTEX_ELEMENT_COUNT];
    vector<string> semanticNames;

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
                CComPtr<ID3D12Resource> pBuf = nullptr;
                pBuf = createBuffer(pRenderer, D3D12_HEAP_TYPE_DEFAULT, gltfBufView.byteLength, D3D12_RESOURCE_STATE_COPY_DEST);
                model.pBuffers.push_back(pBuf);
                pBuf->SetName(L"model_vtx_buffer");

                // upload buffer
                uploadBuffer(pRenderer,
                    model.pBuffers.back(),
                    &pModel->buffers[gltfBufView.buffer].data.at(gltfBufView.byteOffset + accessor.byteOffset),
                    (UINT)gltfBufView.byteLength,
                    0);

                transitionResource(pRenderer, model.pBuffers.back(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

                D3D12_VERTEX_BUFFER_VIEW bufView = {};
                bufView.BufferLocation = model.pBuffers.back()->GetGPUVirtualAddress();
                bufView.SizeInBytes = (UINT)gltfBufView.byteLength;
                bufView.StrideInBytes = (UINT)gltfBufView.byteStride;

                model.BufferViews.push_back(bufView);
            }

            // load index buffer
            UINT accessorIdx = pModel->meshes[i].primitives[j].indices;
            UINT bufViewIdx = pModel->accessors[accessorIdx].bufferView;
            tinygltf::BufferView gltfIdxBufView = pModel->bufferViews[bufViewIdx];

            CComPtr<ID3D12Resource> indexBuffer;
            indexBuffer = createBuffer(pRenderer, D3D12_HEAP_TYPE_DEFAULT, gltfIdxBufView.byteLength, D3D12_RESOURCE_STATE_COPY_DEST);
            indexBuffer->SetName(L"model_index_buffer");
            model.IndexBuffers.push_back(indexBuffer);

            uploadBuffer(pRenderer,
                indexBuffer,
                &pModel->buffers[gltfIdxBufView.buffer].data[gltfIdxBufView.byteOffset + pModel->accessors[accessorIdx].byteOffset],
                (UINT)gltfIdxBufView.byteLength,
                0);

            transitionResource(pRenderer, indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            UINT stride = GetFormatSize(pModel->accessors[accessorIdx].componentType);
            D3D12_INDEX_BUFFER_VIEW indexBufView = {};
            indexBufView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
            indexBufView.Format = (gltfIdxBufView.byteStride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
            indexBufView.SizeInBytes = static_cast<UINT>(gltfIdxBufView.byteLength);

            model.IndexBufViews.push_back(indexBufView);
            model.IndexBufSizes.push_back(static_cast<UINT>(pModel->accessors[accessorIdx].count));
        }
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC boxPsoDesc = {};
    boxPsoDesc.pRootSignature = model.pRootSignature;
    boxPsoDesc.VS = bytecodeFromBlob(model.pModelVs);
    boxPsoDesc.PS = bytecodeFromBlob(model.pModelPs);
    boxPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    boxPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
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

    hr = pDevice->CreateGraphicsPipelineState(&boxPsoDesc, IID_PPV_ARGS(&model.pModelPipeline));

#if defined(_DEBUG)
    dbgModel(*pModel);
#endif 

    if (S_OK != hr)
    {
        ErrorMsg("Model pipeline creation failed.");
        return false;
    }

    // create a descriptor table for cbv and srv
    D3D12_DESCRIPTOR_RANGE descRanges[2] = { cbvRange, srvRange };
    model.DescriptorTable.NumDescriptorRanges = cbvRange.NumDescriptors + srvRange.NumDescriptors;
    model.DescriptorTable.pDescriptorRanges = descRanges;

    return res;
}

void drawModel(Dx12Renderer* pRenderer, GltfModel& model, double dt)
{
    ID3D12Device* pDevice = pRenderer->pDevice;
    ID3D12GraphicsCommandList* pCmdList = pRenderer->cmdSubmissions[pRenderer->currentSubmission].pGfxCmdList;

    // set root sig
    pCmdList->SetGraphicsRootSignature(model.pRootSignature);

    // set pipeline
    pCmdList->SetPipelineState(model.pModelPipeline);

    D3D12_CPU_DESCRIPTOR_HANDLE dest = pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart();
    dest.ptr += pRenderer->cbvSrvUavDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE src = model.TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart = pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart();
    srvTableStart.ptr += pRenderer->cbvSrvUavDescriptorSize;
    // copy descriptor to heap
    pDevice->CopyDescriptorsSimple(1, dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // update model view projection matrix
    glm::mat4 modelMatrix = glm::mat4(1.0);

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
        (float)renderer::width / (float)renderer::height,
        0.1f,
        100.0f);

    // start rotation
    static float angle = 0.0f;
    angle += 0.00000001f * dt;
    modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(1.0f, 1.0f, 1.0f));

    // scale
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.005f, 0.005f, 0.005f));

    glm::mat4 mvp = proj * view * modelMatrix;

    D3D12_RANGE range = {};
    UINT* pData;

    pRenderer->cbvSrvUavUploadHeaps[pRenderer->currentSubmission]->Map(0, &range, reinterpret_cast<void**>(&pData));
    memcpy(pData, &mvp, sizeof(mvp));
    transitionResource(pRenderer, pRenderer->cbvSrvUavHeaps[pRenderer->currentSubmission], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    pCmdList->CopyResource(pRenderer->cbvSrvUavHeaps[pRenderer->currentSubmission], pRenderer->cbvSrvUavUploadHeaps[pRenderer->currentSubmission]);
    transitionResource(pRenderer, pRenderer->cbvSrvUavHeaps[pRenderer->currentSubmission], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    pCmdList->SetDescriptorHeaps(1, &pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].p);
    pCmdList->SetGraphicsRootDescriptorTable(0, pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart());
    pCmdList->SetGraphicsRootDescriptorTable(1, srvTableStart);

    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // draw all meshes
    for (int i = 0; i < model.IndexBuffers.size(); i++)
    {
        // set buffers
        pCmdList->IASetVertexBuffers(0, static_cast<UINT>(model.pBuffers.size()), &model.BufferViews[i]);
        pCmdList->IASetIndexBuffer(&model.IndexBufViews[i]);

        // draw box
        pCmdList->DrawIndexedInstanced(model.IndexBufSizes[i], 1, 0, 0, 0);
    }
}