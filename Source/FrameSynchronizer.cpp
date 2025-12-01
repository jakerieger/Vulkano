// Author: Jake Rieger
// Created: 11/30/25.
//

#include "FrameSynchronizer.hpp"
#include "VulkanContext.hpp"

namespace Vulkano {
    FrameSynchronizer::~FrameSynchronizer() {
        Shutdown();
    }

    Result<void> FrameSynchronizer::Initialize(VulkanContext* context, u32 framesInFlight) {
        if (!context || !context->IsInitialized()) { return std::unexpected("Invalid or uninitialized context"); }

        if (framesInFlight < 1 || framesInFlight > 4) {
            return std::unexpected("Frames in flight must be between 1 and 4");
        }

        mContext = context;
        mFrames.resize(framesInFlight);

        // Create frame contexts
        for (u32 i = 0; i < framesInFlight; i++) {
            if (auto result = CreateFrameContext(mFrames[i]); !result) {
                // Cleanup already created frames
                for (u32 j = 0; j < i; j++) {
                    DestroyFrameContext(mFrames[j]);
                }
                mFrames.clear();
                return result;
            }
        }

        mCurrentFrameIndex = 0;
        return {};
    }

    void FrameSynchronizer::Shutdown() {
        if (!mContext) { return; }

        mContext->WaitIdle();

        for (auto& frame : mFrames) {
            DestroyFrameContext(frame);
        }

        mFrames.clear();
        mContext           = nullptr;
        mCurrentFrameIndex = 0;
    }

    Result<void> FrameSynchronizer::BeginFrame() const {
        if (!IsInitialized()) { return std::unexpected("Frame synchronizer not initialized"); }

        // Wait for this frame's fence
        if (auto result = WaitForFrame(); !result) { return result; }

        // Reset fence for reuse
        ResetFence();

        // Reset command buffer
        VkCommandBuffer cmdBuffer = GetCurrentCommandBuffer();
        if (vkResetCommandBuffer(cmdBuffer, 0) != VK_SUCCESS) {
            return std::unexpected("Failed to reset command buffer");
        }

        return {};
    }

    void FrameSynchronizer::EndFrame() {
        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % GetFramesInFlight();
    }

    Result<void> FrameSynchronizer::WaitForFrame(u64 timeout) const {
        if (!IsInitialized()) { return std::unexpected("Frame synchronizer not initialized"); }

        VkFence fence   = GetCurrentFence();
        VkResult result = vkWaitForFences(mContext->GetDevice(), 1, &fence, VK_TRUE, timeout);

        if (result == VK_TIMEOUT) {
            return std::unexpected("Timeout waiting for fence");
        } else if (result != VK_SUCCESS) {
            return std::unexpected("Failed to wait for fence");
        }

        return {};
    }

    void FrameSynchronizer::ResetFence() const {
        VkFence fence = GetCurrentFence();
        vkResetFences(mContext->GetDevice(), 1, &fence);
    }

    Result<void> FrameSynchronizer::CreateFrameContext(FrameContext& frame) const {
        VkDevice device = mContext->GetDevice();

        // Create fence (signaled initially so first frame doesn't wait)
        VkFenceCreateInfo fenceInfo {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(device, &fenceInfo, nullptr, &frame.inFlightFence) != VK_SUCCESS) {
            return std::unexpected("Failed to create fence");
        }

        // Create semaphores
        VkSemaphoreCreateInfo semaphoreInfo {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) != VK_SUCCESS) {
            vkDestroyFence(device, frame.inFlightFence, nullptr);
            return std::unexpected("Failed to create image available semaphore");
        }

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) != VK_SUCCESS) {
            vkDestroyFence(device, frame.inFlightFence, nullptr);
            vkDestroySemaphore(device, frame.imageAvailableSemaphore, nullptr);
            return std::unexpected("Failed to create render finished semaphore");
        }

        // Create command pool
        VkCommandPoolCreateInfo poolInfo {};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = mContext->GetQueueFamilies().graphicsFamily;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &frame.commandPool) != VK_SUCCESS) {
            vkDestroyFence(device, frame.inFlightFence, nullptr);
            vkDestroySemaphore(device, frame.imageAvailableSemaphore, nullptr);
            vkDestroySemaphore(device, frame.renderFinishedSemaphore, nullptr);
            return std::unexpected("Failed to create command pool");
        }

        // Allocate command buffer
        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = frame.commandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &frame.commandBuffer) != VK_SUCCESS) {
            vkDestroyFence(device, frame.inFlightFence, nullptr);
            vkDestroySemaphore(device, frame.imageAvailableSemaphore, nullptr);
            vkDestroySemaphore(device, frame.renderFinishedSemaphore, nullptr);
            vkDestroyCommandPool(device, frame.commandPool, nullptr);
            return std::unexpected("Failed to allocate command buffer");
        }

        return {};
    }

    void FrameSynchronizer::DestroyFrameContext(FrameContext& frame) const {
        if (!mContext) { return; }

        VkDevice device = mContext->GetDevice();

        if (frame.commandPool != VK_NULL_HANDLE) {
            // Command buffer is freed automatically when pool is destroyed
            vkDestroyCommandPool(device, frame.commandPool, nullptr);
            frame.commandPool   = VK_NULL_HANDLE;
            frame.commandBuffer = VK_NULL_HANDLE;
        }

        if (frame.renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, frame.renderFinishedSemaphore, nullptr);
            frame.renderFinishedSemaphore = VK_NULL_HANDLE;
        }

        if (frame.imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, frame.imageAvailableSemaphore, nullptr);
            frame.imageAvailableSemaphore = VK_NULL_HANDLE;
        }

        if (frame.inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(device, frame.inFlightFence, nullptr);
            frame.inFlightFence = VK_NULL_HANDLE;
        }
    }
}  // namespace Vulkano