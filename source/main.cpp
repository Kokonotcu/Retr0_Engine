#include <vk_engine.h>

int main(int argc, char* argv[])
{

#ifdef _DEBUG
	ShaderCompiler::CompileFromDir("shaders/");
#endif // DEBUG

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
