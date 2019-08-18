/*AMDG*/
#include <d3d12.h>
#include <d3dcompiler.h>
#include <fstream>
#include <string>
#include "util.h"

D3D12_SHADER_BYTECODE bytecodeFromBlob(ID3DBlob* blob)
{
	D3D12_SHADER_BYTECODE bytecode;
	bytecode.pShaderBytecode = blob->GetBufferPointer();
	bytecode.BytecodeLength = blob->GetBufferSize();
	return bytecode;

}
// read shader in from file
ID3DBlob* compileShaderFromFile(char const* filename, const char* profile, char const* entrypt)
{
	ID3DBlob* code;
	ID3DBlob* errors;

	std::string shaderText;

	std::ifstream file(filename, std::ifstream::in | std::ifstream::binary);
	std::string str;
	while (std::getline(file, str))
	{
		shaderText += str;
		shaderText += "\n";
	}

	HRESULT hr = D3DCompile(shaderText.c_str(), strlen(shaderText.c_str()), NULL, NULL, NULL, entrypt, profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
		&code, &errors);

	if (errors)
	{
		char* error = (char*)errors->GetBufferPointer();
		errors->Release();
		ErrorMsg("shader failed to compile!\n");
	}

	file.close();

	return code;
}
