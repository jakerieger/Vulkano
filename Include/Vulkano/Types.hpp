// Author: Jake Rieger
// Created: 11/30/25.
//

#pragma once

#include <vulkan/vulkan.h>
#include <expected>
#include <string>

namespace Vulkano {
    /// @brief Primitive type aliases (why C++ hasn't adopted these yet is beyond me)
    using u8   = uint8_t;
    using u16  = uint16_t;
    using u32  = uint32_t;
    using u64  = uint64_t;
    using uptr = uintptr_t;

    using i8   = int8_t;
    using i16  = int16_t;
    using i32  = int32_t;
    using i64  = int64_t;
    using iptr = intptr_t;

#ifndef _MSC_VER
    using u128 = __uint128_t;
    using i128 = __int128_t;
#endif

    using f32 = float;
    using f64 = double;

    /// @brief Result type for operations that can fail
    template<typename T>
    using Result = std::expected<T, std::string>;

    /// @brief Queue family indices
    struct QueueFamilyIndices {
        u32 graphicsFamily;
        u32 computeFamily;
        u32 transferFamily;
        u32 presentFamily;

        bool hasDiscreteTransfer;
        bool hasDiscreteCompute;
    };

    /// @brief Swapchain configuration
    struct SwapchainConfig {
        VkPresentModeKHR preferredPresentMode {VK_PRESENT_MODE_MAILBOX_KHR};
        VkFormat preferredFormat {VK_FORMAT_B8G8R8A8_UNORM};
        VkColorSpaceKHR preferredColorSpace {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        u32 minImageCount {3};
    };
}  // namespace Vulkano