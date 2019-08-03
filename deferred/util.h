#pragma once

#include <Windows.h>

using int64 = long long;
using uint64 = unsigned long long;
using uint = unsigned int;

void ErrorMsg(const char* msg)
{
    OutputDebugString(msg);
}

void WarningMsg(const char* msg)
{
    OutputDebugString(msg);
}