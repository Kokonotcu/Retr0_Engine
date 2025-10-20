#pragma once
#ifndef __ANDROID__
#include <fmt/core.h>
#endif // !__ANDROID__
#include <string>

namespace retro 
{
#ifdef __ANDROID__
	#include <android/log.h>
	template <class... Args>
	inline void print(const  char* str /*fmt*/, Args&&... /*args*/) noexcept 
	{
		__android_log_print(ANDROID_LOG_DEBUG, "Retr0", "Debug : %s", str);
	}
	template <class... Args>
	inline void print(float flt /*fmt*/, Args&&... /*args*/) noexcept
	{
		__android_log_print(ANDROID_LOG_DEBUG, "Retr0", "Debug : %f", flt);
	}
	inline void print(int val) noexcept
	{
		__android_log_print(ANDROID_LOG_DEBUG, "Retr0", "Debug : %d", val);
	}
	inline void print(const char* str) 
	{
		__android_log_print(ANDROID_LOG_DEBUG, "Retr0", "Debug : %s", str);
	}
	inline void print(const std::string& str) 
	{
		__android_log_print(ANDROID_LOG_DEBUG, "Retr0", "Debug : %s", str.c_str());
	}
#endif // __ANDROID__

#ifndef __ANDROID__
	//Do not use this for the sake of android
	//template <class... Args>
	//inline void print(fmt::format_string<Args...> fmtstr, Args&&... args) 
	//{
	//	fmt::print(fmtstr, std::forward<Args>(args)...);
	//}
	inline void print(float flt)
	{
		fmt::print("{}", flt);
	}
	inline void print(const char* str)
	{
		fmt::print("{}", str);
	}
	inline void print(const std::string& str)
	{
		fmt::print("{}", str);
	}
#endif // __ANDROID__
	
}

