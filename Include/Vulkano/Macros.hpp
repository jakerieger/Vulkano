// Author: Jake Rieger
// Created: 11/30/25.
//

#pragma once

#include <stdexcept>

#define V_ND [[nodiscard]]

namespace Vulkano {
    template<typename T>
    void AssertResult(const T& result) {
        if (!result.has_value()) { throw std::runtime_error(result.error()); }
    }
}  // namespace Vulkan