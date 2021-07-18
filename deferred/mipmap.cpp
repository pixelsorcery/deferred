#include "renderer.h"
#include "shaders.h"

/*
* Create root signature, compile shaders and create a pipeline.
*/
bool initializeMipMapPipelines(Dx12Renderer* pRenderer) {
    D3D12_ROOT_PARAMETER params[3] = {};
    // 4 constants
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[0].Constants.Num32BitValues = 4;

    // 1 src texture
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = static_cast<uint>(1);

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &srvRange;

    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = static_cast<uint>(4);
    
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 1;
    params[2].DescriptorTable.pDescriptorRanges = &uavRange;

    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 3;
    rootSigDesc.pParameters = params;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &samplerDesc;

    CComPtr<ID3DBlob> serializedRootSig;
    ID3DBlob* errors;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &errors);

    if (errors) {
        char* error = (char*)errors->GetBufferPointer();
        char* error2 = error;
    }

    if (FAILED(hr))
    {
        ErrorMsg("Mipmap root signature serialization failed.");
        return false;
    }

    ID3D12Device* pDevice = pRenderer->pDevice;

    hr = pDevice->CreateRootSignature(0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&pRenderer->pMipmapRootSignature));

    if (FAILED(hr))
    {
        ErrorMsg("Mipmap root signature creation failed.");
        return false;
    }

    for (int i = 0; i < NUM_MIPMAP_SHADER_TYPES; i++)
    {
        DynArray<D3D_SHADER_MACRO> macros;

        macros.push({ "NON_POWER_OF_TWO", nonPow2Strings[i].c_str() });
        macros.push({ NULL, NULL });

        pRenderer->pMipmapCS[i] = compileShaderFromFile("genMips.hlsl", "cs_5_1", "main", macros.arr.get());

        D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc = {};
        pipelineDesc.CS = bytecodeFromBlob(pRenderer->pMipmapCS[i]);
        pipelineDesc.pRootSignature = pRenderer->pMipmapRootSignature;

        hr = pDevice->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&pRenderer->pMipmapPipelines[i]));

        if (S_OK != hr)
        {
            ErrorMsg("Mipmap pipeline creation failed.");
            return false;
        }
    }

    return true;
}

bool createMipMaps(Dx12Renderer* pRenderer, const Texture& tex)
{
    if (tex.desc.MipLevels == 1) return true;

    // shader only generates 4 mip levels at a time
    for (int i = 0; i < tex.desc.MipLevels;)
    {
        int topMipWidth = static_cast<int>(tex.desc.Width >> i);
        int topMipHeight = static_cast<int>(tex.desc.Height >> i);
        int destMipWidth = topMipWidth >> 1;
        int destMipHeight = topMipHeight >> 1;

        destMipWidth = destMipWidth == 0 ? 1 : destMipWidth;
        destMipHeight = destMipHeight == 0 ? 1 : destMipHeight;

        // basically every bit is another mip level
        DWORD additionalMips = 0;
        _BitScanForward(&additionalMips, destMipWidth | destMipHeight);
        const int mips2Generate = additionalMips > 3 ? 3 : additionalMips;
        const int totalMips = mips2Generate + 1;

        pRenderer->GetCurrentCmdList()->SetComputeRootSignature(pRenderer->pMipmapRootSignature);

        const int shaderType = (topMipWidth & 1) || ((topMipHeight & 1) << 1);
        pRenderer->GetCurrentCmdList()->SetPipelineState(pRenderer->pMipmapPipelines[shaderType]);

        union param
        {
            int ival;
            float fval;
        };

        param widthParam = {};
        widthParam.fval = 1.0f / destMipWidth;
        param heightParam = {};
        heightParam.fval = 1.0f / destMipHeight;

        pRenderer->GetCurrentCmdList()->SetComputeRoot32BitConstant(0, i, 0);
        pRenderer->GetCurrentCmdList()->SetComputeRoot32BitConstant(0, totalMips, 1);
        pRenderer->GetCurrentCmdList()->SetComputeRoot32BitConstant(0, widthParam.ival, 2); //todo verify this
        pRenderer->GetCurrentCmdList()->SetComputeRoot32BitConstant(0, heightParam.ival, 3);

        HeapMgr* pHeapMgr = &pRenderer->heapMgr;

        pRenderer->GetCurrentCmdList()->SetDescriptorHeaps(1, &pHeapMgr->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].p);

        D3D12_CPU_DESCRIPTOR_HANDLE descHandle = pHeapMgr->getCPUDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tex.srvDescIdx);
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = pRenderer->heapMgr.copyDescriptorsToGpuHeap(pRenderer->pDevice,
                                                                                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                                               descHandle,
                                                                                               1);
        pRenderer->GetCurrentCmdList()->SetComputeRootDescriptorTable(1, srvGpuHandle);

        D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = pHeapMgr->getCPUDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tex.mipIdx[i+1]);
        D3D12_GPU_DESCRIPTOR_HANDLE uavGpuHandle = pRenderer->heapMgr.copyDescriptorsToGpuHeap(pRenderer->pDevice,
                                                                                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                                               uavHandle,
                                                                                               1);
        pRenderer->GetCurrentCmdList()->SetComputeRootDescriptorTable(2, uavGpuHandle);

        for (int mipIdx = 1; mipIdx < totalMips; mipIdx++)
        {
            uavHandle = pHeapMgr->getCPUDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tex.mipIdx[i + 1 + mipIdx]);
            pRenderer->heapMgr.copyDescriptorsToGpuHeap(pRenderer->pDevice,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                        uavHandle,
                                                        1);
        }

        pRenderer->GetCurrentCmdList()->Dispatch(std::max(destMipWidth/8, 8), std::max(destMipHeight/8, 8), 1);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = tex.pRes;
        pRenderer->GetCurrentCmdList()->ResourceBarrier(1, &barrier);

        i += totalMips;
    }

    return true;
}
