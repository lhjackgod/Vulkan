#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>
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
        //check if the valid layer can be used
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

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
        //if we can use the valid layer we should include it to the createInfo
        //but the enabledLayerCount also not only for the validLayer
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        //now we have the necessary info for creating instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("error to create instance!");
        }
    }
    bool checkValidationLayerSupport()
    {
        //in this part we can get total avaliable layers we can use
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties>  avaliableLayer(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayer.data());

        //from now we check whether the layers, we set can be used
        for (const char* layerName : validationLayers)
        {
            bool hasIncluded = false;
            for (VkLayerProperties& avali : avaliableLayer)
            {
                if (strcmp(layerName, avali.layerName) == 0)
                {
                    hasIncluded = true;
                    break;
                }
                if (!hasIncluded)
                    return false;
            }
        }
        return true;
    }
private:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
#ifdef NDEBUG
    cosnt bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

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