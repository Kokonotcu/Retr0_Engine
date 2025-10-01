#include <gfx/vk_engine.h>

int main(int argc, char* argv[])
{

#ifdef DEBUG
	FileManager::ShaderCompiler::CompileFromDir(FileManager::path::GetShadersDirectory().string());
#endif // DEBUG

	//ShaderCompiler::CompileFromDir(FilePathManager::GetShadersDirectory().string());
	VulkanEngine engine;

	engine.Init();	
	try
	{
		engine.Run();	
	}
	catch (const std::exception& e)
	{
		fmt::print("Exception {} caught in main loop \n", e.what());
		return EXIT_FAILURE;
	}
	engine.Cleanup();	

	return 0;
}
