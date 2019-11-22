#pragma once

#include <string>

// if the string is something like TEXCOORD_1
// semantic will be TEXCOORD and index will be 1
static void processSemantics(const std::string& input, std::string& semantic, int& index)
{
    semantic = input;
    if (isdigit(input.back()))
    {
        index = input.back() - '0';
        semantic.pop_back();
        if (semantic.back() == '_')
        {
            semantic.pop_back();
        }
    }
}

static int GetFormatSize(int id)
{
    switch (id)
    {
        case 5120: return 1; //(BYTE)
        case 5121: return 1; //(UNSIGNED_BYTE)1
        case 5122: return 2; //(SHORT)2
        case 5123: return 2; //(UNSIGNED_SHORT)2
        case 5124: return 4; //(SIGNED_INT)4
        case 5125: return 4; //(UNSIGNED_INT)4
        case 5126: return 4; //(FLOAT)
    }
    return -1;
}

// Returns dxgi format based on tinygltf type
static DXGI_FORMAT GetFormat(int type, int id)
{
    if (type == TINYGLTF_TYPE_SCALAR)
    {
        switch (id)
        {
            case 5120: return DXGI_FORMAT_R8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32_FLOAT; //(FLOAT)
        }
    }
    else if (type == TINYGLTF_TYPE_VEC2)
    {
        switch (id)
        {
            case 5120: return DXGI_FORMAT_R8G8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8G8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16G16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16G16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32_FLOAT; //(FLOAT)
        }
    }
    else if (type == TINYGLTF_TYPE_VEC3)
    {
        switch (id)
        {
            case 5120: return DXGI_FORMAT_UNKNOWN; //(BYTE)
            case 5121: return DXGI_FORMAT_UNKNOWN; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_UNKNOWN; //(SHORT)2
            case 5123: return DXGI_FORMAT_UNKNOWN; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32B32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32B32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32B32_FLOAT; //(FLOAT)
        }
    }
    else if (type == TINYGLTF_TYPE_VEC4)
    {
        switch (id)
        {
            case 5120: return DXGI_FORMAT_R8G8B8A8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8G8B8A8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16G16B16A16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16G16B16A16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32B32A32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32B32A32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32B32A32_FLOAT; //(FLOAT)
        }
    }

    return DXGI_FORMAT_UNKNOWN;
}