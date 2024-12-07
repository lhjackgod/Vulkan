#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <optional>

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pMessenger)
{
	
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
VKAPI_ATTR void VKAPI_CALL DestroyDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	VkDebugUtilsMessengerEXT                    messenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, messenger, pAllocator);
	}
}
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	bool isComplete()
	{
		return graphicsFamily.has_value();
	}
};
class Device
{
public:
	Device()
	{
		initVulkan();
	}
	~Device()
	{
		cleanUp();
	}
private:
	void cleanUp()
	{
		DestroyDebugUtilsMessengerEXT(m_vulkanInstace, debugMessenger, nullptr);
		vkDestroyInstance(m_vulkanInstace, nullptr);
	}
	void initVulkan()
	{
		createInstace();
		setupDebugMessager();
	}
	void createInstace()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "my device";
		appInfo.pEngineName = "no engine";
		
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.applicationVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);
		appInfo.engineVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		if (enableValidLayer)
		{
			createInfo.enabledLayerCount = (uint32_t) vaildLayer.size();
			createInfo.ppEnabledLayerNames = vaildLayer.data();
		}
		else
		{
			createInfo.enabledLayerCount = (uint32_t)0;
		}
		auto extensions = getRequireExtension();
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (vkCreateInstance(&createInfo, nullptr, &m_vulkanInstace)!= VK_SUCCESS)
		{
			std::runtime_error("error create instance");
		}
	}
	void propoertyVkDebugUtilsMessengerCreate(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT;
		createInfo.pUserData = nullptr;
	}
	void setupDebugMessager()
	{
		if (!enableValidLayer)return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		propoertyVkDebugUtilsMessengerCreate(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(m_vulkanInstace, &createInfo, nullptr, &debugMessenger);
		if (result != VK_SUCCESS)
		{
			std::runtime_error("error create debugMessenger");
		}
	}
	std::vector<const char*> getRequireExtension()
	{
		uint32_t glfwExtensionCount;
		const char** glfwExtension;

		glfwExtension = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtension, glfwExtension + glfwExtensionCount);

		if (enableValidLayer)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messengeServerity,
		VkDebugUtilsMessageTypeFlagsEXT messengeType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		if (messengeServerity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			std::cerr << "validation Layer: " << pCallbackData->pMessage << std::endl;
		}
		return VK_FALSE;
	}
	void pickPhysicalDevice()
	{
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(m_vulkanInstace, &physicalDeviceCount, nullptr);
		if (physicalDeviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(m_vulkanInstace, &physicalDeviceCount, physicalDevices.data());
		for (auto& device : physicalDevices)
		{
			if (isDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				break;
			}
		}
		if (m_PhysicalDevice == VK_NULL_HANDLE) //if not device is suitable
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}
	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices queueFamily = findQueueFamilies(device);
		//then we can judge whether the device's queues have the one can deal with 
		//the problems

		return queueFamily.isComplete();
	}
	
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		//We need to find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT.
		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			//if the queue can deal with graphics
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
				break;
			}
			i++;
		}
		return indices;
	}
private:
#define NDEBUG
#ifdef NDEBUG
	const bool enableValidLayer = true;
#else
	const bool enableValidLayer = false;
#endif
	
	std::vector<const char*> vaildLayer{
		"VK_LAYER_KHRONOS_validation"
	};
	VkInstance m_vulkanInstace;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice m_PhysicalDevice;

	/*
	* in vulkan the orther must be in a queue
	* so we must to find a queue to include the command
	* but 0 also have it mean in vulkan , so we need to use optional
	* 
	*/
	
};

int main()
{
	
	try
	{
		Device device;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

}