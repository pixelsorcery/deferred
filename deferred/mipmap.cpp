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