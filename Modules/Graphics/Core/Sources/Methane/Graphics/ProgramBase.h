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

FILE: Methane/Graphics/ProgramBase.h
Base implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Program.h>
#include <Methane/Instrumentation.h>

#include "ShaderBase.h"
#include "ProgramBindingsBase.h"
#include "DescriptorHeap.h"

#include <magic_enum.hpp>
#include <memory>
#include <array>
#include <optional>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;
class CommandListBase;

class ProgramBase
    : public Program
    , public ObjectBase
{
    friend class ShaderBase;
    friend class ProgramBindingsBase;

public:
    ProgramBase(const ContextBase& context, const Settings& settings);
    ~ProgramBase() override;

    // Program interface
    const Settings&      GetSettings() const override                       { return m_settings; }
    const Shader::Types& GetShaderTypes() const override                    { return m_shader_types; }
    const Ptr<Shader>&   GetShader(Shader::Type shader_type) const override { return m_shaders_by_type[static_cast<size_t>(shader_type)]; }
    bool                 HasShader(Shader::Type shader_type) const          { return !!GetShader(shader_type); }

    const ContextBase&   GetContext() const { return m_context; }
    Ptr<ProgramBase>     GetProgramPtr()    { return std::static_pointer_cast<ProgramBase>(GetBasePtr()); }

protected:
    using FrameArgumentBindings = std::unordered_map<Program::Argument, Ptrs<ProgramBindingsBase::ArgumentBindingBase>, Program::Argument::Hash>;

    void InitArgumentBindings(const ArgumentAccessors& argument_accessors);
    const ProgramBindingsBase::ArgumentBindings&   GetArgumentBindings() const noexcept      { return m_binding_by_argument; }
    const FrameArgumentBindings&                   GetFrameArgumentBindings() const noexcept { return m_frame_bindings_by_argument; }
    const Ptr<ProgramBindingsBase::ArgumentBindingBase>& GetFrameArgumentBinding(Data::Index frame_index, const Program::ArgumentAccessor& argument_accessor) const;
    Ptr<ProgramBindingsBase::ArgumentBindingBase>  CreateArgumentBindingInstance(const Ptr<ProgramBindingsBase::ArgumentBindingBase>& argument_binding_ptr, Data::Index frame_index) const;
    DescriptorHeap::Range ReserveDescriptorRange(DescriptorHeap& heap, ArgumentAccessor::Type access_type, uint32_t range_length);

    Shader& GetShaderRef(Shader::Type shader_type) const;
    uint32_t GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const;

    using ShadersByType = std::array<Ptr<Shader>, magic_enum::enum_count<Shader::Type>() - 1>;
    static ShadersByType CreateShadersByType(const Ptrs<Shader>& shaders);

private:
    struct DescriptorHeapReservation
    {
        Ref<DescriptorHeap>   heap;
        DescriptorHeap::Range range;
    };

    using DescriptorRangeByHeapAndAccessType = std::map<std::pair<DescriptorHeap::Type, ArgumentAccessor::Type>, DescriptorHeapReservation>;

    const ContextBase&                    m_context;
    const Settings                        m_settings;
    const ShadersByType                   m_shaders_by_type;
    const Shader::Types                   m_shader_types;
    ProgramBindingsBase::ArgumentBindings m_binding_by_argument;
    FrameArgumentBindings                 m_frame_bindings_by_argument;
    DescriptorRangeByHeapAndAccessType    m_constant_descriptor_range_by_heap_and_access_type;
    TracyLockable(std::mutex,             m_constant_descriptor_ranges_reservation_mutex)
};

} // namespace Methane::Graphics
