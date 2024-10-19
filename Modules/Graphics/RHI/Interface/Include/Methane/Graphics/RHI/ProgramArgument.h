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

FILE: Methane/Graphics/RHI/ProgramArgument.h
Methane program argument, accessor and related types.

******************************************************************************/

#pragma once

#include "IShader.h"
#include "ResourceView.h"
#include "RootConstant.h"

#include <Methane/Data/Chunk.hpp>
#include <Methane/Data/EnumMask.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Memory.hpp>
#include <Methane/Checks.hpp>

#include <vector>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <variant>

namespace Methane::Graphics::Rhi
{

class ProgramArgumentNotFoundException;

class ProgramArgument
{
public:
    using NotFoundException = ProgramArgumentNotFoundException;

    struct Hash
    {
        [[nodiscard]] size_t operator()(const ProgramArgument& arg) const { return arg.m_hash; }
    };

    ProgramArgument(ShaderType shader_type, std::string_view argument_name) noexcept;
    virtual ~ProgramArgument() = default;

    [[nodiscard]] ShaderType       GetShaderType() const noexcept { return m_shader_type; }
    [[nodiscard]] std::string_view GetName() const noexcept       { return m_name; }
    [[nodiscard]] size_t           GetHash() const noexcept       { return m_hash; }

    [[nodiscard]] bool operator==(const ProgramArgument& other) const noexcept;
    [[nodiscard]] bool operator<(const ProgramArgument& other) const noexcept;
    [[nodiscard]] virtual explicit operator std::string() const noexcept;

    void MergeShaderTypes(ShaderType shader_type);

private:
    ShaderType       m_shader_type;
    std::string_view m_name;
    size_t           m_hash;
};

struct IProgram;

class ProgramArgumentNotFoundException : public std::invalid_argument
{
public:
    ProgramArgumentNotFoundException(const IProgram& program, const ProgramArgument& argument);

    [[nodiscard]] const IProgram&        GetProgram() const noexcept  { return m_program; }
    [[nodiscard]] const ProgramArgument& GetArgument() const noexcept { return m_argument; }

private:
    const IProgram& m_program;
    ProgramArgument m_argument;
};

// NOTE: Access Type enum values should strictly match with
// register space values of 'META_ARG_*' shader definitions from MethaneShaders.cmake:
enum class ProgramArgumentAccessType : uint32_t
{
    Constant,      // META_ARG_CONSTANT(0)
    FrameConstant, // META_ARG_FRAME_CONSTANT(1)
    Mutable        // META_ARG_MUTABLE(2)
};

using ProgramArgumentAccessMask = Data::EnumMask<ProgramArgumentAccessType>;

enum class ProgramArgumentValueType
{
    ResourceView,    // Default argument access by descriptor from resource view
    ResourceAddress, // Addressable argument access to resource view with offset and size
    RootConstant     // 32-bit constant(s) stored in the root signature
};

using ProgramArguments = std::unordered_set<ProgramArgument, ProgramArgument::Hash>;

class ProgramArgumentAccessor : public ProgramArgument
{
public:
    using Type      = ProgramArgumentAccessType;
    using Mask      = ProgramArgumentAccessMask;
    using ValueType = ProgramArgumentValueType;

    static Type GetTypeByRegisterSpace(uint32_t register_space);

    ProgramArgumentAccessor(ShaderType shader_type,
                            std::string_view arg_name,
                            Type access_type = Type::Mutable,
                            ValueType value_type = ValueType::ResourceView) noexcept;

    ProgramArgumentAccessor(const ProgramArgument& argument,
                            Type access_type = Type::Mutable,
                            ValueType value_type = ValueType::ResourceView) noexcept;

    [[nodiscard]] size_t    GetAccessorIndex() const noexcept;
    [[nodiscard]] Type      GetAccessorType() const noexcept   { return m_access_type; }
    [[nodiscard]] ValueType GetValueType() const noexcept      { return m_value_type; }
    [[nodiscard]] bool      IsAddressable() const noexcept     { return m_value_type == ValueType::ResourceAddress; }
    [[nodiscard]] bool      IsRootConstant() const noexcept    { return m_value_type == ValueType::RootConstant; }
    [[nodiscard]] bool      IsMutable() const noexcept         { return m_access_type == Type::Mutable; }
    [[nodiscard]] bool      IsConstant() const noexcept        { return m_access_type == Type::Constant; }
    [[nodiscard]] bool      IsFrameConstant() const noexcept   { return m_access_type == Type::FrameConstant; }
    [[nodiscard]] explicit operator std::string() const noexcept final;

private:
    Type      m_access_type = Type::Mutable;
    ValueType m_value_type  = ValueType::ResourceView;
};

using ProgramArgumentAccessors      = std::unordered_set<ProgramArgumentAccessor, ProgramArgumentAccessor::Hash>;
using ProgramArgumentBindingValue   = std::variant<ResourceView, ResourceViews, RootConstant>;
using ProgramBindingValueByArgument = std::unordered_map<ProgramArgument, ProgramArgumentBindingValue, ProgramArgument::Hash>;

} // namespace Methane::Graphics::Rhi

// Helper macroses for program argument accessors initialization in program descriptor:

#define META_PROGRAM_ARG(shader_type, arg_name, access_type, value_type) \
    Methane::Graphics::Rhi::ProgramArgumentAccessor{ { shader_type, arg_name }, access_type, value_type }

// Root-Constant argument accessors

#define META_PROGRAM_ARG_ROOT(shader_type, arg_name, access_type) \
    META_PROGRAM_ARG(shader_type, arg_name, access_type, Methane::Graphics::Rhi::ProgramArgumentValueType::RootConstant)

#define META_PROGRAM_ARG_ROOT_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_ROOT(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Constant)

#define META_PROGRAM_ARG_ROOT_FRAME_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_ROOT(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::FrameConstant)

#define META_PROGRAM_ARG_ROOT_MUTABLE(shader_type, arg_name) \
    META_PROGRAM_ARG_ROOT(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Mutable)

// Resource-View argument accessors

#define META_PROGRAM_ARG_RESOURCE_VIEW(shader_type, arg_name, access_type) \
    META_PROGRAM_ARG(shader_type, arg_name, access_type, Methane::Graphics::Rhi::ProgramArgumentValueType::ResourceView)

#define META_PROGRAM_ARG_RESOURCE_VIEW_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_VIEW(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Constant)

#define META_PROGRAM_ARG_RESOURCE_VIEW_FRAME_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_VIEW(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::FrameConstant)

#define META_PROGRAM_ARG_RESOURCE_VIEW_MUTABLE(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_VIEW(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Mutable)

// Resource-Views argument accessors

#define META_PROGRAM_ARG_RESOURCE_VIEWS(shader_type, arg_name, access_type) \
    META_PROGRAM_ARG(shader_type, arg_name, access_type, Methane::Graphics::Rhi::ProgramArgumentValueType::ResourceViews)

#define META_PROGRAM_ARG_RESOURCE_VIEWS_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_VIEWS(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Constant)

#define META_PROGRAM_ARG_RESOURCE_VIEWS_FRAME_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_VIEWS(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::FrameConstant)

#define META_PROGRAM_ARG_RESOURCE_VIEWS_MUTABLE(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_VIEWS(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Mutable)

// Resource-Address argument accessors

#define META_PROGRAM_ARG_RESOURCE_ADDRESS(shader_type, arg_name, access_type) \
    META_PROGRAM_ARG(shader_type, arg_name, access_type, Methane::Graphics::Rhi::ProgramArgumentValueType::ResourceAddress)

#define META_PROGRAM_ARG_RESOURCE_ADDRESS_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_ADDRESS(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Constant)

#define META_PROGRAM_ARG_RESOURCE_ADDRESS_FRAME_CONSTANT(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_ADDRESS(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::FrameConstant)

#define META_PROGRAM_ARG_RESOURCE_ADDRESS_MUTABLE(shader_type, arg_name) \
    META_PROGRAM_ARG_RESOURCE_ADDRESS(shader_type, arg_name, Methane::Graphics::Rhi::ProgramArgumentAccessType::Mutable)
