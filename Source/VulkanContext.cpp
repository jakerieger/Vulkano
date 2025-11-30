// Author: Jake Rieger
// Created: 11/30/25.
//

#include "VulkanContext.hpp"
#include "VkBootstrap.h"

namespace Vulkano {
    VulkanContext::~VulkanContext() {
        Shutdown();
    }

    Result<void> VulkanContext::Initialize(const Config& config) {
        if (mInitialized) { return std::unexpected("Context already initialized"); }

        mValidationEnabled = config.enableValidation;

        // Create instance using vk-bootstrap
        vkb::InstanceBuilder instanceBuilder;
        instanceBuilder.set_app_name(config.applicationName.data())
          .set_app_version(config.applicationVersion)
          .require_api_version(1, 3, 0);

        // Add requested instance extensions
        for (const char* ext : config.instanceExtensions) {
            instanceBuilder.enable_extension(ext);
        }

#ifndef NDEBUG
        if (mValidationEnabled) { instanceBuilder.request_validation_layers().use_default_debug_messenger(); }
#endif

        auto instanceResult = instanceBuilder.build();
        if (!instanceResult) {
            return std::unexpected(std::string("Failed to create instance: ") + instanceResult.error().message());
        }

        mVkbInstance = std::make_unique<vkb::Instance>(instanceResult.value());
        mInstance    = mVkbInstance->instance;

        // Select physical device
        vkb::PhysicalDeviceSelector selector(*mVkbInstance);
        selector.set_minimum_version(1, 3).require_present();

        // Add requested device extensions
        for (const char* ext : config.deviceExtensions) {
            selector.add_required_extension(ext);
        }

        auto physicalDeviceResult = selector.select();
        if (!physicalDeviceResult) {
            return std::unexpected(std::string("Failed to select physical device: ") +
                                   physicalDeviceResult.error().message());
        }

        mVkbPhysicalDevice = std::make_unique<vkb::PhysicalDevice>(physicalDeviceResult.value());
        mPhysicalDevice    = mVkbPhysicalDevice->physical_device;

        // Get device properties and features
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties);
        vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mDeviceFeatures);

        // Create logical device
        vkb::DeviceBuilder deviceBuilder(*mVkbPhysicalDevice);

        auto deviceResult = deviceBuilder.build();
        if (!deviceResult) {
            return std::unexpected(std::string("Failed to create device: ") + deviceResult.error().message());
        }

        mVkbDevice = std::make_unique<vkb::Device>(deviceResult.value());
        mDevice    = mVkbDevice->device;

        // Get queues
        auto graphicsQueueResult = mVkbDevice->get_queue(vkb::QueueType::graphics);
        auto computeQueueResult  = mVkbDevice->get_queue(vkb::QueueType::compute);
        auto transferQueueResult = mVkbDevice->get_queue(vkb::QueueType::transfer);
        auto presentQueueResult  = mVkbDevice->get_queue(vkb::QueueType::present);

        if (!graphicsQueueResult || !presentQueueResult) { return std::unexpected("Failed to get required queues"); }

        mGraphicsQueue = graphicsQueueResult.value();
        mPresentQueue  = presentQueueResult.value();
        mComputeQueue  = computeQueueResult.has_value() ? computeQueueResult.value() : mGraphicsQueue;
        mTransferQueue = transferQueueResult.has_value() ? transferQueueResult.value() : mGraphicsQueue;

        // Get queue family indices
        auto graphicsFamilyResult = mVkbDevice->get_queue_index(vkb::QueueType::graphics);
        auto computeFamilyResult  = mVkbDevice->get_queue_index(vkb::QueueType::compute);
        auto transferFamilyResult = mVkbDevice->get_queue_index(vkb::QueueType::transfer);
        auto presentFamilyResult  = mVkbDevice->get_queue_index(vkb::QueueType::present);

        if (graphicsFamilyResult && presentFamilyResult) {
            mQueueFamilies.graphicsFamily = graphicsFamilyResult.value();
            mQueueFamilies.presentFamily  = presentFamilyResult.value();
            mQueueFamilies.computeFamily =
              computeFamilyResult.has_value() ? computeFamilyResult.value() : mQueueFamilies.graphicsFamily;
            mQueueFamilies.transferFamily =
              transferFamilyResult.has_value() ? transferFamilyResult.value() : mQueueFamilies.graphicsFamily;

            mQueueFamilies.hasDiscreteCompute =
              computeFamilyResult.has_value() && computeFamilyResult.value() != mQueueFamilies.graphicsFamily;
            mQueueFamilies.hasDiscreteTransfer =
              transferFamilyResult.has_value() && transferFamilyResult.value() != mQueueFamilies.graphicsFamily;
        }

        // Initialize VMA
        if (auto result = InitializeAllocator(); !result) { return result; }

        mInitialized = true;
        return {};
    }

    void VulkanContext::Shutdown() {
        if (!mInitialized) { return; }

        WaitIdle();

        if (mAllocator) {
            vmaDestroyAllocator(mAllocator);
            mAllocator = VK_NULL_HANDLE;
        }

        if (mVkbDevice) {
            vkb::destroy_device(*mVkbDevice);
            mVkbDevice.reset();
            mDevice = VK_NULL_HANDLE;
        }

        if (mVkbInstance) {
            vkb::destroy_instance(*mVkbInstance);
            mVkbInstance.reset();
            mInstance = VK_NULL_HANDLE;
        }

        mInitialized = false;
    }

    void VulkanContext::WaitIdle() const {
        if (mDevice) { vkDeviceWaitIdle(mDevice); }
    }

    Result<void> VulkanContext::InitializeAllocator() {
        VmaAllocatorCreateInfo allocatorInfo {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorInfo.instance         = mInstance;
        allocatorInfo.physicalDevice   = mPhysicalDevice;
        allocatorInfo.device           = mDevice;

        if (vmaCreateAllocator(&allocatorInfo, &mAllocator) != VK_SUCCESS) {
            return std::unexpected("Failed to create VMA allocator");
        }

        return {};
    }
}  // namespace Vulkano