#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

class HelloTriangleApplication
{
public:
    HelloTriangleApplication()
    {
        initVulKan();
    }
    void run()
    {
        mainLoop();
    }
    ~HelloTriangleApplication()
    {
        cleanup();
    }
private:
    void initVulKan()
    {
        createInstance();
    }
    void mainLoop()
    {
       
    }
    void cleanup()
    {
        
    }
    void createInstance()
    {
        //the message in vulkan deliver through struct
        VkApplicationInfo appinfo{};
        appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appinfo.pApplicationName = "hello triangle";
        appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appinfo.pEngineName = "no Engine";
        appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appinfo.apiVersion = VK_API_VERSION_1_0;
        //createInstace info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appinfo;
        //this two variable.
        //the glfwExtensionCount is the glfw need extension count
        //glfwExtensions are the glfw need extension name
        //is all about glfw no vulkan
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        //follow variables are vulkan create a instace needs
        //not only have the glfwExtensions, but in this case only has the glfwExtension
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;

        //now we have the necessary info for creating instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("error to create instance!");
        }
    }
private:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    GLFWwindow* m_Window;
    VkInstance m_vulkanInstance;
};

int main() {
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}