#pragma once

#include <atlbase.h>
#include <d3d12.h>

struct FsEffect;
struct Dx12Renderer;

bool initEffect(Dx12Renderer*, FsEffect*, const char*, const char*);
void renderEffect(Dx12Renderer*, FsEffect*);

struct FsEffect
{
    CComPtr<ID3D12RootSignature> pRootSignature;
    CComPtr<ID3D12PipelineState> pPipeline;
    CComPtr<ID3DBlob>            pModelVs, pModelPs;
};