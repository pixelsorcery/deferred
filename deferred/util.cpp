#include "util.h"

#include <Windows.h>


void ErrorMsg(const char* msg)
{
    OutputDebugString(msg);
}

void WarningMsg(const char* msg)
{
    OutputDebugString(msg);
}