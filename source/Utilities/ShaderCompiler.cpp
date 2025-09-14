#include "ShaderCompiler.h"

namespace fs = std::filesystem;

namespace ShaderCompiler
{
	namespace
	{
		enum extensionType 
		{
			COMPUTE,
			VERTEX,
			FRAGMENT,
			SPIRV,
			INVALID
		};

		int determineExtension(std::string extension) 
		{
			if (extension == ".comp" )
				return extensionType::COMPUTE;
			else if (extension == ".frag")
				return extensionType::FRAGMENT;
			else if (extension == ".vert")
				return extensionType::VERTEX;
			else if (extension == ".spv")
				return extensionType::SPIRV;

			throw("File extension isn't supported.");
			return extensionType::INVALID;
		}

		bool compileShader(const fs::path& sourcePath, const fs::path& spvPath, shaderc_shader_kind kind)
		{
			std::ifstream file(sourcePath);
			if (!file.is_open()) throw std::runtime_error("Cannot open shader file: " + sourcePath.string());

			std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

			shaderc::Compiler compiler;
			shaderc::CompileOptions options;

			auto result = compiler.CompileGlslToSpv(source, kind, sourcePath.string().c_str(), options);

			if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
				throw std::runtime_error("Error compiling " + sourcePath.string() + ": " + result.GetErrorMessage());
				return false;
			}

			std::ofstream out(spvPath, std::ios::binary);
			out.write(reinterpret_cast<const char*>(result.cbegin()), (result.cend() - result.cbegin()) * sizeof(uint32_t));
			return true;
		}

		bool needsRecompile(const fs::path& sourcePath, const fs::path& spvPath)
		{
			if (!fs::exists(spvPath)) return true;
			if (!fs::exists(sourcePath)) throw std::runtime_error("Shader source not found: " + sourcePath.string());
			return fs::last_write_time(sourcePath) > fs::last_write_time(spvPath);
		}

		void LoadShaderFromDir(fs::path path, shaderc_shader_kind kind)
		{
			fs::path spvPath = path;
			spvPath.replace_extension(spvPath.extension().string() + ".spv");

			if (needsRecompile(path, spvPath))
			{
				if (!compileShader(path, spvPath, kind))
					throw std::runtime_error("Failed to compile shader: " + path.string());
			}

			//auto data = readSPV(spvPath);

			// Here you can store the SPIR-V binary in a map if you want
			// shaders[path.string()] = std::make_shared<SPIRV>(data);
		}

		void LoadCompFromDir(fs::path path)
		{
			LoadShaderFromDir(path, shaderc_compute_shader);
		}
		
		void LoadFragFromDir(fs::path path)
		{
			LoadShaderFromDir(path, shaderc_fragment_shader);
		}
		void LoadVertFromDir(fs::path path)
		{
			LoadShaderFromDir(path, shaderc_vertex_shader);
		}
		void LoadSpvFromDir(fs::path path)
		{
			//auto data = readSPV(path);
		}

	}

	void CompileFromDir(std::string dir) 
	{
		try
		{
			for (const auto& entry : fs::recursive_directory_iterator(dir))
			{
				if (entry.is_regular_file())
				{
					switch (determineExtension(entry.path().extension().string()))
					{
					case extensionType::COMPUTE:
						LoadCompFromDir(entry.path());
						break;

					case extensionType::FRAGMENT:
						LoadFragFromDir(entry.path());
						break;
					case extensionType::VERTEX:
						LoadVertFromDir(entry.path());
						break;
					case extensionType::SPIRV:
						LoadSpvFromDir(entry.path());
						break;


					default:
						break;
					}
				}
			}
		}
		catch (const std::exception& e)
		{

		}
		
	}

	

	std::vector<char> readSPV(const fs::path& spvPath)
	{
		std::ifstream file(spvPath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) throw std::runtime_error("Cannot open SPV file: " + spvPath.string());

		std::vector<char> buffer(file.tellg());
		file.seekg(0);
		file.read(buffer.data(), buffer.size());
		return buffer;
	}

}





