#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <d3d11.h>

#include <iostream>
#include <fstream>
#include <chrono>
#include <bitset> 
#include <vector>
#include <array>
#include <stack>
#include <mutex>
#include <filesystem>
#include <functional>
#include <future>

#include "imgui.h"
#include "ImGuiHelpers.h"

#include "Logger.h"
#include "WindowControl.h"
#include "DebugTools.h"
#include "TimeHelper.h"
#include "TextureLoader.h"
#include "FileWatcher.h"
#include "Matrix4x4.hpp"


#define PERFORMANCETAG(name) auto _ = Tools::ScopeDiagnostic(name);
#define SAFE_DELETE(pointer) if(pointer) { delete (pointer); (pointer) = nullptr; }
#define SAFE_RELEASE(pointer) if(pointer) { pointer->Release(); pointer = nullptr; }
#define SAFE_DELETE_ARRAY(pointer) if(pointer) { delete[] (pointer); (pointer) = nullptr; }

#ifndef ZEROMEMORY
#define ZEROMEMORY(adr, size) memset(adr, 0, size)
#endif // !ZeroMemory

#define WIPE(item) ZEROMEMORY(&item, sizeof(item))

#define STRING(arg) #arg
#define STRINGVALUE(arg) STRING(arg)

#define TODEG(arg) ((arg) * 57.2957795131f)
#define TORAD(arg) ((arg) * 0.01745329251f)

#define BIT(x) (1ULL << x)

#define LERP(a, b, val) ((a) * (1 - (val)) + (val) * (b))
#define INVERSELERP(a, b, val) (((val) - (a)) / ((b) - (a)))

#define CLAMP(low, high, value) ((value) < (low) ? (low) : ((high) < (value) ? (high) : (value)))

#define STANDARDMARG (0.0001)
#define CLOSEENUF(a, b) (static_cast<double>(abs(a - b)) <= STANDARDMARG)
#define CLOSEENUFCUSTOM(a, b, margin) (abs(a - b) <= margin)


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define PFSTRING "%s"
#define PFSIZET "%zu"
#define PFFLOAT "%.f"
#define PFINT "%d"
#define PFUINT "%u"

#define PI 3.14159265359f
#define TAU (2.0 * PI)
#define CUBEHALFSIZETOENCAPSULATINGSPHERERADIUS 1.73f


#define NAMETHREAD(name) SetThreadDescription(GetCurrentThread(), name);