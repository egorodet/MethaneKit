/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Vulkan/Types.h
Methane graphics types converters to Vulkan native types.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Volume.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

class TypeConverter
{
public:
    [[nodiscard]] static vk::Format PixelFormatToVulkan(PixelFormat pixel_format);
    [[nodiscard]] static vk::CompareOp CompareFunctionToVulkan(Compare compare_func);
    [[nodiscard]] static vk::Extent3D DimensionsToExtent3D(const Dimensions& dimensions);
    [[nodiscard]] static vk::Extent3D FrameSizeToExtent3D(const FrameSize& frame_size);
    [[nodiscard]] static vk::SampleCountFlagBits SampleCountToVulkan(uint32_t sample_count);

private:
    TypeConverter() = default;
};

} // namespace Methane::Graphics::Vulkan
