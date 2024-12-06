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
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_Window = glfwCreateWindow(WIDTH, HEIGHT, "VulKan", nullptr, nullptr);
    }
    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_Window))
        {
            glfwPollEvents();
        }
    }
    void cleanup()
    {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }
private:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    GLFWwindow* m_Window;
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