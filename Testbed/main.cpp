// Author: Jake Rieger
// Created: 11/30/25.
//

#include <Vulkano/VulkanContext.hpp>
#include <Vulkano/SwapchainManager.hpp>

int main() {
    using namespace Vulkano;

    VulkanContext ctx;
    const auto result = ctx.Initialize({.applicationName = "Testbed"});
    AssertResult(result);

    ctx.Shutdown();
}
