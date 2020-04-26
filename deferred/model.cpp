#include <vector>
#include <string>
#include <algorithm>

#include "model.h"
#include "gltfHelper.h"
#include "util.h"
#include "dynArray.h"
#include "shaders.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

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

void transformNodes(GltfModel& model, vector<int>& nodes, glm::mat4 matrix)
{
    if (nodes.empty()) return;

    tinygltf::Model* pModel = &model.TinyGltfModel;

    for (int i = 0; i < nodes.size(); i++)
    {
        glm::mat4 localMatrix(1.0);
        int nodeIdx = nodes[i];
        if (pModel->nodes[nodeIdx].matrix.size() > 0)
        {
            vector<float> floatMat(pModel->nodes[nodeIdx].matrix.begin(), pModel->nodes[nodeIdx].matrix.end());
            memcpy(glm::value_ptr(localMatrix), &floatMat[0], sizeof(float) * floatMat.size());
            localMatrix = matrix * localMatrix;
            if (pModel->nodes[nodeIdx].mesh != -1)
            {
                model.Matrices[pModel->nodes[nodeIdx].mesh] = localMatrix;
            }
        }

        transformNodes(model, pModel->nodes[nodeIdx].children, localMatrix);
    }
}

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
    srvRange.NumDescriptors = static_cast<uint>(model.Textures.size());

    D3D12_DESCRIPTOR_RANGE cbvRange = {};
    cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvRange.NumDescriptors = 1;

    D3D12_ROOT_PARAMETER params[2];
    params[0] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
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
    rootSigDesc.NumParameters = 1; // todo fix this magic number
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSigDesc.pParameters = params;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.NumStaticSamplers = 0;

    if (model.Textures.size() > 0)
    {
        rootSigDesc.NumParameters++;
        rootSigDesc.NumStaticSamplers++;
        rootSigDesc.pStaticSamplers = &sampDesc;
    }

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
    if (model.Textures.size() > 0)
    {
        model.pModelVs = compileShaderFromFile("boxTexturedVS.hlsl", "vs_5_1", "main");
        model.pModelPs = compileShaderFromFile("boxTexturedPs.hlsl", "ps_5_1", "main");
    }
    else
    {
        model.pModelVs = compileShaderFromFile("untexturedModel.hlsl", "vs_5_1", "VS");
        model.pModelPs = compileShaderFromFile("untexturedModel.hlsl", "ps_5_1", "PS");
    }

    // Read in all buffers
    for (int i = 0; i < pModel->buffers.size(); i++)
    {
        CComPtr<ID3D12Resource> buffer;
        uint size = static_cast<uint>(pModel->buffers[i].data.size());
        buffer = createBuffer(pRenderer, D3D12_HEAP_TYPE_DEFAULT, size, D3D12_RESOURCE_STATE_COPY_DEST);
        buffer->SetName(L"model_vtx_buffer");
        model.pBuffers.push_back(buffer);

        // upload data
        bool result = uploadBuffer(pRenderer,
            model.pBuffers.back(),
            &pModel->buffers[i].data[0],
            size,
            0);

        if (result == false)
        {
            ErrorMsg("Failed to create vertex buffer.");
            return false;
        }

        transitionResource(pRenderer, model.pBuffers.back(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    }

    // Create pipelines for every mesh and primitive
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[D3D12_STANDARD_VERTEX_ELEMENT_COUNT];
    vector<string> semanticNames;
    for (int i = 0; i < pModel->meshes.size(); i++)
    {
        for (int j = 0; j < pModel->meshes[i].primitives.size(); j++)
        {
            UINT numInputElements = 0;
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

                // create buffer views
                tinygltf::BufferView gltfBufView = pModel->bufferViews[accessor.bufferView];

                uint length = static_cast<uint>(accessor.count) *
                    GetTypeSize(accessor.type) *
                    GetComponentTypeSize(accessor.componentType);

                D3D12_VERTEX_BUFFER_VIEW bufView = {};
                bufView.BufferLocation = model.pBuffers[gltfBufView.buffer]->GetGPUVirtualAddress() + gltfBufView.byteOffset + accessor.byteOffset;
                bufView.SizeInBytes = length;
                bufView.StrideInBytes = (UINT)gltfBufView.byteStride;

                model.BufferViews.push_back(bufView);
            }

            model.PrimitiveBufCounts.push_back(numInputElements);

            // load index buffer
            UINT accessorIdx = pModel->meshes[i].primitives[j].indices;
            UINT bufViewIdx = pModel->accessors[accessorIdx].bufferView;
            tinygltf::BufferView gltfIdxBufView = pModel->bufferViews[bufViewIdx];
            tinygltf::Accessor* pIdxAcessor = &pModel->accessors[accessorIdx];

            // size of data is the componentType (e.g. uint, float, etc) size * Type (vec2, vec3, mat4, etc.) size * count
            unsigned int byteLength = static_cast<uint>(pIdxAcessor->count) *
                                      GetTypeSize(pIdxAcessor->type) *
                                      GetComponentTypeSize(pIdxAcessor->componentType);
            CComPtr<ID3D12Resource> indexBuffer;
            indexBuffer = createBuffer(pRenderer, D3D12_HEAP_TYPE_DEFAULT, byteLength, D3D12_RESOURCE_STATE_COPY_DEST);
            indexBuffer->SetName(L"model_index_buffer");
            model.IndexBuffers.push_back(indexBuffer);

            // pointer to data, offset in buffer view + offset in accessor
            void* data = (void*)pModel->buffers[gltfIdxBufView.buffer].data[gltfIdxBufView.byteOffset + pIdxAcessor->byteOffset];
            
            uploadBuffer(pRenderer,
                indexBuffer,
                &pModel->buffers[gltfIdxBufView.buffer].data[gltfIdxBufView.byteOffset + pIdxAcessor->byteOffset],
                byteLength,
                0);

            transitionResource(pRenderer, indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            UINT stride = GetFormatSize(pIdxAcessor->componentType);
            D3D12_INDEX_BUFFER_VIEW indexBufView = {};
            indexBufView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
            indexBufView.Format = (gltfIdxBufView.byteStride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
            indexBufView.SizeInBytes = byteLength;

            model.IndexBufViews.push_back(indexBufView);
            model.IndexBufSizes.push_back(static_cast<UINT>(pIdxAcessor->count));

            D3D12_DEPTH_STENCIL_DESC dsDesc = {};
            dsDesc.DepthEnable = true;
            dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
            dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
            { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
            dsDesc.FrontFace = defaultStencilOp;
            dsDesc.BackFace = defaultStencilOp;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC modelPsoDesc = {};
            modelPsoDesc.pRootSignature = model.pRootSignature;
            modelPsoDesc.VS = bytecodeFromBlob(model.pModelVs);
            modelPsoDesc.PS = bytecodeFromBlob(model.pModelPs);
            modelPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            modelPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            modelPsoDesc.RasterizerState.DepthClipEnable = TRUE;
            modelPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            modelPsoDesc.SampleMask = UINT_MAX;
            modelPsoDesc.DepthStencilState = dsDesc;
            modelPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            modelPsoDesc.NumRenderTargets = 1;
            modelPsoDesc.RTVFormats[0] = pRenderer->colorFormat;
            modelPsoDesc.SampleDesc.Count = 1;
            modelPsoDesc.InputLayout.pInputElementDescs = inputElementDescs;
            modelPsoDesc.InputLayout.NumElements = numInputElements;
            modelPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

            CComPtr<ID3D12PipelineState> pso;
            hr = pDevice->CreateGraphicsPipelineState(&modelPsoDesc, IID_PPV_ARGS(&pso));

            if (S_OK != hr)
            {
                ErrorMsg("Model pipeline creation failed.");
                return false;
            }

            model.ModelPipelines.push_back(pso);
        }
    }

    // process matrices
    uint defaultScene = pModel->defaultScene;

    tinygltf::Scene scene = pModel->scenes[defaultScene];

    vector<int> curNodes = scene.nodes;
    glm::mat4 initWorldMatrix = glm::mat4(1.0);
    model.Matrices.resize(model.IndexBuffers.size());

    // initialize all mesh world positions using scene hierarchy
    transformNodes(model, scene.nodes, initWorldMatrix);

    // make sure the matrices are the same as the number of prims we're drawing otherwise this whole thing blow us
    assert(model.Matrices.size() == model.IndexBuffers.size());

    // view projection
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));


    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
        (float)renderer::width / (float)renderer::height,
        0.1f,
        100.0f);

    // create constant heap for matrices
    uint alignedMatrixSize = ((sizeof(glm::mat4) + 255) & ~255);
    model.ConstantBuffer = createBuffer(pRenderer, D3D12_HEAP_TYPE_DEFAULT, model.Matrices.size() * alignedMatrixSize, D3D12_RESOURCE_STATE_COPY_DEST);

    // start rotation
    //static float angle = 0.0f;
    //angle += 0.01f;

    model.ConstantBufferIncrement = alignedMatrixSize;
    glm::mat4* pMatrices = new glm::mat4[model.Matrices.size() * alignedMatrixSize];
    glm::mat4* ptr = pMatrices;
    for (int i = 0; i < model.Matrices.size(); i++)
    {
        // update buffer
        *ptr = model.Matrices[i];
        //glm::rotate(pMatrices[i], angle, glm::vec3(1.0f, 1.0f, 1.0f));
        ptr += alignedMatrixSize / sizeof(glm::mat4);
    }

    // upload buffer
    uploadBuffer(pRenderer, model.ConstantBuffer, pMatrices, model.Matrices.size() * alignedMatrixSize, 0);
    transitionResource(pRenderer, model.ConstantBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    delete[] pMatrices;

#if defined(_DEBUG)
    dbgModel(*pModel);
#endif 

    // create a descriptor table for cbv and srv
    //D3D12_DESCRIPTOR_RANGE descRanges[2] = { cbvRange, srvRange };
    //model.DescriptorTable.NumDescriptorRanges = cbvRange.NumDescriptors + srvRange.NumDescriptors;
    //model.DescriptorTable.pDescriptorRanges = descRanges;

    return res;
}

void drawModel(Dx12Renderer* pRenderer, GltfModel& model, double dt)
{
    ID3D12Device* pDevice = pRenderer->pDevice;
    ID3D12GraphicsCommandList* pCmdList = pRenderer->cmdSubmissions[pRenderer->currentSubmission].pGfxCmdList;

    // todo: update matrices and copy them here

    // set root sig
    pCmdList->SetGraphicsRootSignature(model.pRootSignature);

    D3D12_CPU_DESCRIPTOR_HANDLE dest = pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart();
    dest.ptr += pRenderer->cbvSrvUavDescriptorSize;

    D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart;
    if (model.Textures.size() > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE src = model.TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        srvTableStart = pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart();
        srvTableStart.ptr += pRenderer->cbvSrvUavDescriptorSize;
        // copy descriptor to heap
        pDevice->CopyDescriptorsSimple(1, dest, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    pCmdList->SetDescriptorHeaps(1, &pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].p);

    if (model.Textures.size() > 0)
    {
        pCmdList->SetGraphicsRootDescriptorTable(1, srvTableStart);
    }

    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // set pipeline
    uint bufferViewIdx = 0;

    D3D12_RANGE range = {};
    UINT* pData;

    D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddr = model.ConstantBuffer->GetGPUVirtualAddress();

    for (uint i = 0; i < model.IndexBuffers.size(); i++)
    {
        glm::mat4 modelMatrix = model.Matrices[i];

        // set up transforms
        static float angle = 0.0f;
        angle += 0.00000001f * static_cast<float>(dt);
        modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(1.0f, 1.0f, 1.0f));

        // scale
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.005f, 0.005f, 0.005f));

        pCmdList->SetGraphicsRootConstantBufferView(0, constantBufferAddr);

        constantBufferAddr += model.ConstantBufferIncrement;

        pCmdList->SetPipelineState(model.ModelPipelines[i]);

        // set buffers
        pCmdList->IASetVertexBuffers(0, model.PrimitiveBufCounts[i], &model.BufferViews[bufferViewIdx]);
        pCmdList->IASetIndexBuffer(&model.IndexBufViews[i]);

        // draw box
        pCmdList->DrawIndexedInstanced(model.IndexBufSizes[i], 1, 0, 0, 0);

        bufferViewIdx += model.PrimitiveBufCounts[i];
    }
}