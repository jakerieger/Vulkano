// Author: Jake Rieger
// Created: 11/30/25.
//

#pragma once

#include "Types.hpp"
#include "Macros.hpp"

#include <vector>
#include <memory>

namespace Vulkano {
    class VulkanContext;

    /// @brief Manages swapchain creation, recreation, and presentation
    class SwapchainManager {
    public:
        SwapchainManager();
        ~SwapchainManager();

        SwapchainManager(const SwapchainManager&)            = delete;
        SwapchainManager& operator=(const SwapchainManager&) = delete;
        SwapchainManager(SwapchainManager&&) noexcept;
        SwapchainManager& operator=(SwapchainManager&&) noexcept;

        /// @brief Initialize swapchain
        /// @param context Vulkan context
        /// @param surface Window surface (created by user with GLFW/SDL/etc)
        /// @param width Swapchain width
        /// @param height Swapchain height
        /// @param config Swapchain configuration
        /// @return Result containing success or error message
        Result<void>
        Initialize(VulkanContext* context, VkSurfaceKHR surface, u32 width, u32 height, const SwapchainConfig& config);

        /// @brief Initialize with default configuration
        Result<void> Initialize(VulkanContext* context, VkSurfaceKHR surface, u32 width, u32 height) {
            return Initialize(context, surface, width, height, SwapchainConfig {});
        }

        /// @brief Recreate swapchain (e.g., after window resize)
        /// @param width New width
        /// @param height New height
        /// @return Result containing success or error message
        Result<void> Recreate(u32 width, u32 height);

        /// @brief Acquire next swapchain image
        /// @param signalSemaphore Semaphore to signal when image is acquired
        /// @param timeout Timeout in nanoseconds
        /// @return Result containing image index or error message
        Result<u32> AcquireNextImage(VkSemaphore signalSemaphore, uint64_t timeout = UINT64_MAX) const;

        /// @brief Present swapchain image
        /// @param imageIndex Index of image to present
        /// @param waitSemaphore Semaphore to wait on before presenting
        /// @return Result containing success or error message
        Result<void> Present(u32 imageIndex, VkSemaphore waitSemaphore) const;

        /// @brief Cleanup swapchain resources
        void Shutdown();

        // Getters
        V_ND VkSwapchainKHR GetSwapchain() const {
            return mSwapchain;
        }

        V_ND VkFormat GetFormat() const {
            return mFormat;
        }

        V_ND VkColorSpaceKHR GetColorSpace() const {
            return mColorSpace;
        }

        V_ND VkExtent2D GetExtent() const {
            return mExtent;
        }

        V_ND VkPresentModeKHR GetPresentMode() const {
            return mPresentMode;
        }

        V_ND u32 GetImageCount() const {
            return CAST<u32>(mImages.size());
        }

        V_ND const std::vector<VkImage>& GetImages() const {
            return mImages;
        }

        V_ND const std::vector<VkImageView>& GetImageViews() const {
            return mImageViews;
        }

        V_ND VkImageView GetImageView(u32 index) const {
            return mImageViews[index];
        }

        V_ND bool IsInitialized() const {
            return mSwapchain != VK_NULL_HANDLE;
        }

    private:
        /// @brief Create image views for swapchain images
        Result<void> CreateImageViews();

        /// @brief Destroy image views
        void DestroyImageViews();

        VulkanContext* mContext {nullptr};
        VkSurfaceKHR mSurface {VK_NULL_HANDLE};
        VkSwapchainKHR mSwapchain {VK_NULL_HANDLE};

        VkFormat mFormat {VK_FORMAT_UNDEFINED};
        VkColorSpaceKHR mColorSpace {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        VkExtent2D mExtent {0, 0};
        VkPresentModeKHR mPresentMode {VK_PRESENT_MODE_FIFO_KHR};

        std::vector<VkImage> mImages;
        std::vector<VkImageView> mImageViews;

        SwapchainConfig mConfig {};

        // Pimpl for vk-bootstrap swapchain
        struct Impl;
        std::unique_ptr<Impl> mImpl;
    };
}  // namespace Vulkano