#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>

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
	std::optional<uint32_t> presentFamily;//because of that we have chose the 
	//device which can support graphics but maybe it can't support show the image on the
	//surface we created
	//so we need to chose both needings have
	bool hasValue()
	{
		return indices.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails // these attributes are all we need to check the swap chain
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

//we need to choose the swapChian way from the physical device
VkSurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats)
{
	for (auto& format : formats)
	{
		if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
			format.format == VK_FORMAT_B8G8R8_SRGB)
		{
			return format;
		}
	}
	return formats[0];
}

VkPresentModeKHR chooseSwapSurfacePresentMode(std::vector<VkPresentModeKHR>& presentmodes)
{
	for (auto& presentmode : presentmodes)
	{
		if (presentmode == VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR)//check if the present mode can be used
		{
			return presentmode;
		}
	}
	return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR; //because this presentMode can be guaranteed to be avaliable
}

/*first of all the glfw use the screen space to set window
* it means that we will get actually more pixels as we want because of the api
* plus, the vulkan only need to swap data by pixels, so we need to set the pixels avange for out vulkan
* 1. if the vksurfacecapacities.currentExtent get a vaild value ,we just return it
* 2. if the value have not set yet, we need to set the value by using glfwgetFrambufferSize
* this function return the actual count of pixels by using reference
* in the end we also need to check the limit about the value(clamp)
*/
VkExtent2D chooseSwapExten(GLFWwindow* window,const VkSurfaceCapabilitiesKHR& capacilies)
{
	if (capacilies.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capacilies.currentExtent;
	}
	VkExtent2D actualExtent;
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	actualExtent.width = (uint32_t) width;
	actualExtent.height = (uint32_t)height;
	actualExtent.width = std::clamp(actualExtent.width, capacilies.minImageExtent.width, capacilies.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capacilies.minImageExtent.height, capacilies.maxImageExtent.height);
	return actualExtent;
}
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
		vkDestroySwapchainKHR(m_LogicalDevice, m_swapChain, nullptr);
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
		createSwapSurface();
		
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
			}
			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, indices, m_Surface, &presentationSupport);
			if (presentationSupport)
			{
				index.presentFamily = indices;
			}
			if (index.hasValue())
			{
				break;
			}
			indices++;
		}
		return index;
	}
	SwapChainSupportDetails queueSwapChainSupport(VkPhysicalDevice& device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

		uint32_t formatKHRCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatKHRCount, nullptr);
		if (formatKHRCount)
		{
			details.formats.resize(formatKHRCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatKHRCount, details.formats.data());
		}
		uint32_t presentationModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentationModesCount, nullptr);
		if (presentationModesCount)
		{
			details.presentModes.resize(presentationModesCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentationModesCount, details.presentModes.data());
		}
		return details;//this also the necessary for window show
	}
	bool hasTheExtensions(VkPhysicalDevice device)
	{
		//get total extension of a device
		uint32_t deviceExtensionsCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionsCount, nullptr);

		std::vector<VkExtensionProperties> avaliableExtensions(deviceExtensionsCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionsCount, avaliableExtensions.data());

		std::set<std::string> requireExtensions(deviceExtensions.begin(), deviceExtensions.end());
		//to check if the extensions include the the extensions we need
		for (auto& extension : avaliableExtensions)
		{
			requireExtensions.erase(extension.extensionName);
		}
		return requireExtensions.empty();
	}
	bool physicalSuitable(VkPhysicalDevice& physicalDevice)
	{
		QueueFamilyIndex index = GetQueueFamilyIndex(physicalDevice);

		bool has_extensions = hasTheExtensions(physicalDevice);

		bool enableSwapChain = false;
		if (has_extensions)
		{
			SwapChainSupportDetails swapChainSupportDetails = queueSwapChainSupport(physicalDevice);
			enableSwapChain = !(swapChainSupportDetails.formats.empty() || swapChainSupportDetails.presentModes.empty());
		}
		return index.hasValue() && has_extensions && enableSwapChain;
	}
	void createLogicalDevice()
	{
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
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

		QueueFamilyIndex index = GetQueueFamilyIndex(m_PhysicalDevice);
		//because the families might be the same one
		std::set<uint32_t> queueFamilyIndices{ index.indices.value(), index.presentFamily.value() };
		//create Multi_FamilyQueue
		std::vector<VkDeviceQueueCreateInfo> deviceCreateInfos;
		const float priority = 1.0f;
		for (auto& queueFamilyindex : queueFamilyIndices)
		{
			VkDeviceQueueCreateInfo queueFamilyCreateInfo{};
			queueFamilyCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueFamilyCreateInfo.queueCount = 1;
			queueFamilyCreateInfo.pQueuePriorities = &priority;
			queueFamilyCreateInfo.queueFamilyIndex = queueFamilyindex;
			deviceCreateInfos.push_back(queueFamilyCreateInfo);
		}

		createInfo.queueCreateInfoCount = (uint32_t) deviceCreateInfos.size();
		createInfo.pQueueCreateInfos = deviceCreateInfos.data();

		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device");
		}
		vkGetDeviceQueue(m_LogicalDevice, index.indices.value(), 0, &m_queue);
		vkGetDeviceQueue(m_LogicalDevice, index.presentFamily.value(), 0, &m_presentQueue);
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
	void createSwapSurface()
	{
		//first we need to get the device's swapchainSupportdetails
		SwapChainSupportDetails details = queueSwapChainSupport(m_PhysicalDevice);

		//the suitable detail
		VkExtent2D extent = chooseSwapExten(window, details.capabilities);
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
		VkPresentModeKHR presentMode = chooseSwapSurfacePresentMode(details.presentModes);
		//the information to create swapChain
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.imageExtent = extent;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		uint32_t imageCount = details.capabilities.minImageCount + 1;

		if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
		{
			imageCount = details.capabilities.maxImageCount;
		}
		createInfo.minImageCount = imageCount;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		QueueFamilyIndex queueFamilyindices = GetQueueFamilyIndex(m_PhysicalDevice);
		uint32_t queueFamily[]{ queueFamilyindices.indices.value(), queueFamilyindices.presentFamily.value() };
		if (queueFamilyindices.indices != queueFamilyindices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.pQueueFamilyIndices = queueFamily;
			createInfo.queueFamilyIndexCount = 2;
		}
		else 
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;//because in the mode of VK_SHARING_MODE_EXCLUSIVE
			//the swapchain need not to set the queueFamily

		}
		createInfo.oldSwapchain = VK_NULL_HANDLE;
		createInfo.preTransform = details.capabilities.currentTransform;

		if (vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
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
	VkQueue m_presentQueue;
	std::vector<const char*> validLayer
	{
		"VK_LAYER_KHRONOS_validation"
	};
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_swapChain;
	//in some reason that not all graphics cards can show image on the actual window
	//so we need to check whether it can swap 
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
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