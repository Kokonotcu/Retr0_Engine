#pragma once
#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <unordered_map>
#include <fmt/core.h>

namespace ShaderCompiler
{	
	void CompileFromDir(std::string dir);
};