#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <exception>
#include <vector>
#include <optional>
#include <set>

VKAPI_ATTR VkResult VKAPI_CALL createDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pMessenger)
{
	VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
	auto createFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (createFunc != NULL)
	{
		result = createFunc(instance, pCreateInfo, pAllocator, pMessenger);
	}
	return result;
	
}

VKAPI_ATTR void VKAPI_CALL destroyDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	VkDebugUtilsMessengerEXT                    messenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto destroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (destroyFunc != NULL)
	{
		destroyFunc(instance, messenger, pAllocator);
	}
}

struct  m_QueueFamily
{
	std::optional<uint32_t> graphicsQueueFamily;
	std::optional<uint32_t> presentQueueFamily;
	operator bool() const
	{
		return graphicsQueueFamily.has_value() && presentQueueFamily.has_value();
	}
};

struct m_SwapChainAttributes
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};

class DepthUse
{
public:
	DepthUse()
	{
		initVulkan();
	}
	void Run()
	{
		mainLoop();
	}
	~DepthUse()
	{
		cleanEnvironment();
	}
private:
	void initVulkan()
	{
		createGLFWWindow();
		createInstance();
		createDebugUtilsMessenger();
		createSurface();
		pickPhysicalDevice();
		createSwapChainKHR();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_GLFWWindow.window))
		{
			glfwPollEvents();
		}
		glfwTerminate();
	}

	void cleanEnvironment()
	{
		vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
		destroyDebugUtilsMessengerEXT(m_VulkanInstance, m_VulkanValidation, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
		glfwDestroyWindow(m_GLFWWindow.window);
	}

	void createGLFWWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_GLFWWindow.screenWidth = 800;
		m_GLFWWindow.screenHeight = 600;
		m_GLFWWindow.screenName = "depthUse";
		m_GLFWWindow.window = glfwCreateWindow(m_GLFWWindow.screenWidth, m_GLFWWindow.screenHeight, m_GLFWWindow.screenName, nullptr, nullptr);
	}

	std::vector<const char*> getInstanceRequiredExtensions()
	{
		uint32_t glfwExtensionCount;
		const char** glfwExtensionName;
		glfwExtensionName = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensionsName(glfwExtensionName, glfwExtensionName + glfwExtensionCount);
		extensionsName.insert(extensionsName.end(), validationRequirement.validationExtensionName.begin(), validationRequirement.validationExtensionName.end());
		return extensionsName;
	}

	void createInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = m_GLFWWindow.screenName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);
		appInfo.engineVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);
		appInfo.pEngineName = "no engine";
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//acquire the extensions
		std::vector<const char*> instnaceExtensionsName = getInstanceRequiredExtensions();

		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationRequirement.validationLayerName.size());
		instanceCreateInfo.ppEnabledLayerNames = validationRequirement.validationLayerName.data();
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instnaceExtensionsName.size());
		instanceCreateInfo.ppEnabledExtensionNames = instnaceExtensionsName.data();

		VkDebugUtilsMessengerCreateInfoEXT nextDebugUtilsMessengerCreateInfo{};
		createDebugUtilsMessagerCreateInfo(nextDebugUtilsMessengerCreateInfo);
		instanceCreateInfo.pNext = &nextDebugUtilsMessengerCreateInfo;

		if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_VulkanInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("error create instance!");
		}
	}

	static VkBool32 validationCallbackFunc(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			std::cerr << "validation layer output: " << pCallbackData->pMessage << std::endl;
		}
		return VK_FALSE;
	}

	void createDebugUtilsMessagerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsCreateInfo)
	{
		debugUtilsCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
		debugUtilsCreateInfo.pfnUserCallback = validationCallbackFunc;
		debugUtilsCreateInfo.pUserData = nullptr;
	}

	void createDebugUtilsMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
		createDebugUtilsMessagerCreateInfo(debugUtilsMessengerCreateInfo);

		if (createDebugUtilsMessengerEXT(m_VulkanInstance, &debugUtilsMessengerCreateInfo, nullptr, &m_VulkanValidation) != VK_SUCCESS)
		{
			throw std::runtime_error("error create validation layer!");
		}
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(m_VulkanInstance, m_GLFWWindow.window, nullptr, &m_Surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create surface!");
		}
	}

	m_QueueFamily getQueueFamily(const VkPhysicalDevice& device)
	{
		m_QueueFamily queue_family{};
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());
		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			VkQueueFamilyProperties queueFamily = queueFamilyProperties[i];
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queue_family.graphicsQueueFamily = i;
			}

			VkBool32 supportSurface = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &supportSurface);
			if (supportSurface)
			{
				queue_family.presentQueueFamily = i;
			}
		}
		return queue_family;
	}

	m_SwapChainAttributes getPhysicalDeviceSurfaceSupport(const VkPhysicalDevice& physicalDevice)
	{
		static m_SwapChainAttributes swapChainAttribs{};
		swapChainAttribs.presentModes.clear();
		swapChainAttribs.surfaceCapabilities = {};
		swapChainAttribs.surfaceFormats.clear();
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &(swapChainAttribs.surfaceCapabilities));

		uint32_t formatsCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatsCount, nullptr);
		swapChainAttribs.surfaceFormats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatsCount, swapChainAttribs.surfaceFormats.data());

		uint32_t presentModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModesCount, nullptr);
		swapChainAttribs.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModesCount, swapChainAttribs.presentModes.data());

		return swapChainAttribs;
	}

	bool isPhysicalDeivceHasExtensions(const VkPhysicalDevice& physicalDeivce)
	{
		std::set<const char*> extensionsNeededName(swapChainExtensionsName.begin(), swapChainExtensionsName.end());
		uint32_t physicalDeivceExtensionsCount;
		vkEnumerateDeviceExtensionProperties(physicalDeivce, nullptr, &physicalDeivceExtensionsCount, nullptr);
		std::vector<VkExtensionProperties> hasExtensions(physicalDeivceExtensionsCount);
		vkEnumerateDeviceExtensionProperties(physicalDeivce, nullptr, &physicalDeivceExtensionsCount, hasExtensions.data());
		for (auto& extensionProperty : hasExtensions)
		{
			extensionsNeededName.erase(extensionProperty.extensionName);
		}
		return extensionsNeededName.empty();
	}

	bool physicalDeviceSuitable(const VkPhysicalDevice& device)
	{
		m_QueueFamily queue_family = getQueueFamily(device);

		bool supportSwapChin = isPhysicalDeivceHasExtensions(device);
		
		bool hasTotalMessages = false;
		if (supportSwapChin)
		{
			m_SwapChainAttributes deviceSupport = getPhysicalDeviceSurfaceSupport(device);
			hasTotalMessages = !(deviceSupport.presentModes.empty() || deviceSupport.surfaceFormats.empty());
		}
		return queue_family && hasTotalMessages;
	}

	void pickPhysicalDevice()
	{
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, nullptr);
		//get the physicalDeviceCount
		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(m_VulkanInstance, &physicalDeviceCount, physicalDevices.data());
		for (auto& physicalDevice : physicalDevices)
		{
			if (physicalDeviceSuitable(physicalDevice))
			{
				m_PhysicalDevice = physicalDevice;
				break;
			}
		}
	}

	void getSurfaceCapabily(const VkSurfaceCapabilitiesKHR& surfaceCapability)
	{
		
	}

	void createSwapChainKHR()
	{
		VkSwapchainCreateInfoKHR swapChainCreateInfo{};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = m_Surface;
		
	}

private:
	VkInstance m_VulkanInstance;
	VkDebugUtilsMessengerEXT m_VulkanValidation;
	VkPhysicalDevice m_PhysicalDevice;

	struct GLFWMessage
	{
		int screenWidth;
		int screenHeight;
		const char* screenName;
		GLFWwindow* window;
	} m_GLFWWindow;

	struct ValidationRequirement
	{
		std::vector<const char*> validationExtensionName
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		std::vector<const char*> validationLayerName
		{
			"VK_LAYER_KHRONOS_validation"
		};
	} validationRequirement;

	std::vector<const char*> swapChainExtensionsName
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkSurfaceKHR m_Surface;
};

int main()
{
	try
	{
		DepthUse depthUse;
		depthUse.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}