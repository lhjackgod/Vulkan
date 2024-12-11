#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <set>
#include<optional>
#include <vector>
#include <limits>
#include <algorithm>
#include <sstream>

#ifdef NDEBUG
	const bool enableValidation = false;
#else
	const bool enableValidation = true;
#endif

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, pCreateInfo, pAllocator, pMessenger);
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

struct queueFamilyIndex
{
	std::optional<uint32_t> graphicsIndex;
	std::optional<uint32_t> presentIndex;
	operator bool() const
	{
		return graphicsIndex.has_value() && presentIndex.has_value();
	}
};

struct surfaceAttributes
{
	VkSurfaceCapabilitiesKHR surfaceCapability;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Triangle
{
public:
	Triangle()
	{
		initVulkan();
	}
	~Triangle()
	{
		clearUp();
	}
	void run()
	{
		mainLoop();
	}
private:
	void initVulkan()
	{
		createGLFWWindow();
		createVulkanInstance();
		createDebugUtilsMessengerEXT();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChainKHR();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramBuffers();
		createCommandPool();
		createCommandBuffer();
	}

	void clearUp()
	{
		vkDestroyCommandPool(m_LogicalDevice, m_GrapgicsCommandPool, nullptr);
		for (int i = 0; i < m_Frambuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_LogicalDevice, m_Frambuffers[i], nullptr);
		}
		vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
		vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);
		for (int i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			vkDestroyImageView(m_LogicalDevice, m_SwapChainImageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
		}
	}

	void createGLFWWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_Window = glfwCreateWindow(800, 600, "triangle", nullptr, nullptr);
	}

	std::vector<const char*> getInstanceRequiredExtensions()
	{
		uint32_t extensionsCount;
		const char** extensionsName;
		extensionsName = glfwGetRequiredInstanceExtensions(&extensionsCount);
		std::vector<const char*> extensions(extensionsName, extensionsName + extensionsCount);

		if (enableValidation)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	void createVulkanInstance()
	{
		VkApplicationInfo appInfo{};
		
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Triangle";
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.applicationVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);
		appInfo.engineVersion = VK_MAKE_VERSION(1.0, 0.0, 0.0);
		appInfo.pEngineName = "no engine";
		

		VkInstanceCreateInfo instanceCreateInfo{};
		
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t> (m_RequiredLayer.size());
		if (instanceCreateInfo.enabledLayerCount > 0)
		{
			instanceCreateInfo.ppEnabledLayerNames = m_RequiredLayer.data();
		}
		else
		{
			instanceCreateInfo.ppEnabledExtensionNames = nullptr;
		}
		std::vector<const char*> instanceRequiredExtensions = getInstanceRequiredExtensions();
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceRequiredExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceRequiredExtensions.data();
		VkDebugUtilsMessengerCreateInfoEXT instanceCreatationDebug{};
		if (enableValidation)
		{
			setDebugUtilsMessengerCreateInfoEXT(instanceCreatationDebug);
			instanceCreateInfo.pNext = &instanceCreatationDebug;
		}
		
		if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance)!=VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance");
		}
	}

	VKAPI_ATTR static VkBool32 VKAPI_CALL debugUtilsMessengerEXTCallBack(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
		void* pUserData
	) 
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			std::cerr << "valid Layer: " << pCallbackData->pMessage << std::endl;
		}
		return VK_FALSE;
	}

	void setDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& debugInfo)
	{
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
		debugInfo.pUserData = nullptr;
		debugInfo.pfnUserCallback = debugUtilsMessengerEXTCallBack;
	}

	void createDebugUtilsMessengerEXT()
	{
		if (!enableValidation)
		{
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
		setDebugUtilsMessengerCreateInfoEXT(debugUtilsMessengerCreateInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(m_Instance, &debugUtilsMessengerCreateInfo, nullptr, &m_DebugMessenger);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create debugUtilsMessengerEXT");
		}
	}

	queueFamilyIndex getPhysicalQueueFamilyIndices(const VkPhysicalDevice& physicalDevice)
	{
		queueFamilyIndex familyIndex{};
		uint32_t familyQueueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyQueueCount, nullptr);
		std::vector<VkQueueFamilyProperties> PhysicalDeviceTotalQueueFamilies(familyQueueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyQueueCount, PhysicalDeviceTotalQueueFamilies.data());

		uint32_t i = 0;
		for (auto& queueFamily : PhysicalDeviceTotalQueueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				familyIndex.graphicsIndex = i;
			}
			VkBool32 hasPresentModel;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_Surface, &hasPresentModel);
			if (hasPresentModel == VK_TRUE)
			{
				familyIndex.presentIndex = i;
			}
			if (familyIndex)
			{
				break;
			}
			i++;
		}
		return familyIndex;
	}

	bool isPhysicalDeviceHasExtensions(const VkPhysicalDevice& physicalDevice)
	{
		uint32_t physicalDeviceExtensionsCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &physicalDeviceExtensionsCount, nullptr);
		std::vector<VkExtensionProperties> physicalDeviceTotalExtensions(physicalDeviceExtensionsCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &physicalDeviceExtensionsCount, physicalDeviceTotalExtensions.data());
		std::set<std::string> requiredExtensions(physicalDeivceRequiredExtensions.begin(), physicalDeivceRequiredExtensions.end());
		
		for (auto& extension : physicalDeviceTotalExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	surfaceAttributes getPhysicalDeviceSupportSurfaceAttributes(const VkPhysicalDevice& physicalDevice)
	{
		surfaceAttributes physicalDeiceSupportSurfaceAttributes{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &physicalDeiceSupportSurfaceAttributes.surfaceCapability);

		uint32_t supportFormatsCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportFormatsCount, nullptr);
		if (supportFormatsCount)
		{
			physicalDeiceSupportSurfaceAttributes.surfaceFormats.resize(supportFormatsCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportFormatsCount, physicalDeiceSupportSurfaceAttributes.surfaceFormats.data());
		}
		
		uint32_t supportPrensentModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportPrensentModesCount, nullptr);
		if (supportPrensentModesCount)
		{
			physicalDeiceSupportSurfaceAttributes.presentModes.resize(supportPrensentModesCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportPrensentModesCount, physicalDeiceSupportSurfaceAttributes.presentModes.data());
		}
		return physicalDeiceSupportSurfaceAttributes;
	}

	bool physicalDeviceSuitable(const VkPhysicalDevice& physicalDevice)
	{
		queueFamilyIndex familyIndex = getPhysicalQueueFamilyIndices(physicalDevice);

		bool physicalHasExtensions = isPhysicalDeviceHasExtensions(physicalDevice);

		bool enableSurface = false;
		if (physicalHasExtensions)
		{
			surfaceAttributes supportAttributes = getPhysicalDeviceSupportSurfaceAttributes(physicalDevice);
			enableSurface = !(supportAttributes.presentModes.empty() || supportAttributes.surfaceFormats.empty());
		}

		return familyIndex && physicalHasExtensions && enableSurface;
	}

	void pickPhysicalDevice()
	{
		uint32_t physicalDevicesCount;
		vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, nullptr);
		std::vector<VkPhysicalDevice> totalPhysicalDevices(physicalDevicesCount);
		vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, totalPhysicalDevices.data());

		for (auto& physicalDevice : totalPhysicalDevices)
		{
			if (physicalDeviceSuitable(physicalDevice))
			{
				m_PhysicalDevice = physicalDevice;
				break;
			}
		}
		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to pick physicalDevice!");
		}
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create surface!");
		}
	}

	VkExtent2D getSurfaceImageExtent(const surfaceAttributes& attributes)
	{
		const VkSurfaceCapabilitiesKHR& capability = attributes.surfaceCapability;
		if (capability.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capability.currentExtent;
		}
		VkExtent2D surfaceExtent;
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		surfaceExtent.width = std::clamp(static_cast<uint32_t>(width), capability.minImageExtent.width, capability.maxImageExtent.width);
		surfaceExtent.height = std::clamp(static_cast<uint32_t>(height), capability.minImageExtent.height, capability.maxImageExtent.height);
		return surfaceExtent;
	}

	VkSurfaceFormatKHR getSurfaceFormat(const surfaceAttributes& attributes)
	{
		const std::vector<VkSurfaceFormatKHR>& formats = attributes.surfaceFormats;
		for (const auto& format : formats)
		{
			if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
				format.format == VK_FORMAT_B8G8R8A8_SRGB)
			{
				return format;
			}
		}
		return formats[0];
	}

	VkPresentModeKHR getSurfacePresentMode(const surfaceAttributes& attributes)
	{
		const std::vector<VkPresentModeKHR>& presentModes = attributes.presentModes;

		for (const auto& presentMode : presentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void createLogicalDevice()
	{
		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		//createQueue

		queueFamilyIndex physicalQueueFamily = getPhysicalQueueFamilyIndices(m_PhysicalDevice);
		
		std::vector<VkDeviceQueueCreateInfo> totalQueueFamilies;
		std::set<uint32_t> queueFamilies{ physicalQueueFamily.graphicsIndex.value(), physicalQueueFamily.presentIndex.value() };
		const float priority = 1.0f;
		for (auto& queueFamily : queueFamilies)
		{
			VkDeviceQueueCreateInfo perDeviceQueueCreateInfo{};
			perDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			perDeviceQueueCreateInfo.queueFamilyIndex = queueFamily;
			perDeviceQueueCreateInfo.queueCount = 1;
			perDeviceQueueCreateInfo.pQueuePriorities = &priority;
			totalQueueFamilies.push_back(perDeviceQueueCreateInfo);
		}
		deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(totalQueueFamilies.size());
		deviceInfo.pQueueCreateInfos = totalQueueFamilies.data();
		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(physicalDeivceRequiredExtensions.size());
		deviceInfo.ppEnabledExtensionNames = physicalDeivceRequiredExtensions.data();
		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		deviceInfo.pEnabledFeatures = &physicalDeviceFeatures;

		if (vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device");
		}
		vkGetDeviceQueue(m_LogicalDevice, physicalQueueFamily.graphicsIndex.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, physicalQueueFamily.presentIndex.value(), 0, &m_SurfaceQueue);
	}

	void createSwapChainKHR()
	{
		VkSwapchainCreateInfoKHR swapChainCreateInfo{};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = m_Surface;

		surfaceAttributes physicalDeviceSupportAttributes = getPhysicalDeviceSupportSurfaceAttributes(m_PhysicalDevice);
		VkExtent2D surfaceExtent = getSurfaceImageExtent(physicalDeviceSupportAttributes);
		VkSurfaceFormatKHR surfaceFormat = getSurfaceFormat(physicalDeviceSupportAttributes);
		VkPresentModeKHR presentMode = getSurfacePresentMode(physicalDeviceSupportAttributes);

		uint32_t imageCount = physicalDeviceSupportAttributes.surfaceCapability.minImageCount + 1;
		if (physicalDeviceSupportAttributes.surfaceCapability.maxImageCount > 0 &&
			imageCount > physicalDeviceSupportAttributes.surfaceCapability.maxImageCount)
		{
			imageCount = physicalDeviceSupportAttributes.surfaceCapability.maxImageCount;
		}
		swapChainCreateInfo.minImageCount = imageCount;
		swapChainCreateInfo.imageFormat = surfaceFormat.format;
		swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainCreateInfo.imageExtent = surfaceExtent;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		
		queueFamilyIndex queueFamily = getPhysicalQueueFamilyIndices(m_PhysicalDevice);
		uint32_t familyIndex[2] = { queueFamily.graphicsIndex.value(), queueFamily.presentIndex.value() };
		if (familyIndex[0] != familyIndex[1])
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainCreateInfo.queueFamilyIndexCount = 2;
			swapChainCreateInfo.pQueueFamilyIndices = familyIndex;
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCreateInfo.queueFamilyIndexCount = 0;
			swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		}
		swapChainCreateInfo.preTransform = physicalDeviceSupportAttributes.surfaceCapability.currentTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		if (vkCreateSwapchainKHR(m_LogicalDevice, &swapChainCreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swapChain!");
		}
		uint32_t swapChainImagesCount;
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &swapChainImagesCount, nullptr);
		m_SwapChainImages.resize(swapChainImagesCount);
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &swapChainImagesCount, m_SwapChainImages.data());

		m_ImageFormat = surfaceFormat.format;
		m_ImageExtent = surfaceExtent;
	}

	void createImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		for (int i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			VkImageViewCreateInfo imageViewInfo{};
			imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewInfo.image = m_SwapChainImages[i];
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.format = m_ImageFormat;

			VkComponentMapping componentMapping{};
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_G;
			componentMapping.b = VK_COMPONENT_SWIZZLE_B;
			componentMapping.a = VK_COMPONENT_SWIZZLE_A;

			imageViewInfo.components = componentMapping;
			VkImageSubresourceRange imageSubsourceRange{};
			imageSubsourceRange.baseArrayLayer = 0;
			imageSubsourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageSubsourceRange.baseMipLevel = 0;
			imageSubsourceRange.levelCount = 1;
			imageSubsourceRange.layerCount = 1;

			imageViewInfo.subresourceRange = imageSubsourceRange;

			if (vkCreateImageView(m_LogicalDevice, &imageViewInfo, nullptr, &m_SwapChainImageViews[i]))
			{
				throw std::runtime_error("failed to create ImageView!");
			}
		}
	}

	void createRenderPass()
	{
		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_ImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentReference{};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subPassDescription{};
		subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPassDescription.colorAttachmentCount = 1;
		subPassDescription.pColorAttachments = &colorAttachmentReference;

		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subPassDescription;
		renderPassCreateInfo.dependencyCount = 0;
		renderPassCreateInfo.pDependencies = nullptr;

		if (vkCreateRenderPass(m_LogicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create renderPass!");
		}
	}

	std::vector<char> getShaderCode(const std::string& shaderPath)
	{
		std::ifstream file(shaderPath, std::ifstream::ate | std::ifstream::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file :" + shaderPath);
		}

		size_t bufferSize = file.tellg();
		file.seekg(0);
		std::vector<char> code(bufferSize);
		file.read(code.data(), bufferSize);
		
		return code;
	}

	VkShaderModule createShaderModule(const std::string& shaderPath)
	{
		VkShaderModule shaderModule;
		std::vector<char> shaderCode = getShaderCode(shaderPath);
		
		VkShaderModuleCreateInfo shaderModuleCreateInfo{};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = shaderCode.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
		if (vkCreateShaderModule(m_LogicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to createShaderModule!");
		}
		return shaderModule;
	}

	void createGraphicsPipeline()
	{
		VkShaderModule vershaderModule = createShaderModule("src/shader/vert.spv");
		VkShaderModule fragShaderModule = createShaderModule("src/shader/frag.spv");
		

		VkPipelineShaderStageCreateInfo vershaderStageCreateInfo{};
		vershaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vershaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vershaderStageCreateInfo.module = vershaderModule;
		vershaderStageCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
		fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageCreateInfo.module = fragShaderModule;
		fragShaderStageCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStage[2] = { vershaderStageCreateInfo, fragShaderStageCreateInfo };
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembleCreateInfo{};
		inputAssembleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembleCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembleCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = m_ImageExtent.width;
		viewport.height = m_ImageExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent = m_ImageExtent;
		scissor.offset = { 0, 0 };

		VkPipelineViewportStateCreateInfo viewportCreateInfo{};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.pViewports = &viewport;
		viewportCreateInfo.scissorCount = 1;
		viewportCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationStateCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo{};
		multiSampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multiSampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multiSampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multiSampleCreateInfo.minSampleShading = 1.0f;
		multiSampleCreateInfo.pSampleMask = nullptr;
		multiSampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multiSampleCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorAttachment{};
		colorAttachment.blendEnable = VK_FALSE;
		colorAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorAttachment;
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

		VkDynamicState dynamicStates[2]{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicsInfo{};
		dynamicsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicsInfo.dynamicStateCount = 2;
		dynamicsInfo.pDynamicStates = dynamicStates;

		VkPipelineLayout pipelineLayout;
		VkPipelineLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.setLayoutCount = 0;
		layoutCreateInfo.pSetLayouts = nullptr;
		layoutCreateInfo.pushConstantRangeCount = 0;
		layoutCreateInfo.pPushConstantRanges = nullptr;
		if (vkCreatePipelineLayout(m_LogicalDevice, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipelineLayout!");
		}

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.stageCount = 2;
		graphicsPipelineCreateInfo.pStages = shaderStage;
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembleCreateInfo;
		graphicsPipelineCreateInfo.pTessellationState = nullptr;
		graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &multiSampleCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
		graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState = &dynamicsInfo;
		graphicsPipelineCreateInfo.layout = pipelineLayout;
		graphicsPipelineCreateInfo.renderPass = m_RenderPass;
		graphicsPipelineCreateInfo.subpass = 0;
		graphicsPipelineCreateInfo.basePipelineIndex = -1;
		graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphicsPipline");
		}
		vkDestroyShaderModule(m_LogicalDevice, vershaderModule, nullptr);
		vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);
		vkDestroyPipelineLayout(m_LogicalDevice, pipelineLayout, nullptr);
	}

	void createFramBuffers()
	{
		m_Frambuffers.resize(m_SwapChainImageViews.size());
		for (int i = 0; i < m_Frambuffers.size(); i++)
		{
			VkFramebufferCreateInfo frambufferCreateInfo{};
			frambufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frambufferCreateInfo.renderPass = m_RenderPass;
			frambufferCreateInfo.attachmentCount = 1;
			frambufferCreateInfo.pAttachments = &m_SwapChainImageViews[i];
			frambufferCreateInfo.width = m_ImageExtent.width;
			frambufferCreateInfo.height = m_ImageExtent.height;
			frambufferCreateInfo.layers = 1;
			
			if (vkCreateFramebuffer(m_LogicalDevice, &frambufferCreateInfo, nullptr, &m_Frambuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create FramBuffer!");
			}
		}
	}

	void createCommandPool()
	{
		queueFamilyIndex queueFamilies = getPhysicalQueueFamilyIndices(m_PhysicalDevice);
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilies.graphicsIndex.value();
		if (vkCreateCommandPool(m_LogicalDevice, &commandPoolCreateInfo, nullptr, &m_GrapgicsCommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create commandPool!");
		}
	}

	void createCommandBuffer()
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandBufferCount = 1;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool = m_GrapgicsCommandPool;
		if (vkAllocateCommandBuffers(m_LogicalDevice, &commandBufferAllocateInfo, &m_GraphicsCommandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create commandBuffer!");
		}
	}
private:
#ifdef NDEBUG

	std::vector<const char*> m_RequiredLayer;
#else
	std::vector<const char*> m_RequiredLayer{
		"VK_LAYER_KHRONOS_validation"
	};
#endif
	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkPhysicalDevice m_PhysicalDevice;
	VkSurfaceKHR m_Surface;
	GLFWwindow* m_Window;
	VkSwapchainKHR m_SwapChain;
	std::vector<const char*> physicalDeivceRequiredExtensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	VkDevice m_LogicalDevice;
	VkQueue m_GraphicsQueue, m_SurfaceQueue;
	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;
	VkFormat m_ImageFormat;
	VkExtent2D m_ImageExtent;
	VkRenderPass m_RenderPass;
	VkPipeline m_GraphicsPipeline;
	std::vector<VkFramebuffer> m_Frambuffers;
	VkCommandPool m_GrapgicsCommandPool;
	VkCommandBuffer m_GraphicsCommandBuffer;
};
int main()
{
	try
	{
		Triangle triangle;
		triangle.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}