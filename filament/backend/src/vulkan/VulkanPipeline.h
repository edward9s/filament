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

#ifndef TNT_FILAMENT_BACKEND_VULKANPIPELINE_H
#define TNT_FILAMENT_BACKEND_VULKANPIPELINE_H

#include "vulkan/memory/Resource.h"

#include <bluevk/BlueVK.h>

namespace filament::backend {

// The cache and every command buffer using this pipeline hold independent references.
// Cache eviction only drops its reference; GPU completion releases command references.
struct VulkanPipeline : fvkmemory::Resource {
    VulkanPipeline(VkDevice device, VkPipeline handle) noexcept
            : device(device), handle(handle) {}

    VulkanPipeline(VulkanPipeline const&) = delete;
    VulkanPipeline& operator=(VulkanPipeline const&) = delete;

    ~VulkanPipeline() {
        bluevk::vkDestroyPipeline(device, handle, nullptr);
    }

    VkDevice const device;
    VkPipeline const handle;
};

} // namespace filament::backend

#endif // TNT_FILAMENT_BACKEND_VULKANPIPELINE_H
