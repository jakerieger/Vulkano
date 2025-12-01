// Author: Jake Rieger
// Created: 11/30/25.
//

#include <Vulkano/VulkanContext.hpp>
#include <Vulkano/SwapchainManager.hpp>
#include <Vulkano/FrameSynchronizer.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static Vulkano::VulkanContext gContext;
static Vulkano::SwapchainManager gSwapchain;
static Vulkano::FrameSynchronizer gFrameSync;
static GLFWwindow* gWindow;
static VkSurfaceKHR gSurface;

inline constexpr int kWindowWidth {1280};
inline constexpr int kWindowHeight {720};
inline constexpr std::string_view kWindowTitle {"Testbed"};

static std::vector<const char*> InitializeGLFW() {
    if (!glfwInit()) { throw std::runtime_error("Failed to initialize GLFW"); }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    gWindow = glfwCreateWindow(kWindowWidth, kWindowHeight, kWindowTitle.data(), nullptr, nullptr);
    if (!gWindow) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return instanceExtensions;
}

static void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    // Transition image to transfer dst layout
    VkImageMemoryBarrier barrier {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = gSwapchain.GetImages()[imageIndex];
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    // Clear to a nice blue color
    VkClearColorValue clearColor = {0.1f, 0.2f, 0.4f, 1.0f};
    VkImageSubresourceRange range {};
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = 1;

    vkCmdClearColorImage(commandBuffer,
                         gSwapchain.GetImages()[imageIndex],
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &clearColor,
                         1,
                         &range);

    // Transition image to present layout
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }
}

static void DrawFrame() {
    // Begin frame (waits on fence, resets command buffer)
    Vulkano::AssertResult(gFrameSync.BeginFrame());

    // Acquire image from swapchain
    auto imageIndexResult = gSwapchain.AcquireNextImage(gFrameSync.GetCurrentImageAvailableSemaphore());
    if (!imageIndexResult) {
        // Swapchain out of date, skip this frame
        gFrameSync.EndFrame();
        return;
    }
    uint32_t imageIndex = imageIndexResult.value();

    // Record command buffer
    RecordCommandBuffer(gFrameSync.GetCurrentCommandBuffer(), imageIndex);

    // Submit command buffer
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    auto currentCommandBuffer = gFrameSync.GetCurrentCommandBuffer();

    VkSemaphore waitSemaphores[]      = {gFrameSync.GetCurrentImageAvailableSemaphore()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphores;
    submitInfo.pWaitDstStageMask      = waitStages;
    submitInfo.commandBufferCount     = 1;
    submitInfo.pCommandBuffers        = &currentCommandBuffer;

    VkSemaphore signalSemaphores[]  = {gFrameSync.GetCurrentRenderFinishedSemaphore()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(gContext.GetGraphicsQueue(), 1, &submitInfo, gFrameSync.GetCurrentFence()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    // Present
    auto presentResult = gSwapchain.Present(imageIndex, gFrameSync.GetCurrentRenderFinishedSemaphore());
    if (!presentResult) {
        // Swapchain out of date, will be handled next frame
    }

    // End frame (advance to next frame)
    gFrameSync.EndFrame();
}

static void InitializeVulkano(const std::vector<const char*>& instanceExtensions) {
    // Step 1: Create Vulkan instance
    Vulkano::VulkanContext::InstanceConfig instanceConfig;
    instanceConfig.instanceExtensions = instanceExtensions;
    Vulkano::AssertResult(gContext.CreateInstance(instanceConfig));

    // Step 2: Create surface using the instance
    VkResult result = glfwCreateWindowSurface(gContext.GetInstance(), gWindow, nullptr, &gSurface);
    if (result != VK_SUCCESS) {
        gContext.Shutdown();
        glfwDestroyWindow(gWindow);
        glfwTerminate();
        throw std::runtime_error("Failed to create window surface");
    }

    // Step 3: Create device with surface for presentation support
    Vulkano::VulkanContext::DeviceConfig deviceConfig;
    deviceConfig.surface = gSurface;
    Vulkano::AssertResult(gContext.CreateDevice(deviceConfig));

    // Step 4: Create swapchain
    Vulkano::AssertResult(gSwapchain.Initialize(&gContext, gSurface, kWindowWidth, kWindowHeight));

    // Step 5: Create frame synchronizer (manages all sync objects and command buffers)
    Vulkano::AssertResult(gFrameSync.Initialize(&gContext, 2));  // 2 frames in flight
}

static void Run() {
    while (!glfwWindowShouldClose(gWindow)) {
        glfwPollEvents();
        DrawFrame();
    }

    // Wait for device to finish before cleanup
    gContext.WaitIdle();
}

static void Cleanup() {
    // Cleanup in reverse order of creation
    gFrameSync.Shutdown();
    gSwapchain.Shutdown();
    vkDestroySurfaceKHR(gContext.GetInstance(), gSurface, nullptr);
    gContext.Shutdown();
    glfwDestroyWindow(gWindow);
    glfwTerminate();
}

int main() {
    const auto instanceExtensions = InitializeGLFW();
    InitializeVulkano(instanceExtensions);
    Run();
    Cleanup();
}