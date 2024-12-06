#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

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
        vkDestroyInstance(m_vulkanInstance, nullptr);
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
    //in this way you can get the instance-avaliable extension name
    //important! extension((uint32_t)2) there must be (uint32) if use int will get wrong result
    //you can first use way one to get total count of extension
    //
    uint32_t totalExtensionCount = 2;
    //first way get the suitable count of extension
    {
        vkEnumerateInstanceExtensionProperties(nullptr, &totalExtensionCount, nullptr);
        std::vector<VkExtensionProperties> extension(totalExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &totalExtensionCount, extension.data());
    }
    //second way may be not suit
    std::vector<VkExtensionProperties> extension((uint32_t)2);
    vkEnumerateInstanceExtensionProperties(nullptr, &totalExtensionCount, extension.data());

    for (auto& v : extension)
    {
        std::cout << v.extensionName << std::endl;
    }
    std::cout << totalExtensionCount << std::endl;
    return EXIT_SUCCESS;
}