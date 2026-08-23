/******************************************************************************

Copyright 2019-2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBinding.hh
Metal implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/ProgramArgumentBinding.h>

#import <Metal/Metal.h>

#include <map>
#include <memory>
#include <mutex>

namespace Methane::Graphics::Metal
{

struct ProgramArgumentBindingSettings final
    : Rhi::ProgramArgumentBindingSettings
{
    using StructOffset = uint32_t;
    using StructOffsetByShaderType = std::map<Rhi::ShaderType, StructOffset>;

    uint32_t argument_index;
    StructOffsetByShaderType argument_buffer_offset_by_shader_type;
};

class Program;

class ProgramArgumentBinding final
    : public Base::ProgramArgumentBinding
{
public:
    using Settings             = ProgramArgumentBindingSettings;
    using NativeResources      = std::vector<__unsafe_unretained id<MTLResource>>;
    using NativeBuffers        = std::vector<__unsafe_unretained id<MTLBuffer>>;
    using NativeTextures       = std::vector<__unsafe_unretained id<MTLTexture>>;
    using NativeSamplerStates  = std::vector<__unsafe_unretained id<MTLSamplerState>>;
    using NativeOffsets        = std::vector<NSUInteger>;

    struct NativeResourceViews
    {
        using Ptr = std::shared_ptr<const NativeResourceViews>;

        MTLResourceUsage    resource_usage = MTLResourceUsageRead;
        NativeResources     resources;
        NativeSamplerStates sampler_states;
        NativeTextures      textures;
        NativeBuffers       buffers;
        NativeOffsets       buffer_offsets;
    };

    ProgramArgumentBinding(const Base::Context& context, const Settings& settings);
    ProgramArgumentBinding(const ProgramArgumentBinding& other);

    // Base::ProgramArgumentBinding interface
    [[nodiscard]] Ptr<Base::ProgramArgumentBinding> CreateCopy() const override;
    void MergeSettings(const Base::ProgramArgumentBinding& other) override;

    // IArgumentBinding interface
    bool SetResourceViewSpan(Rhi::ResourceViewSpan resource_views) override;

    void UpdateArgumentBufferOffsets(const Program& program);

    bool                       IsArgumentBufferMode() const noexcept   { return !m_settings_mt.argument_buffer_offset_by_shader_type.empty(); }
    const Settings&            GetMetalSettings() const noexcept       { return m_settings_mt; }
    MTLRenderStages            GetNativeRenderStages() const noexcept  { return m_mtl_render_stages; }
    NativeResourceViews::Ptr   GetNativeResourceViews() const noexcept;

protected:
    // Base::ProgramArgumentBinding overrides...
    bool UpdateRootConstantResourceViews() override;

private:
    void SetMetalResourcesForViews(Rhi::ResourceViewSpan resource_views);

    Settings                          m_settings_mt;
    MTLRenderStages                   m_mtl_render_stages;
    NativeResourceViews::Ptr          m_mtl_resource_views;
    mutable TracyLockable(std::mutex, m_mtl_resource_views_mutex);
};

} // namespace Methane::Graphics::Metal
