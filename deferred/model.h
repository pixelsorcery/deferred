#pragma once

#include<string>
#include "tiny_gltf.h"

class Model
{

};

static bool loadModel(tinygltf::Model& model, const char* filename)
{
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	return res;
}