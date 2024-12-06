workspace "Vulkan"
    architecture "x64"
    configurations {"Debug", "Release"}
    location "Vulkan"
    startproject "Vulkan"

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    IncludeDir = {}
    IncludeDir["GLFW"] = "D:/openglEnv/GLFW/glfw/include"
    IncludeDir["GLM"] = "D:/openglEnv/glm-1.0.1"
    IncludeDir["Vulkan"] = "C:/VulkanSDK/1.3.296.0/Include"

project "Vulkan"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/**.cpp",
        "%{prj.name}/**.h"
    }
    includedirs
    {
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLM}",
        "%{IncludeDir.Vulkan}",
        "%{prj.name}/src"
    }
    links
    {
        "D:/openglEnv/GLFW/glfw/build/src/Debug/glfw3.lib",
        "C:/VulkanSDK/1.3.296.0/Lib/vulkan-1.lib"
    }
    defines{
        "GLFW_INCLUDE_NONE"
    }
    filter "system:windows"
        systemversion "latest"
    filter "configurations:Debug"
        symbols "on"
    filter "configurations:Release"
        optimize "on"
    filter "configurations:Dist"
        optimize "on"