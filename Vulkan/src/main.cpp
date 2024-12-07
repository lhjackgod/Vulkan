#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT*
    pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    /*
    * This struct should be passed to the vkCreateDebugUtilsMessengerEXT function to create the 
    VkDebugUtilsMessengerEXT object. Unfortunately, because this function is an extension function, 
    it is not automatically loaded. We have to look up its address ourselves using vkGetInstanceProcAddr. 
    We're going to create our own proxy function that handles this in the background.
    I've added it right above the HelloTriangleApplication class definition.
    */
    //we can not use vkCreateDebugUtilsMessengerEXT as we can not find it
    //so we must to find the funtion
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}


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
        setupDebugMessenger();
    }
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        //this structure include the parameter the function has set
        //if you set in this info, vulkan can filter the type which only has the parameters you have
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }
    void setupDebugMessenger()
    {
        
        if (!enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);
        //after you create instance ,you can use instance to create vkDebugUtilsMessagerEXT 
        VkResult result = CreateDebugUtilsMessengerEXT(m_vulkanInstance,
            &createInfo, nullptr,
            &debugMessager);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
        //then we set a vkDebugUtilsMessengerEXT for our instance
    }
    void mainLoop()
    {
       
    }
    void cleanup()
    {
        //we can also destroy the debugMessenger
        if (enableValidationLayers)
        {
            //DestroyDebugUtilsMessengerEXT(m_vulkanInstance, debugMessager, nullptr);
        }
        /*vkDestroyDebugUtilsMessengerEXT()*/
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



        auto extensions = getRequireExtensions();
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();


        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {

            //if we can use the valid layer we should include it to the createInfo
            //but the enabledLayerCount also not only for the validLayer
            createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
            /*
            * 就像前面提到的VkDebugUtilsMessengerCreateInfoEXT结构体，
            通过将其指针放在VkInstanceCreateInfo的pNext字段，
            可以在创建实例的同时，
            为调试工具消息信使（Debug Utils Messenger）配置相关信息，
            使得在实例创建阶段就能够为调试功能做好准备。
            可以将多个不同的扩展信息结构体通过pNext字段链接成一个链表结构。
            每个结构体的第一个成员通常是一个VkStructureType类型的字段，
            用于标识结构体的类型，以及一个pNext字段用于指向下一个扩展信息结构体。
            这种链式结构使得可以同时传递多个不同类型的扩展信息，以满足复杂的应用需求。
            */
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
            
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
            }
            if (!hasIncluded)
                return false;
        }
        return true;
    }
    std::vector<const char*> getRequireExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        //create a extension vector and put the VK_EXT_DEBUG_UTILS_EXTENSION_NAME into
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            //or extensions.push_back(VK_EXT_debug_utils)
        }
        return extensions;
    }

    //messageServerity is the parameter specifies the severity of the message
    /*
    * VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
    * VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource
    * VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error, but very likely a bug in your application
    * VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes
    * 
    */
    /*
    * messageType
    * VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened that is unrelated to the specification or performance
    * VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the specification or indicates a possible mistake
    * VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan
    */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageServerity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    )
    {
        //you can use this way to show the message whose severity is bigger than you set 
        if (messageServerity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }
        
        return VK_FALSE;
    }
private:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
#define NDEBUG

#ifdef NDEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

    GLFWwindow* m_Window;
    VkInstance m_vulkanInstance;
    VkDebugUtilsMessengerEXT debugMessager;
};

int main() {
    

    try
    {
        HelloTriangleApplication app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}