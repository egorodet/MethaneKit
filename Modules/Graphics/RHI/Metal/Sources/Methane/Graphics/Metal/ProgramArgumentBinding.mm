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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBinding.mm
Metal implementation of the program argument binding interface.

******************************************************************************/

#include <Methane/Graphics/Metal/ProgramArgumentBinding.hh>
#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/Shader.hh>
#include <Methane/Graphics/Metal/Buffer.hh>
#include <Methane/Graphics/Metal/Texture.hh>
#include <Methane/Graphics/Metal/Sampler.hh>

#include <magic_enum/magic_enum.hpp>

namespace Methane::Graphics::Metal
{

using NativeBuffers       = ProgramArgumentBinding::NativeBuffers;
using NativeTextures      = ProgramArgumentBinding::NativeTextures;
using NativeSamplerStates = ProgramArgumentBinding::NativeSamplerStates;
using NativeOffsets       = ProgramArgumentBinding::NativeOffsets;

class CallbackBlocker
{
public:
    CallbackBlocker(ProgramArgumentBinding& program_argument_binding)
        : m_program_argument_binding(program_argument_binding)
    {
        m_program_argument_binding.SetEmitCallbackEnabled(false);
    }

    ~CallbackBlocker()
    {
        m_program_argument_binding.SetEmitCallbackEnabled(true);
    }

private:
    ProgramArgumentBinding& m_program_argument_binding;
};

static MTLRenderStages ConvertShaderTypeToMetalRenderStages(Rhi::ShaderType shader_type)
{
    META_FUNCTION_TASK();
    MTLRenderStages mtl_render_stages{};
    switch(shader_type)
    {
        using enum Rhi::ShaderType;
        case All:    mtl_render_stages |= MTLRenderStageVertex
                                       |  MTLRenderStageFragment; break;
        case Vertex: mtl_render_stages |= MTLRenderStageVertex; break;
        case Pixel:  mtl_render_stages |= MTLRenderStageFragment; break;
        case Compute: /* Compute is not Render stage */ break;
        default: META_UNEXPECTED(shader_type);
    }
    return mtl_render_stages;
}

static const ProgramArgumentBinding::NativeResourceViews::Ptr& GetEmptyNativeResourceViews()
{
    static const auto s_empty_resource_views = std::make_shared<const ProgramArgumentBinding::NativeResourceViews>();
    return s_empty_resource_views;
}

ProgramArgumentBinding::ProgramArgumentBinding(const Base::Context& context, const Settings& settings)
    : Base::ProgramArgumentBinding(context, settings)
    , m_settings_mt(settings)
    , m_mtl_render_stages(ConvertShaderTypeToMetalRenderStages(settings.argument.GetShaderType()))
    , m_mtl_resource_views(GetEmptyNativeResourceViews())
{
}

ProgramArgumentBinding::ProgramArgumentBinding(const ProgramArgumentBinding& other)
    : Base::ProgramArgumentBinding(other)
    , m_settings_mt(other.m_settings_mt)
    , m_mtl_render_stages(other.m_mtl_render_stages)
    , m_mtl_resource_views(other.m_mtl_resource_views)
{
}

Ptr<Base::ProgramArgumentBinding> ProgramArgumentBinding::CreateCopy() const
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramArgumentBinding>(*this);
}

void ProgramArgumentBinding::MergeSettings(const Base::ProgramArgumentBinding& other)
{
    META_FUNCTION_TASK();
    Base::ProgramArgumentBinding::MergeSettings(other);
    m_settings_mt.argument = Base::ProgramArgumentBinding::GetSettings().argument;

    const Settings& metal_settings = dynamic_cast<const ProgramArgumentBinding&>(other).GetMetalSettings();
    META_CHECK_EQUAL(m_settings_mt.argument_index, metal_settings.argument_index);
    for(const auto& [shader_type, struct_offset] : metal_settings.argument_buffer_offset_by_shader_type)
    {
        const auto argument_buffer_offset_it = m_settings_mt.argument_buffer_offset_by_shader_type.find(shader_type);
        if (argument_buffer_offset_it == m_settings_mt.argument_buffer_offset_by_shader_type.end())
            m_settings_mt.argument_buffer_offset_by_shader_type.emplace(shader_type, struct_offset);
        else if (!argument_buffer_offset_it->second)
            argument_buffer_offset_it->second = struct_offset;
    }
}

bool ProgramArgumentBinding::SetResourceViewSpan(Rhi::ResourceViewSpan resource_views)
{
    META_FUNCTION_TASK();
    CallbackBlocker callback_blocker(*this);

    const Rhi::ResourceViews prev_resource_views = GetResourceViews();
    if (!Base::ProgramArgumentBinding::SetResourceViewSpan(resource_views))
        return false;

    SetMetalResourcesForViews(resource_views);

    Data::Emitter<Rhi::IProgramBindings::IArgumentBindingCallback>::Emit(
        &Rhi::IProgramBindings::IArgumentBindingCallback::OnProgramArgumentBindingResourceViewsChanged,
        std::cref(*this), std::cref(prev_resource_views), std::cref(GetResourceViews())
    );
    return true;
}

void ProgramArgumentBinding::UpdateArgumentBufferOffsets(const Program& program)
{
    META_FUNCTION_TASK();
    if (m_settings_mt.argument_buffer_offset_by_shader_type.empty())
        return;

    Data::Size arg_buffer_offset = 0U;
    for(Rhi::ShaderType shader_type : program.GetShaderTypes())
    {
        if (arg_buffer_offset)
        {
            if (const auto argument_buffer_offset_it = m_settings_mt.argument_buffer_offset_by_shader_type.find(shader_type);
                argument_buffer_offset_it != m_settings_mt.argument_buffer_offset_by_shader_type.end())
                argument_buffer_offset_it->second += arg_buffer_offset;
        }

        const Rhi::ProgramArgumentAccessType arg_access_type = m_settings_mt.argument.GetAccessorType();
        if (const ArgumentBufferLayout* layout_ptr = program.GetMetalShader(shader_type).GetArgumentBufferLayoutPtr(arg_access_type))
        {
            arg_buffer_offset += layout_ptr->data_size;
        }
    }
}

bool ProgramArgumentBinding::UpdateRootConstantResourceViews()
{
    if (!Base::ProgramArgumentBinding::UpdateRootConstantResourceViews())
        return false;

    SetMetalResourcesForViews(Base::ProgramArgumentBinding::GetResourceViews());

    const Rhi::RootConstant root_constants = GetRootConstant();
    Data::Emitter<Rhi::IProgramBindings::IArgumentBindingCallback>::Emit(
        &Rhi::IProgramArgumentBindingCallback::OnProgramArgumentBindingRootConstantChanged,
        std::cref(*this), std::cref(root_constants)
    );
    return true;
}

ProgramArgumentBinding::NativeResourceViews::Ptr ProgramArgumentBinding::GetNativeResourceViews() const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_mtl_resource_views_mutex);
    return m_mtl_resource_views;
}

void ProgramArgumentBinding::SetMetalResourcesForViews(Rhi::ResourceViewSpan resource_views)
{
    META_FUNCTION_TASK();

    // The new snapshot is built entirely in a local object, so that concurrent readers keep using
    // the previous one until the fully built snapshot is published in one step at the end.
    auto native_resource_views_ptr = std::make_shared<NativeResourceViews>();
    NativeResourceViews& native_resource_views = *native_resource_views_ptr;

    std::set<id<MTLResource>> mtl_resource_set;

    switch(m_settings_mt.resource_type)
    {
    using enum Rhi::ResourceType;
    case Sampler:
        native_resource_views.sampler_states.reserve(resource_views.size());
        std::ranges::transform(resource_views, std::back_inserter(native_resource_views.sampler_states),
                               [](const Rhi::ResourceView& resource_view)
                               { return dynamic_cast<const class Sampler&>(resource_view.GetResource()).GetNativeSamplerState(); });
        break;

    case Texture:
        native_resource_views.textures.reserve(resource_views.size());
        for(const Rhi::ResourceView& resource_view : resource_views)
        {
            const auto& texture = dynamic_cast<const class Texture&>(resource_view.GetResource());
            native_resource_views.resource_usage |= texture.GetNativeResourceUsage();
            mtl_resource_set.insert(static_cast<id<MTLResource>>(texture.GetNativeTexture()));
            native_resource_views.textures.push_back(texture.GetNativeTexture());
        }
        break;

    case Buffer:
        native_resource_views.buffers.reserve(resource_views.size());
        native_resource_views.buffer_offsets.reserve(resource_views.size());
        for (const Rhi::ResourceView& resource_view : resource_views)
        {
            const auto& buffer = dynamic_cast<const class Buffer&>(resource_view.GetResource());
            native_resource_views.resource_usage |= buffer.GetNativeResourceUsage();
            mtl_resource_set.insert(static_cast<id<MTLResource>>(buffer.GetNativeBuffer()));
            native_resource_views.buffers.push_back(buffer.GetNativeBuffer());
            native_resource_views.buffer_offsets.push_back(static_cast<NSUInteger>(resource_view.GetOffset()));
        }
        break;

    default: META_UNEXPECTED(m_settings_mt.resource_type);
    }

    std::copy(mtl_resource_set.begin(), mtl_resource_set.end(), std::back_inserter(native_resource_views.resources));

    std::scoped_lock lock_guard(m_mtl_resource_views_mutex);
    m_mtl_resource_views = std::move(native_resource_views_ptr);
}

} // namespace Methane::Graphics::Metal
