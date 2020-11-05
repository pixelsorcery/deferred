#include "fsEffect.h"
#include "shaders.h"
#include "renderer.h"

bool initEffect(Dx12Renderer* pRenderer, FsEffect* pEffect, const char* vs, const char* ps)
{
    ID3D12Device* pDevice = pRenderer->pDevice;

    // compile shaders
    pEffect->pModelVs = compileShaderFromFile(vs, "vs_5_1", "main");
    pEffect->pModelPs = compileShaderFromFile(ps, "ps_5_1", "main");

    // empty root signature
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    //rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Serialize and create root signature
    CComPtr<ID3DBlob> serializedRootSig;
    ID3DBlob* errors;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errors);

    if (errors)
    {
        char* error = (char*)errors->GetBufferPointer();
        ErrorMsg(error);
    }


    if (S_OK != hr)
    {
        ErrorMsg("D3D12SerializeRootSignature() failed.\n");
        return false;
    }

    hr = pDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&pEffect->pRootSignature));

    if (S_OK != hr)
    {
        ErrorMsg("CreateRootSignature() failed.\n");
        return false;
    }

    // create pso
    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = false;
    dsDesc.StencilEnable = false;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC effectPsoDesc = {};
    effectPsoDesc.pRootSignature = pEffect->pRootSignature;
    effectPsoDesc.VS = bytecodeFromBlob(pEffect->pModelVs);
    effectPsoDesc.PS = bytecodeFromBlob(pEffect->pModelPs);
    effectPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    effectPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    effectPsoDesc.RasterizerState.DepthClipEnable = TRUE;
    effectPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    effectPsoDesc.SampleMask = UINT_MAX;
    effectPsoDesc.DepthStencilState = dsDesc;
    effectPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    effectPsoDesc.NumRenderTargets = 1;
    effectPsoDesc.RTVFormats[0] = pRenderer->colorFormat;
    effectPsoDesc.SampleDesc.Count = 1;
    //effectPsoDesc.InputLayout.pInputElementDescs = inputElementDescs;
    //effectPsoDesc.InputLayout.NumElements = numInputElements;
    effectPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    hr = pDevice->CreateGraphicsPipelineState(&effectPsoDesc, IID_PPV_ARGS(&pEffect->pPipeline));

    if (S_OK != hr)
    {
        ErrorMsg("Model pipeline creation failed.");
        return false;
    }

    return true;
}

void renderEffect(Dx12Renderer* pRenderer, FsEffect* pEffect)
{
    ID3D12GraphicsCommandList* pCmdList = pRenderer->cmdSubmissions[pRenderer->currentSubmission].pGfxCmdList;

    // bind rs
    // set root sig
    pCmdList->SetGraphicsRootSignature(pEffect->pRootSignature);

    // bind pso
    pCmdList->SetPipelineState(pEffect->pPipeline);

    //draw (3,0)
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmdList->DrawInstanced(3, 1, 0, 0);
}