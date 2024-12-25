#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <set>
#include<optional>
#include <vector>
#include <limits>
#include <algorithm>
#include <sstream>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <stb_image.h>
const static int MAX_FAMER_IN_FLIGHT = 2;
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
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramBuffers();
		createCommandPool();
		createVertexBuffer();
		createIndicesBuffer();
		createUniformBuffer();
		createTextureImage();
		createTextureImageView();
		createDescriptorPool();
		createDescriptorSet();
		createCommandBuffer();
		createSyncObjects();
	}

	void cleanUpSwapChain()
	{
		
		for (int i = 0; i < m_Frambuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_LogicalDevice, m_Frambuffers[i], nullptr);
		}
		for (int i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			vkDestroyImageView(m_LogicalDevice, m_SwapChainImageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);
	}

	void clearUp()
	{
		vkDestroyImage(m_LogicalDevice, m_TextureImage, nullptr);
		vkDestroyImageView(m_LogicalDevice, m_TextureImageView, nullptr);
		vkFreeMemory(m_LogicalDevice, m_TextureImageMemory, nullptr);
		vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
		for (int i = 0; i < MAX_FAMER_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(m_LogicalDevice, m_UniformBuffers[i], nullptr);
			vkFreeMemory(m_LogicalDevice, m_UniformDeviceMemory[i], nullptr);
		}
		vkDestroyBuffer(m_LogicalDevice, m_IndicesBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, m_IndicesMemory, nullptr);
		vkDestroyBuffer(m_LogicalDevice, m_VertexBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, m_Memory, nullptr);
		for (int i = 0; i < MAX_FAMER_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_LogicalDevice, m_ImageAvaliableSemaphore[i], nullptr);
			vkDestroySemaphore(m_LogicalDevice, m_RenderFinishedSemaphore[i], nullptr);
			vkDestroyFence(m_LogicalDevice, m_InFlightFence[i], nullptr);
		}

		vkDestroyCommandPool(m_LogicalDevice, m_GrapgicsCommandPool, nullptr);
		for (int i = 0; i < m_Frambuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_LogicalDevice, m_Frambuffers[i], nullptr);

		}
		vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
		vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
		vkDestroyDescriptorSetLayout(m_LogicalDevice, m_DescriptorSetLayout, nullptr);
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
			drawFrame();
		}
		vkDeviceWaitIdle(m_LogicalDevice);
	}

	void UpdateUniformBuffer(uint32_t currentFame)
	{
		static auto lastTime = std::chrono::high_resolution_clock().now();

		auto currentTime = std::chrono::high_resolution_clock().now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.perspective = glm::perspective(glm::radians(45.0f), (float)m_ImageExtent.width / (float)m_ImageExtent.height, 0.1f, 10.0f);
		ubo.perspective[1][1] *= -1; //glm is designed for opengl which has a y coordinate invertion
		memcpy(m_UniformBuffersMapped[currentFame], &ubo, sizeof(ubo));
	}

	void drawFrame() 
	{
		vkWaitForFences(m_LogicalDevice, 1, &m_InFlightFence[currentFame], VK_TRUE, UINT64_MAX);
		
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, m_SwapChain, UINT64_MAX, m_ImageAvaliableSemaphore[currentFame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		UpdateUniformBuffer(currentFame);
		vkResetFences(m_LogicalDevice, 1, &m_InFlightFence[currentFame]);

		vkResetCommandBuffer(m_GraphicsCommandBuffer[currentFame], 0);
		recordCommandBuffer(m_GraphicsCommandBuffer[currentFame], imageIndex);
		
		

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[]{ m_ImageAvaliableSemaphore[currentFame] };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		//pWaitDstStageMask is wait for the specific stage of the pipeline such as 
		//if the pipeline have run the vershader stage we can keep on
		//wait for the color attachment out
		VkPipelineStageFlags pipelineStageFlags[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = pipelineStageFlags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_GraphicsCommandBuffer[currentFame];
		VkSemaphore signalSemaphore[]{ m_RenderFinishedSemaphore[currentFame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphore;
		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFence[currentFame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}
		//finish the pic render
		//predent to screen
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphore;
		presentInfo.swapchainCount = 1;

		VkSwapchainKHR swapChains[]{ m_SwapChain };
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = VK_NULL_HANDLE;

		vkQueuePresentKHR(m_SurfaceQueue, &presentInfo);

		currentFame = (currentFame + 1) % MAX_FAMER_IN_FLIGHT;
	}

	void createGLFWWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
			m_SwapChainImageViews[i] = createImageView(m_SwapChainImages[i], m_ImageFormat);
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

		VkSubpassDependency subpassDependency{};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;//the subpass not this renderpass
		subpassDependency.dstSubpass = 0; //we only have one subpass 
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //if the dst in this stage
		//it will wait for src
		//set the subpass action for buffer
		subpassDependency.srcAccessMask = 0;//the other subpass will not impact this renderpass
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &subpassDependency;

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
		
		std::array<VkVertexInputAttributeDescription, 2> vertexAttributes = Vertex::getAttributeDescription();
		VkVertexInputBindingDescription vertexBinding = Vertex::getBindingDescription();

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributes.data();
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBinding;

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
		rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

		VkPipelineLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.setLayoutCount = 1;
		layoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
		layoutCreateInfo.pushConstantRangeCount = 0;
		layoutCreateInfo.pPushConstantRanges = nullptr;
		if (vkCreatePipelineLayout(m_LogicalDevice, &layoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
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
		graphicsPipelineCreateInfo.layout = m_PipelineLayout;
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
		commandBufferAllocateInfo.commandBufferCount = 2;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool = m_GrapgicsCommandPool;
		if (vkAllocateCommandBuffers(m_LogicalDevice, &commandBufferAllocateInfo, m_GraphicsCommandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create commandBuffer!");
		}
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		commandBufferBeginInfo.flags = 0;
		if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_RenderPass;
		renderPassBeginInfo.framebuffer = m_Frambuffers[imageIndex]; 
		renderPassBeginInfo.renderArea.extent = m_ImageExtent;
		renderPassBeginInfo.renderArea.offset = { 0,0 };
		VkClearValue clearValue{};
		clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		
		//before session we have set the dynamic pipeline attributes
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_ImageExtent.width);
		viewport.height = static_cast<float>(m_ImageExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = m_ImageExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffer[] = { m_VertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, offsets);
		
		
		vkCmdBindIndexBuffer(commandBuffer, m_IndicesBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentFame], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
		//finish
		vkCmdEndRenderPass(commandBuffer);

		//close commandBuffer
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer");
		}
	}

	void createSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < MAX_FAMER_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_ImageAvaliableSemaphore[i]) ||
				vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore[i]) ||
				vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &m_InFlightFence[i]))
			{
				throw std::runtime_error("failed to create syncVerb!");
			}
		}
	}

	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetWindowSize(m_Window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetWindowSize(m_Window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_LogicalDevice);

		cleanUpSwapChain();
		createSwapChainKHR();
		createImageViews();
		createFramBuffers();
	}
	
	uint32_t getMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags memoryFlags)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags)
			{
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}

	void createBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryFlags,
		VkBuffer& buffer, VkDeviceMemory& deviceMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.usage = bufferUsageFlags;

		if (vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("error create buffer!");
		}
		VkMemoryRequirements memeoryRequirements{};
		vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memeoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memeoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memeoryRequirements.memoryTypeBits, memoryFlags);
		if (vkAllocateMemory(m_LogicalDevice, &memoryAllocateInfo, nullptr, &deviceMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("error allocate memory!");
		}

		vkBindBufferMemory(m_LogicalDevice, buffer, deviceMemory, 0);
	}

	VkCommandBuffer beginSignleTimeCommands()
	{
		VkCommandBuffer signleCommandBuffer;
		VkCommandBufferAllocateInfo signleCommandBufferAllocateInfo{};
		signleCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		signleCommandBufferAllocateInfo.commandPool = m_GrapgicsCommandPool;
		signleCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		signleCommandBufferAllocateInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(m_LogicalDevice, &signleCommandBufferAllocateInfo, &signleCommandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create signle command buffer!");
		}
		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(signleCommandBuffer, &commandBufferBeginInfo);
		return signleCommandBuffer; // you can do something by using this commandbuffer
	}

	void endSignelTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_GraphicsQueue);

		vkFreeCommandBuffers(m_LogicalDevice, m_GrapgicsCommandPool, 1, &commandBuffer);
	}

	void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize& size)
	{
		VkCommandBuffer tempCommandBuffer = beginSignleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(tempCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSignelTimeCommands(tempCommandBuffer);
	}

	void createVertexBuffer()
	{
		VkDeviceSize size = sizeof(Vertex) * vertices.size();

		VkBuffer stagingBuffer; // temporary store data cup to buffer
		VkDeviceMemory stagingMemory;

		createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			stagingBuffer, stagingMemory);
		
		void* data;
		vkMapMemory(m_LogicalDevice, stagingMemory, 0, size, 0, &data);
		memcpy(data, vertices.data(), (size_t)size);
		vkUnmapMemory(m_LogicalDevice, stagingMemory);//copy the data cpu to memory

		//create a buffer only for gpu, that gpu can use data through this buffer
		createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_Memory);
		
		copyBuffer(stagingBuffer, m_VertexBuffer, size);

		vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, stagingMemory, nullptr);
	}

	void createIndicesBuffer()
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;
		VkDeviceSize indicesSize = sizeof(uint32_t) * m_Indices.size();

		createBuffer(indicesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

		void* data;
		vkMapMemory(m_LogicalDevice, stagingMemory, 0, indicesSize, 0, &data);
		memcpy(data, m_Indices.data(), indicesSize);
		vkUnmapMemory(m_LogicalDevice, stagingMemory);

		createBuffer(indicesSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndicesBuffer, m_IndicesMemory);
		
		copyBuffer(stagingBuffer, m_IndicesBuffer, indicesSize);
		vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, stagingMemory, nullptr);
	}

	void createDescriptorSetLayout() 
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = 1;
		createInfo.pBindings = &binding;

		if (vkCreateDescriptorSetLayout(m_LogicalDevice, &createInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create DescriptorSetLayout!");
		}
	}

	void createUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		m_UniformBuffers.resize(MAX_FAMER_IN_FLIGHT);
		m_UniformDeviceMemory.resize(MAX_FAMER_IN_FLIGHT);
		m_UniformBuffersMapped.resize(MAX_FAMER_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FAMER_IN_FLIGHT; i++)
		{
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformDeviceMemory[i]);
			vkMapMemory(m_LogicalDevice, m_UniformDeviceMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
		}
	}

	void createDescriptorPool()
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FAMER_IN_FLIGHT);

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FAMER_IN_FLIGHT);
		descriptorPoolCreateInfo.poolSizeCount = 1;
		descriptorPoolCreateInfo.pPoolSizes = &poolSize;

		if (vkCreateDescriptorPool(m_LogicalDevice, &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to createDescriptorPool!");
		}
	}

	void createDescriptorSet()
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FAMER_IN_FLIGHT, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FAMER_IN_FLIGHT); // one flight one set
		descriptorSetAllocateInfo.pSetLayouts = layouts.data();
		m_DescriptorSets.resize(MAX_FAMER_IN_FLIGHT);
		if (vkAllocateDescriptorSets(m_LogicalDevice, &descriptorSetAllocateInfo, m_DescriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptorSets!");
		}
		for (int i = 0; i < MAX_FAMER_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = m_DescriptorSets[i];
			writeDescriptorSet.dstBinding = 0;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			
			writeDescriptorSet.pBufferInfo = &bufferInfo;
	

			vkUpdateDescriptorSets(m_LogicalDevice, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	void createImage2D(const int& width, const int& height,
		const VkFormat& imageFormat, const VkImageTiling& tilling,
		const VkImageUsageFlags& usage, const VkMemoryPropertyFlagBits& properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = imageFormat;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = tilling;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (vkCreateImage(m_LogicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Image");
		}

		VkMemoryRequirements requireMents;
		vkGetImageMemoryRequirements(m_LogicalDevice, image, &requireMents);

		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = requireMents.size;
		memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(requireMents.memoryTypeBits, properties);

		if (vkAllocateMemory(m_LogicalDevice, &memoryAllocateInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate image memory!");
		}
		vkBindImageMemory(m_LogicalDevice, image, imageMemory, 0);
	}

	void copyBufferToImage(VkBuffer srcBuffer,VkImage dstImage, uint32_t width, uint32_t height)
	{
		VkCommandBuffer copyCommandBuffer = beginSignleTimeCommands();

		VkBufferImageCopy copy{};
		copy.bufferOffset = 0;
		copy.bufferImageHeight = 0;
		copy.bufferRowLength = 0;
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.baseArrayLayer = 0;
		copy.imageSubresource.layerCount = 1;
		copy.imageSubresource.mipLevel = 0;
		copy.imageOffset = { 0,0,0 };
		copy.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(copyCommandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		endSignelTimeCommands(copyCommandBuffer);
	}

	void transilationLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer translateLayoutCommandBuffer = beginSignleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		VkPipelineStageFlags srcStageFlags;
		VkPipelineStageFlags dstStageFlags;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw std::invalid_argument("unsopported layout transition!");
		}
		vkCmdPipelineBarrier(translateLayoutCommandBuffer, srcStageFlags, dstStageFlags,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		endSignelTimeCommands(translateLayoutCommandBuffer);
	}

	void createTextureImage()
	{
		int width, height, channel;
		stbi_uc* data = stbi_load("textures/texture.jpg", &width, &height, &channel, STBI_rgb_alpha);
		VkDeviceSize dataSize = width * height * 4;

		if (!data)
		{
			throw std::runtime_error("error load texture!");
		}
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		void* imageData;
		vkMapMemory(m_LogicalDevice, stagingBufferMemory, 0, dataSize, 0, &imageData);
		memcpy(imageData, data, dataSize);
		vkUnmapMemory(m_LogicalDevice, stagingBufferMemory);
		stbi_image_free(data);

		createImage2D(width, height, VK_FORMAT_R8G8B8A8_SRGB, 
			VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			m_TextureImage, m_TextureImageMemory);

		transilationLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);;
		copyBufferToImage(stagingBuffer, m_TextureImage, width, height);
		transilationLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, stagingBufferMemory, nullptr);
	}

	VkImageView createImageView(VkImage image, VkFormat imageFormat)
	{
		VkImageView imageView;
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = imageFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		
		if (vkCreateImageView(m_LogicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("error to create image view");
		}
		return imageView;
	}

	void createTextureImageView()
	{
		m_TextureImageView = createImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB);
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
	

	
	VkSemaphore m_ImageAvaliableSemaphore[MAX_FAMER_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphore[MAX_FAMER_IN_FLIGHT];
	VkFence m_InFlightFence[MAX_FAMER_IN_FLIGHT];
	VkCommandBuffer m_GraphicsCommandBuffer[MAX_FAMER_IN_FLIGHT];
	int currentFame = 0;
	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_Memory;
	//vertex buffers
	struct Vertex
	{
		glm::vec2 aPos;
		glm::vec3 aColor;

		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription vertexInputBindingDescription{};
			vertexInputBindingDescription.binding = 0; //this is the set id of the attributes of apos and color
			vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexInputBindingDescription.stride = sizeof(Vertex);
			return vertexInputBindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			//apos
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].offset = offsetof(Vertex, aPos);

			//color
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].offset = offsetof(Vertex, aColor);

			return attributeDescriptions;
		}
	};

	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 perspective;
	};

	std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} },
		{{ -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	std::vector<uint32_t> m_Indices = {
		0, 1, 2, 2, 3, 0
	};

	VkBuffer m_IndicesBuffer;
	VkDeviceMemory m_IndicesMemory;

	VkDescriptorSetLayout m_DescriptorSetLayout;
	
	VkPipelineLayout m_PipelineLayout;
	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformDeviceMemory;
	std::vector<void*> m_UniformBuffersMapped;

	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	VkImage m_TextureImage;
	VkImageView m_TextureImageView;
	VkDeviceMemory m_TextureImageMemory;
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