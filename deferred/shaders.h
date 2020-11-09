#pragma once

#include <d3d12.h>

enum SHADER_TYPES
{
    BUMP_MAPPED_VS,
    BUMP_MAPPED_PS,
    TEXTURED_VS,
    TEXTURED_PS,
    UNTEXTURED_VS,
    UNTEXTURED_PS,
    NUM_SHADER_TYPES
};

static const char* shaderStrings[] = {
    "bumpMapped.hlsl",
    "bumpMapped.hlsl",
    "texturedVs.hlsl",
    "texturedPs.hlsl",
    "untexturedModel.hlsl",
    "untexturedModel.hlsl"
};

static const char* shaderMain[] = {
    "mainVS",
    "mainPS",
    "main",
    "main",
    "VS",
    "PS"
};

ID3DBlob* compileShaderFromFile(char const* filename, char const* profile, char const* entrypt);
D3D12_SHADER_BYTECODE bytecodeFromBlob(ID3DBlob* blob);
ID3DBlob* compileSource(char const* source, char const* profile, char const* entrypt);
bool initShaders();