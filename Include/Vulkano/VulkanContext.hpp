// Author: Jake Rieger
// Created: 11/30/25.
//

#pragma once

#include "Types.hpp"
#include "Macros.hpp"

#include <vk_mem_alloc.h>
#include <memory>
#include <vector>

namespace vkb {
    struct Instance;
    struct PhysicalDevice;
    struct Device;
}  // namespace vkb

namespace Vulkano {
    class SwapchainManager;

    /// @brief Core Vulkan context managing instance, device, and queues
    class VulkanContext {
    public:
        /// @brief Configuration for context creation
        struct Config {
            const std::string_view applicationName {"Vulkano Application"};
            u32 applicationVersion {VK_MAKE_VERSION(1, 0, 0)};
            bool enableValidation {true};
            std::vector<const char*> instanceExtensions;
            std::vector<const char*> deviceExtensions;
        };

        VulkanContext() = default;
        ~VulkanContext();

        VulkanContext(const VulkanContext&)            = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&&)                 = delete;
        VulkanContext& operator=(VulkanContext&&)      = delete;

        /// @brief Initialize the Vulkan context
        /// @param config Configuration parameters
        /// @return Result containing success or error message
        Result<void> Initialize(const Config& config);

        /// @brief Initialize with default configuration
        Result<void> Initialize() {
            return Initialize(Config {});
        }

        /// @brief Shutdown and cleanup all Vulkan resources
        void Shutdown();

        /// @brief Wait for all device operations to complete
        void WaitIdle() const;

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

        V_ND bool IsInitialized() const {
            return mInitialized;
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

        // vk-bootstrap objects (stored as opaque pointers to avoid header pollution)
        std::unique_ptr<vkb::Instance> mVkbInstance;
        std::unique_ptr<vkb::PhysicalDevice> mVkbPhysicalDevice;
        std::unique_ptr<vkb::Device> mVkbDevice;

        bool mInitialized {false};
        bool mValidationEnabled {false};
    };
}  // namespace Vulkano