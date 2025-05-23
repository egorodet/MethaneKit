/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/ProgramArgumentBinding.cpp
Base implementation of the program argument binding interface.

******************************************************************************/

#include <Methane/Graphics/Base/ProgramArgumentBinding.h>
#include <Methane/Graphics/Base/ProgramBindings.h>
#include <Methane/Graphics/Base/Program.h>
#include <Methane/Graphics/RHI/TypeFormatters.hpp>
#include <Methane/Data/SpanComparable.hpp>
#include <Methane/Data/EnumMaskUtil.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace Methane::Graphics::Base
{

ProgramArgumentBinding::ProgramArgumentBinding(const Context& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{ }

ProgramArgumentBinding::ProgramArgumentBinding(const ProgramArgumentBinding& other)
    : Emitter(other)
    , enable_shared_from_this(other)
    , Receiver(other)
    , m_context(other.m_context)
    , m_settings(other.m_settings)
    , m_resource_views(other.m_resource_views)
    , m_emit_callback_enabled(other.m_emit_callback_enabled)
{ }

ProgramArgumentBinding::~ProgramArgumentBinding() = default;

void ProgramArgumentBinding::MergeSettings(const ProgramArgumentBinding& other)
{
    META_FUNCTION_TASK();
    const Settings& settings = other.GetSettings();
    META_CHECK_EQUAL(settings.argument.GetName(), m_settings.argument.GetName());
    META_CHECK_EQUAL(settings.argument.GetAccessorType(), m_settings.argument.GetAccessorType());
    META_CHECK_EQUAL(settings.resource_type, m_settings.resource_type);
    META_CHECK_EQUAL(settings.resource_count, m_settings.resource_count);
    m_settings.argument.MergeShaderTypes(settings.argument.GetShaderType());
}

bool ProgramArgumentBinding::SetResourceViewSpan(Rhi::ResourceViewSpan resource_views)
{
    META_FUNCTION_TASK();
    META_CHECK_FALSE_DESCR(m_settings.argument.IsRootConstant(),
                           "Can not set resource view for argument which is marked with "
                           "\"ValueType::RootConstant\" in \"ProgramSettings::argument_accessors\".");

    if (resource_views == Rhi::ResourceViewSpan(m_resource_views))
        return false;

    if (m_settings.argument.IsConstant() && !m_resource_views.empty())
        throw ConstantModificationException(GetSettings().argument);

    META_CHECK_NOT_EMPTY_DESCR(resource_views, "can not set empty resources for resource binding");

    [[maybe_unused]] const bool              is_addressable_binding = m_settings.argument.IsAddressable();
    [[maybe_unused]] const Rhi::IResource::Type bound_resource_type = m_settings.resource_type;

    for (const Rhi::ResourceView& resource_view : resource_views)
    {
        META_CHECK_NAME_DESCR("resource_view", resource_view.GetResource().GetResourceType() == bound_resource_type,
                                  "incompatible resource type '{}' is bound to argument '{}' of type '{}'",
                                  magic_enum::enum_name(resource_view.GetResource().GetResourceType()),
                                  m_settings.argument.GetName(), magic_enum::enum_name(bound_resource_type));

        const Rhi::ResourceUsageMask resource_usage_mask = resource_view.GetResource().GetUsage();
        META_CHECK_EQUAL_DESCR(resource_usage_mask.HasAnyBit(Rhi::ResourceUsage::Addressable), is_addressable_binding,
                             "resource usage mask {} does not have addressable flag", Data::GetEnumMaskName(resource_usage_mask));
        META_CHECK_NAME_DESCR("resource_view", is_addressable_binding || !resource_view.GetOffset(),
                                  "can not set resource view_id with non-zero offset to non-addressable resource binding");
    }

    Rhi::ResourceViews prev_resource_views;
    if (m_emit_callback_enabled)
        prev_resource_views = m_resource_views;

    m_resource_views.assign(resource_views.begin(), resource_views.end());

    if (m_emit_callback_enabled)
        Data::Emitter<Rhi::IProgramBindings::IArgumentBindingCallback>::Emit(
            &Rhi::IProgramBindings::IArgumentBindingCallback::OnProgramArgumentBindingResourceViewsChanged,
            std::cref(*this), std::cref(prev_resource_views), std::cref(m_resource_views)
        );

    return true;
}

bool ProgramArgumentBinding::SetResourceViews(const Rhi::ResourceViews& resource_views)
{
    return SetResourceViewSpan(Rhi::ResourceViewSpan(resource_views));
}

bool ProgramArgumentBinding::SetResourceView(const Rhi::ResourceView& resource_view)
{
    return SetResourceViewSpan(Rhi::ResourceViewSpan(&resource_view, 1U));
}

Rhi::RootConstant ProgramArgumentBinding::GetRootConstant() const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL_DESCR(m_root_constant_accessor_ptr,
                              "Root constant accessor of argument binding is not initialized!");
    return m_root_constant_accessor_ptr->GetRootConstant();
}

bool ProgramArgumentBinding::SetRootConstant(const Rhi::RootConstant& root_constant)
{
    META_FUNCTION_TASK();
    META_CHECK_TRUE_DESCR(m_settings.argument.IsRootConstant(),
                          "Can not set root constant for argument with is not marked with "
                          "\"ValueType::RootConstant\" in \"ProgramSettings::argument_accessors\"");
    META_CHECK_NOT_NULL_DESCR(m_root_constant_accessor_ptr,
                              "program argument root constant accessor is not initialized");
    META_CHECK_FALSE_DESCR(root_constant.IsEmptyOrNull(),
                           "Can not set empty or null root constant to shader argument.");
    META_CHECK_EQUAL_DESCR(root_constant.GetDataSize(), m_settings.buffer_size,
                           "Size of root constant does not match buffer size ({}) for shader argument '{}'",
                           m_settings.buffer_size, static_cast<std::string>(m_settings.argument).c_str());

    if (!m_root_constant_accessor_ptr->SetRootConstant(root_constant))
        return false;

    if (m_settings.argument.IsRootConstantBuffer())
        UpdateRootConstantResourceViews();

    if (m_emit_callback_enabled)
        Data::Emitter<Rhi::IProgramBindings::IArgumentBindingCallback>::Emit(
            &Rhi::IProgramBindings::IArgumentBindingCallback::OnProgramArgumentBindingRootConstantChanged,
            std::cref(*this), std::cref(root_constant)
        );

    return true;
}

bool ProgramArgumentBinding::UpdateRootConstantResourceViews()
{
    META_FUNCTION_TASK();
    if (!m_root_constant_accessor_ptr ||
        !m_settings.argument.IsRootConstantBuffer())
        return false;

    // NOTE: Backend buffer may change inside GetResourceView call,
    //       which executes callback OnRootConstantBufferChanged, which in turn updates resource views again.
    const Rhi::ResourceView root_constant_resource_view = m_root_constant_accessor_ptr->GetResourceView();

    if (m_resource_views.size() == 1 &&
        m_resource_views.back() == root_constant_resource_view)
        return false;

    m_resource_views.clear();
    m_resource_views.emplace_back(root_constant_resource_view);
    return true;
}

ProgramArgumentBinding::operator std::string() const
{
    META_FUNCTION_TASK();
    if (m_settings.argument.IsRootConstantValue())
        return fmt::format("{} is bound to value of {} bytes", m_settings.argument, m_root_constant_accessor_ptr->GetDataSize());

    return m_resource_views.empty()
         ? fmt::format("{} is unbound", m_settings.argument)
         : fmt::format("{} is bound to {}", m_settings.argument, fmt::join(m_resource_views, ", "));
}

void ProgramArgumentBinding::Initialize(Program& program, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    if (!m_settings.argument.IsRootConstant() || m_root_constant_accessor_ptr)
        return;

    if (m_settings.argument.IsRootConstantValue())
    {
        m_root_constant_accessor_ptr = program.GetRootConstantStorage().ReserveRootConstant(m_settings.buffer_size);
    }
    else
    {
        RootConstantBuffer& root_constant_buffer = program.GetRootConstantBuffer(m_settings.argument.GetAccessorType(), frame_index);
        m_root_constant_accessor_ptr = root_constant_buffer.ReserveRootConstant(m_settings.buffer_size);
        root_constant_buffer.Connect(*this);
    }
}

bool ProgramArgumentBinding::IsAlreadyApplied(const Rhi::IProgram& program,
                                              const ProgramBindings& applied_program_bindings,
                                              bool check_binding_value_changes) const
{
    META_FUNCTION_TASK();
    if (std::addressof(applied_program_bindings.GetProgram()) != std::addressof(program))
        return false;

    // 1) No need in setting constant resource binding
    //    when another binding was previously set in the same command list for the same program
    if (m_settings.argument.IsConstant())
        return true;

    if (!check_binding_value_changes)
        return false;

    // 2) No need in setting resource binding to the same location
    //    as a previous resource binding set in the same command list for the same program
    if (applied_program_bindings.Get(m_settings.argument).GetResourceViews() == m_resource_views)
        return true;

    return false;
}

void ProgramArgumentBinding::OnRootConstantBufferChanged(RootConstantBuffer&, const Ptr<Rhi::IBuffer>&)
{
    META_FUNCTION_TASK();
    UpdateRootConstantResourceViews();
}

} // namespace Methane::Graphics::Base
