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

FILE: Methane/Graphics/DirectX12/ProgramBindingsDX.cpp
DirectX 12 implementation of the program bindings interface.

******************************************************************************/

#include "ProgramBindingsDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "CommandListDX.h"
#include "DescriptorHeapDX.h"

#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <d3dx12.h>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

static Resource::State GetBoundResourceState(const ProgramBindingsDX::ArgumentBindingDX::SettingsDX& binding_settings)
{
    META_FUNCTION_TASK();
    if (binding_settings.argument.IsConstant() && binding_settings.resource_type == Resource::Type::Buffer)
        return Resource::State::VertexAndConstantBuffer;

    const Shader::Type shader_type = binding_settings.argument.GetShaderType();
    switch(shader_type)
    {
    case Shader::Type::All:
    case Shader::Type::Vertex:  return Resource::State::NonPixelShaderResource;
    case Shader::Type::Pixel:   return Resource::State::PixelShaderResource;
    default: META_UNEXPECTED_ARG_RETURN(shader_type, Resource::State::Common);
    }
}

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBindingsBase::ArgumentBindingBase::CreateCopy(const ArgumentBindingBase& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsDX::ArgumentBindingDX>(static_cast<const ProgramBindingsDX::ArgumentBindingDX&>(other_argument_binding));
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(const ContextBase& context, const SettingsDX& settings)
    : ProgramBindingsBase::ArgumentBindingBase(context, settings)
    , m_settings_dx(settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME("m_p_descriptor_heap_reservation", !m_p_descriptor_heap_reservation);
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(const ArgumentBindingDX& other)
    : ArgumentBindingBase(other)
    , m_settings_dx(other.m_settings_dx)
    , m_root_parameter_index(other.m_root_parameter_index)
    , m_descriptor_range(other.m_descriptor_range)
    , m_p_descriptor_heap_reservation(other.m_p_descriptor_heap_reservation)
    , m_resource_locations_dx(other.m_resource_locations_dx)
{
    META_FUNCTION_TASK();
    if (m_p_descriptor_heap_reservation)
    {
        META_CHECK_ARG_TRUE( m_p_descriptor_heap_reservation->heap.get().IsShaderVisible());
        META_CHECK_ARG_EQUAL(m_p_descriptor_heap_reservation->heap.get().GetSettings().type, m_descriptor_range.heap_type);
    }
}

void ProgramBindingsDX::ArgumentBindingDX::SetResourceLocations(const Resource::Locations& resource_locations)
{
    META_FUNCTION_TASK();
    if (GetResourceLocations() == resource_locations)
        return;

    ArgumentBindingBase::SetResourceLocations(resource_locations);

    if (m_settings_dx.type == Type::DescriptorTable)
    {
        META_CHECK_ARG_LESS_DESCR(resource_locations.size(), m_descriptor_range.count + 1, "the number of bound resources exceeds reserved descriptors count");
    }

    const uint32_t             descriptor_range_start = m_p_descriptor_heap_reservation
                                                      ? m_p_descriptor_heap_reservation->GetRange(m_settings_dx.argument.GetAccessorIndex()).GetStart()
                                                      : std::numeric_limits<uint32_t>::max();
    const DescriptorHeapDX*      p_dx_descriptor_heap = m_p_descriptor_heap_reservation
                                                      ? static_cast<const DescriptorHeapDX*>(&m_p_descriptor_heap_reservation->heap.get())
                                                      : nullptr;
    const DescriptorHeap::Type   descriptor_heap_type = p_dx_descriptor_heap
                                                      ? p_dx_descriptor_heap->GetSettings().type
                                                      : DescriptorHeap::Type::Undefined;
    const D3D12_DESCRIPTOR_HEAP_TYPE native_heap_type = p_dx_descriptor_heap
                                                      ? p_dx_descriptor_heap->GetNativeDescriptorHeapType()
                                                      : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    const wrl::ComPtr<ID3D12Device>& cp_native_device = static_cast<const IContextDX&>(GetContext()).GetDeviceDX().GetNativeDevice();
    META_CHECK_ARG_NOT_NULL(cp_native_device);

    uint32_t resource_index = 0;
    m_resource_locations_dx.clear();
    m_resource_locations_dx.reserve(resource_locations.size());
    for(const Resource::Location& resource_location : resource_locations)
    {
        m_resource_locations_dx.emplace_back(resource_location);

        if (!p_dx_descriptor_heap)
            continue;

        const IResourceDX::LocationDX& dx_resource_location = m_resource_locations_dx.back();
        META_CHECK_ARG_EQUAL_DESCR(m_descriptor_range.heap_type, descriptor_heap_type,
                                   "incompatible heap type '{}' is set for resource binding on argument '{}' of {} shader",
                                   magic_enum::enum_name(descriptor_heap_type), m_settings_dx.argument.GetName(),
                                   magic_enum::enum_name(m_settings_dx.argument.GetShaderType()));

        const uint32_t descriptor_index = descriptor_range_start + m_descriptor_range.offset + resource_index;
        cp_native_device->CopyDescriptorsSimple(
            1,
            p_dx_descriptor_heap->GetNativeCpuDescriptorHandle(descriptor_index),
            dx_resource_location.GetResourceDX().GetNativeCpuDescriptorHandle(ResourceBase::Usage::ShaderRead),
            native_heap_type
        );

        resource_index++;
    }

    GetContext().RequestDeferredAction(Context::DeferredAction::CompleteInitialization);
}

void ProgramBindingsDX::ArgumentBindingDX::SetDescriptorRange(const DescriptorRange& descriptor_range)
{
    META_FUNCTION_TASK();
    const DescriptorHeap::Type expected_heap_type = GetDescriptorHeapType();
    META_CHECK_ARG_EQUAL_DESCR(descriptor_range.heap_type, expected_heap_type,
                               "descriptor heap type '{}' is incompatible with the resource binding, expected heap type is '{}'",
                               magic_enum::enum_name(descriptor_range.heap_type),
                               magic_enum::enum_name(expected_heap_type));
    META_CHECK_ARG_LESS_DESCR(descriptor_range.count, m_settings_dx.resource_count + 1,
                              "descriptor range size {} will not fit bound shader resources count {}",
                              descriptor_range.count, m_settings_dx.resource_count);

    m_descriptor_range = descriptor_range;
}

void ProgramBindingsDX::ArgumentBindingDX::SetDescriptorHeapReservation(const DescriptorHeap::Reservation* p_reservation)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME_DESCR("p_reservation",
                              !p_reservation || (p_reservation->heap.get().IsShaderVisible() && p_reservation->heap.get().GetSettings().type == m_descriptor_range.heap_type),
                              "argument binding reservation must be made in shader visible descriptor heap of type '{}'",
                              magic_enum::enum_name(m_descriptor_range.heap_type));
    m_p_descriptor_heap_reservation = p_reservation;
}

Ptr<ProgramBindings> ProgramBindings::Create(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<ProgramBindingsDX>(program_ptr, resource_locations_by_argument, frame_index);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument, const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<ProgramBindingsDX>(static_cast<const ProgramBindingsDX&>(other_program_bindings), replace_resource_locations_by_argument, frame_index);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

ProgramBindingsDX::ProgramBindingsDX(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, resource_locations_by_argument, frame_index)
{
    META_FUNCTION_TASK();
}

ProgramBindingsDX::ProgramBindingsDX(const ProgramBindingsDX& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument, const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, replace_resource_locations_by_argument, frame_index)
{
    META_FUNCTION_TASK();
}

void ProgramBindingsDX::Initialize()
{
    META_FUNCTION_TASK();
    const ContextBase& context = static_cast<ProgramBase&>(GetProgram()).GetContext();
    ResourceManager&   resource_manager = context.GetResourceManager();

    resource_manager.AddProgramBindings(*this);

    if (resource_manager.IsDeferredHeapAllocation())
    {
        context.RequestDeferredAction(Context::DeferredAction::CompleteInitialization);
    }
    else
    {
        CompleteInitialization();
    }
}

void ProgramBindingsDX::CompleteInitialization()
{
    META_FUNCTION_TASK();
    CopyDescriptorsToGpu();
    UpdateRootParameterBindings();
}

void ProgramBindingsDX::Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const
{
    Apply(dynamic_cast<ICommandListDX&>(command_list), command_list.GetProgramBindings().get(), apply_behavior);
}

void ProgramBindingsDX::Apply(ICommandListDX& command_list_dx, const ProgramBindingsBase* p_applied_program_bindings, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    Program::ArgumentAccessor::Type apply_access_mask = Program::ArgumentAccessor::Type::Mutable;
    if (apply_behavior != ApplyBehavior::ConstantOnce || !p_applied_program_bindings)
    {
        apply_access_mask |= Program::ArgumentAccessor::Type::Constant;
        apply_access_mask |= Program::ArgumentAccessor::Type::FrameConstant;
    }

    // Set resource transition barriers before applying resource bindings
    ID3D12GraphicsCommandList& d3d12_command_list = command_list_dx.GetNativeCommandList();
    if (magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::StateBarriers) &&
        ApplyResourceStates(apply_access_mask) &&
        m_resource_transition_barriers_ptr && !m_resource_transition_barriers_ptr->IsEmpty())
    {
        command_list_dx.SetResourceBarriersDX(*m_resource_transition_barriers_ptr);
    }

    // Apply root parameter bindings after resource barriers
    ApplyRootParameterBindings(apply_access_mask, d3d12_command_list, p_applied_program_bindings,
                               magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::ChangesOnly));
}

// ProgramBindings::IArgumentBindingCallback
void ProgramBindingsDX::OnProgramArgumentBindingResourceLocationsChanged(const ArgumentBinding&, const Resource::Locations& old_resource_locations, const Resource::Locations& new_resource_locations)
{
    META_FUNCTION_TASK();
    if (!m_resource_transition_barriers_ptr)
        return;

    // Find resources that are not used anymore for resource binding
    std::set<Resource*> processed_resources;
    for(const Resource::Location& old_resource_location : old_resource_locations)
    {
        if (!old_resource_location.IsInitialized() || old_resource_location.GetResource().GetResourceType() == Resource::Type::Sampler ||
             processed_resources.count(old_resource_location.GetResourcePtr().get()))
            continue;

        // Check if resource is still used in new resource locations
        if (std::find_if(new_resource_locations.begin(), new_resource_locations.end(),
                [&old_resource_location](const Resource::Location& new_resource_location)
                { return new_resource_location.GetResourcePtr() == old_resource_location.GetResourcePtr(); }
            ) != new_resource_locations.end())
        {
            processed_resources.insert(old_resource_location.GetResourcePtr().get());
            continue;
        }

        // Remove unused resources from transition barriers applied for program bindings:
        m_resource_transition_barriers_ptr->RemoveTransition(old_resource_location.GetResource());
    }
}

template<typename FuncType>
void ProgramBindingsDX::ForEachArgumentBinding(FuncType argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        auto& argument_binding = static_cast<ArgumentBindingDX&>(*argument_binding_ptr);
        const ArgumentBindingDX::DescriptorRange& descriptor_range = argument_binding.GetDescriptorRange();

        if (descriptor_range.heap_type == DescriptorHeap::Type::Undefined)
        {
            argument_binding_function(argument_binding, nullptr);
            continue;
        }

        if (const std::optional<DescriptorHeap::Reservation>& desc_heap_reservation_opt = GetDescriptorHeapReservationByType(descriptor_range.heap_type);
            desc_heap_reservation_opt.has_value())
        {
            argument_binding_function(argument_binding, &*desc_heap_reservation_opt);
            continue;
        }

        argument_binding_function(argument_binding, nullptr);
    }
}

void ProgramBindingsDX::AddRootParameterBinding(const Program::ArgumentAccessor& argument_accessor, const RootParameterBinding& root_parameter_binding)
{
    META_FUNCTION_TASK();
    m_root_parameter_bindings_by_access[argument_accessor.GetAccessorIndex()].emplace_back(root_parameter_binding);
}

void ProgramBindingsDX::AddResourceState(const Program::ArgumentAccessor& argument_accessor, ResourceState resource_state)
{
    META_FUNCTION_TASK();
    m_resource_states_by_access[argument_accessor.GetAccessorIndex()].emplace_back(resource_state);
}

void ProgramBindingsDX::UpdateRootParameterBindings()
{
    META_FUNCTION_TASK();
    for(RootParameterBindings& root_parameter_bindings : m_root_parameter_bindings_by_access)
    {
        root_parameter_bindings.clear();
    }

    for(ResourceStates& resource_states : m_resource_states_by_access)
    {
        resource_states.clear();
    }

    ForEachArgumentBinding(std::bind(&ProgramBindingsDX::AddRootParameterBindingsForArgument, this, std::placeholders::_1, std::placeholders::_2));
}

void ProgramBindingsDX::AddRootParameterBindingsForArgument(ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
{
    META_FUNCTION_TASK();
    using DXBindingType     = ArgumentBindingDX::Type;
    using DXDescriptorRange = ArgumentBindingDX::DescriptorRange;

    const ArgumentBindingDX::SettingsDX& binding_settings = argument_binding.GetSettingsDX();

    if (binding_settings.type == DXBindingType::DescriptorTable)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(p_heap_reservation, "descriptor heap reservation is not available for \"Descriptor Table\" resource binding");
        const auto&              dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
        const DXDescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
        const uint32_t           descriptor_index   = p_heap_reservation->GetRange(binding_settings.argument.GetAccessorIndex()).GetStart() + descriptor_range.offset;

        AddRootParameterBinding(binding_settings.argument, {
            argument_binding,
            argument_binding.GetRootParameterIndex(),
            dx_descriptor_heap.GetNativeGpuDescriptorHandle(descriptor_index),
            0
        });
    }

    for (const IResourceDX::LocationDX& resource_location_dx : argument_binding.GetResourceLocationsDX())
    {
        if (binding_settings.type == DXBindingType::ConstantBufferView ||
            binding_settings.type == DXBindingType::ShaderResourceView)
        {
            AddRootParameterBinding(binding_settings.argument, {
                argument_binding,
                argument_binding.GetRootParameterIndex(),
                D3D12_GPU_DESCRIPTOR_HANDLE{},
                resource_location_dx.GetNativeGpuAddress()
            });
        }

        // Samplers do not have underlying resource and do not need state changing
        if (binding_settings.resource_type == Resource::Type::Sampler)
            continue;

        AddResourceState(binding_settings.argument, {
            std::dynamic_pointer_cast<ResourceBase>(resource_location_dx.GetResourcePtr()),
            GetBoundResourceState(binding_settings)
        });
    }
}

bool ProgramBindingsDX::ApplyResourceStates(Program::ArgumentAccessor::Type access_types_mask) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    bool resource_states_changed = false;
    for(Program::ArgumentAccessor::Type access_type : magic_enum::enum_values<Program::ArgumentAccessor::Type>())
    {
        if (!magic_enum::flags::enum_contains(access_types_mask & access_type))
            continue;

        const ResourceStates& resource_states = m_resource_states_by_access[magic_enum::enum_index(access_type).value()];
        for(const ResourceState& resource_state : resource_states)
        {
            META_CHECK_ARG_NOT_NULL(resource_state.resource_ptr);
            resource_states_changed |= resource_state.resource_ptr->SetState(resource_state.state, m_resource_transition_barriers_ptr);
        }
    }

    return resource_states_changed;
}

void ProgramBindingsDX::ApplyRootParameterBindings(Program::ArgumentAccessor::Type access_types_mask, ID3D12GraphicsCommandList& d3d12_command_list,
                                                   const ProgramBindingsBase* p_applied_program_bindings, bool apply_changes_only) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    for(Program::ArgumentAccessor::Type access_type : magic_enum::flags::enum_values<Program::ArgumentAccessor::Type>())
    {
        if (!magic_enum::flags::enum_contains(access_types_mask & access_type))
            continue;

        const RootParameterBindings& root_parameter_bindings = m_root_parameter_bindings_by_access[magic_enum::enum_index(access_type).value()];
        for (const RootParameterBinding& root_parameter_binding : root_parameter_bindings)
        {
            if (access_type == Program::ArgumentAccessor::Type::Mutable && apply_changes_only &&
                p_applied_program_bindings && root_parameter_binding.argument_binding.IsAlreadyApplied(GetProgram(), *p_applied_program_bindings))
                continue;

            ApplyRootParameterBinding(root_parameter_binding, d3d12_command_list);
        }
    }
}

void ProgramBindingsDX::ApplyRootParameterBinding(const RootParameterBinding& root_parameter_binding, ID3D12GraphicsCommandList& d3d12_command_list) const
{
    META_FUNCTION_TASK();
    switch (const ArgumentBindingDX::Type binding_type = root_parameter_binding.argument_binding.GetSettingsDX().type;
            binding_type)
    {
    case ArgumentBindingDX::Type::DescriptorTable:
        d3d12_command_list.SetGraphicsRootDescriptorTable(root_parameter_binding.root_parameter_index, root_parameter_binding.base_descriptor);
        break;

    case ArgumentBindingDX::Type::ConstantBufferView:
        d3d12_command_list.SetGraphicsRootConstantBufferView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    case ArgumentBindingDX::Type::ShaderResourceView:
        d3d12_command_list.SetComputeRootShaderResourceView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    default:
        META_UNEXPECTED_ARG(binding_type);
    }
}

void ProgramBindingsDX::CopyDescriptorsToGpu()
{
    META_FUNCTION_TASK();
    META_LOG("Copy descriptors to GPU for program '{}'", GetProgram().GetName());

    const wrl::ComPtr<ID3D12Device>& cp_d3d12_device = static_cast<const ProgramDX&>(GetProgram()).GetContextDX().GetDeviceDX().GetNativeDevice();
    ForEachArgumentBinding(std::bind(&ProgramBindingsDX::CopyDescriptorsToGpuForArgument, this, cp_d3d12_device, std::placeholders::_1, std::placeholders::_2));
}

void ProgramBindingsDX::CopyDescriptorsToGpuForArgument(const wrl::ComPtr<ID3D12Device>& d3d12_device, ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation) const
{
    META_FUNCTION_TASK();
    if (!p_heap_reservation)
        return;

    using AcceessType = Program::ArgumentAccessor::Type;

    const auto&                               dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
    const ArgumentBindingDX::DescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
    const DescriptorHeap::Type                heap_type          = dx_descriptor_heap.GetSettings().type;
    const DescriptorHeap::Range&              heap_range         = p_heap_reservation->GetRange(argument_binding.GetSettings().argument.GetAccessorIndex());
    const D3D12_DESCRIPTOR_HEAP_TYPE          native_heap_type   = dx_descriptor_heap.GetNativeDescriptorHeapType();

    argument_binding.SetDescriptorHeapReservation(p_heap_reservation);
    META_CHECK_ARG_NOT_NULL(d3d12_device);
    META_CHECK_ARG_LESS_DESCR(descriptor_range.offset, heap_range.GetLength(),
                              "descriptor range offset is out of reserved descriptor range bounds");

    uint32_t resource_index = 0;
    for (const IResourceDX::LocationDX& resource_location_dx : argument_binding.GetResourceLocationsDX())
    {
        const DescriptorHeap::Types used_heap_types = resource_location_dx.GetResourceDX().GetDescriptorHeapTypes();
        META_CHECK_ARG_DESCR(heap_type, used_heap_types.find(heap_type) != used_heap_types.end(),
                             "can not create binding for resource used for {} on descriptor heap of incompatible type '{}'",
                             magic_enum::enum_name(resource_location_dx.GetResourceDX().GetUsage()),
                             magic_enum::enum_name(dx_descriptor_heap.GetSettings().type));

        const uint32_t descriptor_index = heap_range.GetStart() + descriptor_range.offset + resource_index;
        META_LOG("  - Resource '{}' binding with {} access has descriptor heap range [{}, {}), CPU descriptor index {}",
                 resource_location_dx.GetResourceDX().GetName(),
                 magic_enum::enum_name(argument_binding.GetSettings().argument.GetAccessorType()),
                 descriptor_range.offset, descriptor_range.offset + descriptor_range.count, descriptor_index);

        d3d12_device->CopyDescriptorsSimple(1,
            dx_descriptor_heap.GetNativeCpuDescriptorHandle(descriptor_index),
            resource_location_dx.GetResourceDX().GetNativeCpuDescriptorHandle(ResourceBase::Usage::ShaderRead),
            native_heap_type
        );
        resource_index++;
    }
}

} // namespace Methane::Graphics
