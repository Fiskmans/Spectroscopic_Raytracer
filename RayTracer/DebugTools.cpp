#include "stdafx.h"
#include "DebugTools.h"
#include <bitset>

std::map<std::string, std::vector<std::string>>* DebugTools::FileList = nullptr;
std::string DebugTools::myUsername;
std::set<std::wstring> DebugTools::CommandLineFlags;
