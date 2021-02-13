/*AMDG*/
#include <d3d12.h>
#include <d3dcompiler.h>
#include <fstream>
#include <string>
#include <assert.h>
#include "util.h"
#include "shaders.h"
#include <vector>
#include <atlbase.h>

std::vector<CComPtr<ID3DBlob>> shaders;

D3D12_SHADER_BYTECODE bytecodeFromBlob(ID3DBlob* blob)
{
    D3D12_SHADER_BYTECODE bytecode;
    bytecode.pShaderBytecode = blob->GetBufferPointer();
    bytecode.BytecodeLength = blob->GetBufferSize();
    return bytecode;

}
// read shader in from file
ID3DBlob* compileShaderFromFile(char const* filename, char const* profile, char const* entrypt, D3D_SHADER_MACRO* args)
{
    ID3DBlob* code = nullptr;

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
ID3DBlob* compileSource(char const* source, char const* profile, char const* entrypt, D3D_SHADER_MACRO* args)
{
    ID3DBlob* code   = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompile(source, strlen(source), NULL, NULL, NULL, entrypt, profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG /*D3DCOMPILE_OPTIMIZATION_LEVEL3*/, 0,
        &code, &errors);

    if (errors)
    {
        char* error = (char*)errors->GetBufferPointer();
        errors->Release();
        assert(!"shader failed to compile!\n");
    }

    return code;
}

bool initShaders()
{
    for (int i = 0; i < SHADER_TYPES::NUM_SHADER_TYPES; i++)
    {
        ID3DBlob* shader = compileShaderFromFile(shaderStrings[i], ((i % 2 == 0) ? "vs_5_1" : "ps_5_1"), shaderMain[i], nullptr);
        if (shader == nullptr)
        {
            return false;
        }
        shaders.push_back(shader);
    }

    return true;
}