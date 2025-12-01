// Author: Jake Rieger
// Created: 11/30/25.
//

#pragma once

#include "Types.hpp"
#include "Macros.hpp"

#include <vector>
#include <vulkan/vulkan.h>

namespace Vulkano {
    class VulkanContext;

    /// @brief Represents synchronization primitives for a single frame in flight
    struct FrameContext {
        VkFence inFlightFence {VK_NULL_HANDLE};
        VkSemaphore imageAvailableSemaphore {VK_NULL_HANDLE};
        VkSemaphore renderFinishedSemaphore {VK_NULL_HANDLE};
        VkCommandPool commandPool {VK_NULL_HANDLE};
        VkCommandBuffer commandBuffer {VK_NULL_HANDLE};
    };

    /// @brief Manages frame-in-flight synchronization and resources
    class FrameSynchronizer {
    public:
        FrameSynchronizer() = default;
        ~FrameSynchronizer();

        FrameSynchronizer(const FrameSynchronizer&)            = delete;
        FrameSynchronizer& operator=(const FrameSynchronizer&) = delete;
        FrameSynchronizer(FrameSynchronizer&&)                 = delete;
        FrameSynchronizer& operator=(FrameSynchronizer&&)      = delete;

        /// @brief Initialize frame synchronization
        /// @param context Vulkan context
        /// @param framesInFlight Number of frames to allow in flight (2-3 recommended)
        /// @return Result containing success or error message
        Result<void> Initialize(VulkanContext* context, u32 framesInFlight = 2);

        /// @brief Shutdown and cleanup all synchronization resources
        void Shutdown();

        /// @brief Begin a new frame (waits on fence, resets command buffer)
        /// @return Result containing success or error message
        Result<void> BeginFrame() const;

        /// @brief End current frame (advances to next frame)
        void EndFrame();

        /// @brief Wait for current frame's fence
        /// @param timeout Timeout in nanoseconds
        /// @return Result containing success or error message
        Result<void> WaitForFrame(u64 timeout = UINT64_MAX) const;

        /// @brief Reset current frame's fence
        void ResetFence() const;

        // Getters for current frame
        V_ND FrameContext& GetCurrentFrame() {
            return mFrames[mCurrentFrameIndex];
        }

        V_ND const FrameContext& GetCurrentFrame() const {
            return mFrames[mCurrentFrameIndex];
        }

        V_ND VkFence GetCurrentFence() const {
            return mFrames[mCurrentFrameIndex].inFlightFence;
        }

        V_ND VkSemaphore GetCurrentImageAvailableSemaphore() const {
            return mFrames[mCurrentFrameIndex].imageAvailableSemaphore;
        }

        V_ND VkSemaphore GetCurrentRenderFinishedSemaphore() const {
            return mFrames[mCurrentFrameIndex].renderFinishedSemaphore;
        }

        V_ND VkCommandBuffer GetCurrentCommandBuffer() const {
            return mFrames[mCurrentFrameIndex].commandBuffer;
        }

        V_ND u32 GetCurrentFrameIndex() const {
            return mCurrentFrameIndex;
        }

        V_ND u32 GetFramesInFlight() const {
            return static_cast<u32>(mFrames.size());
        }

        V_ND bool IsInitialized() const {
            return mContext != nullptr && !mFrames.empty();
        }

    private:
        /// @brief Create synchronization objects for a single frame
        Result<void> CreateFrameContext(FrameContext& frame) const;

        /// @brief Destroy synchronization objects for a single frame
        void DestroyFrameContext(FrameContext& frame) const;

        VulkanContext* mContext {nullptr};
        std::vector<FrameContext> mFrames;
        u32 mCurrentFrameIndex {0};
    };
}  // namespace Vulkano