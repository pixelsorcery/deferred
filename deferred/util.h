#pragma once

using int64 = long long;
using uint64 = unsigned long long;
using uint = unsigned int;

void ErrorMsg(const char* msg);

void WarningMsg(const char* msg);

enum RendererType
{
    dx12,
    vulkan,
};