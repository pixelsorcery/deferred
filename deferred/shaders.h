#pragma once

#include <d3d12.h>

ID3DBlob* compileShaderFromFile(char const* filename, char const* profile, char const* entrypt);
D3D12_SHADER_BYTECODE bytecodeFromBlob(ID3DBlob* blob);
ID3DBlob* compileSource(char const* source, char const* profile, char const* entrypt);