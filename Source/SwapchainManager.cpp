// Author: Jake Rieger
// Created: 11/30/25.
//

#include "SwapchainManager.hpp"
#include "VulkanContext.hpp"

#include <VkBootstrap.h>

namespace Vulkano {
    struct SwapchainManager::Impl {
        std::unique_ptr<vkb::Swapchain> vkbSwapchain;
    };

    SwapchainManager::SwapchainManager() : mImpl(std::make_unique<Impl>()) {}

    SwapchainManager::~SwapchainManager() {
        Shutdown();
    }

    SwapchainManager::SwapchainManager(SwapchainManager&&) noexcept            = default;
    SwapchainManager& SwapchainManager::operator=(SwapchainManager&&) noexcept = default;

    Result<void> SwapchainManager::Initialize(
      VulkanContext* context, VkSurfaceKHR surface, u32 width, u32 height, const SwapchainConfig& config) {
        if (!context || !context->IsInitialized()) { return std::unexpected("Invalid or uninitialized context"); }

        if (surface == VK_NULL_HANDLE) { return std::unexpected("Invalid surface provided"); }

        if (width == 0 || height == 0) { return std::unexpected("Invalid swapchain dimensions"); }

        mContext = context;
        mSurface = surface;
        mConfig  = config;

        // Create swapchain using vk-bootstrap
        vkb::SwapchainBuilder swapchainBuilder(context->GetPhysicalDevice(), context->GetDevice(), surface);

        swapchainBuilder.set_desired_format({config.preferredFormat, config.preferredColorSpace})
          .set_desired_present_mode(config.preferredPresentMode)
          .set_desired_min_image_count(config.minImageCount)
          .set_desired_extent(width, height)
          .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)  // Always supported fallback
          .add_fallback_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .add_fallback_format({VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});

        auto swapchainResult = swapchainBuilder.build();
        if (!swapchainResult) {
            return std::unexpected(std::string("Failed to create swapchain: ") + swapchainResult.error().message());
        }

        mImpl->vkbSwapchain = std::make_unique<vkb::Swapchain>(swapchainResult.value());
        mSwapchain          = mImpl->vkbSwapchain->swapchain;
        mFormat             = mImpl->vkbSwapchain->image_format;
        mColorSpace         = mImpl->vkbSwapchain->color_space;
        mExtent             = mImpl->vkbSwapchain->extent;
        mPresentMode        = mImpl->vkbSwapchain->present_mode;

        // Get swapchain images
        auto imagesResult = mImpl->vkbSwapchain->get_images();
        if (!imagesResult) { return std::unexpected("Failed to get swapchain images"); }
        mImages = imagesResult.value();

        // Create image views
        return CreateImageViews();
    }

    Result<void> SwapchainManager::Recreate(u32 width, u32 height) {
        if (!mContext) { return std::unexpected("Context not set"); }

        if (width == 0 || height == 0) { return std::unexpected("Invalid swapchain dimensions"); }

        // Wait for device to be idle before recreating
        mContext->WaitIdle();

        // Destroy old image views
        DestroyImageViews();

        // Create new swapchain with old one as reference for optimization
        vkb::SwapchainBuilder swapchainBuilder(mContext->GetPhysicalDevice(), mContext->GetDevice(), mSurface);

        swapchainBuilder.set_desired_format({mConfig.preferredFormat, mConfig.preferredColorSpace})
          .set_desired_present_mode(mConfig.preferredPresentMode)
          .set_desired_min_image_count(mConfig.minImageCount)
          .set_desired_extent(width, height)
          .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .add_fallback_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .add_fallback_format({VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});

        // Use old swapchain for better performance
        if (mImpl->vkbSwapchain) { swapchainBuilder.set_old_swapchain(*mImpl->vkbSwapchain); }

        auto swapchainResult = swapchainBuilder.build();
        if (!swapchainResult) {
            return std::unexpected(std::string("Failed to recreate swapchain: ") + swapchainResult.error().message());
        }

        // Destroy old swapchain
        if (mImpl->vkbSwapchain) { vkb::destroy_swapchain(*mImpl->vkbSwapchain); }

        mImpl->vkbSwapchain = std::make_unique<vkb::Swapchain>(swapchainResult.value());
        mSwapchain          = mImpl->vkbSwapchain->swapchain;
        mFormat             = mImpl->vkbSwapchain->image_format;
        mColorSpace         = mImpl->vkbSwapchain->color_space;
        mExtent             = mImpl->vkbSwapchain->extent;
        mPresentMode        = mImpl->vkbSwapchain->present_mode;

        // Get new swapchain images
        auto imagesResult = mImpl->vkbSwapchain->get_images();
        if (!imagesResult) { return std::unexpected("Failed to get swapchain images"); }
        mImages = imagesResult.value();

        return CreateImageViews();
    }

    Result<u32> SwapchainManager::AcquireNextImage(VkSemaphore signalSemaphore, u64 timeout) const {
        if (!mContext || mSwapchain == VK_NULL_HANDLE) { return std::unexpected("Swapchain not initialized"); }

        u32 imageIndex;
        const VkResult result = vkAcquireNextImageKHR(mContext->GetDevice(),
                                                      mSwapchain,
                                                      timeout,
                                                      signalSemaphore,
                                                      VK_NULL_HANDLE,
                                                      &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return std::unexpected("Swapchain out of date - needs recreation");
        } else if (result == VK_SUBOPTIMAL_KHR) {
            // Still return the image but log that it's suboptimal
            // User may want to recreate on next frame
            return imageIndex;
        } else if (result != VK_SUCCESS) {
            return std::unexpected("Failed to acquire swapchain image");
        }

        return imageIndex;
    }

    Result<void> SwapchainManager::Present(u32 imageIndex, VkSemaphore waitSemaphore) const {
        if (!mContext || mSwapchain == VK_NULL_HANDLE) { return std::unexpected("Swapchain not initialized"); }

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &waitSemaphore;
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &mSwapchain;
        presentInfo.pImageIndices      = &imageIndex;
        presentInfo.pResults           = nullptr;  // Optional

        const VkResult result = vkQueuePresentKHR(mContext->GetPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return std::unexpected("Swapchain out of date - needs recreation");
        } else if (result != VK_SUCCESS) {
            return std::unexpected("Failed to present swapchain image");
        }

        return {};
    }

    void SwapchainManager::Shutdown() {
        if (!mContext) { return; }

        DestroyImageViews();

        if (mImpl->vkbSwapchain) {
            vkb::destroy_swapchain(*mImpl->vkbSwapchain);
            mImpl->vkbSwapchain.reset();
            mSwapchain = VK_NULL_HANDLE;
        }

        mImages.clear();
    }

    Result<void> SwapchainManager::CreateImageViews() {
        auto imageViewsResult = mImpl->vkbSwapchain->get_image_views();
        if (!imageViewsResult) { return std::unexpected("Failed to create swapchain image views"); }

        mImageViews = imageViewsResult.value();
        return {};
    }

    void SwapchainManager::DestroyImageViews() {
        if (!mContext) { return; }

        for (VkImageView imageView : mImageViews) {
            if (imageView != VK_NULL_HANDLE) { vkDestroyImageView(mContext->GetDevice(), imageView, nullptr); }
        }
        mImageViews.clear();
    }
}  // namespace Vulkano