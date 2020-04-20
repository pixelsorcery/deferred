/*AMDG*/
#include <d3d12.h>
#include <d3dcompiler.h>
#include <fstream>
#include <string>
#include <assert.h>
#include "util.h"
#include "shaders.h"

D3D12_SHADER_BYTECODE bytecodeFromBlob(ID3DBlob* blob)
{
    D3D12_SHADER_BYTECODE bytecode;
    bytecode.pShaderBytecode = blob->GetBufferPointer();
    bytecode.BytecodeLength = blob->GetBufferSize();
    return bytecode;

}
// read shader in from file
ID3DBlob* compileShaderFromFile(char const* filename, char const* profile, char const* entrypt)
{
    ID3DBlob* code;

    std::string shaderText;

    std::ifstream file(filename);
    if (file.is_open())
    {
        std::string str;
        while (std::getline(file, str))
        {
            shaderText += str;
            shaderText += "\n";
        }
    }
    else
    {
        ErrorMsg("Cannot find shader file.");
        return nullptr;
    }

    code = compileSource(shaderText.c_str(), profile, entrypt);

    file.close();

    return code;
}

// compile shader
ID3DBlob* compileSource(char const* source, char const* profile, char const* entrypt)
{
    ID3DBlob* code;
    ID3DBlob* errors;
    HRESULT hr = D3DCompile(source, strlen(source), NULL, NULL, NULL, entrypt, profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG /*D3DCOMPILE_OPTIMIZATION_LEVEL3*/, 0,
        &code, &errors);

    assert(SUCCEEDED(hr));

    if (errors)
    {
        char* error = (char*)errors->GetBufferPointer();
        errors->Release();
        assert(!"shader failed to compile!\n");
    }

    return code;
}