#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
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
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, messenger, pAllocator);
	}
	
}

struct QueueFamilyIndex
{
	std::optional<uint32_t> indices;

	bool hasValue()
	{
		return indices.has_value();
	}
};
class Presentation
{
public:
	Presentation()
	{
		initVulkan();
	}
	~Presentation()
	{
		clearUp();
	}
	void runApp()
	{
		mainLoop();
	}
private:
	void clearUp()
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void initVulkan()
	{
		createGLfWWindow();
		createInstance();
		createDebugUtilsMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}
	void createGLfWWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(800, 600, "vulkan", nullptr, nullptr);
	}
	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}
	void createInstance() {
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.pEngineName = "no engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);
		appInfo.engineVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);
		appInfo.pApplicationName = "Presentation";
		
		std::vector<const char*> extensions = getRequiredExtensions();

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (enableValidation)
		{
			createInfo.enabledLayerCount = (uint32_t) validLayer.size();
			createInfo.ppEnabledLayerNames = validLayer.data();
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
		}
		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance");
		}

	}
	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if(enableValidation)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}
	void propoertyVkDebugUtilsMessengerCreate(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
		createInfo.pUserData = nullptr;
		
	}
	void createDebugUtilsMessenger()
	{
		if (!enableValidation)
		{
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		propoertyVkDebugUtilsMessengerCreate(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to createDebugUtilsMessenger ");
		}
	}
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			std::cerr << "validation Layer: " << pCallbackData->pMessage << std::endl;
		}
		return VK_FALSE;
	}

	void pickPhysicalDevice()
	{
		uint32_t physicalCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &physicalCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(physicalCount);

		vkEnumeratePhysicalDevices(m_Instance, &physicalCount, physicalDevices.data());

		for (auto& physicalDevice : physicalDevices)
		{
			if (physicalSuitable(physicalDevice))
			{
				m_PhysicalDevice = physicalDevice;
				break;
			}
		}
		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to pick physical device");
		}
	}
	QueueFamilyIndex GetQueueFamilyIndex(VkPhysicalDevice& device)
	{
		QueueFamilyIndex index;
		uint32_t queueFamiliesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, queueFamilies.data());

		uint32_t indices = 0;
		for (auto& queue : queueFamilies)
		{
			if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				index.indices = indices;
				break;
			}
			indices++;
		}
		return index;
	}
	bool physicalSuitable(VkPhysicalDevice& physicalDevice)
	{
		QueueFamilyIndex index = GetQueueFamilyIndex(physicalDevice);
		return index.hasValue();
	}
	void createLogicalDevice()
	{
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.enabledExtensionCount = 0;
		if (enableValidation)
		{
			createInfo.enabledLayerCount = (uint32_t)validLayer.size();
			createInfo.ppEnabledLayerNames = validLayer.data();

		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		createInfo.pEnabledFeatures = &physicalDeviceFeatures;

		VkDeviceQueueCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceCreateInfo.queueCount = 1;
		QueueFamilyIndex index = GetQueueFamilyIndex(m_PhysicalDevice);
		const float priority = 1.0f;
		deviceCreateInfo.pQueuePriorities = &priority;
		deviceCreateInfo.queueFamilyIndex = index.indices.value();
		createInfo.pQueueCreateInfos = &deviceCreateInfo;
		createInfo.queueCreateInfoCount = 1;

		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device");
		}
		vkGetDeviceQueue(m_LogicalDevice, index.indices.value(), 0, &m_queue);
	}
	void createSurface()
	{
		//now we use a way that no specific for any platform, 
		//as glfwcreatewindowsurface will help to delete this different
		if (glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface");
		}
	}
private:
#define NDEBUG

#ifdef NDEBUG
	const bool enableValidation = true;
#else
	const bool enableValidation = false;
#endif
	GLFWwindow* window;
	VkInstance m_Instance;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_LogicalDevice;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkQueue m_queue;
	std::vector<const char*> validLayer
	{
		"VK_LAYER_KHRONOS_validation"
	};
	VkSurfaceKHR m_Surface;
};

int main()
{
	try
	{
		Presentation presentation;
		presentation.runApp();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}