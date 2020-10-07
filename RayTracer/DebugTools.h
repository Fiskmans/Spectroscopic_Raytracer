#pragma once
#include <map>
#include <vector>
#include <string>
#include <set>

class ModelInstance;
class Scene;
class DebugTools
{
public:
	static std::map<std::string, std::vector<std::string>>* FileList;
	static std::string myUsername;
	static std::set<std::wstring> CommandLineFlags;
};
