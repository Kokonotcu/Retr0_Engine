#include <gfx/vk_engine.h>

int retr0_main(int argc, char** argv);

#ifdef __ANDROID__
#include <SDL3/SDL_main.h>
//#include <android/log.h>
extern "C" int SDL_main(int argc, char** argv) {
	try {
		return retr0_main(argc, argv);
	}
	catch (const std::exception& e) {
		__android_log_print(retro::ANDROID_LOG_FATAL, "Retr0", "Uncaught exception: %s", e.what());
	}
	catch (...) {
		__android_log_print(retro::ANDROID_LOG_FATAL, "Retr0", "Uncaught non-std exception");
	}
	return EXIT_FAILURE;
}

#else
int main(int argc, char** argv)
{
	return retr0_main(argc,argv);
}
#endif

int retr0_main(int argc, char** argv)
{

#ifndef __ANDROID__
	FileManager::ShaderCompiler::CompileFromDir(FileManager::path::GetShadersDirectory().string());
#endif

	VulkanEngine engine;

	engine.Init();
	try
	{
		engine.Run();
	}
	catch (const std::exception& e)
	{
		retro::print("Exception caught in main loop : ");
		retro::print(e.what());
		return EXIT_FAILURE;
	}
	engine.Cleanup();

	return 0;
}