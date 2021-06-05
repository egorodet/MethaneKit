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

FILE: Methane/Graphics/Vulkan/TextureVK.h
Vulkan implementation of the texture interface.

******************************************************************************/

#pragma once

#include "ResourceVK.h"

#include <Methane/Graphics/TextureBase.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

class RenderContextVK;

class TextureVK : public ResourceVK<TextureBase>
{
public:
    // Temporary constructor, to be removed
    TextureVK(const RenderContextVK& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage);
    ~TextureVK() override;

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue*) override;

    // Object interface
    void SetName(const std::string& name) override;

    const vk::Image&     GetNativeImage() const noexcept      { return m_vk_image; }
    const vk::ImageView& GetNativeImageView() const noexcept  { return m_vk_image_view; }

protected:
    TextureVK(const RenderContextVK& context, const Settings& settings,
              const DescriptorByUsage& descriptor_by_usage,
              const vk::Image& image, vk::ImageView&& image_view);

private:
    void GenerateMipLevels();

    vk::Image     m_vk_image;
    vk::ImageView m_vk_image_view;
};

class FrameBufferTextureVK final : public TextureVK
{
public:
    FrameBufferTextureVK(const RenderContextVK& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage,
                         FrameBufferIndex frame_buffer_index);

    [[nodiscard]] FrameBufferIndex GetFrameBufferIndex() const noexcept { return m_frame_buffer_index; }

private:
    FrameBufferTextureVK(const RenderContextVK& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage,
                         FrameBufferIndex frame_buffer_index, const vk::Image& image);

    const FrameBufferIndex m_frame_buffer_index;
};

} // namespace Methane::Graphics
