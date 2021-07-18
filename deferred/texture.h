#pragma once
#include <d3d12.h>
#include <atlbase.h>

struct Texture
{
    CComPtr<ID3D12Resource> pRes;
    D3D12_RESOURCE_DESC     desc;
    int srvDescIdx;
    int mipIdx[12];
};