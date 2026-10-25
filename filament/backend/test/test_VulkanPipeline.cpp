/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vulkan/memory/ResourceManager.h"
#include "vulkan/memory/ResourcePointer.h"
#include "vulkan/VulkanPipeline.h"

#include <bluevk/BlueVK.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using namespace filament::backend;
using namespace filament::backend::fvkmemory;

namespace {
uint32_t gDestroyedPipelines;
VKAPI_ATTR void VKAPI_CALL destroyPipeline(VkDevice, VkPipeline, const VkAllocationCallbacks*) {
    ++gDestroyedPipelines;
}
class VulkanPipelineLifetime : public testing::Test {
protected:
    void SetUp() override {
        gDestroyedPipelines = 0;
        mDestroyPipeline = bluevk::vkDestroyPipeline;
        bluevk::vkDestroyPipeline = destroyPipeline;
    }
    void TearDown() override { bluevk::vkDestroyPipeline = mDestroyPipeline; }
private:
    PFN_vkDestroyPipeline mDestroyPipeline = nullptr;
};
TEST_F(VulkanPipelineLifetime, EvictionRetainsAllPendingUses) {
    ResourceManager manager(8 * 1024 * 1024, false, false);
    auto cached = resource_ptr<VulkanPipeline>::construct(&manager, VK_NULL_HANDLE,
            (VkPipeline) uintptr_t(1));
    std::vector<resource_ptr<Resource>> firstCommands{cached};
    std::vector<resource_ptr<Resource>> secondCommands{cached};
    cached = {};
    manager.gc();
    EXPECT_EQ(gDestroyedPipelines, 0u);
    firstCommands.clear();
    manager.gc();
    EXPECT_EQ(gDestroyedPipelines, 0u);
    secondCommands.clear();
    EXPECT_EQ(gDestroyedPipelines, 0u); // Destruction stays on the manager's GC path.
    manager.gc();
    EXPECT_EQ(gDestroyedPipelines, 1u);
    manager.terminate();
    EXPECT_EQ(gDestroyedPipelines, 1u);
}
TEST_F(VulkanPipelineLifetime, CompletionDoesNotEvictCachedPipeline) {
    ResourceManager manager(8 * 1024 * 1024, false, false);
    auto cached = resource_ptr<VulkanPipeline>::construct(&manager, VK_NULL_HANDLE,
            (VkPipeline) uintptr_t(1));
    std::vector<resource_ptr<Resource>> commands{cached};
    commands.clear();
    manager.gc();
    EXPECT_EQ(gDestroyedPipelines, 0u);
    cached = {};
    manager.terminate();
    EXPECT_EQ(gDestroyedPipelines, 1u);
}
TEST_F(VulkanPipelineLifetime, ReplacementDoesNotReleaseOldSubmission) {
    ResourceManager manager(8 * 1024 * 1024, false, false);
    auto cached = resource_ptr<VulkanPipeline>::construct(&manager, VK_NULL_HANDLE,
            (VkPipeline) uintptr_t(1));
    std::vector<resource_ptr<Resource>> pending{cached};
    cached = resource_ptr<VulkanPipeline>::construct(&manager, VK_NULL_HANDLE,
            (VkPipeline) uintptr_t(2));
    manager.gc();
    EXPECT_EQ(gDestroyedPipelines, 0u);
    cached = {};
    manager.gc();
    EXPECT_EQ(gDestroyedPipelines, 1u);
    pending.clear();
    manager.terminate();
    EXPECT_EQ(gDestroyedPipelines, 2u);
}
} // namespace
