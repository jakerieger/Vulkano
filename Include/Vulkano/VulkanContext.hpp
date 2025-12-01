// Author: Jake Rieger
// Created: 11/30/25.
//

#pragma once

#include "Types.hpp"
#include "Macros.hpp"

#include <vk_mem_alloc.h>
#include <memory>
#include <vector>

namespace Vulkano {
    class SwapchainManager;

    /// @brief Core Vulkan context managing instance, device, and queues
    class VulkanContext {
    public:
        /// @brief Configuration for instance creation
        struct InstanceConfig {
            const char* applicationName {"Vulkano Application"};
            u32 applicationVersion {VK_MAKE_VERSION(1, 0, 0)};
            bool enableValidation {true};
            std::vector<const char*> instanceExtensions {};
        };

        /// @brief Configuration for device creation
        struct DeviceConfig {
            std::vector<const char*> deviceExtensions {};
            VkSurfaceKHR surface {VK_NULL_HANDLE};  // Optional, for presentation support
        };

        /// @brief Full configuration (for convenience method)
        struct Config {
            InstanceConfig instance;
            DeviceConfig device;
        };

        VulkanContext();
        ~VulkanContext();

        VulkanContext(const VulkanContext&)            = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&&)                 = delete;
        VulkanContext& operator=(VulkanContext&&)      = delete;

        /// @brief Create Vulkan instance
        /// @param config Instance configuration
        /// @return Result containing success or error message
        Result<void> CreateInstance(const InstanceConfig& config);

        /// @brief Create logical device (call after CreateInstance and optionally after surface creation)
        /// @param config Device configuration
        /// @return Result containing success or error message
        Result<void> CreateDevice(const DeviceConfig& config);

        /// @brief Convenience method to create instance and device in one call
        /// @param config Full configuration
        /// @return Result containing success or error message
        Result<void> Initialize(const Config& config);

        /// @brief Shutdown and cleanup all Vulkan resources
        void Shutdown();

        /// @brief Wait for all device operations to complete
        void WaitIdle() const;

        // State queries
        V_ND bool HasInstance() const {
            return mInstance != VK_NULL_HANDLE;
        }

        V_ND bool HasDevice() const {
            return mDevice != VK_NULL_HANDLE;
        }

        V_ND bool IsInitialized() const {
            return HasInstance() && HasDevice();
        }

        // Getters
        V_ND VkInstance GetInstance() const {
            return mInstance;
        }

        V_ND VkPhysicalDevice GetPhysicalDevice() const {
            return mPhysicalDevice;
        }

        V_ND VkDevice GetDevice() const {
            return mDevice;
        }

        V_ND VmaAllocator GetAllocator() const {
            return mAllocator;
        }

        V_ND VkQueue GetGraphicsQueue() const {
            return mGraphicsQueue;
        }

        V_ND VkQueue GetComputeQueue() const {
            return mComputeQueue;
        }

        V_ND VkQueue GetTransferQueue() const {
            return mTransferQueue;
        }

        V_ND VkQueue GetPresentQueue() const {
            return mPresentQueue;
        }

        V_ND const QueueFamilyIndices& GetQueueFamilies() const {
            return mQueueFamilies;
        }

        V_ND const VkPhysicalDeviceProperties& GetDeviceProperties() const {
            return mDeviceProperties;
        }

        V_ND const VkPhysicalDeviceFeatures& GetDeviceFeatures() const {
            return mDeviceFeatures;
        }

    private:
        /// @brief Initialize VMA allocator
        Result<void> InitializeAllocator();

        VkInstance mInstance {VK_NULL_HANDLE};
        VkPhysicalDevice mPhysicalDevice {VK_NULL_HANDLE};
        VkDevice mDevice {VK_NULL_HANDLE};
        VmaAllocator mAllocator {VK_NULL_HANDLE};

        VkQueue mGraphicsQueue {VK_NULL_HANDLE};
        VkQueue mComputeQueue {VK_NULL_HANDLE};
        VkQueue mTransferQueue {VK_NULL_HANDLE};
        VkQueue mPresentQueue {VK_NULL_HANDLE};

        QueueFamilyIndices mQueueFamilies {};
        VkPhysicalDeviceProperties mDeviceProperties {};
        VkPhysicalDeviceFeatures mDeviceFeatures {};

        // Pimpl for vk-bootstrap objects
        struct Impl;
        std::unique_ptr<Impl> mImpl;

        bool mValidationEnabled {false};
    };
}  // namespace Vulkano