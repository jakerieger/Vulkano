// Author: Jake Rieger
// Created: 11/30/25.
//

#include "VulkanContext.hpp"

#include <VkBootstrap.h>

namespace Vulkano {
    struct VulkanContext::Impl {
        std::unique_ptr<vkb::Instance> vkbInstance;
        std::unique_ptr<vkb::PhysicalDevice> vkbPhysicalDevice;
        std::unique_ptr<vkb::Device> vkbDevice;
    };

    VulkanContext::VulkanContext() : mImpl(std::make_unique<Impl>()) {}

    VulkanContext::~VulkanContext() {
        Shutdown();
    }

    Result<void> VulkanContext::CreateInstance(const InstanceConfig& config) {
        if (HasInstance()) { return std::unexpected("Instance already created"); }

        mValidationEnabled = config.enableValidation;

        // Create instance using vk-bootstrap
        vkb::InstanceBuilder instanceBuilder;
        instanceBuilder.set_app_name(config.applicationName)
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

        mImpl->vkbInstance = std::make_unique<vkb::Instance>(instanceResult.value());
        mInstance          = mImpl->vkbInstance->instance;

        return {};
    }

    Result<void> VulkanContext::CreateDevice(const DeviceConfig& config) {
        if (!HasInstance()) { return std::unexpected("Instance must be created before device"); }

        if (HasDevice()) { return std::unexpected("Device already created"); }

        // Select physical device
        vkb::PhysicalDeviceSelector selector(*mImpl->vkbInstance);
        selector.set_minimum_version(1, 3).prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);

        // Set surface if provided for presentation support
        if (config.surface != VK_NULL_HANDLE) { selector.set_surface(config.surface); }

        // Add requested device extensions
        for (const char* ext : config.deviceExtensions) {
            selector.add_required_extension(ext);
        }

        auto physicalDeviceResult = selector.select();
        if (!physicalDeviceResult) {
            return std::unexpected(std::string("Failed to select physical device: ") +
                                   physicalDeviceResult.error().message());
        }

        mImpl->vkbPhysicalDevice = std::make_unique<vkb::PhysicalDevice>(physicalDeviceResult.value());
        mPhysicalDevice          = mImpl->vkbPhysicalDevice->physical_device;

        // Get device properties and features
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties);
        vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mDeviceFeatures);

        // Create logical device
        vkb::DeviceBuilder deviceBuilder(*mImpl->vkbPhysicalDevice);

        auto deviceResult = deviceBuilder.build();
        if (!deviceResult) {
            return std::unexpected(std::string("Failed to create device: ") + deviceResult.error().message());
        }

        mImpl->vkbDevice = std::make_unique<vkb::Device>(deviceResult.value());
        mDevice          = mImpl->vkbDevice->device;

        // Get queues
        auto graphicsQueueResult = mImpl->vkbDevice->get_queue(vkb::QueueType::graphics);
        if (!graphicsQueueResult) { return std::unexpected("Failed to get graphics queue"); }
        mGraphicsQueue = graphicsQueueResult.value();

        // Get optional queues (compute, transfer)
        auto computeQueueResult  = mImpl->vkbDevice->get_queue(vkb::QueueType::compute);
        auto transferQueueResult = mImpl->vkbDevice->get_queue(vkb::QueueType::transfer);

        mComputeQueue  = computeQueueResult.has_value() ? computeQueueResult.value() : mGraphicsQueue;
        mTransferQueue = transferQueueResult.has_value() ? transferQueueResult.value() : mGraphicsQueue;

        // Only get present queue if surface was provided
        if (config.surface != VK_NULL_HANDLE) {
            auto presentQueueResult = mImpl->vkbDevice->get_queue(vkb::QueueType::present);
            mPresentQueue           = presentQueueResult.has_value() ? presentQueueResult.value() : mGraphicsQueue;
        } else {
            mPresentQueue = mGraphicsQueue;
        }

        // Get queue family indices
        auto graphicsFamilyResult = mImpl->vkbDevice->get_queue_index(vkb::QueueType::graphics);
        auto computeFamilyResult  = mImpl->vkbDevice->get_queue_index(vkb::QueueType::compute);
        auto transferFamilyResult = mImpl->vkbDevice->get_queue_index(vkb::QueueType::transfer);

        if (!graphicsFamilyResult) { return std::unexpected("Failed to get graphics queue family index"); }

        mQueueFamilies.graphicsFamily = graphicsFamilyResult.value();
        mQueueFamilies.computeFamily =
          computeFamilyResult.has_value() ? computeFamilyResult.value() : mQueueFamilies.graphicsFamily;
        mQueueFamilies.transferFamily =
          transferFamilyResult.has_value() ? transferFamilyResult.value() : mQueueFamilies.graphicsFamily;

        if (config.surface != VK_NULL_HANDLE) {
            auto presentFamilyResult = mImpl->vkbDevice->get_queue_index(vkb::QueueType::present);
            mQueueFamilies.presentFamily =
              presentFamilyResult.has_value() ? presentFamilyResult.value() : mQueueFamilies.graphicsFamily;
        } else {
            mQueueFamilies.presentFamily = mQueueFamilies.graphicsFamily;
        }

        mQueueFamilies.hasDiscreteCompute =
          computeFamilyResult.has_value() && computeFamilyResult.value() != mQueueFamilies.graphicsFamily;
        mQueueFamilies.hasDiscreteTransfer =
          transferFamilyResult.has_value() && transferFamilyResult.value() != mQueueFamilies.graphicsFamily;

        // Initialize VMA
        if (auto result = InitializeAllocator(); !result) { return result; }

        return {};
    }

    Result<void> VulkanContext::Initialize(const Config& config) {
        if (auto result = CreateInstance(config.instance); !result) { return result; }

        return CreateDevice(config.device);
    }

    void VulkanContext::Shutdown() {
        WaitIdle();

        if (mAllocator) {
            vmaDestroyAllocator(mAllocator);
            mAllocator = VK_NULL_HANDLE;
        }

        if (mImpl->vkbDevice) {
            vkb::destroy_device(*mImpl->vkbDevice);
            mImpl->vkbDevice.reset();
            mDevice = VK_NULL_HANDLE;
        }

        if (mImpl->vkbPhysicalDevice) {
            mImpl->vkbPhysicalDevice.reset();
            mPhysicalDevice = VK_NULL_HANDLE;
        }

        if (mImpl->vkbInstance) {
            vkb::destroy_instance(*mImpl->vkbInstance);
            mImpl->vkbInstance.reset();
            mInstance = VK_NULL_HANDLE;
        }
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