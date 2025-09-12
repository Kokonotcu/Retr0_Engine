-- xmake.lua — Retr0 Engine (SDL3 vendored on Windows, system on Linux)
set_project("Retr0 Engine")
set_languages("c++20")
add_rules("mode.release", "mode.debug")

if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
	set_symbols("debug")
    set_optimize("none")
end

if is_mode("release") then
    set_symbols("hidden")
    set_strip ("all")
    set_optimize("fastest")
end


-- Put outputs in a predictable place
set_targetdir("bin/$(plat)/$(mode)")

-- ---- Root-scope requirements (OK here; NOT inside target) ----
-- Linux uses system packages via pkg-config; shaderc optional.
add_requires("pkgconfig::vulkan",  {system = true, optional = true})
add_requires("pkgconfig::sdl3",    {system = true, optional = true})
add_requires("pkgconfig::shaderc", {system = true, optional = true})

target("retr0_engine")
    set_kind("binary")

    -- Your sources
    add_includedirs("source", {public = true})
	
	-- Define groups (patterns are relative to rootdir)
	add_filegroups("Source Files", { rootdir = "source", files = {"**.cpp"},           group = "Source Files" })
	add_filegroups("Header Files", { rootdir = "source", files = {"**.h", "**.hpp"},   group = "Header Files" })
	
	-- Add those groups to the target
	add_files("source/**.cpp",    { group = "Source Files" })
	add_headerfiles("source/**.h", "source/**.hpp",    { group = "Header Files" })
	

    -- Vendored header-only libs
    add_includedirs(
        "external/glm",
        "external/stb_image",
        "external/vma",
        "external/vkbootstrap",
        "external/fastgltf/include",
        "external/fmt/include",
        "external/imgui",
		"external/SDL/include",
		"source/Utilities",
        {public = true}
    )
    add_defines("FMT_HEADER_ONLY")

    add_files(
        "external/imgui/imgui.cpp",
        "external/imgui/imgui_demo.cpp",
        "external/imgui/imgui_draw.cpp",
        "external/imgui/imgui_tables.cpp",
        "external/imgui/imgui_widgets.cpp",
        "external/imgui/imgui_impl_sdl3.cpp",
        "external/imgui/imgui_impl_vulkan.cpp",
		"external/vkbootstrap/**.cpp",
		"external/fastgltf/src/**.cpp",
		"source/Utilities/**.cpp"
    )
	add_headerfiles("source/Utilities/**.h", "source/Utilities/**.hpp")

    -- -------------------- Linux --------------------
    if is_plat("linux") then
		add_syslinks("pthread", "dl")
		add_ldflags("-fuse-ld=lld", {tools = {"cc", "cxx", "ld"}, force = false})

		-- Use system libs via pkg-config (declared at root)
		if has_package("pkgconfig::sdl3") then
			add_packages("pkgconfig::sdl3")
		else
			raise("libsdl3-dev not found (pkg-config sdl3). Install it.")
		end
		if has_package("pkgconfig::vulkan") then
			add_packages("pkgconfig::vulkan")
		else
			raise("Vulkan dev packages not found (pkg-config vulkan).")
		end
		if has_package("pkgconfig::shaderc") then
			add_packages("pkgconfig::shaderc") -- optional
		end
	
		-- If you drop .so next to the binary, loader will find them
		add_rpathdirs("$ORIGIN")
	end


    -- -------------------- Windows (MSVC, vendored SDL) --------------------
    if is_plat("windows") then
	
        set_toolchains("msvc")
        add_cxflags("/MP")
        add_ldflags("/DEBUG")
		


        -- Vulkan via LunarG SDK (headers + import lib)
        local vksdk = os.getenv("VULKAN_SDK")
        if vksdk and #vksdk > 0 then
            add_includedirs(path.join(vksdk, "Include"), {public = true})
			add_includedirs(path.join(vksdk, "Include/shaderc"))
            add_linkdirs(path.join(vksdk, "Lib"))
            add_links("vulkan-1")
        else
            raise("VULKAN_SDK not set. Install LunarG Vulkan SDK.")
        end

        -- SDL3 from your repo: external/SDL
        local sdl = path.join(os.projectdir(), "external", "SDL")
		add_includedirs(path.join(sdl, "include"), {public = true})
		
		sub = ""
        do
            arch = (get_config("arch") or ""):lower()
            if arch == "x64" or arch == "x86_64" then
                sub = "x64"		
				print("platform : x64")
            elseif arch == "arm64" then
                sub = "arm64"
				print("platform : arm64")
			else
                sub = "x86"
				print("platform : x86")
            end
			
			add_linkdirs(path.join(sdl, "lib", sub))
            add_links("SDL3") -- dynamic; ship SDL3.dll
            
        end

        -- Optional: shaderc from Vulkan SDK if present
        if vksdk and os.isfile(path.join(vksdk, "Lib", "shaderc_shared.lib")) then
            add_linkdirs(path.join(vksdk, "Lib"))
            add_links("shaderc_shared")
        end

        -- After build, drop DLLs beside the .exe
        after_build(function (t)
			local out = t:targetdir()
			arch = (get_config("arch") or ""):lower()
            if arch == "x64" or arch == "x86_64" then
                sub = "x64"		
				print("platform : x64")
            elseif arch == "arm64" then
                sub = "arm64"
				print("platform : arm64")
			else
                sub = "x86"
				print("platform : x86")
            end
			os.cp(path.join(sdl, "lib", sub, "SDL3.dll"), path.join(os.projectdir(),out))
			os.cp(path.join(os.projectdir(),"assets"), path.join(os.projectdir(),out))
			os.cp(path.join(os.projectdir(),"shaders"), path.join(os.projectdir(),out))
		
			-- optional shaderc from Vulkan SDK
			local vksdk = os.getenv("VULKAN_SDK")
			if vksdk then
				os.trycp(path.join(vksdk, "Bin", "shaderc_shared.dll"), out)
				os.trycp(path.join(vksdk, "Bin", "shaderc.dll"), out)
			end
		end)
    end

		-- Output name
	if is_plat("windows") then
		set_filename("retr0_engine.exe")
	else
		set_filename("retr0_engine") -- ELF on Linux, no extension
	end